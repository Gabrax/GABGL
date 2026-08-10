#include "DirectX12Renderer.h"

#if defined(GABGL_ENABLE_DX12) && defined(_WIN32)

#include "AudioManager.h"
#include "Camera.h"
#include "DeltaTime.hpp"
#include "Logger.h"
#include "ModelManager.h"
#include "PhysX.h"
#include "SceneManager.h"
#include "Texture.h"
#include "Window.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef APIENTRY
#undef APIENTRY
#endif
#include <Windows.h>
#ifdef DrawText
#undef DrawText
#endif
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

using Microsoft::WRL::ComPtr;

namespace
{
  constexpr uint32_t FrameCount = 2;
  constexpr uint32_t MaxDescriptors = 2048;
  constexpr uint64_t ConstantBufferBytes = 64ull * 1024ull * 1024ull;
  constexpr uint64_t UIVertexBufferBytes = 8ull * 1024ull * 1024ull;
  constexpr uint32_t FontAtlasWidth = 1024;
  constexpr uint32_t FontAtlasHeight = 512;
  constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
  constexpr DXGI_FORMAT DepthFormat = DXGI_FORMAT_D32_FLOAT;

  struct UIVertex
  {
    glm::vec2 Position;
    glm::vec4 Color;
    glm::vec2 UV;
  };

  struct Glyph
  {
    glm::vec2 UVTopLeft{};
    glm::vec2 UVBottomRight{};
    glm::ivec2 Size{};
    glm::ivec2 Bearing{};
    uint32_t Advance = 0;
  };

  struct GPUTexture
  {
    ComPtr<ID3D12Resource> Resource;
    uint32_t DescriptorIndex = 0;
  };

  struct GPUMesh
  {
    ComPtr<ID3D12Resource> VertexBuffer;
    ComPtr<ID3D12Resource> IndexBuffer;
    D3D12_VERTEX_BUFFER_VIEW VertexView{};
    D3D12_INDEX_BUFFER_VIEW IndexView{};
    uint32_t IndexCount = 0;
    uint32_t TextureDescriptorIndex = 0;
  };

  struct alignas(256) SceneConstants
  {
    glm::mat4 ViewProjection{1.0f};
    glm::mat4 Model{1.0f};
    std::array<glm::mat4, MAX_BONES> Bones{};
    glm::vec4 LightDirection{0.4f, -1.0f, 0.3f, 0.0f};
    glm::vec4 LightColor{1.0f};
    glm::vec4 CameraPosition{0.0f};
  };

  struct FrameUploadData
  {
    ComPtr<ID3D12Resource> Constants;
    uint8_t* ConstantsMapped = nullptr;
    uint64_t ConstantsOffset = 0;
    ComPtr<ID3D12Resource> UIVertices;
    uint8_t* UIVerticesMapped = nullptr;
    uint64_t UIVerticesOffset = 0;
  };

  struct DirectX12Data
  {
    ComPtr<IDXGIFactory6> Factory;
    ComPtr<IDXGIAdapter1> Adapter;
    ComPtr<ID3D12Device> Device;
    ComPtr<ID3D12CommandQueue> CommandQueue;
    ComPtr<IDXGISwapChain3> SwapChain;
    ComPtr<ID3D12DescriptorHeap> RTVHeap;
    ComPtr<ID3D12DescriptorHeap> DSVHeap;
    ComPtr<ID3D12DescriptorHeap> SRVHeap;
    std::array<ComPtr<ID3D12Resource>, FrameCount> RenderTargets;
    ComPtr<ID3D12Resource> DepthBuffer;
    std::array<ComPtr<ID3D12CommandAllocator>, FrameCount> CommandAllocators;
    ComPtr<ID3D12GraphicsCommandList> CommandList;
    ComPtr<ID3D12CommandAllocator> UploadAllocator;
    ComPtr<ID3D12GraphicsCommandList> UploadCommandList;
    ComPtr<ID3D12Fence> Fence;
    std::array<uint64_t, FrameCount> FrameFenceValues{};
    uint64_t NextFenceValue = 0;
    HANDLE FenceEvent = nullptr;

    ComPtr<ID3D12RootSignature> SceneRootSignature;
    ComPtr<ID3D12PipelineState> ScenePipeline;
    ComPtr<ID3D12RootSignature> UIRootSignature;
    ComPtr<ID3D12PipelineState> UIPipeline;
    std::array<FrameUploadData, FrameCount> FrameUploads;

    GPUTexture WhiteTexture;
    GPUTexture FontAtlas;
    std::array<Glyph, 128> Glyphs{};
    float FontAscender = 48.0f;
    float FontDescender = 0.0f;
    std::unordered_map<const Texture*, GPUTexture> Textures;
    std::unordered_map<const Mesh*, GPUMesh> Meshes;
    std::vector<ComPtr<ID3D12Resource>> RetiredResources;
    std::vector<UIVertex> PendingUIVertices;
    uint32_t NextDescriptor = 0;

    D3D12_VIEWPORT Viewport{};
    D3D12_RECT ScissorRect{};
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t FrameIndex = 0;
    uint32_t RTVDescriptorSize = 0;
    uint32_t SRVDescriptorSize = 0;
    bool TearingSupported = false;
    bool FrameStarted = false;
    bool SceneRendererInitialized = false;
    bool Initialized = false;
  } s_Data;

  [[noreturn]] void ThrowHRESULT(HRESULT result, const char* operation)
  {
    std::ostringstream message;
    message << operation << " failed (HRESULT 0x" << std::hex << std::uppercase
            << static_cast<uint32_t>(result) << ')';
    throw std::runtime_error(message.str());
  }

  void CheckHRESULT(HRESULT result, const char* operation)
  {
    if (FAILED(result)) ThrowHRESULT(result, operation);
  }

  std::string WideToUTF8(const wchar_t* value)
  {
    if (!value || *value == L'\0') return {};
    const int requiredSize = WideCharToMultiByte(
      CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (requiredSize <= 1) return {};
    std::string result(static_cast<size_t>(requiredSize), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), requiredSize, nullptr, nullptr);
    result.resize(static_cast<size_t>(requiredSize - 1));
    return result;
  }

  uint64_t AlignUp(uint64_t value, uint64_t alignment)
  {
    return (value + alignment - 1) & ~(alignment - 1);
  }

  D3D12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE type)
  {
    D3D12_HEAP_PROPERTIES properties{};
    properties.Type = type;
    properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;
    return properties;
  }

  D3D12_RESOURCE_DESC BufferDescription(uint64_t size)
  {
    D3D12_RESOURCE_DESC description{};
    description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    description.Width = size;
    description.Height = 1;
    description.DepthOrArraySize = 1;
    description.MipLevels = 1;
    description.Format = DXGI_FORMAT_UNKNOWN;
    description.SampleDesc.Count = 1;
    description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    return description;
  }

  ComPtr<ID3D12Resource> CreateUploadBuffer(uint64_t size, uint8_t** mappedData = nullptr)
  {
    ComPtr<ID3D12Resource> resource;
    const D3D12_HEAP_PROPERTIES heap = HeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_RESOURCE_DESC description = BufferDescription(size);
    CheckHRESULT(s_Data.Device->CreateCommittedResource(
                   &heap, D3D12_HEAP_FLAG_NONE, &description,
                   D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                   IID_PPV_ARGS(&resource)),
                 "ID3D12Device::CreateCommittedResource (upload buffer)");
    if (mappedData)
    {
      const D3D12_RANGE noRead{0, 0};
      CheckHRESULT(resource->Map(0, &noRead, reinterpret_cast<void**>(mappedData)),
                   "ID3D12Resource::Map");
    }
    return resource;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(uint32_t index)
  {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = s_Data.RTVHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * s_Data.RTVDescriptorSize;
    return handle;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle(uint32_t index)
  {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = s_Data.SRVHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * s_Data.SRVDescriptorSize;
    return handle;
  }

  D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(uint32_t index)
  {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = s_Data.SRVHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(index) * s_Data.SRVDescriptorSize;
    return handle;
  }

  uint32_t AllocateDescriptor()
  {
    if (s_Data.NextDescriptor >= MaxDescriptors)
      throw std::runtime_error("DX12 SRV descriptor heap is full");
    return s_Data.NextDescriptor++;
  }

  void UpdateViewport(uint32_t width, uint32_t height)
  {
    s_Data.Viewport = {0.0f, 0.0f, static_cast<float>(width),
                      static_cast<float>(height), 0.0f, 1.0f};
    s_Data.ScissorRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
  }

  void CreateRenderTargets()
  {
    for (uint32_t i = 0; i < FrameCount; ++i)
    {
      CheckHRESULT(s_Data.SwapChain->GetBuffer(i, IID_PPV_ARGS(&s_Data.RenderTargets[i])),
                   "IDXGISwapChain::GetBuffer");
      s_Data.Device->CreateRenderTargetView(s_Data.RenderTargets[i].Get(), nullptr, GetRTVHandle(i));
    }
  }

  void CreateDepthBuffer(uint32_t width, uint32_t height)
  {
    D3D12_RESOURCE_DESC depthDesc{};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DepthFormat;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DepthFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;
    const D3D12_HEAP_PROPERTIES heap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    CheckHRESULT(s_Data.Device->CreateCommittedResource(
                   &heap, D3D12_HEAP_FLAG_NONE, &depthDesc,
                   D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
                   IID_PPV_ARGS(&s_Data.DepthBuffer)),
                 "ID3D12Device::CreateCommittedResource (depth buffer)");

    D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
    viewDesc.Format = DepthFormat;
    viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    s_Data.Device->CreateDepthStencilView(
      s_Data.DepthBuffer.Get(), &viewDesc, s_Data.DSVHeap->GetCPUDescriptorHandleForHeapStart());
  }

  void ReleaseRenderTargets()
  {
    for (auto& renderTarget : s_Data.RenderTargets) renderTarget.Reset();
    s_Data.DepthBuffer.Reset();
  }

  void WaitForGPU()
  {
    if (!s_Data.CommandQueue || !s_Data.Fence || !s_Data.FenceEvent) return;
    const uint64_t fenceValue = ++s_Data.NextFenceValue;
    CheckHRESULT(s_Data.CommandQueue->Signal(s_Data.Fence.Get(), fenceValue),
                 "ID3D12CommandQueue::Signal");
    if (s_Data.Fence->GetCompletedValue() < fenceValue)
    {
      CheckHRESULT(s_Data.Fence->SetEventOnCompletion(fenceValue, s_Data.FenceEvent),
                   "ID3D12Fence::SetEventOnCompletion");
      WaitForSingleObject(s_Data.FenceEvent, INFINITE);
    }
  }

  void WaitForFrame(uint32_t frameIndex)
  {
    const uint64_t fenceValue = s_Data.FrameFenceValues[frameIndex];
    if (fenceValue == 0 || s_Data.Fence->GetCompletedValue() >= fenceValue) return;
    CheckHRESULT(s_Data.Fence->SetEventOnCompletion(fenceValue, s_Data.FenceEvent),
                 "ID3D12Fence::SetEventOnCompletion");
    WaitForSingleObject(s_Data.FenceEvent, INFINITE);
  }

  ComPtr<ID3DBlob> CompileShader(const char* source, const char* entryPoint, const char* target)
  {
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    ComPtr<ID3DBlob> shader;
    ComPtr<ID3DBlob> errors;
    const HRESULT result = D3DCompile(source, std::strlen(source), "GABGL.DX12.hlsl", nullptr,
                                     nullptr, entryPoint, target, flags, 0, &shader, &errors);
    if (FAILED(result))
    {
      if (errors)
        GABGL_ERROR("DX12 shader compilation failed: {}",
                    static_cast<const char*>(errors->GetBufferPointer()));
      ThrowHRESULT(result, "D3DCompile");
    }
    return shader;
  }

  ComPtr<ID3D12RootSignature> CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& description)
  {
    ComPtr<ID3DBlob> serialized;
    ComPtr<ID3DBlob> errors;
    const HRESULT result = D3D12SerializeRootSignature(
      &description, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors);
    if (FAILED(result))
    {
      if (errors)
        GABGL_ERROR("DX12 root signature creation failed: {}",
                    static_cast<const char*>(errors->GetBufferPointer()));
      ThrowHRESULT(result, "D3D12SerializeRootSignature");
    }
    ComPtr<ID3D12RootSignature> rootSignature;
    CheckHRESULT(s_Data.Device->CreateRootSignature(
                   0, serialized->GetBufferPointer(), serialized->GetBufferSize(),
                   IID_PPV_ARGS(&rootSignature)),
                 "ID3D12Device::CreateRootSignature");
    return rootSignature;
  }

  D3D12_RASTERIZER_DESC DefaultRasterizer()
  {
    D3D12_RASTERIZER_DESC state{};
    state.FillMode = D3D12_FILL_MODE_SOLID;
    state.CullMode = D3D12_CULL_MODE_NONE;
    state.FrontCounterClockwise = FALSE;
    state.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    state.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    state.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    state.DepthClipEnable = TRUE;
    return state;
  }

  D3D12_BLEND_DESC DefaultBlendState()
  {
    D3D12_BLEND_DESC state{};
    state.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    return state;
  }

  void CreateScenePipeline()
  {
    static constexpr const char* SceneShader = R"(
#define MAX_BONES 100

cbuffer SceneConstants : register(b0)
{
  float4x4 ViewProjection;
  float4x4 Model;
  float4x4 Bones[MAX_BONES];
  float4 LightDirection;
  float4 LightColor;
  float4 CameraPosition;
};

Texture2D DiffuseTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct VSInput
{
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
  int4 boneIds : BONEIDS;
  float4 weights : BONEWEIGHTS;
};

struct PSInput
{
  float4 position : SV_POSITION;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
  float4x4 skin = float4x4(
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1);
  if (CameraPosition.w > 0.5f)
  {
    skin = float4x4(
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0);
    float totalWeight = 0.0f;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
      if (input.boneIds[i] >= 0 && input.boneIds[i] < MAX_BONES && input.weights[i] > 0.0f)
      {
        skin += Bones[input.boneIds[i]] * input.weights[i];
        totalWeight += input.weights[i];
      }
    }
    if (totalWeight < 0.00001f)
      skin = float4x4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
    else
      skin /= totalWeight;
  }

  const float4 localPosition = mul(skin, float4(input.position, 1.0f));
  const float4 worldPosition = mul(Model, localPosition);
  PSInput output;
  output.position = mul(ViewProjection, worldPosition);
  output.normal = normalize(mul((float3x3)Model, mul((float3x3)skin, input.normal)));
  output.uv = input.uv;
  return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
  const float4 albedo = DiffuseTexture.Sample(LinearSampler, input.uv);
  const float3 normal = normalize(input.normal);
  const float3 toLight = normalize(-LightDirection.xyz);
  const float diffuse = saturate(dot(normal, toLight));
  const float lighting = 0.18f + diffuse * 0.82f;
  return float4(albedo.rgb * LightColor.rgb * lighting, albedo.a);
}
)";

    const auto vertexShader = CompileShader(SceneShader, "VSMain", "vs_5_1");
    const auto pixelShader = CompileShader(SceneShader, "PSMain", "ps_5_1");

    D3D12_DESCRIPTOR_RANGE textureRange{};
    textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    textureRange.NumDescriptors = 1;
    textureRange.BaseShaderRegister = 0;
    textureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    std::array<D3D12_ROOT_PARAMETER, 2> parameters{};
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameters[0].Descriptor.ShaderRegister = 0;
    parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameters[1].DescriptorTable.NumDescriptorRanges = 1;
    parameters[1].DescriptorTable.pDescriptorRanges = &textureRange;
    parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootDesc{};
    rootDesc.NumParameters = static_cast<UINT>(parameters.size());
    rootDesc.pParameters = parameters.data();
    rootDesc.NumStaticSamplers = 1;
    rootDesc.pStaticSamplers = &sampler;
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    s_Data.SceneRootSignature = CreateRootSignature(rootDesc);

    static constexpr D3D12_INPUT_ELEMENT_DESC InputLayout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Position),
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Normal),
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, TexCoords),
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, offsetof(Vertex, m_BoneIDs),
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, m_Weights),
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};
    pipeline.pRootSignature = s_Data.SceneRootSignature.Get();
    pipeline.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    pipeline.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    pipeline.BlendState = DefaultBlendState();
    pipeline.SampleMask = std::numeric_limits<UINT>::max();
    pipeline.RasterizerState = DefaultRasterizer();
    pipeline.DepthStencilState.DepthEnable = TRUE;
    pipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    pipeline.DepthStencilState.StencilEnable = FALSE;
    pipeline.InputLayout = {InputLayout, static_cast<UINT>(std::size(InputLayout))};
    pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline.NumRenderTargets = 1;
    pipeline.RTVFormats[0] = BackBufferFormat;
    pipeline.DSVFormat = DepthFormat;
    pipeline.SampleDesc.Count = 1;
    CheckHRESULT(s_Data.Device->CreateGraphicsPipelineState(
                   &pipeline, IID_PPV_ARGS(&s_Data.ScenePipeline)),
                 "ID3D12Device::CreateGraphicsPipelineState (scene)");
  }

  void CreateUIPipeline()
  {
    static constexpr const char* UIShader = R"(
cbuffer UIConstants : register(b0)
{
  float2 ScreenSize;
};

Texture2D FontAtlas : register(t0);
SamplerState FontSampler : register(s0);

struct VSInput
{
  float2 position : POSITION;
  float4 color : COLOR;
  float2 uv : TEXCOORD;
};

struct PSInput
{
  float4 position : SV_POSITION;
  float4 color : COLOR;
  float2 uv : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
  PSInput output;
  const float2 safeSize = max(ScreenSize, float2(1.0f, 1.0f));
  output.position = float4(input.position.x / safeSize.x * 2.0f - 1.0f,
                           input.position.y / safeSize.y * 2.0f - 1.0f,
                           0.0f, 1.0f);
  output.color = input.color;
  output.uv = input.uv;
  return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
  const float coverage = FontAtlas.Sample(FontSampler, input.uv).r;
  return float4(input.color.rgb, input.color.a * coverage);
}
)";

    const auto vertexShader = CompileShader(UIShader, "VSMain", "vs_5_1");
    const auto pixelShader = CompileShader(UIShader, "PSMain", "ps_5_1");

    D3D12_DESCRIPTOR_RANGE textureRange{};
    textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    textureRange.NumDescriptors = 1;
    textureRange.BaseShaderRegister = 0;
    textureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    std::array<D3D12_ROOT_PARAMETER, 2> parameters{};
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[0].Constants.ShaderRegister = 0;
    parameters[0].Constants.Num32BitValues = 2;
    parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameters[1].DescriptorTable.NumDescriptorRanges = 1;
    parameters[1].DescriptorTable.pDescriptorRanges = &textureRange;
    parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootDesc{};
    rootDesc.NumParameters = static_cast<UINT>(parameters.size());
    rootDesc.pParameters = parameters.data();
    rootDesc.NumStaticSamplers = 1;
    rootDesc.pStaticSamplers = &sampler;
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    s_Data.UIRootSignature = CreateRootSignature(rootDesc);

    static constexpr D3D12_INPUT_ELEMENT_DESC InputLayout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(UIVertex, Position),
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(UIVertex, Color),
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(UIVertex, UV),
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};
    pipeline.pRootSignature = s_Data.UIRootSignature.Get();
    pipeline.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    pipeline.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    pipeline.BlendState = DefaultBlendState();
    auto& blend = pipeline.BlendState.RenderTarget[0];
    blend.BlendEnable = TRUE;
    blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend.BlendOp = D3D12_BLEND_OP_ADD;
    blend.SrcBlendAlpha = D3D12_BLEND_ONE;
    blend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    pipeline.SampleMask = std::numeric_limits<UINT>::max();
    pipeline.RasterizerState = DefaultRasterizer();
    pipeline.DepthStencilState.DepthEnable = FALSE;
    pipeline.DepthStencilState.StencilEnable = FALSE;
    pipeline.InputLayout = {InputLayout, static_cast<UINT>(std::size(InputLayout))};
    pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline.NumRenderTargets = 1;
    pipeline.RTVFormats[0] = BackBufferFormat;
    pipeline.SampleDesc.Count = 1;
    CheckHRESULT(s_Data.Device->CreateGraphicsPipelineState(
                   &pipeline, IID_PPV_ARGS(&s_Data.UIPipeline)),
                 "ID3D12Device::CreateGraphicsPipelineState (UI)");
  }

  GPUTexture UploadTextureBytes(const uint8_t* pixels, uint32_t width, uint32_t height,
                                DXGI_FORMAT format, uint32_t bytesPerPixel)
  {
    if (!pixels || width == 0 || height == 0)
      return s_Data.WhiteTexture;

    GPUTexture texture;
    texture.DescriptorIndex = AllocateDescriptor();

    D3D12_RESOURCE_DESC textureDesc{};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    const D3D12_HEAP_PROPERTIES defaultHeap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    CheckHRESULT(s_Data.Device->CreateCommittedResource(
                   &defaultHeap, D3D12_HEAP_FLAG_NONE, &textureDesc,
                   D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                   IID_PPV_ARGS(&texture.Resource)),
                 "ID3D12Device::CreateCommittedResource (texture)");

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT rowCount = 0;
    UINT64 rowSize = 0;
    UINT64 uploadSize = 0;
    s_Data.Device->GetCopyableFootprints(
      &textureDesc, 0, 1, 0, &footprint, &rowCount, &rowSize, &uploadSize);
    ComPtr<ID3D12Resource> upload = CreateUploadBuffer(uploadSize);
    uint8_t* mapped = nullptr;
    const D3D12_RANGE noRead{0, 0};
    CheckHRESULT(upload->Map(0, &noRead, reinterpret_cast<void**>(&mapped)),
                 "ID3D12Resource::Map (texture upload)");
    const uint64_t sourcePitch = static_cast<uint64_t>(width) * bytesPerPixel;
    for (uint32_t row = 0; row < height; ++row)
    {
      std::memcpy(mapped + footprint.Offset + static_cast<uint64_t>(row) * footprint.Footprint.RowPitch,
                  pixels + static_cast<uint64_t>(row) * sourcePitch,
                  static_cast<size_t>(sourcePitch));
    }
    upload->Unmap(0, nullptr);

    CheckHRESULT(s_Data.UploadAllocator->Reset(), "ID3D12CommandAllocator::Reset (upload)");
    CheckHRESULT(s_Data.UploadCommandList->Reset(s_Data.UploadAllocator.Get(), nullptr),
                 "ID3D12GraphicsCommandList::Reset (upload)");

    D3D12_TEXTURE_COPY_LOCATION destination{};
    destination.pResource = texture.Resource.Get();
    destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destination.SubresourceIndex = 0;
    D3D12_TEXTURE_COPY_LOCATION source{};
    source.pResource = upload.Get();
    source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    source.PlacedFootprint = footprint;
    s_Data.UploadCommandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture.Resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    s_Data.UploadCommandList->ResourceBarrier(1, &barrier);
    CheckHRESULT(s_Data.UploadCommandList->Close(), "ID3D12GraphicsCommandList::Close (upload)");
    ID3D12CommandList* commandLists[] = {s_Data.UploadCommandList.Get()};
    s_Data.CommandQueue->ExecuteCommandLists(1, commandLists);
    WaitForGPU();

    D3D12_SHADER_RESOURCE_VIEW_DESC view{};
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Format = format;
    view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    view.Texture2D.MipLevels = 1;
    s_Data.Device->CreateShaderResourceView(
      texture.Resource.Get(), &view, GetSRVCPUHandle(texture.DescriptorIndex));
    return texture;
  }

  uint32_t GetOrCreateModelTexture(const std::shared_ptr<Texture>& source)
  {
    if (!source) return s_Data.WhiteTexture.DescriptorIndex;
    if (const auto existing = s_Data.Textures.find(source.get()); existing != s_Data.Textures.end())
      return existing->second.DescriptorIndex;

    const uint8_t* raw = source->GetRawData();
    const uint32_t width = source->GetWidth();
    const uint32_t height = source->GetHeight();
    std::vector<uint8_t> rgba;

    if (source->IsUnCompressed())
    {
      const aiTexture* embedded = source->GetEmbeddedTexture();
      if (embedded && embedded->pcData && embedded->mHeight > 0)
      {
        rgba.resize(static_cast<size_t>(width) * height * 4);
        for (size_t i = 0; i < static_cast<size_t>(width) * height; ++i)
        {
          rgba[i * 4 + 0] = embedded->pcData[i].r;
          rgba[i * 4 + 1] = embedded->pcData[i].g;
          rgba[i * 4 + 2] = embedded->pcData[i].b;
          rgba[i * 4 + 3] = embedded->pcData[i].a;
        }
        raw = rgba.data();
      }
    }
    else if (raw && source->GetDataFormat() != GL_RGBA)
    {
      const uint32_t channels = source->GetDataFormat() == GL_RGB ? 3u : 1u;
      rgba.resize(static_cast<size_t>(width) * height * 4);
      for (size_t i = 0; i < static_cast<size_t>(width) * height; ++i)
      {
        const uint8_t r = raw[i * channels];
        rgba[i * 4 + 0] = r;
        rgba[i * 4 + 1] = channels > 1 ? raw[i * channels + 1] : r;
        rgba[i * 4 + 2] = channels > 1 ? raw[i * channels + 2] : r;
        rgba[i * 4 + 3] = 255;
      }
      raw = rgba.data();
    }

    if (!raw || width == 0 || height == 0)
      return s_Data.WhiteTexture.DescriptorIndex;

    GPUTexture texture = UploadTextureBytes(raw, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 4);
    const uint32_t descriptorIndex = texture.DescriptorIndex;
    s_Data.Textures.emplace(source.get(), std::move(texture));
    return descriptorIndex;
  }

  void CreateFontAtlas()
  {
    std::vector<uint8_t> atlas(static_cast<size_t>(FontAtlasWidth) * FontAtlasHeight, 0);
    atlas[0] = 255;

    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0)
      throw std::runtime_error("Could not initialize FreeType for DX12");

    std::filesystem::path fontPath = "../res/fonts/dpcomic.ttf";
    if (!std::filesystem::exists(fontPath)) fontPath = "res/fonts/dpcomic.ttf";
    FT_Face face = nullptr;
    if (FT_New_Face(library, fontPath.string().c_str(), 0, &face) != 0)
    {
      FT_Done_FreeType(library);
      throw std::runtime_error("Could not load DX12 font atlas source");
    }
    FT_Set_Pixel_Sizes(face, 0, 48);
    s_Data.FontAscender = static_cast<float>(face->size->metrics.ascender) / 64.0f;
    s_Data.FontDescender = static_cast<float>(-face->size->metrics.descender) / 64.0f;

    uint32_t x = 2;
    uint32_t y = 2;
    uint32_t rowHeight = 0;
    for (uint32_t character = 32; character < 127; ++character)
    {
      if (FT_Load_Char(face, character, FT_LOAD_RENDER) != 0) continue;
      const FT_Bitmap& bitmap = face->glyph->bitmap;
      if (x + bitmap.width + 2 >= FontAtlasWidth)
      {
        x = 2;
        y += rowHeight + 2;
        rowHeight = 0;
      }
      if (y + bitmap.rows + 2 >= FontAtlasHeight) break;

      for (uint32_t row = 0; row < bitmap.rows; ++row)
      {
        std::memcpy(atlas.data() + static_cast<size_t>(y + row) * FontAtlasWidth + x,
                    bitmap.buffer + static_cast<size_t>(row) * std::abs(bitmap.pitch),
                    bitmap.width);
      }

      Glyph& glyph = s_Data.Glyphs[character];
      glyph.UVTopLeft = {static_cast<float>(x) / FontAtlasWidth,
                         static_cast<float>(y) / FontAtlasHeight};
      glyph.UVBottomRight = {static_cast<float>(x + bitmap.width) / FontAtlasWidth,
                             static_cast<float>(y + bitmap.rows) / FontAtlasHeight};
      glyph.Size = {static_cast<int>(bitmap.width), static_cast<int>(bitmap.rows)};
      glyph.Bearing = {face->glyph->bitmap_left, face->glyph->bitmap_top};
      glyph.Advance = static_cast<uint32_t>(face->glyph->advance.x);
      x += bitmap.width + 2;
      rowHeight = std::max(rowHeight, bitmap.rows);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);
    s_Data.FontAtlas = UploadTextureBytes(
      atlas.data(), FontAtlasWidth, FontAtlasHeight, DXGI_FORMAT_R8_UNORM, 1);
  }

  D3D12_GPU_VIRTUAL_ADDRESS UploadSceneConstants(const SceneConstants& constants)
  {
    auto& frame = s_Data.FrameUploads[s_Data.FrameIndex];
    const uint64_t offset = AlignUp(frame.ConstantsOffset, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    if (offset + sizeof(SceneConstants) > ConstantBufferBytes)
      throw std::runtime_error("DX12 per-frame constant buffer exhausted");
    std::memcpy(frame.ConstantsMapped + offset, &constants, sizeof(SceneConstants));
    frame.ConstantsOffset = offset + sizeof(SceneConstants);
    return frame.Constants->GetGPUVirtualAddress() + offset;
  }

  glm::mat4 DirectXViewProjection()
  {
    glm::mat4 correction(1.0f);
    correction[2][2] = 0.5f;
    correction[3][2] = 0.5f;
    return correction * Camera::GetViewProjection();
  }

  void DrawModels()
  {
    if (s_Data.Meshes.empty()) return;

    glm::vec3 lightDirection(0.4f, -1.0f, 0.3f);
    glm::vec3 lightColor(1.0f);
    for (const SceneLight& light : SceneManager::GetLights())
    {
      if (light.type == LightType::DIRECT)
      {
        lightDirection = light.rotation;
        lightColor = light.color;
        break;
      }
    }

    ID3D12DescriptorHeap* descriptorHeaps[] = {s_Data.SRVHeap.Get()};
    s_Data.CommandList->SetDescriptorHeaps(1, descriptorHeaps);
    s_Data.CommandList->SetPipelineState(s_Data.ScenePipeline.Get());
    s_Data.CommandList->SetGraphicsRootSignature(s_Data.SceneRootSignature.Get());
    s_Data.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    SceneConstants constants;
    constants.ViewProjection = DirectXViewProjection();
    constants.LightDirection = glm::vec4(lightDirection, 0.0f);
    constants.LightColor = glm::vec4(lightColor, 1.0f);
    constants.CameraPosition = glm::vec4(Camera::GetPosition(), 0.0f);
    constants.Bones.fill(glm::mat4(1.0f));

    for (const std::string& modelName : ModelManager::GetModelNames())
    {
      const std::shared_ptr<Model> model = ModelManager::GetModel(modelName);
      if (!model || !model->m_IsRendered || model->GetPhysXMeshType() == MeshType::CONVEXMESH)
        continue;

      const bool animated = model->IsAnimated();
      constants.CameraPosition.w = animated ? 1.0f : 0.0f;
      constants.Bones.fill(glm::mat4(1.0f));
      if (animated)
      {
        const auto& bones = model->GetFinalBoneMatrices();
        const size_t count = std::min(bones.size(), constants.Bones.size());
        std::copy_n(bones.begin(), count, constants.Bones.begin());
      }

      const auto& instances = model->m_InstanceTransforms;
      if (instances.empty()) continue;
      for (const glm::mat4& transform : instances)
      {
        constants.Model = transform;
        for (const Mesh& mesh : model->GetMeshes())
        {
          const auto gpuIt = s_Data.Meshes.find(&mesh);
          if (gpuIt == s_Data.Meshes.end()) continue;
          const GPUMesh& gpuMesh = gpuIt->second;
          s_Data.CommandList->SetGraphicsRootConstantBufferView(0, UploadSceneConstants(constants));
          s_Data.CommandList->SetGraphicsRootDescriptorTable(
            1, GetSRVGPUHandle(gpuMesh.TextureDescriptorIndex));
          s_Data.CommandList->IASetVertexBuffers(0, 1, &gpuMesh.VertexView);
          s_Data.CommandList->IASetIndexBuffer(&gpuMesh.IndexView);
          s_Data.CommandList->DrawIndexedInstanced(gpuMesh.IndexCount, 1, 0, 0, 0);
        }
      }
    }
  }

  void AppendUIQuad(const glm::vec2& bottomLeft, const glm::vec2& topRight,
                    const glm::vec4& color, const glm::vec2& uvTopLeft,
                    const glm::vec2& uvBottomRight)
  {
    const UIVertex bottomLeftVertex{bottomLeft, color, {uvTopLeft.x, uvBottomRight.y}};
    const UIVertex bottomRightVertex{{topRight.x, bottomLeft.y}, color, uvBottomRight};
    const UIVertex topRightVertex{topRight, color, {uvBottomRight.x, uvTopLeft.y}};
    const UIVertex topLeftVertex{{bottomLeft.x, topRight.y}, color, uvTopLeft};
    s_Data.PendingUIVertices.insert(s_Data.PendingUIVertices.end(), {
      bottomLeftVertex, bottomRightVertex, topRightVertex,
      topRightVertex, topLeftVertex, bottomLeftVertex
    });
  }

  void RetireSceneResources()
  {
    for (auto& [key, mesh] : s_Data.Meshes)
    {
      if (mesh.VertexBuffer) s_Data.RetiredResources.push_back(std::move(mesh.VertexBuffer));
      if (mesh.IndexBuffer) s_Data.RetiredResources.push_back(std::move(mesh.IndexBuffer));
    }
    for (auto& [key, texture] : s_Data.Textures)
      if (texture.Resource) s_Data.RetiredResources.push_back(std::move(texture.Resource));
    s_Data.Meshes.clear();
    s_Data.Textures.clear();
  }

  void ResetState()
  {
    s_Data = {};
  }
}

bool DirectX12Renderer::Init(void* nativeWindow, uint32_t width, uint32_t height)
{
  if (s_Data.Initialized) return true;
  if (!nativeWindow || width == 0 || height == 0)
  {
    GABGL_ERROR("Cannot initialize DirectX 12 without a valid window and dimensions");
    return false;
  }

  try
  {
    UINT factoryFlags = 0;
#ifdef DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
      debugController->EnableDebugLayer();
      factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
    else
      GABGL_WARN("DirectX 12 debug layer is not available");
#endif

    CheckHRESULT(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&s_Data.Factory)),
                 "CreateDXGIFactory2");
    ComPtr<IDXGIFactory5> factory5;
    if (SUCCEEDED(s_Data.Factory.As(&factory5)))
    {
      BOOL allowTearing = FALSE;
      if (SUCCEEDED(factory5->CheckFeatureSupport(
            DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
        s_Data.TearingSupported = allowTearing == TRUE;
    }

    for (uint32_t adapterIndex = 0;; ++adapterIndex)
    {
      ComPtr<IDXGIAdapter1> adapter;
      const HRESULT result = s_Data.Factory->EnumAdapterByGpuPreference(
        adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
      if (result == DXGI_ERROR_NOT_FOUND) break;
      CheckHRESULT(result, "IDXGIFactory6::EnumAdapterByGpuPreference");
      DXGI_ADAPTER_DESC1 description{};
      adapter->GetDesc1(&description);
      if ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) continue;
      if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                      IID_PPV_ARGS(&s_Data.Device))))
      {
        s_Data.Adapter = adapter;
        break;
      }
    }
    if (!s_Data.Device)
    {
      ComPtr<IDXGIAdapter> warp;
      CheckHRESULT(s_Data.Factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)),
                   "IDXGIFactory::EnumWarpAdapter");
      CheckHRESULT(warp.As(&s_Data.Adapter), "Query WARP adapter");
      CheckHRESULT(D3D12CreateDevice(s_Data.Adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                     IID_PPV_ARGS(&s_Data.Device)),
                   "D3D12CreateDevice (WARP)");
      GABGL_WARN("No hardware DX12 adapter found; using WARP software renderer");
    }

    DXGI_ADAPTER_DESC1 selectedAdapter{};
    s_Data.Adapter->GetDesc1(&selectedAdapter);
    GABGL_INFO("DirectX 12 adapter: {}", WideToUTF8(selectedAdapter.Description));

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    CheckHRESULT(s_Data.Device->CreateCommandQueue(
                   &queueDesc, IID_PPV_ARGS(&s_Data.CommandQueue)),
                 "ID3D12Device::CreateCommandQueue");

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = BackBufferFormat;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    if (s_Data.TearingSupported) swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    ComPtr<IDXGISwapChain1> swapChain;
    CheckHRESULT(s_Data.Factory->CreateSwapChainForHwnd(
                   s_Data.CommandQueue.Get(), static_cast<HWND>(nativeWindow),
                   &swapChainDesc, nullptr, nullptr, &swapChain),
                 "IDXGIFactory::CreateSwapChainForHwnd");
    CheckHRESULT(s_Data.Factory->MakeWindowAssociation(
                   static_cast<HWND>(nativeWindow), DXGI_MWA_NO_ALT_ENTER),
                 "IDXGIFactory::MakeWindowAssociation");
    CheckHRESULT(swapChain.As(&s_Data.SwapChain), "Query IDXGISwapChain3");
    s_Data.FrameIndex = s_Data.SwapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.NumDescriptors = FrameCount;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    CheckHRESULT(s_Data.Device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&s_Data.RTVHeap)),
                 "ID3D12Device::CreateDescriptorHeap (RTV)");
    s_Data.RTVDescriptorSize = s_Data.Device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
    dsvDesc.NumDescriptors = 1;
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    CheckHRESULT(s_Data.Device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&s_Data.DSVHeap)),
                 "ID3D12Device::CreateDescriptorHeap (DSV)");
    CreateRenderTargets();
    CreateDepthBuffer(width, height);

    for (auto& allocator : s_Data.CommandAllocators)
      CheckHRESULT(s_Data.Device->CreateCommandAllocator(
                     D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)),
                   "ID3D12Device::CreateCommandAllocator");
    CheckHRESULT(s_Data.Device->CreateCommandList(
                   0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                   s_Data.CommandAllocators[s_Data.FrameIndex].Get(), nullptr,
                   IID_PPV_ARGS(&s_Data.CommandList)),
                 "ID3D12Device::CreateCommandList");
    CheckHRESULT(s_Data.CommandList->Close(), "ID3D12GraphicsCommandList::Close");

    CheckHRESULT(s_Data.Device->CreateCommandAllocator(
                   D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s_Data.UploadAllocator)),
                 "ID3D12Device::CreateCommandAllocator (upload)");
    CheckHRESULT(s_Data.Device->CreateCommandList(
                   0, D3D12_COMMAND_LIST_TYPE_DIRECT, s_Data.UploadAllocator.Get(), nullptr,
                   IID_PPV_ARGS(&s_Data.UploadCommandList)),
                 "ID3D12Device::CreateCommandList (upload)");
    CheckHRESULT(s_Data.UploadCommandList->Close(), "ID3D12GraphicsCommandList::Close (upload)");

    CheckHRESULT(s_Data.Device->CreateFence(
                   0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&s_Data.Fence)),
                 "ID3D12Device::CreateFence");
    s_Data.FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!s_Data.FenceEvent) ThrowHRESULT(HRESULT_FROM_WIN32(GetLastError()), "CreateEvent");

    UpdateViewport(width, height);
    s_Data.Width = width;
    s_Data.Height = height;
    s_Data.Initialized = true;
    GABGL_INFO("DirectX 12 initialized ({}x{}, tearing: {})",
               width, height, s_Data.TearingSupported ? "supported" : "unavailable");
    return true;
  }
  catch (const std::exception& error)
  {
    GABGL_ERROR("DirectX 12 initialization failed: {}", error.what());
    Shutdown();
    return false;
  }
}

bool DirectX12Renderer::InitSceneRenderer()
{
  if (!s_Data.Initialized) return false;
  if (s_Data.SceneRendererInitialized) return true;
  try
  {
    D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
    srvDesc.NumDescriptors = MaxDescriptors;
    srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    CheckHRESULT(s_Data.Device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&s_Data.SRVHeap)),
                 "ID3D12Device::CreateDescriptorHeap (SRV)");
    s_Data.SRVDescriptorSize = s_Data.Device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    for (auto& frame : s_Data.FrameUploads)
    {
      frame.Constants = CreateUploadBuffer(ConstantBufferBytes, &frame.ConstantsMapped);
      frame.UIVertices = CreateUploadBuffer(UIVertexBufferBytes, &frame.UIVerticesMapped);
    }

    CreateScenePipeline();
    CreateUIPipeline();
    constexpr std::array<uint8_t, 4> whitePixel = {255, 255, 255, 255};
    s_Data.WhiteTexture = UploadTextureBytes(whitePixel.data(), 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 4);
    CreateFontAtlas();
    s_Data.PendingUIVertices.reserve(32768);
    s_Data.SceneRendererInitialized = true;
    GABGL_INFO("DirectX 12 scene renderer initialized");
    return true;
  }
  catch (const std::exception& error)
  {
    GABGL_ERROR("DirectX 12 scene renderer initialization failed: {}", error.what());
    return false;
  }
}

void DirectX12Renderer::ShutdownSceneRenderer()
{
  if (!s_Data.SceneRendererInitialized) return;
  try { WaitForGPU(); } catch (...) {}
  RetireSceneResources();
  for (auto& frame : s_Data.FrameUploads)
  {
    if (frame.Constants && frame.ConstantsMapped) frame.Constants->Unmap(0, nullptr);
    if (frame.UIVertices && frame.UIVerticesMapped) frame.UIVertices->Unmap(0, nullptr);
    frame = {};
  }
  s_Data.RetiredResources.clear();
  s_Data.WhiteTexture = {};
  s_Data.FontAtlas = {};
  s_Data.SceneRootSignature.Reset();
  s_Data.ScenePipeline.Reset();
  s_Data.UIRootSignature.Reset();
  s_Data.UIPipeline.Reset();
  s_Data.SRVHeap.Reset();
  s_Data.PendingUIVertices.clear();
  s_Data.SceneRendererInitialized = false;
}

void DirectX12Renderer::ResetSceneResources()
{
  if (s_Data.SceneRendererInitialized) RetireSceneResources();
}

bool DirectX12Renderer::UploadModel(const std::shared_ptr<Model>& model)
{
  if (!s_Data.SceneRendererInitialized || !model) return false;
  try
  {
    for (Mesh& mesh : model->GetMeshes())
    {
      if (mesh.m_Vertices.empty() || mesh.m_Indices.empty()) continue;
      GPUMesh gpuMesh;
      const uint64_t vertexBytes = mesh.m_Vertices.size() * sizeof(Vertex);
      const uint64_t indexBytes = mesh.m_Indices.size() * sizeof(uint32_t);
      gpuMesh.VertexBuffer = CreateUploadBuffer(vertexBytes);
      gpuMesh.IndexBuffer = CreateUploadBuffer(indexBytes);
      void* mapped = nullptr;
      const D3D12_RANGE noRead{0, 0};
      CheckHRESULT(gpuMesh.VertexBuffer->Map(0, &noRead, &mapped), "Map model vertex buffer");
      std::memcpy(mapped, mesh.m_Vertices.data(), static_cast<size_t>(vertexBytes));
      gpuMesh.VertexBuffer->Unmap(0, nullptr);
      CheckHRESULT(gpuMesh.IndexBuffer->Map(0, &noRead, &mapped), "Map model index buffer");
      std::memcpy(mapped, mesh.m_Indices.data(), static_cast<size_t>(indexBytes));
      gpuMesh.IndexBuffer->Unmap(0, nullptr);
      gpuMesh.VertexView = {gpuMesh.VertexBuffer->GetGPUVirtualAddress(),
                            static_cast<UINT>(vertexBytes), sizeof(Vertex)};
      gpuMesh.IndexView = {gpuMesh.IndexBuffer->GetGPUVirtualAddress(),
                           static_cast<UINT>(indexBytes), DXGI_FORMAT_R32_UINT};
      gpuMesh.IndexCount = static_cast<uint32_t>(mesh.m_Indices.size());
      gpuMesh.TextureDescriptorIndex = s_Data.WhiteTexture.DescriptorIndex;
      for (const auto& texture : mesh.m_Textures)
      {
        if (!texture) continue;
        if (texture->GetType().find("diffuse") != std::string::npos ||
            gpuMesh.TextureDescriptorIndex == s_Data.WhiteTexture.DescriptorIndex)
          gpuMesh.TextureDescriptorIndex = GetOrCreateModelTexture(texture);
        if (texture->GetType().find("diffuse") != std::string::npos) break;
      }
      s_Data.Meshes[&mesh] = std::move(gpuMesh);
    }
    return true;
  }
  catch (const std::exception& error)
  {
    GABGL_ERROR("DX12 model upload failed for '{}': {}", model->m_Name, error.what());
    return false;
  }
}

void DirectX12Renderer::Shutdown()
{
  ShutdownSceneRenderer();
  try
  {
    if (s_Data.CommandQueue && s_Data.Fence && s_Data.FenceEvent) WaitForGPU();
  }
  catch (const std::exception& error)
  {
    GABGL_ERROR("DirectX 12 shutdown synchronization failed: {}", error.what());
  }
  if (s_Data.FenceEvent)
  {
    CloseHandle(s_Data.FenceEvent);
    s_Data.FenceEvent = nullptr;
  }
  ResetState();
}

bool DirectX12Renderer::Resize(uint32_t width, uint32_t height)
{
  if (!s_Data.Initialized || width == 0 || height == 0) return false;
  if (width == s_Data.Width && height == s_Data.Height) return true;
  if (s_Data.FrameStarted) return false;
  try
  {
    WaitForGPU();
    ReleaseRenderTargets();
    s_Data.FrameFenceValues.fill(0);
    const UINT flags = s_Data.TearingSupported
      ? static_cast<UINT>(DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) : 0u;
    CheckHRESULT(s_Data.SwapChain->ResizeBuffers(
                   FrameCount, width, height, BackBufferFormat, flags),
                 "IDXGISwapChain::ResizeBuffers");
    s_Data.FrameIndex = s_Data.SwapChain->GetCurrentBackBufferIndex();
    CreateRenderTargets();
    CreateDepthBuffer(width, height);
    UpdateViewport(width, height);
    s_Data.Width = width;
    s_Data.Height = height;
    return true;
  }
  catch (const std::exception& error)
  {
    GABGL_ERROR("DirectX 12 resize failed: {}", error.what());
    return false;
  }
}

bool DirectX12Renderer::BeginFrame()
{
  if (!s_Data.Initialized || s_Data.FrameStarted) return false;
  try
  {
    WaitForFrame(s_Data.FrameIndex);
    auto& allocator = s_Data.CommandAllocators[s_Data.FrameIndex];
    CheckHRESULT(allocator->Reset(), "ID3D12CommandAllocator::Reset");
    CheckHRESULT(s_Data.CommandList->Reset(allocator.Get(), nullptr),
                 "ID3D12GraphicsCommandList::Reset");
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = s_Data.RenderTargets[s_Data.FrameIndex].Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    s_Data.CommandList->ResourceBarrier(1, &barrier);

    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = GetRTVHandle(s_Data.FrameIndex);
    const D3D12_CPU_DESCRIPTOR_HANDLE dsv = s_Data.DSVHeap->GetCPUDescriptorHandleForHeapStart();
    static constexpr float clearColor[] = {0.008f, 0.012f, 0.025f, 1.0f};
    s_Data.CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    s_Data.CommandList->ClearDepthStencilView(
      dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    s_Data.CommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    s_Data.CommandList->RSSetViewports(1, &s_Data.Viewport);
    s_Data.CommandList->RSSetScissorRects(1, &s_Data.ScissorRect);

    auto& frame = s_Data.FrameUploads[s_Data.FrameIndex];
    frame.ConstantsOffset = 0;
    frame.UIVerticesOffset = 0;
    s_Data.PendingUIVertices.clear();
    s_Data.FrameStarted = true;
    return true;
  }
  catch (const std::exception& error)
  {
    GABGL_ERROR("DirectX 12 frame start failed: {}", error.what());
    return false;
  }
}

bool DirectX12Renderer::EndFrame(bool vSync)
{
  if (!s_Data.Initialized || !s_Data.FrameStarted) return false;
  try
  {
    if (!s_Data.PendingUIVertices.empty()) EndScene();
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = s_Data.RenderTargets[s_Data.FrameIndex].Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    s_Data.CommandList->ResourceBarrier(1, &barrier);
    CheckHRESULT(s_Data.CommandList->Close(), "ID3D12GraphicsCommandList::Close");
    ID3D12CommandList* commandLists[] = {s_Data.CommandList.Get()};
    s_Data.CommandQueue->ExecuteCommandLists(1, commandLists);
    const UINT syncInterval = vSync ? 1u : 0u;
    const UINT presentFlags = !vSync && s_Data.TearingSupported
      ? static_cast<UINT>(DXGI_PRESENT_ALLOW_TEARING) : 0u;
    CheckHRESULT(s_Data.SwapChain->Present(syncInterval, presentFlags),
                 "IDXGISwapChain::Present");
    const uint64_t fenceValue = ++s_Data.NextFenceValue;
    CheckHRESULT(s_Data.CommandQueue->Signal(s_Data.Fence.Get(), fenceValue),
                 "ID3D12CommandQueue::Signal");
    s_Data.FrameFenceValues[s_Data.FrameIndex] = fenceValue;
    s_Data.FrameIndex = s_Data.SwapChain->GetCurrentBackBufferIndex();
    s_Data.FrameStarted = false;
    return true;
  }
  catch (const std::exception& error)
  {
    s_Data.FrameStarted = false;
    GABGL_ERROR("DirectX 12 presentation failed: {}", error.what());
    return false;
  }
}

void DirectX12Renderer::DrawScene(DeltaTime& dt, const std::function<void()>& sceneLogic,
                                  bool advanceSimulation)
{
  if (!s_Data.FrameStarted || !s_Data.SceneRendererInitialized) return;
  if (advanceSimulation) sceneLogic();
  if (advanceSimulation)
  {
    ModelManager::UpdateControllers(dt);
    PhysX::Simulate(dt);
    ModelManager::UpdateTransforms(dt);
    Camera::OnUpdate(dt);
    AudioManager::SetListenerLocation(Camera::GetPosition());
    AudioManager::SetListenerOrientation(Camera::GetForwardDirection(), Camera::GetUpDirection());
    AudioManager::UpdateAllMusic();
  }
  try { DrawModels(); }
  catch (const std::exception& error)
  {
    GABGL_ERROR("DirectX 12 scene rendering failed: {}", error.what());
  }
}

void DirectX12Renderer::BeginScene()
{
  s_Data.PendingUIVertices.clear();
}

void DirectX12Renderer::EndScene()
{
  if (!s_Data.FrameStarted || s_Data.PendingUIVertices.empty()) return;
  try
  {
    auto& frame = s_Data.FrameUploads[s_Data.FrameIndex];
    const uint64_t byteCount = s_Data.PendingUIVertices.size() * sizeof(UIVertex);
    const uint64_t offset = AlignUp(frame.UIVerticesOffset, alignof(UIVertex));
    if (offset + byteCount > UIVertexBufferBytes)
      throw std::runtime_error("DX12 UI vertex buffer exhausted");
    std::memcpy(frame.UIVerticesMapped + offset, s_Data.PendingUIVertices.data(),
                static_cast<size_t>(byteCount));

    D3D12_VERTEX_BUFFER_VIEW view{};
    view.BufferLocation = frame.UIVertices->GetGPUVirtualAddress() + offset;
    view.SizeInBytes = static_cast<UINT>(byteCount);
    view.StrideInBytes = sizeof(UIVertex);
    frame.UIVerticesOffset = offset + byteCount;

    ID3D12DescriptorHeap* heaps[] = {s_Data.SRVHeap.Get()};
    s_Data.CommandList->SetDescriptorHeaps(1, heaps);
    s_Data.CommandList->SetPipelineState(s_Data.UIPipeline.Get());
    s_Data.CommandList->SetGraphicsRootSignature(s_Data.UIRootSignature.Get());
    const std::array<float, 2> screenSize = {
      static_cast<float>(s_Data.Width), static_cast<float>(s_Data.Height)};
    s_Data.CommandList->SetGraphicsRoot32BitConstants(0, 2, screenSize.data(), 0);
    s_Data.CommandList->SetGraphicsRootDescriptorTable(
      1, GetSRVGPUHandle(s_Data.FontAtlas.DescriptorIndex));
    s_Data.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    s_Data.CommandList->IASetVertexBuffers(0, 1, &view);
    s_Data.CommandList->DrawInstanced(
      static_cast<UINT>(s_Data.PendingUIVertices.size()), 1, 0, 0);
    s_Data.PendingUIVertices.clear();
  }
  catch (const std::exception& error)
  {
    s_Data.PendingUIVertices.clear();
    GABGL_ERROR("DirectX 12 UI rendering failed: {}", error.what());
  }
}

void DirectX12Renderer::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
{
  static constexpr std::array<glm::vec4, 4> corners = {
    glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f), glm::vec4(0.5f, -0.5f, 0.0f, 1.0f),
    glm::vec4(0.5f, 0.5f, 0.0f, 1.0f), glm::vec4(-0.5f, 0.5f, 0.0f, 1.0f)};
  std::array<glm::vec2, 4> positions{};
  for (size_t i = 0; i < corners.size(); ++i)
    positions[i] = glm::vec2(transform * corners[i]);
  const glm::vec2 whiteUV(0.5f / FontAtlasWidth, 0.5f / FontAtlasHeight);
  const UIVertex vertices[] = {
    {positions[0], color, whiteUV}, {positions[1], color, whiteUV},
    {positions[2], color, whiteUV}, {positions[2], color, whiteUV},
    {positions[3], color, whiteUV}, {positions[0], color, whiteUV}
  };
  s_Data.PendingUIVertices.insert(s_Data.PendingUIVertices.end(),
                                  std::begin(vertices), std::end(vertices));
}

void DirectX12Renderer::DrawText(const std::string& text, const glm::vec2& position,
                                 float size, const glm::vec4& color)
{
  if (text.empty()) return;
  float textWidth = 0.0f;
  for (const unsigned char character : text)
    if (character < s_Data.Glyphs.size())
      textWidth += static_cast<float>(s_Data.Glyphs[character].Advance >> 6) * size;

  const float baselineY = (s_Data.FontDescender - s_Data.FontAscender) * size * 0.5f;
  float cursorX = -textWidth * 0.5f;
  for (const unsigned char character : text)
  {
    if (character >= s_Data.Glyphs.size()) continue;
    const Glyph& glyph = s_Data.Glyphs[character];
    const float x = position.x + cursorX + glyph.Bearing.x * size;
    const float y = position.y + baselineY + (glyph.Bearing.y - glyph.Size.y) * size;
    const float width = glyph.Size.x * size;
    const float height = glyph.Size.y * size;
    if (width > 0.0f && height > 0.0f)
      AppendUIQuad({x, y}, {x + width, y + height}, color,
                   glyph.UVTopLeft, glyph.UVBottomRight);
    cursorX += static_cast<float>(glyph.Advance >> 6) * size;
  }
}

bool DirectX12Renderer::IsInitialized()
{
  return s_Data.Initialized;
}

#else

bool DirectX12Renderer::Init(void*, uint32_t, uint32_t) { return false; }
void DirectX12Renderer::Shutdown() {}
bool DirectX12Renderer::Resize(uint32_t, uint32_t) { return false; }
bool DirectX12Renderer::BeginFrame() { return false; }
bool DirectX12Renderer::EndFrame(bool) { return false; }
bool DirectX12Renderer::IsInitialized() { return false; }
bool DirectX12Renderer::InitSceneRenderer() { return false; }
void DirectX12Renderer::ShutdownSceneRenderer() {}
void DirectX12Renderer::ResetSceneResources() {}
bool DirectX12Renderer::UploadModel(const std::shared_ptr<Model>&) { return false; }
void DirectX12Renderer::DrawScene(DeltaTime&, const std::function<void()>&, bool) {}
void DirectX12Renderer::BeginScene() {}
void DirectX12Renderer::EndScene() {}
void DirectX12Renderer::DrawQuad(const glm::mat4&, const glm::vec4&) {}
void DirectX12Renderer::DrawText(const std::string&, const glm::vec2&, float, const glm::vec4&) {}

#endif
