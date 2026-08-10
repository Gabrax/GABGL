#include "DirectX12Renderer.h"

#if defined(GABGL_ENABLE_DX12) && defined(_WIN32)

#include "AudioManager.h"
#include "Camera.h"
#include "DeltaTime.hpp"
#include "Logger.h"
#include "ModelManager.h"
#include "ParticleRenderer.h"
#include "PhysX.h"
#include "RenderBackend.h"
#include "Renderer.h"
#include "SceneManager.h"
#include "Settings.h"
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

#include <imgui.h>
#include "backends/imgui_impl_dx12.h"

#include <algorithm>
#include <array>
#include <cmath>
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
  constexpr uint32_t MaxSceneLights = 32;
  constexpr uint32_t MaxShadowedPointLights = 4;
  constexpr uint32_t PointShadowFaceCount = 6;
  constexpr float PointShadowRadius = 20.0f;
  constexpr uint64_t ConstantBufferBytes = 64ull * 1024ull * 1024ull;
  constexpr uint64_t UIVertexBufferBytes = 8ull * 1024ull * 1024ull;
  constexpr uint32_t FontAtlasWidth = 1024;
  constexpr uint32_t FontAtlasHeight = 512;
  constexpr uint32_t SceneRTVIndex = FrameCount;
  constexpr uint32_t BloomARTVIndex = FrameCount + 1;
  constexpr uint32_t BloomBRTVIndex = FrameCount + 2;
  constexpr uint32_t PostProcessRTVIndex = FrameCount + 3;
  constexpr uint32_t PointShadowRTVBaseIndex = FrameCount + 4;
  constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
  constexpr DXGI_FORMAT SceneColorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
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
    uint32_t DiffuseDescriptorIndex = 0;
    uint32_t NormalDescriptorIndex = 0;
    uint32_t SpecularDescriptorIndex = 0;
    bool HasNormalMap = false;
    bool HasSpecularMap = false;
  };

  struct alignas(256) SceneConstants
  {
    glm::mat4 ViewProjection{1.0f};
    glm::mat4 Model{1.0f};
    glm::mat4 LightViewProjection{1.0f};
    std::array<glm::mat4, MAX_BONES> Bones{};
    glm::vec4 CameraPosition{0.0f};
    glm::vec4 MaterialFlags{0.0f};
    std::array<glm::vec4, MaxSceneLights> LightPositions{};
    std::array<glm::vec4, MaxSceneLights> LightDirections{};
    std::array<glm::vec4, MaxSceneLights> LightColors{};
    std::array<glm::vec4, MaxSceneLights> LightTypes{};
    glm::uvec4 LightCount{0};
    glm::vec4 Resolution{1.0f};
  };

  struct alignas(256) ShadowConstants
  {
    glm::mat4 LightViewProjection{1.0f};
    glm::mat4 Model{1.0f};
    std::array<glm::mat4, MAX_BONES> Bones{};
    glm::vec4 Animated{0.0f};
  };

  struct alignas(256) SkyboxConstants
  {
    glm::mat4 ViewProjection{1.0f};
  };

  struct ParticleVertex
  {
    glm::vec3 Position;
    glm::vec4 Color;
    glm::vec2 LocalPosition;
    float IsSquare = 0.0f;
  };

  struct DebugLineVertex
  {
    glm::vec3 Position;
    glm::vec4 Color;
  };

  struct alignas(256) PhysicsDebugConstants
  {
    glm::mat4 ViewProjection{1.0f};
    glm::mat4 Model{1.0f};
    glm::vec4 Color{0.15f, 1.0f, 0.35f, 1.0f};
  };

  struct alignas(256) PointShadowConstants
  {
    glm::mat4 LightViewProjection{1.0f};
    glm::mat4 Model{1.0f};
    std::array<glm::mat4, MAX_BONES> Bones{};
    glm::vec4 LightPositionAndAnimated{0.0f};
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
    ComPtr<ID3D12InfoQueue> InfoQueue;
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
    ComPtr<ID3D12RootSignature> ShadowRootSignature;
    ComPtr<ID3D12PipelineState> ShadowPipeline;
    ComPtr<ID3D12RootSignature> PointShadowRootSignature;
    ComPtr<ID3D12PipelineState> PointShadowPipeline;
    ComPtr<ID3D12RootSignature> SkyboxRootSignature;
    ComPtr<ID3D12PipelineState> SkyboxPipeline;
    ComPtr<ID3D12RootSignature> ParticleRootSignature;
    ComPtr<ID3D12PipelineState> ParticlePipeline;
    ComPtr<ID3D12RootSignature> DebugLineRootSignature;
    ComPtr<ID3D12PipelineState> DebugLinePipeline;
    ComPtr<ID3D12RootSignature> PhysicsDebugRootSignature;
    ComPtr<ID3D12PipelineState> PhysicsDebugPipeline;
    ComPtr<ID3D12RootSignature> PostRootSignature;
    ComPtr<ID3D12PipelineState> BloomExtractPipeline;
    ComPtr<ID3D12PipelineState> BloomBlurPipeline;
    ComPtr<ID3D12PipelineState> CompositePipeline;
    ComPtr<ID3D12RootSignature> UIRootSignature;
    ComPtr<ID3D12PipelineState> UIPipeline;
    ComPtr<ID3D12PipelineState> SceneUIPipeline;
    std::array<FrameUploadData, FrameCount> FrameUploads;

    GPUTexture WhiteTexture;
    GPUTexture FontAtlas;
    GPUTexture SkyboxTexture;
    GPUTexture SceneColor;
    GPUTexture BloomA;
    GPUTexture BloomB;
    GPUTexture PostProcessColor;
    GPUTexture ShadowMap;
    GPUTexture PointShadowMap;
    ComPtr<ID3D12Resource> PointShadowDepth;
    ComPtr<ID3D12Resource> SkyboxVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW SkyboxVertexView{};
    glm::mat4 LightViewProjection{1.0f};
    std::array<Glyph, 128> Glyphs{};
    float FontAscender = 48.0f;
    float FontDescender = 0.0f;
    std::unordered_map<const Texture*, GPUTexture> Textures;
    std::unordered_map<const Mesh*, GPUMesh> Meshes;
    std::vector<ComPtr<ID3D12Resource>> RetiredResources;
    std::vector<UIVertex> PendingUIVertices;
    std::vector<DebugLineVertex> PendingDebugLines;
    std::vector<uint32_t> PointShadowLightIndices;
    uint32_t NextDescriptor = 0;
    uint32_t ImGuiFontDescriptor = std::numeric_limits<uint32_t>::max();
    uint32_t ShadowMapSize = 0;
    uint32_t PointShadowMapSize = 0;

    D3D12_VIEWPORT Viewport{};
    D3D12_RECT ScissorRect{};
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t FrameIndex = 0;
    uint32_t RTVDescriptorSize = 0;
    uint32_t SRVDescriptorSize = 0;
    bool TearingSupported = false;
    bool FrameStarted = false;
    bool ShadowMapReadable = false;
    bool PointShadowMapReadable = false;
    bool ImGuiInitialized = false;
    bool SceneRendererInitialized = false;
    bool UIToSceneColor = false;
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

  D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle(uint32_t index)
  {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = s_Data.DSVHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * s_Data.Device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
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

  void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
                          D3D12_RESOURCE_STATES after)
  {
    if (!resource || before == after) return;
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    s_Data.CommandList->ResourceBarrier(1, &barrier);
  }

  GPUTexture CreateColorTarget(uint32_t width, uint32_t height, uint32_t rtvIndex,
                               uint32_t descriptorIndex,
                               DXGI_FORMAT format = SceneColorFormat)
  {
    GPUTexture target;
    target.DescriptorIndex = descriptorIndex;
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_CLEAR_VALUE clear{};
    clear.Format = format;
    const D3D12_HEAP_PROPERTIES heap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    CheckHRESULT(s_Data.Device->CreateCommittedResource(
                   &heap, D3D12_HEAP_FLAG_NONE, &desc,
                   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clear,
                   IID_PPV_ARGS(&target.Resource)),
                 "Create DX12 color target");
    s_Data.Device->CreateRenderTargetView(target.Resource.Get(), nullptr, GetRTVHandle(rtvIndex));
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Format = format;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MipLevels = 1;
    s_Data.Device->CreateShaderResourceView(
      target.Resource.Get(), &srv, GetSRVCPUHandle(descriptorIndex));
    return target;
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
      s_Data.DepthBuffer.Get(), &viewDesc, GetDSVHandle(0));
  }

  void CreatePostProcessResources(uint32_t width, uint32_t height)
  {
    if (s_Data.SceneColor.DescriptorIndex == 0)
    {
      s_Data.SceneColor.DescriptorIndex = AllocateDescriptor();
      s_Data.BloomA.DescriptorIndex = AllocateDescriptor();
      s_Data.BloomB.DescriptorIndex = AllocateDescriptor();
      s_Data.PostProcessColor.DescriptorIndex = AllocateDescriptor();
    }
    const uint32_t sceneDescriptor = s_Data.SceneColor.DescriptorIndex;
    const uint32_t bloomADescriptor = s_Data.BloomA.DescriptorIndex;
    const uint32_t bloomBDescriptor = s_Data.BloomB.DescriptorIndex;
    const uint32_t postProcessDescriptor = s_Data.PostProcessColor.DescriptorIndex;
    s_Data.SceneColor = CreateColorTarget(width, height, SceneRTVIndex, sceneDescriptor);
    const uint32_t bloomWidth = std::max(1u, width / 2u);
    const uint32_t bloomHeight = std::max(1u, height / 2u);
    s_Data.BloomA = CreateColorTarget(bloomWidth, bloomHeight, BloomARTVIndex, bloomADescriptor);
    s_Data.BloomB = CreateColorTarget(bloomWidth, bloomHeight, BloomBRTVIndex, bloomBDescriptor);
    s_Data.PostProcessColor = CreateColorTarget(
      width, height, PostProcessRTVIndex, postProcessDescriptor, BackBufferFormat);
  }

  void CreateShadowMap(uint32_t size)
  {
    if (s_Data.ShadowMap.DescriptorIndex == 0)
      s_Data.ShadowMap.DescriptorIndex = AllocateDescriptor();
    const uint32_t descriptorIndex = s_Data.ShadowMap.DescriptorIndex;

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = size;
    desc.Height = size;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE clear{};
    clear.Format = DepthFormat;
    clear.DepthStencil.Depth = 1.0f;
    const D3D12_HEAP_PROPERTIES heap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    CheckHRESULT(s_Data.Device->CreateCommittedResource(
                   &heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                   &clear, IID_PPV_ARGS(&s_Data.ShadowMap.Resource)),
                 "Create DX12 shadow map");
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
    dsv.Format = DepthFormat;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    s_Data.Device->CreateDepthStencilView(s_Data.ShadowMap.Resource.Get(), &dsv, GetDSVHandle(1));
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Format = DXGI_FORMAT_R32_FLOAT;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Texture2D.MipLevels = 1;
    s_Data.Device->CreateShaderResourceView(
      s_Data.ShadowMap.Resource.Get(), &srv, GetSRVCPUHandle(descriptorIndex));
    s_Data.ShadowMapSize = size;
  }

  void CreatePointShadowMap(uint32_t size)
  {
    if (s_Data.PointShadowMap.DescriptorIndex == 0)
      s_Data.PointShadowMap.DescriptorIndex = AllocateDescriptor();
    const uint32_t descriptorIndex = s_Data.PointShadowMap.DescriptorIndex;

    D3D12_RESOURCE_DESC textureDesc{};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = size;
    textureDesc.Height = size;
    textureDesc.DepthOrArraySize = static_cast<UINT16>(
      MaxShadowedPointLights * PointShadowFaceCount);
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    D3D12_CLEAR_VALUE colorClear{};
    colorClear.Format = DXGI_FORMAT_R32_FLOAT;
    colorClear.Color[0] = PointShadowRadius;
    colorClear.Color[1] = PointShadowRadius;
    colorClear.Color[2] = PointShadowRadius;
    colorClear.Color[3] = PointShadowRadius;
    const D3D12_HEAP_PROPERTIES defaultHeap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    CheckHRESULT(s_Data.Device->CreateCommittedResource(
                   &defaultHeap, D3D12_HEAP_FLAG_NONE, &textureDesc,
                   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &colorClear,
                   IID_PPV_ARGS(&s_Data.PointShadowMap.Resource)),
                 "Create DX12 point shadow cube array");

    for (uint32_t slice = 0;
         slice < MaxShadowedPointLights * PointShadowFaceCount; ++slice)
    {
      D3D12_RENDER_TARGET_VIEW_DESC rtv{};
      rtv.Format = DXGI_FORMAT_R32_FLOAT;
      rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
      rtv.Texture2DArray.MipSlice = 0;
      rtv.Texture2DArray.FirstArraySlice = slice;
      rtv.Texture2DArray.ArraySize = 1;
      s_Data.Device->CreateRenderTargetView(
        s_Data.PointShadowMap.Resource.Get(), &rtv,
        GetRTVHandle(PointShadowRTVBaseIndex + slice));
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Format = DXGI_FORMAT_R32_FLOAT;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
    srv.TextureCubeArray.MostDetailedMip = 0;
    srv.TextureCubeArray.MipLevels = 1;
    srv.TextureCubeArray.First2DArrayFace = 0;
    srv.TextureCubeArray.NumCubes = MaxShadowedPointLights;
    s_Data.Device->CreateShaderResourceView(
      s_Data.PointShadowMap.Resource.Get(), &srv, GetSRVCPUHandle(descriptorIndex));

    D3D12_RESOURCE_DESC depthDesc{};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = size;
    depthDesc.Height = size;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DepthFormat;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE depthClear{};
    depthClear.Format = DepthFormat;
    depthClear.DepthStencil.Depth = 1.0f;
    CheckHRESULT(s_Data.Device->CreateCommittedResource(
                   &defaultHeap, D3D12_HEAP_FLAG_NONE, &depthDesc,
                   D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClear,
                   IID_PPV_ARGS(&s_Data.PointShadowDepth)),
                 "Create DX12 point shadow depth buffer");
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
    dsv.Format = DepthFormat;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    s_Data.Device->CreateDepthStencilView(
      s_Data.PointShadowDepth.Get(), &dsv, GetDSVHandle(2));

    s_Data.PointShadowMapSize = size;
    s_Data.PointShadowMapReadable = true;
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

  void LogD3D12Messages()
  {
#ifdef DEBUG
    if (!s_Data.InfoQueue) return;
    const uint64_t messageCount = s_Data.InfoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
    for (uint64_t i = 0; i < messageCount; ++i)
    {
      SIZE_T size = 0;
      s_Data.InfoQueue->GetMessage(i, nullptr, &size);
      if (size == 0) continue;
      std::vector<uint8_t> storage(size);
      auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
      if (SUCCEEDED(s_Data.InfoQueue->GetMessage(i, message, &size)) &&
          message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING)
      {
        if (message->Severity <= D3D12_MESSAGE_SEVERITY_ERROR)
          GABGL_ERROR("D3D12 validation: {}", message->pDescription);
        else
          GABGL_WARN("D3D12 validation: {}", message->pDescription);
      }
    }
    s_Data.InfoQueue->ClearStoredMessages();
#endif
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
#define MAX_LIGHTS 32

cbuffer SceneConstants : register(b0)
{
  float4x4 ViewProjection;
  float4x4 Model;
  float4x4 LightViewProjection;
  float4x4 Bones[MAX_BONES];
  float4 CameraPosition;
  float4 MaterialFlags;
  float4 LightPositions[MAX_LIGHTS];
  float4 LightDirections[MAX_LIGHTS];
  float4 LightColors[MAX_LIGHTS];
  float4 LightTypes[MAX_LIGHTS];
  uint4 LightCount;
  float4 Resolution;
};

Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D SpecularTexture : register(t2);
Texture2D ShadowTexture : register(t3);
TextureCubeArray PointShadowTexture : register(t4);
SamplerState LinearSampler : register(s0);

struct VSInput
{
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
  float3 tangent : TANGENT;
  float3 bitangent : BITANGENT;
  int4 boneIds : BONEIDS;
  float4 weights : BONEWEIGHTS;
};

struct PSInput
{
  float4 position : SV_POSITION;
  float3 worldPosition : POSITION0;
  float3 normal : NORMAL;
  float3 tangent : TANGENT;
  float3 bitangent : BITANGENT;
  float2 uv : TEXCOORD;
  float4 shadowPosition : TEXCOORD1;
};

float4x4 SkinMatrix(VSInput input)
{
  float4x4 skin = float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
  if (CameraPosition.w < 0.5f) return skin;
  skin = float4x4(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0);
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
  return totalWeight > 0.00001f ? skin / totalWeight
    : float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
}

PSInput VSMain(VSInput input)
{
  const float4x4 skin = SkinMatrix(input);
  const float4 worldPosition = mul(Model, mul(skin, float4(input.position, 1.0f)));
  const float3x3 linearTransform = mul((float3x3)Model, (float3x3)skin);
  const float3 normal = normalize(mul(linearTransform, input.normal));
  float3 tangent = mul(linearTransform, input.tangent);
  tangent = normalize(tangent - normal * dot(normal, tangent));
  const float handedness = dot(cross(input.normal, input.tangent), input.bitangent) < 0.0f ? -1.0f : 1.0f;
  PSInput output;
  float4 clipPosition = mul(ViewProjection, worldPosition);
  const float2 outputResolution = max(Resolution.xy, float2(1.0f, 1.0f));
  const float virtualHeight = max(Resolution.z, 1.0f);
  const float2 snapResolution = float2(
    max(floor(virtualHeight * outputResolution.x / outputResolution.y + 0.5f), 1.0f),
    virtualHeight);
  const float safeW = abs(clipPosition.w) > 0.00001f ? clipPosition.w : 0.00001f;
  if (Resolution.w > 0.5f)
  {
    const float2 ndc = clipPosition.xy / safeW;
    const float2 snappedNdc =
      (floor((ndc * 0.5f + 0.5f) * snapResolution + 0.5f) / snapResolution) * 2.0f - 1.0f;
    clipPosition.xy = snappedNdc * clipPosition.w;
  }
  output.position = clipPosition;
  output.worldPosition = worldPosition.xyz;
  output.normal = normal;
  output.tangent = tangent;
  output.bitangent = normalize(cross(normal, tangent)) * handedness;
  output.uv = input.uv;
  output.shadowPosition = mul(LightViewProjection, worldPosition);
  return output;
}

float ShadowFactor(float4 shadowPosition, float3 normal, float3 lightDirection)
{
  if (LightCount.y == 0) return 1.0f;
  if (shadowPosition.w <= 0.0f) return 1.0f;
  const float3 ndc = shadowPosition.xyz / shadowPosition.w;
  const float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;
  if (ndc.z <= 0.0f || ndc.z >= 1.0f || any(uv < 0.0f) || any(uv > 1.0f)) return 1.0f;
  uint width, height;
  ShadowTexture.GetDimensions(width, height);
  const float2 texel = 1.0f / float2(width, height);
  const float bias = max(0.0015f * (1.0f - saturate(dot(normal, -lightDirection))), 0.0002f);
  float visibility = 0.0f;
  [unroll]
  for (int y = -1; y <= 1; ++y)
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
      const float depth = ShadowTexture.SampleLevel(LinearSampler, uv + float2(x, y) * texel, 0).r;
      visibility += ndc.z - bias <= depth ? 1.0f : 0.18f;
    }
  return visibility / 9.0f;
}

float PointShadowFactor(float3 worldPosition, float3 normal, float3 lightPosition,
                        float shadowSlotPlusOne)
{
  if (LightCount.y == 0 || shadowSlotPlusOne < 0.5f) return 1.0f;
  const float3 fragToLight = worldPosition - lightPosition;
  const float currentDepth = length(fragToLight);
  if (currentDepth >= 20.0f || currentDepth <= 0.0001f) return 1.0f;
  const float3 direction = fragToLight / currentDepth;
  const float closestDepth = PointShadowTexture.SampleLevel(
    LinearSampler, float4(direction, shadowSlotPlusOne - 1.0f), 0).r;
  const float bias = max(0.03f * (1.0f - dot(normal, -direction)), 0.003f);
  return currentDepth - bias > closestDepth ? 0.15f : 1.0f;
}

float4 PSMain(PSInput input) : SV_TARGET
{
  const float4 albedoSample = DiffuseTexture.Sample(LinearSampler, input.uv);
  float3 normal = normalize(input.normal);
  if (MaterialFlags.x > 0.5f)
  {
    const float3 tangentNormal = NormalTexture.Sample(LinearSampler, input.uv).xyz * 2.0f - 1.0f;
    normal = normalize(tangentNormal.x * input.tangent + tangentNormal.y * input.bitangent + tangentNormal.z * normal);
  }
  const float specularStrength = MaterialFlags.y > 0.5f
    ? SpecularTexture.Sample(LinearSampler, input.uv).r : 0.12f;
  const float3 viewDirection = normalize(CameraPosition.xyz - input.worldPosition);
  const float shininess = lerp(8.0f, 128.0f, specularStrength);
  float3 lighting = 0.0f;
  [loop]
  for (uint i = 0; i < min(LightCount.x, (uint)MAX_LIGHTS); ++i)
  {
    const int type = (int)(LightTypes[i].x + 0.5f);
    const float3 lightColor = LightColors[i].rgb;
    const float3 ambient = lightColor * 0.1f;
    float3 toLight;
    float attenuation = 1.0f;
    float intensity = 1.0f;
    float shadow = 1.0f;
    if (type == 0)
    {
      const float3 direction = normalize(LightDirections[i].xyz);
      toLight = -direction;
      shadow = ShadowFactor(input.shadowPosition, normal, direction);
      const float diffuse = saturate(dot(normal, toLight));
      const float specular = pow(saturate(dot(viewDirection, reflect(-toLight, normal))), shininess);
      lighting += ambient * albedoSample.rgb;
      lighting += lightColor * diffuse * albedoSample.rgb * shadow;
      lighting += specularStrength * specular * shadow;
    }
    else if (type == 1)
    {
      const float3 offset = LightPositions[i].xyz - input.worldPosition;
      const float distanceToLight = max(length(offset), 0.001f);
      toLight = offset / distanceToLight;
      attenuation = 1.0f / (1.0f + 0.09f * distanceToLight + 0.032f * distanceToLight * distanceToLight);
      const float diffuse = saturate(dot(normal, toLight));
      const float specular = pow(saturate(dot(viewDirection, reflect(-toLight, normal))), shininess);
      shadow = PointShadowFactor(input.worldPosition, normal, LightPositions[i].xyz, LightTypes[i].y);
      lighting += ambient * albedoSample.rgb * attenuation;
      lighting += lightColor * diffuse * albedoSample.rgb * attenuation * shadow;
      lighting += specularStrength * specular * attenuation * shadow;
    }
    else
    {
      const float3 offset = LightPositions[i].xyz - input.worldPosition;
      const float distanceToLight = max(length(offset), 0.001f);
      toLight = offset / distanceToLight;
      attenuation = 1.0f / (1.0f + 0.09f * distanceToLight + 0.032f * distanceToLight * distanceToLight);
      const float theta = dot(-toLight, normalize(LightDirections[i].xyz));
      intensity = saturate((theta - cos(0.3054f)) / max(cos(0.2182f) - cos(0.3054f), 0.0001f));
      const float diffuse = saturate(dot(normal, toLight));
      const float specular = pow(saturate(dot(viewDirection, reflect(-toLight, normal))), shininess);
      lighting += (ambient * albedoSample.rgb + lightColor * diffuse * albedoSample.rgb
        + specularStrength * specular) * attenuation * intensity;
    }
  }
  return float4(lighting, albedoSample.a);
}
)";

    const auto vertexShader = CompileShader(SceneShader, "VSMain", "vs_5_1");
    const auto pixelShader = CompileShader(SceneShader, "PSMain", "ps_5_1");

    std::array<D3D12_DESCRIPTOR_RANGE, 5> textureRanges{};
    for (uint32_t i = 0; i < textureRanges.size(); ++i)
    {
      textureRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      textureRanges[i].NumDescriptors = 1;
      textureRanges[i].BaseShaderRegister = i;
      textureRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    std::array<D3D12_ROOT_PARAMETER, 6> parameters{};
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameters[0].Descriptor.ShaderRegister = 0;
    parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    for (uint32_t i = 0; i < textureRanges.size(); ++i)
    {
      parameters[i + 1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      parameters[i + 1].DescriptorTable.NumDescriptorRanges = 1;
      parameters[i + 1].DescriptorTable.pDescriptorRanges = &textureRanges[i];
      parameters[i + 1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }

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
      {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Tangent),
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Bitangent),
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
    pipeline.RTVFormats[0] = SceneColorFormat;
    pipeline.DSVFormat = DepthFormat;
    pipeline.SampleDesc.Count = 1;
    CheckHRESULT(s_Data.Device->CreateGraphicsPipelineState(
                   &pipeline, IID_PPV_ARGS(&s_Data.ScenePipeline)),
                 "ID3D12Device::CreateGraphicsPipelineState (scene)");
  }

  void CreateShadowPipeline()
  {
    static constexpr const char* shaderSource = R"(
#define MAX_BONES 100
cbuffer ShadowConstants : register(b0)
{
  float4x4 LightViewProjection;
  float4x4 Model;
  float4x4 Bones[MAX_BONES];
  float4 Animated;
};
struct VSInput { float3 position : POSITION; int4 boneIds : BONEIDS; float4 weights : BONEWEIGHTS; };
float4 VSMain(VSInput input) : SV_POSITION
{
  float4x4 skin = float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
  if (Animated.x > 0.5f)
  {
    skin = float4x4(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0);
    float total = 0.0f;
    [unroll] for (int i = 0; i < 4; ++i)
      if (input.boneIds[i] >= 0 && input.boneIds[i] < MAX_BONES && input.weights[i] > 0.0f)
      { skin += Bones[input.boneIds[i]] * input.weights[i]; total += input.weights[i]; }
    if (total > 0.00001f) skin /= total;
    else skin = float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
  }
  return mul(LightViewProjection, mul(Model, mul(skin, float4(input.position, 1.0f))));
}
)";
    const auto vertexShader = CompileShader(shaderSource, "VSMain", "vs_5_1");
    D3D12_ROOT_PARAMETER parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameter.Descriptor.ShaderRegister = 0;
    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{};
    rootDesc.NumParameters = 1;
    rootDesc.pParameters = &parameter;
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    s_Data.ShadowRootSignature = CreateRootSignature(rootDesc);
    static constexpr D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, offsetof(Vertex, m_BoneIDs), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, m_Weights), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};
    pipeline.pRootSignature = s_Data.ShadowRootSignature.Get();
    pipeline.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    pipeline.BlendState = DefaultBlendState();
    pipeline.SampleMask = std::numeric_limits<UINT>::max();
    pipeline.RasterizerState = DefaultRasterizer();
    pipeline.RasterizerState.DepthBias = 1200;
    pipeline.RasterizerState.SlopeScaledDepthBias = 1.5f;
    pipeline.DepthStencilState.DepthEnable = TRUE;
    pipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    pipeline.InputLayout = {inputLayout, static_cast<UINT>(std::size(inputLayout))};
    pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline.DSVFormat = DepthFormat;
    pipeline.SampleDesc.Count = 1;
    CheckHRESULT(s_Data.Device->CreateGraphicsPipelineState(&pipeline, IID_PPV_ARGS(&s_Data.ShadowPipeline)),
                 "Create DX12 shadow pipeline");
  }

  void CreatePointShadowPipeline()
  {
    static constexpr const char* shaderSource = R"(
#define MAX_BONES 100
cbuffer PointShadowConstants : register(b0)
{
  float4x4 LightViewProjection;
  float4x4 Model;
  float4x4 Bones[MAX_BONES];
  float4 LightPositionAndAnimated;
};
struct VSInput { float3 position : POSITION; int4 boneIds : BONEIDS; float4 weights : BONEWEIGHTS; };
struct PSInput { float4 position : SV_POSITION; float3 worldPosition : POSITION0; };
PSInput VSMain(VSInput input)
{
  float4x4 skin = float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
  if (LightPositionAndAnimated.w > 0.5f)
  {
    skin = float4x4(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0);
    float total = 0.0f;
    [unroll] for (int i = 0; i < 4; ++i)
      if (input.boneIds[i] >= 0 && input.boneIds[i] < MAX_BONES && input.weights[i] > 0.0f)
      { skin += Bones[input.boneIds[i]] * input.weights[i]; total += input.weights[i]; }
    if (total > 0.00001f) skin /= total;
    else skin = float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
  }
  const float4 world = mul(Model, mul(skin, float4(input.position, 1.0f)));
  PSInput output;
  output.position = mul(LightViewProjection, world);
  output.worldPosition = world.xyz;
  return output;
}
float PSMain(PSInput input) : SV_TARGET
{
  return length(input.worldPosition - LightPositionAndAnimated.xyz);
}
)";
    const auto vertexShader = CompileShader(shaderSource, "VSMain", "vs_5_1");
    const auto pixelShader = CompileShader(shaderSource, "PSMain", "ps_5_1");
    D3D12_ROOT_PARAMETER parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameter.Descriptor.ShaderRegister = 0;
    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{};
    rootDesc.NumParameters = 1;
    rootDesc.pParameters = &parameter;
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    s_Data.PointShadowRootSignature = CreateRootSignature(rootDesc);
    static constexpr D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, offsetof(Vertex, m_BoneIDs), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"BONEWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, m_Weights), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};
    pipeline.pRootSignature = s_Data.PointShadowRootSignature.Get();
    pipeline.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    pipeline.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    pipeline.BlendState = DefaultBlendState();
    pipeline.SampleMask = std::numeric_limits<UINT>::max();
    pipeline.RasterizerState = DefaultRasterizer();
    pipeline.DepthStencilState.DepthEnable = TRUE;
    pipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    pipeline.InputLayout = {inputLayout, static_cast<UINT>(std::size(inputLayout))};
    pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline.NumRenderTargets = 1;
    pipeline.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
    pipeline.DSVFormat = DepthFormat;
    pipeline.SampleDesc.Count = 1;
    CheckHRESULT(s_Data.Device->CreateGraphicsPipelineState(
                   &pipeline, IID_PPV_ARGS(&s_Data.PointShadowPipeline)),
                 "Create DX12 point shadow pipeline");
  }

  void CreateSkyboxPipeline()
  {
    static constexpr const char* shaderSource = R"(
cbuffer SkyboxConstants : register(b0) { float4x4 ViewProjection; };
TextureCube SkyboxTexture : register(t0);
SamplerState SkyboxSampler : register(s0);
struct PSInput { float4 position : SV_POSITION; float3 direction : TEXCOORD; };
PSInput VSMain(float3 position : POSITION)
{
  PSInput output;
  float4 clip = mul(ViewProjection, float4(position, 1.0f));
  output.position = clip.xyww;
  output.direction = position;
  return output;
}
float4 PSMain(PSInput input) : SV_TARGET
{
  return float4(SkyboxTexture.Sample(SkyboxSampler, input.direction).rgb * 0.65f, 1.0f);
}
)";
    const auto vertexShader = CompileShader(shaderSource, "VSMain", "vs_5_1");
    const auto pixelShader = CompileShader(shaderSource, "PSMain", "ps_5_1");
    D3D12_DESCRIPTOR_RANGE range{};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;
    std::array<D3D12_ROOT_PARAMETER, 2> parameters{};
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameters[0].Descriptor.ShaderRegister = 0;
    parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameters[1].DescriptorTable = {1, &range};
    parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{};
    rootDesc.NumParameters = static_cast<UINT>(parameters.size());
    rootDesc.pParameters = parameters.data();
    rootDesc.NumStaticSamplers = 1;
    rootDesc.pStaticSamplers = &sampler;
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    s_Data.SkyboxRootSignature = CreateRootSignature(rootDesc);
    const D3D12_INPUT_ELEMENT_DESC input = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};
    pipeline.pRootSignature = s_Data.SkyboxRootSignature.Get();
    pipeline.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    pipeline.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    pipeline.BlendState = DefaultBlendState();
    pipeline.SampleMask = std::numeric_limits<UINT>::max();
    pipeline.RasterizerState = DefaultRasterizer();
    pipeline.DepthStencilState.DepthEnable = TRUE;
    pipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    pipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    pipeline.InputLayout = {&input, 1};
    pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline.NumRenderTargets = 1;
    pipeline.RTVFormats[0] = SceneColorFormat;
    pipeline.DSVFormat = DepthFormat;
    pipeline.SampleDesc.Count = 1;
    CheckHRESULT(s_Data.Device->CreateGraphicsPipelineState(&pipeline, IID_PPV_ARGS(&s_Data.SkyboxPipeline)),
                 "Create DX12 skybox pipeline");

    static constexpr float vertices[] = {
      -1,1,-1, -1,-1,-1, 1,-1,-1, 1,-1,-1, 1,1,-1, -1,1,-1,
      -1,-1,1, -1,-1,-1, -1,1,-1, -1,1,-1, -1,1,1, -1,-1,1,
      1,-1,-1, 1,-1,1, 1,1,1, 1,1,1, 1,1,-1, 1,-1,-1,
      -1,-1,1, -1,1,1, 1,1,1, 1,1,1, 1,-1,1, -1,-1,1,
      -1,1,-1, 1,1,-1, 1,1,1, 1,1,1, -1,1,1, -1,1,-1,
      -1,-1,-1, -1,-1,1, 1,-1,-1, 1,-1,-1, -1,-1,1, 1,-1,1
    };
    s_Data.SkyboxVertexBuffer = CreateUploadBuffer(sizeof(vertices));
    void* mapped = nullptr;
    const D3D12_RANGE noRead{0, 0};
    CheckHRESULT(s_Data.SkyboxVertexBuffer->Map(0, &noRead, &mapped), "Map DX12 skybox vertices");
    std::memcpy(mapped, vertices, sizeof(vertices));
    s_Data.SkyboxVertexBuffer->Unmap(0, nullptr);
    s_Data.SkyboxVertexView = {s_Data.SkyboxVertexBuffer->GetGPUVirtualAddress(), sizeof(vertices), 3 * sizeof(float)};
  }

  void CreateParticlePipeline()
  {
    static constexpr const char* shaderSource = R"(
cbuffer ParticleConstants : register(b0) { float4x4 ViewProjection; };
struct VSInput { float3 position : POSITION; float4 color : COLOR; float2 localPosition : TEXCOORD; float isSquare : STYLE; };
struct PSInput { float4 position : SV_POSITION; float4 color : COLOR; float2 localPosition : TEXCOORD; nointerpolation float isSquare : STYLE; };
PSInput VSMain(VSInput input) { PSInput o; o.position=mul(ViewProjection,float4(input.position,1)); o.color=input.color; o.localPosition=input.localPosition; o.isSquare=input.isSquare; return o; }
float4 PSMain(PSInput input) : SV_TARGET { float edge=input.isSquare>0.5?1.0:1.0-smoothstep(0.15,1.0,length(input.localPosition)); float4 c=float4(input.color.rgb,input.color.a*edge); clip(c.a-0.01); return c; }
)";
    const auto vs = CompileShader(shaderSource, "VSMain", "vs_5_1");
    const auto ps = CompileShader(shaderSource, "PSMain", "ps_5_1");
    D3D12_ROOT_PARAMETER parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameter.Descriptor.ShaderRegister = 0;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{};
    rootDesc.NumParameters = 1; rootDesc.pParameters = &parameter;
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    s_Data.ParticleRootSignature = CreateRootSignature(rootDesc);
    static constexpr D3D12_INPUT_ELEMENT_DESC inputs[] = {
      {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,offsetof(ParticleVertex,Position),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
      {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,offsetof(ParticleVertex,Color),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
      {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,offsetof(ParticleVertex,LocalPosition),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
      {"STYLE",0,DXGI_FORMAT_R32_FLOAT,0,offsetof(ParticleVertex,IsSquare),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};
    pipeline.pRootSignature=s_Data.ParticleRootSignature.Get(); pipeline.VS={vs->GetBufferPointer(),vs->GetBufferSize()}; pipeline.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
    pipeline.BlendState=DefaultBlendState(); auto& blend=pipeline.BlendState.RenderTarget[0]; blend.BlendEnable=TRUE; blend.SrcBlend=D3D12_BLEND_SRC_ALPHA; blend.DestBlend=D3D12_BLEND_INV_SRC_ALPHA; blend.BlendOp=D3D12_BLEND_OP_ADD; blend.SrcBlendAlpha=D3D12_BLEND_ONE; blend.DestBlendAlpha=D3D12_BLEND_INV_SRC_ALPHA; blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;
    pipeline.SampleMask=std::numeric_limits<UINT>::max(); pipeline.RasterizerState=DefaultRasterizer(); pipeline.DepthStencilState.DepthEnable=TRUE; pipeline.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO; pipeline.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;
    pipeline.InputLayout={inputs,static_cast<UINT>(std::size(inputs))}; pipeline.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; pipeline.NumRenderTargets=1; pipeline.RTVFormats[0]=SceneColorFormat; pipeline.DSVFormat=DepthFormat; pipeline.SampleDesc.Count=1;
    CheckHRESULT(s_Data.Device->CreateGraphicsPipelineState(&pipeline,IID_PPV_ARGS(&s_Data.ParticlePipeline)),"Create DX12 particle pipeline");
  }

  void CreateDebugLinePipeline()
  {
    static constexpr const char* shaderSource = R"(
cbuffer LineConstants : register(b0) { float4x4 ViewProjection; };
struct VSInput { float3 position : POSITION; float4 color : COLOR; };
struct PSInput { float4 position : SV_POSITION; float4 color : COLOR; };
PSInput VSMain(VSInput input) { PSInput o; o.position=mul(ViewProjection,float4(input.position,1)); o.color=input.color; return o; }
float4 PSMain(PSInput input) : SV_TARGET { return input.color; }
)";
    const auto vs=CompileShader(shaderSource,"VSMain","vs_5_1"); const auto ps=CompileShader(shaderSource,"PSMain","ps_5_1");
    D3D12_ROOT_PARAMETER parameter{}; parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV; parameter.Descriptor.ShaderRegister=0;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{}; rootDesc.NumParameters=1; rootDesc.pParameters=&parameter; rootDesc.Flags=D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; s_Data.DebugLineRootSignature=CreateRootSignature(rootDesc);
    static constexpr D3D12_INPUT_ELEMENT_DESC inputs[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,offsetof(DebugLineVertex,Position),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,offsetof(DebugLineVertex,Color),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{}; pipeline.pRootSignature=s_Data.DebugLineRootSignature.Get(); pipeline.VS={vs->GetBufferPointer(),vs->GetBufferSize()}; pipeline.PS={ps->GetBufferPointer(),ps->GetBufferSize()}; pipeline.BlendState=DefaultBlendState(); auto& blend=pipeline.BlendState.RenderTarget[0]; blend.BlendEnable=TRUE; blend.SrcBlend=D3D12_BLEND_SRC_ALPHA; blend.DestBlend=D3D12_BLEND_INV_SRC_ALPHA; blend.BlendOp=D3D12_BLEND_OP_ADD; blend.SrcBlendAlpha=D3D12_BLEND_ONE; blend.DestBlendAlpha=D3D12_BLEND_INV_SRC_ALPHA; blend.BlendOpAlpha=D3D12_BLEND_OP_ADD; pipeline.SampleMask=std::numeric_limits<UINT>::max(); pipeline.RasterizerState=DefaultRasterizer(); pipeline.DepthStencilState.DepthEnable=TRUE; pipeline.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO; pipeline.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL; pipeline.InputLayout={inputs,static_cast<UINT>(std::size(inputs))}; pipeline.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE; pipeline.NumRenderTargets=1; pipeline.RTVFormats[0]=SceneColorFormat; pipeline.DSVFormat=DepthFormat; pipeline.SampleDesc.Count=1;
    CheckHRESULT(s_Data.Device->CreateGraphicsPipelineState(&pipeline,IID_PPV_ARGS(&s_Data.DebugLinePipeline)),"Create DX12 debug line pipeline");
  }

  void CreatePhysicsDebugPipeline()
  {
    static constexpr const char* shaderSource = R"(
cbuffer PhysicsDebugConstants : register(b0)
{
  float4x4 ViewProjection;
  float4x4 Model;
  float4 Color;
};
struct VSInput { float3 position : POSITION; };
float4 VSMain(VSInput input) : SV_POSITION
{
  return mul(ViewProjection, mul(Model, float4(input.position, 1.0f)));
}
float4 PSMain() : SV_TARGET { return Color; }
)";
    const auto vertexShader = CompileShader(shaderSource, "VSMain", "vs_5_1");
    const auto pixelShader = CompileShader(shaderSource, "PSMain", "ps_5_1");

    D3D12_ROOT_PARAMETER parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameter.Descriptor.ShaderRegister = 0;
    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{};
    rootDesc.NumParameters = 1;
    rootDesc.pParameters = &parameter;
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    s_Data.PhysicsDebugRootSignature = CreateRootSignature(rootDesc);

    static constexpr D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, Position),
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};
    pipeline.pRootSignature = s_Data.PhysicsDebugRootSignature.Get();
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
    pipeline.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    pipeline.DepthStencilState.DepthEnable = TRUE;
    pipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    pipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    pipeline.InputLayout = {inputLayout, static_cast<UINT>(std::size(inputLayout))};
    pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline.NumRenderTargets = 1;
    pipeline.RTVFormats[0] = SceneColorFormat;
    pipeline.DSVFormat = DepthFormat;
    pipeline.SampleDesc.Count = 1;
    CheckHRESULT(s_Data.Device->CreateGraphicsPipelineState(
                   &pipeline, IID_PPV_ARGS(&s_Data.PhysicsDebugPipeline)),
                 "Create DX12 physics debug pipeline");
  }

  void CreatePostProcessPipelines()
  {
    static constexpr const char* shaderSource = R"(
cbuffer PostConstants : register(b0) { float4 Params; float4 EffectParams; };
Texture2D SourceTexture : register(t0); Texture2D BloomTexture : register(t1); SamplerState LinearSampler : register(s0);
struct PSInput { float4 position : SV_POSITION; float2 uv : TEXCOORD; };
PSInput VSMain(uint id : SV_VertexID) { PSInput o; float2 p=float2((id<<1)&2,id&2); o.uv=p; o.position=float4(p*float2(2,-2)+float2(-1,1),0,1); return o; }
float4 Extract(PSInput input) : SV_TARGET { float3 c=SourceTexture.Sample(LinearSampler,input.uv).rgb; float b=dot(c,float3(0.2126,0.7152,0.0722)); float threshold=max(EffectParams.x,0.0f); return b>threshold ? float4(c,1) : float4(0,0,0,1); }
float4 Blur(PSInput input) : SV_TARGET { float2 d=Params.xy; float3 c=SourceTexture.Sample(LinearSampler,input.uv).rgb*0.227027; c+=SourceTexture.Sample(LinearSampler,input.uv+d*1.384615).rgb*0.316216; c+=SourceTexture.Sample(LinearSampler,input.uv-d*1.384615).rgb*0.316216; c+=SourceTexture.Sample(LinearSampler,input.uv+d*3.230769).rgb*0.070270; c+=SourceTexture.Sample(LinearSampler,input.uv-d*3.230769).rgb*0.070270; return float4(c,1); }
float3 ACES(float3 x) { return saturate((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14)); }
static const float Bayer4x4[16] = {
   0.0f,  8.0f,  2.0f, 10.0f,
  12.0f,  4.0f, 14.0f,  6.0f,
   3.0f, 11.0f,  1.0f,  9.0f,
  15.0f,  7.0f, 13.0f,  5.0f
};
float4 Composite(PSInput input) : SV_TARGET
{
  float2 sampleUV = input.uv;
  float2 virtualPixel = 0.0f;
  if (Params.w > 0.5f)
  {
    const float2 resolution = max(Params.yz, float2(1.0f, 1.0f));
    const float aspect = resolution.x / resolution.y;
    const float2 virtualResolution = float2(
      max(floor(max(EffectParams.y, 1.0f) * aspect + 0.5f), 1.0f), max(EffectParams.y, 1.0f));
    virtualPixel = floor(input.uv * virtualResolution);
    sampleUV = (virtualPixel + 0.5f) / virtualResolution;
  }

  float3 color = SourceTexture.Sample(LinearSampler, sampleUV).rgb;
  color += BloomTexture.Sample(LinearSampler, sampleUV).rgb * Params.x;
  color = pow(ACES(color * EffectParams.x), 1.0f / max(EffectParams.w, 0.001f));

  if (Params.w > 0.5f)
  {
    const uint2 ditherCoord = (uint2)virtualPixel & uint2(3, 3);
    const uint ditherIndex = ditherCoord.x + ditherCoord.y * 4;
    const float threshold = (Bayer4x4[ditherIndex] + 0.5f) / 16.0f;
    const float colorLevels = max(EffectParams.z, 1.0f);
    color = floor(saturate(color) * colorLevels + threshold) / colorLevels;
  }
  return float4(color, 1.0f);
}
)";
    const auto vs=CompileShader(shaderSource,"VSMain","vs_5_1"); const auto extract=CompileShader(shaderSource,"Extract","ps_5_1"); const auto blur=CompileShader(shaderSource,"Blur","ps_5_1"); const auto composite=CompileShader(shaderSource,"Composite","ps_5_1");
    std::array<D3D12_DESCRIPTOR_RANGE,2> ranges{}; for(uint32_t i=0;i<2;++i){ranges[i].RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;ranges[i].NumDescriptors=1;ranges[i].BaseShaderRegister=i;}
    std::array<D3D12_ROOT_PARAMETER,3> parameters{}; parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; parameters[0].Constants.ShaderRegister=0; parameters[0].Constants.Num32BitValues=8; for(uint32_t i=0;i<2;++i){parameters[i+1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[i+1].DescriptorTable={1,&ranges[i]};parameters[i+1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
    D3D12_STATIC_SAMPLER_DESC sampler{}; sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR; sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP; sampler.MaxLOD=D3D12_FLOAT32_MAX; sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{}; rootDesc.NumParameters=static_cast<UINT>(parameters.size()); rootDesc.pParameters=parameters.data(); rootDesc.NumStaticSamplers=1; rootDesc.pStaticSamplers=&sampler; s_Data.PostRootSignature=CreateRootSignature(rootDesc);
    auto createPipeline=[&](ID3DBlob* pixelShader,DXGI_FORMAT format,ComPtr<ID3D12PipelineState>& result,const char* name){D3D12_GRAPHICS_PIPELINE_STATE_DESC p{};p.pRootSignature=s_Data.PostRootSignature.Get();p.VS={vs->GetBufferPointer(),vs->GetBufferSize()};p.PS={pixelShader->GetBufferPointer(),pixelShader->GetBufferSize()};p.BlendState=DefaultBlendState();p.SampleMask=std::numeric_limits<UINT>::max();p.RasterizerState=DefaultRasterizer();p.DepthStencilState.DepthEnable=FALSE;p.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;p.NumRenderTargets=1;p.RTVFormats[0]=format;p.SampleDesc.Count=1;CheckHRESULT(s_Data.Device->CreateGraphicsPipelineState(&p,IID_PPV_ARGS(&result)),name);};
    createPipeline(extract.Get(),SceneColorFormat,s_Data.BloomExtractPipeline,"Create DX12 bloom extract pipeline"); createPipeline(blur.Get(),SceneColorFormat,s_Data.BloomBlurPipeline,"Create DX12 bloom blur pipeline"); createPipeline(composite.Get(),BackBufferFormat,s_Data.CompositePipeline,"Create DX12 composite pipeline");
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
    pipeline.RTVFormats[0] = SceneColorFormat;
    CheckHRESULT(s_Data.Device->CreateGraphicsPipelineState(
                   &pipeline, IID_PPV_ARGS(&s_Data.SceneUIPipeline)),
                 "ID3D12Device::CreateGraphicsPipelineState (scene UI)");
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

  GPUTexture UploadCubeTexture(const std::shared_ptr<Texture>& source)
  {
    if (!source || source->GetWidth() == 0 || source->GetHeight() == 0)
      return {};
    const uint32_t width = source->GetWidth();
    const uint32_t height = source->GetHeight();
    const uint32_t channels = static_cast<uint32_t>(source->GetChannels());
    auto& faces = source->GetPixels();
    for (const auto* face : faces) if (!face) return {};

    GPUTexture texture;
    texture.DescriptorIndex = AllocateDescriptor();
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 6;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    const D3D12_HEAP_PROPERTIES heap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    CheckHRESULT(s_Data.Device->CreateCommittedResource(
                   &heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST,
                   nullptr, IID_PPV_ARGS(&texture.Resource)),
                 "Create DX12 cubemap");

    std::array<D3D12_PLACED_SUBRESOURCE_FOOTPRINT, 6> footprints{};
    std::array<UINT, 6> rowCounts{};
    std::array<UINT64, 6> rowSizes{};
    UINT64 uploadSize = 0;
    s_Data.Device->GetCopyableFootprints(
      &desc, 0, 6, 0, footprints.data(), rowCounts.data(), rowSizes.data(), &uploadSize);
    ComPtr<ID3D12Resource> upload = CreateUploadBuffer(uploadSize);
    uint8_t* mapped = nullptr;
    const D3D12_RANGE noRead{0, 0};
    CheckHRESULT(upload->Map(0, &noRead, reinterpret_cast<void**>(&mapped)), "Map DX12 cubemap upload");
    std::vector<uint8_t> rgba(static_cast<size_t>(width) * height * 4);
    for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
    {
      const uint8_t* face = faces[faceIndex];
      for (size_t pixel = 0; pixel < static_cast<size_t>(width) * height; ++pixel)
      {
        rgba[pixel * 4] = face[pixel * channels];
        rgba[pixel * 4 + 1] = channels > 1 ? face[pixel * channels + 1] : rgba[pixel * 4];
        rgba[pixel * 4 + 2] = channels > 2 ? face[pixel * channels + 2] : rgba[pixel * 4];
        rgba[pixel * 4 + 3] = channels > 3 ? face[pixel * channels + 3] : 255;
      }
      for (uint32_t row = 0; row < height; ++row)
        std::memcpy(mapped + footprints[faceIndex].Offset + static_cast<uint64_t>(row) * footprints[faceIndex].Footprint.RowPitch,
                    rgba.data() + static_cast<size_t>(row) * width * 4,
                    static_cast<size_t>(width) * 4);
    }
    upload->Unmap(0, nullptr);

    CheckHRESULT(s_Data.UploadAllocator->Reset(), "Reset DX12 cubemap allocator");
    CheckHRESULT(s_Data.UploadCommandList->Reset(s_Data.UploadAllocator.Get(), nullptr), "Reset DX12 cubemap list");
    for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
    {
      D3D12_TEXTURE_COPY_LOCATION destination{};
      destination.pResource = texture.Resource.Get();
      destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
      destination.SubresourceIndex = faceIndex;
      D3D12_TEXTURE_COPY_LOCATION sourceLocation{};
      sourceLocation.pResource = upload.Get();
      sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      sourceLocation.PlacedFootprint = footprints[faceIndex];
      s_Data.UploadCommandList->CopyTextureRegion(&destination, 0, 0, 0, &sourceLocation, nullptr);
    }
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture.Resource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    s_Data.UploadCommandList->ResourceBarrier(1, &barrier);
    CheckHRESULT(s_Data.UploadCommandList->Close(), "Close DX12 cubemap list");
    ID3D12CommandList* lists[] = {s_Data.UploadCommandList.Get()};
    s_Data.CommandQueue->ExecuteCommandLists(1, lists);
    WaitForGPU();

    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srv.TextureCube.MipLevels = 1;
    s_Data.Device->CreateShaderResourceView(texture.Resource.Get(), &srv, GetSRVCPUHandle(texture.DescriptorIndex));
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

  template <typename T>
  D3D12_GPU_VIRTUAL_ADDRESS UploadConstants(const T& constants)
  {
    auto& frame = s_Data.FrameUploads[s_Data.FrameIndex];
    const uint64_t offset = AlignUp(frame.ConstantsOffset, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    if (offset + sizeof(T) > ConstantBufferBytes)
      throw std::runtime_error("DX12 per-frame constant buffer exhausted");
    std::memcpy(frame.ConstantsMapped + offset, &constants, sizeof(T));
    frame.ConstantsOffset = offset + sizeof(T);
    return frame.Constants->GetGPUVirtualAddress() + offset;
  }

  D3D12_GPU_VIRTUAL_ADDRESS UploadSceneConstants(const SceneConstants& constants)
  {
    return UploadConstants(constants);
  }

  glm::mat4 DirectXViewProjection()
  {
    glm::mat4 correction(1.0f);
    correction[2][2] = 0.5f;
    correction[3][2] = 0.5f;
    return correction * Camera::GetViewProjection();
  }

  void DrawModels(const RenderEffectSettings& effects)
  {
    RenderStatistics statistics;
    if (s_Data.Meshes.empty())
    {
      RenderBackend::SetStatistics(statistics);
      return;
    }

    ID3D12DescriptorHeap* descriptorHeaps[] = {s_Data.SRVHeap.Get()};
    s_Data.CommandList->SetDescriptorHeaps(1, descriptorHeaps);
    s_Data.CommandList->SetPipelineState(s_Data.ScenePipeline.Get());
    s_Data.CommandList->SetGraphicsRootSignature(s_Data.SceneRootSignature.Get());
    s_Data.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    SceneConstants constants;
    constants.ViewProjection = DirectXViewProjection();
    constants.LightViewProjection = s_Data.LightViewProjection;
    constants.CameraPosition = glm::vec4(Camera::GetPosition(), 0.0f);
    constants.Bones.fill(glm::mat4(1.0f));
    const auto& lights = SceneManager::GetLights();
    const uint32_t lightCount = static_cast<uint32_t>(std::min(lights.size(), static_cast<size_t>(MaxSceneLights)));
    constants.LightCount.x = lightCount;
    constants.LightCount.y = effects.ShadowQuality == GraphicsQuality::Off ? 0u : 1u;
    constants.Resolution = glm::vec4(
      static_cast<float>(s_Data.Width), static_cast<float>(s_Data.Height),
      effects.PS1VirtualHeight, effects.PS1Enabled ? 1.0f : 0.0f);
    for (uint32_t i = 0; i < lightCount; ++i)
    {
      constants.LightPositions[i] = glm::vec4(lights[i].position, 1.0f);
      constants.LightDirections[i] = glm::vec4(lights[i].rotation, 0.0f);
      constants.LightColors[i] = glm::vec4(lights[i].color, 1.0f);
      constants.LightTypes[i].x = static_cast<float>(lights[i].type);
    }
    for (uint32_t slot = 0;
         slot < static_cast<uint32_t>(s_Data.PointShadowLightIndices.size()); ++slot)
    {
      const uint32_t lightIndex = s_Data.PointShadowLightIndices[slot];
      if (lightIndex < lightCount)
        constants.LightTypes[lightIndex].y = static_cast<float>(slot + 1u);
    }

    const RenderFrustum frustum(Camera::GetViewProjection());
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
      statistics.RenderableInstances += static_cast<uint32_t>(instances.size());
      for (const glm::mat4& transform : instances)
      {
        const WorldBoundingSphere sphere = CalculateWorldBoundingSphere(*model, transform);
        if (!frustum.IntersectsSphere(sphere.center, sphere.radius)) continue;
        ++statistics.VisibleInstances;
        constants.Model = transform;
        for (const Mesh& mesh : model->GetMeshes())
        {
          const auto gpuIt = s_Data.Meshes.find(&mesh);
          if (gpuIt == s_Data.Meshes.end()) continue;
          const GPUMesh& gpuMesh = gpuIt->second;
          constants.MaterialFlags = glm::vec4(
            gpuMesh.HasNormalMap ? 1.0f : 0.0f,
            gpuMesh.HasSpecularMap ? 1.0f : 0.0f, 0.0f, 0.0f);
          s_Data.CommandList->SetGraphicsRootConstantBufferView(0, UploadSceneConstants(constants));
          s_Data.CommandList->SetGraphicsRootDescriptorTable(1, GetSRVGPUHandle(gpuMesh.DiffuseDescriptorIndex));
          s_Data.CommandList->SetGraphicsRootDescriptorTable(2, GetSRVGPUHandle(gpuMesh.NormalDescriptorIndex));
          s_Data.CommandList->SetGraphicsRootDescriptorTable(3, GetSRVGPUHandle(gpuMesh.SpecularDescriptorIndex));
          s_Data.CommandList->SetGraphicsRootDescriptorTable(4, GetSRVGPUHandle(s_Data.ShadowMap.DescriptorIndex));
          s_Data.CommandList->SetGraphicsRootDescriptorTable(5, GetSRVGPUHandle(s_Data.PointShadowMap.DescriptorIndex));
          s_Data.CommandList->IASetVertexBuffers(0, 1, &gpuMesh.VertexView);
          s_Data.CommandList->IASetIndexBuffer(&gpuMesh.IndexView);
          s_Data.CommandList->DrawIndexedInstanced(gpuMesh.IndexCount, 1, 0, 0, 0);
        }
      }
    }
    RenderBackend::SetStatistics(statistics);
  }

  glm::mat4 CalculateLightViewProjection()
  {
    glm::vec3 direction(-1.0f, -2.0f, -1.0f);
    for (const SceneLight& light : SceneManager::GetLights())
      if (light.type == LightType::DIRECT) { direction = light.rotation; break; }
    if (glm::dot(direction, direction) < 0.0001f) direction = glm::vec3(-1.0f, -2.0f, -1.0f);
    direction = glm::normalize(direction);
    const glm::vec3 focus = Camera::GetPosition() + Camera::GetForwardDirection() * 45.0f;
    const glm::vec3 up = std::abs(glm::dot(direction, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.98f
      ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat4 view = glm::lookAt(focus - direction * 90.0f, focus, up);
    const glm::mat4 projection = glm::ortho(-65.0f, 65.0f, -65.0f, 65.0f, 1.0f, 220.0f);
    glm::mat4 correction(1.0f);
    correction[2][2] = 0.5f;
    correction[3][2] = 0.5f;
    return correction * projection * view;
  }

  void DrawShadowMap(const RenderEffectSettings& effects)
  {
    if (!s_Data.ShadowMap.Resource) return;
    if (s_Data.ShadowMapReadable)
      TransitionResource(s_Data.ShadowMap.Resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                         D3D12_RESOURCE_STATE_DEPTH_WRITE);
    s_Data.ShadowMapReadable = false;
    const D3D12_CPU_DESCRIPTOR_HANDLE dsv = GetDSVHandle(1);
    s_Data.CommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
    s_Data.CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    const D3D12_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(s_Data.ShadowMapSize), static_cast<float>(s_Data.ShadowMapSize), 0.0f, 1.0f};
    const D3D12_RECT scissor{0, 0, static_cast<LONG>(s_Data.ShadowMapSize), static_cast<LONG>(s_Data.ShadowMapSize)};
    s_Data.CommandList->RSSetViewports(1, &viewport);
    s_Data.CommandList->RSSetScissorRects(1, &scissor);
    s_Data.LightViewProjection = CalculateLightViewProjection();

    if (effects.ShadowQuality != GraphicsQuality::Off)
    {
      s_Data.CommandList->SetPipelineState(s_Data.ShadowPipeline.Get());
      s_Data.CommandList->SetGraphicsRootSignature(s_Data.ShadowRootSignature.Get());
      s_Data.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      ShadowConstants constants;
      constants.LightViewProjection = s_Data.LightViewProjection;
      constants.Bones.fill(glm::mat4(1.0f));
      for (const std::string& modelName : ModelManager::GetModelNames())
      {
        const auto model = ModelManager::GetModel(modelName);
        if (!model || !model->m_IsRendered || model->GetPhysXMeshType() == MeshType::CONVEXMESH) continue;
        const bool animated = model->IsAnimated();
        constants.Animated.x = animated ? 1.0f : 0.0f;
        constants.Bones.fill(glm::mat4(1.0f));
        if (animated)
        {
          const auto& bones = model->GetFinalBoneMatrices();
          std::copy_n(bones.begin(), std::min(bones.size(), constants.Bones.size()), constants.Bones.begin());
        }
        for (const glm::mat4& transform : model->m_InstanceTransforms)
        {
          constants.Model = transform;
          for (const Mesh& mesh : model->GetMeshes())
          {
            const auto found = s_Data.Meshes.find(&mesh);
            if (found == s_Data.Meshes.end()) continue;
            const GPUMesh& gpu = found->second;
            s_Data.CommandList->SetGraphicsRootConstantBufferView(0, UploadConstants(constants));
            s_Data.CommandList->IASetVertexBuffers(0, 1, &gpu.VertexView);
            s_Data.CommandList->IASetIndexBuffer(&gpu.IndexView);
            s_Data.CommandList->DrawIndexedInstanced(gpu.IndexCount, 1, 0, 0, 0);
          }
        }
      }
    }
    TransitionResource(s_Data.ShadowMap.Resource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    s_Data.ShadowMapReadable = true;
  }

  void DrawPointShadowMaps(const RenderEffectSettings& effects)
  {
    s_Data.PointShadowLightIndices.clear();
    if (effects.ShadowQuality == GraphicsQuality::Off || !s_Data.PointShadowMap.Resource)
      return;

    struct Candidate
    {
      float DistanceSquared = 0.0f;
      uint32_t LightIndex = 0;
    };
    const auto& lights = SceneManager::GetLights();
    std::vector<Candidate> candidates;
    candidates.reserve(lights.size());
    const glm::vec3 cameraPosition = Camera::GetPosition();
    const RenderFrustum cameraFrustum(Camera::GetViewProjection());
    uint32_t supportedPointLights = 0;
    for (uint32_t i = 0; i < static_cast<uint32_t>(lights.size()); ++i)
    {
      if (lights[i].type != LightType::POINT) continue;
      if (supportedPointLights++ >= 20u) break;
      if (!cameraFrustum.IntersectsSphere(lights[i].position, PointShadowRadius)) continue;
      const glm::vec3 delta = lights[i].position - cameraPosition;
      candidates.push_back({glm::dot(delta, delta), i});
    }
    std::ranges::sort(candidates, {}, &Candidate::DistanceSquared);
    const uint32_t shadowCount = static_cast<uint32_t>(std::min(
      candidates.size(), static_cast<size_t>(MaxShadowedPointLights)));
    for (uint32_t i = 0; i < shadowCount; ++i)
      s_Data.PointShadowLightIndices.push_back(candidates[i].LightIndex);
    if (s_Data.PointShadowLightIndices.empty()) return;

    if (s_Data.PointShadowMapReadable)
      TransitionResource(s_Data.PointShadowMap.Resource.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    s_Data.PointShadowMapReadable = false;

    static constexpr std::array<glm::vec3, PointShadowFaceCount> directions = {
      glm::vec3(1, 0, 0), glm::vec3(-1, 0, 0),
      glm::vec3(0, 1, 0), glm::vec3(0, -1, 0),
      glm::vec3(0, 0, 1), glm::vec3(0, 0, -1)
    };
    static constexpr std::array<glm::vec3, PointShadowFaceCount> upDirections = {
      glm::vec3(0, 1, 0), glm::vec3(0, 1, 0),
      glm::vec3(0, 0, -1), glm::vec3(0, 0, 1),
      glm::vec3(0, 1, 0), glm::vec3(0, 1, 0)
    };
    glm::mat4 correction(1.0f);
    correction[2][2] = 0.5f;
    correction[3][2] = 0.5f;
    const glm::mat4 projection = correction * glm::perspective(
      glm::radians(90.0f), 1.0f, 0.1f, PointShadowRadius);
    const D3D12_VIEWPORT viewport{0.0f, 0.0f,
      static_cast<float>(s_Data.PointShadowMapSize),
      static_cast<float>(s_Data.PointShadowMapSize), 0.0f, 1.0f};
    const D3D12_RECT scissor{0, 0, static_cast<LONG>(s_Data.PointShadowMapSize),
      static_cast<LONG>(s_Data.PointShadowMapSize)};
    const D3D12_CPU_DESCRIPTOR_HANDLE dsv = GetDSVHandle(2);
    s_Data.CommandList->RSSetViewports(1, &viewport);
    s_Data.CommandList->RSSetScissorRects(1, &scissor);
    s_Data.CommandList->SetPipelineState(s_Data.PointShadowPipeline.Get());
    s_Data.CommandList->SetGraphicsRootSignature(s_Data.PointShadowRootSignature.Get());
    s_Data.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    static constexpr float clearDistance[] = {
      PointShadowRadius, PointShadowRadius, PointShadowRadius, PointShadowRadius};
    PointShadowConstants constants;
    constants.Bones.fill(glm::mat4(1.0f));
    for (uint32_t slot = 0; slot < shadowCount; ++slot)
    {
      const glm::vec3 lightPosition = lights[s_Data.PointShadowLightIndices[slot]].position;
      for (uint32_t face = 0; face < PointShadowFaceCount; ++face)
      {
        const D3D12_CPU_DESCRIPTOR_HANDLE rtv = GetRTVHandle(
          PointShadowRTVBaseIndex + slot * PointShadowFaceCount + face);
        s_Data.CommandList->ClearRenderTargetView(rtv, clearDistance, 0, nullptr);
        s_Data.CommandList->ClearDepthStencilView(
          dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        s_Data.CommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
        constants.LightViewProjection = projection * glm::lookAt(
          lightPosition, lightPosition + directions[face], upDirections[face]);

        for (const std::string& modelName : ModelManager::GetModelNames())
        {
          const auto model = ModelManager::GetModel(modelName);
          if (!model || !model->m_IsRendered || model->GetPhysXMeshType() == MeshType::CONVEXMESH)
            continue;
          const bool animated = model->IsAnimated();
          constants.LightPositionAndAnimated = glm::vec4(
            lightPosition, animated ? 1.0f : 0.0f);
          constants.Bones.fill(glm::mat4(1.0f));
          if (animated)
          {
            const auto& bones = model->GetFinalBoneMatrices();
            std::copy_n(bones.begin(), std::min(bones.size(), constants.Bones.size()),
              constants.Bones.begin());
          }
          for (const glm::mat4& transform : model->m_InstanceTransforms)
          {
            const WorldBoundingSphere sphere = CalculateWorldBoundingSphere(*model, transform);
            if (glm::distance(sphere.center, lightPosition) > PointShadowRadius + sphere.radius)
              continue;
            constants.Model = transform;
            const D3D12_GPU_VIRTUAL_ADDRESS constantsAddress = UploadConstants(constants);
            for (const Mesh& mesh : model->GetMeshes())
            {
              const auto found = s_Data.Meshes.find(&mesh);
              if (found == s_Data.Meshes.end()) continue;
              const GPUMesh& gpu = found->second;
              s_Data.CommandList->SetGraphicsRootConstantBufferView(0, constantsAddress);
              s_Data.CommandList->IASetVertexBuffers(0, 1, &gpu.VertexView);
              s_Data.CommandList->IASetIndexBuffer(&gpu.IndexView);
              s_Data.CommandList->DrawIndexedInstanced(gpu.IndexCount, 1, 0, 0, 0);
            }
          }
        }
      }
    }

    TransitionResource(s_Data.PointShadowMap.Resource.Get(),
      D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    s_Data.PointShadowMapReadable = true;
  }

  void BeginSceneColorPass()
  {
    TransitionResource(s_Data.SceneColor.Resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                       D3D12_RESOURCE_STATE_RENDER_TARGET);
    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = GetRTVHandle(SceneRTVIndex);
    const D3D12_CPU_DESCRIPTOR_HANDLE dsv = GetDSVHandle(0);
    static constexpr float clear[] = {0.0f, 0.0f, 0.0f, 0.0f};
    s_Data.CommandList->ClearRenderTargetView(rtv, clear, 0, nullptr);
    s_Data.CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    s_Data.CommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    s_Data.CommandList->RSSetViewports(1, &s_Data.Viewport);
    s_Data.CommandList->RSSetScissorRects(1, &s_Data.ScissorRect);
  }

  void DrawSkybox()
  {
    if (!s_Data.SkyboxTexture.Resource) return;
    ID3D12DescriptorHeap* heaps[] = {s_Data.SRVHeap.Get()};
    s_Data.CommandList->SetDescriptorHeaps(1, heaps);
    s_Data.CommandList->SetPipelineState(s_Data.SkyboxPipeline.Get());
    s_Data.CommandList->SetGraphicsRootSignature(s_Data.SkyboxRootSignature.Get());
    SkyboxConstants constants;
    glm::mat4 correction(1.0f);
    correction[2][2] = 0.5f;
    correction[3][2] = 0.5f;
    constants.ViewProjection = correction * Camera::GetProjection() * glm::mat4(glm::mat3(Camera::GetViewMatrix()));
    s_Data.CommandList->SetGraphicsRootConstantBufferView(0, UploadConstants(constants));
    s_Data.CommandList->SetGraphicsRootDescriptorTable(1, GetSRVGPUHandle(s_Data.SkyboxTexture.DescriptorIndex));
    s_Data.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    s_Data.CommandList->IASetVertexBuffers(0, 1, &s_Data.SkyboxVertexView);
    s_Data.CommandList->DrawInstanced(36, 1, 0, 0);
  }

  void DrawFullscreenPass(ID3D12PipelineState* pipeline, uint32_t sourceDescriptor,
                          uint32_t bloomDescriptor, const glm::vec4& parameters,
                          const glm::vec4& effectParameters = glm::vec4(0.0f))
  {
    ID3D12DescriptorHeap* heaps[] = {s_Data.SRVHeap.Get()};
    s_Data.CommandList->SetDescriptorHeaps(1, heaps);
    s_Data.CommandList->SetPipelineState(pipeline);
    s_Data.CommandList->SetGraphicsRootSignature(s_Data.PostRootSignature.Get());
    const std::array<glm::vec4, 2> constants{parameters, effectParameters};
    s_Data.CommandList->SetGraphicsRoot32BitConstants(0, 8, constants.data(), 0);
    s_Data.CommandList->SetGraphicsRootDescriptorTable(1, GetSRVGPUHandle(sourceDescriptor));
    s_Data.CommandList->SetGraphicsRootDescriptorTable(2, GetSRVGPUHandle(bloomDescriptor));
    s_Data.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    s_Data.CommandList->DrawInstanced(3, 1, 0, 0);
  }

  void PostProcessScene(bool renderForEditor, const RenderEffectSettings& effects)
  {
    TransitionResource(s_Data.SceneColor.Resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    const bool bloomEnabled = effects.BloomQuality != GraphicsQuality::Off;
    const uint32_t bloomWidth = std::max(1u, s_Data.Width / 2u);
    const uint32_t bloomHeight = std::max(1u, s_Data.Height / 2u);
    const D3D12_VIEWPORT bloomViewport{0.0f, 0.0f, static_cast<float>(bloomWidth), static_cast<float>(bloomHeight), 0.0f, 1.0f};
    const D3D12_RECT bloomScissor{0, 0, static_cast<LONG>(bloomWidth), static_cast<LONG>(bloomHeight)};

    if (bloomEnabled)
    {
      TransitionResource(s_Data.BloomA.Resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                         D3D12_RESOURCE_STATE_RENDER_TARGET);
      auto rtv = GetRTVHandle(BloomARTVIndex);
      static constexpr float clear[] = {0, 0, 0, 0};
      s_Data.CommandList->ClearRenderTargetView(rtv, clear, 0, nullptr);
      s_Data.CommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
      s_Data.CommandList->RSSetViewports(1, &bloomViewport);
      s_Data.CommandList->RSSetScissorRects(1, &bloomScissor);
      DrawFullscreenPass(s_Data.BloomExtractPipeline.Get(), s_Data.SceneColor.DescriptorIndex,
                         s_Data.WhiteTexture.DescriptorIndex, glm::vec4(0.0f),
                         glm::vec4(effects.BloomThreshold, 0.0f, 0.0f, 0.0f));
      TransitionResource(s_Data.BloomA.Resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

      const uint32_t iterations = effects.BloomPassCount();
      for (uint32_t i = 0; i < iterations; ++i)
      {
        TransitionResource(s_Data.BloomB.Resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                           D3D12_RESOURCE_STATE_RENDER_TARGET);
        rtv = GetRTVHandle(BloomBRTVIndex);
        s_Data.CommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        DrawFullscreenPass(s_Data.BloomBlurPipeline.Get(), s_Data.BloomA.DescriptorIndex,
                           s_Data.WhiteTexture.DescriptorIndex,
                           glm::vec4(1.0f / bloomWidth, 0.0f, 0.0f, 0.0f));
        TransitionResource(s_Data.BloomB.Resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        TransitionResource(s_Data.BloomA.Resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                           D3D12_RESOURCE_STATE_RENDER_TARGET);
        rtv = GetRTVHandle(BloomARTVIndex);
        s_Data.CommandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        DrawFullscreenPass(s_Data.BloomBlurPipeline.Get(), s_Data.BloomB.DescriptorIndex,
                           s_Data.WhiteTexture.DescriptorIndex,
                           glm::vec4(0.0f, 1.0f / bloomHeight, 0.0f, 0.0f));
        TransitionResource(s_Data.BloomA.Resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
      }
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE backBuffer = GetRTVHandle(s_Data.FrameIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE outputTarget = backBuffer;
    if (renderForEditor)
    {
      TransitionResource(s_Data.PostProcessColor.Resource.Get(),
                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                         D3D12_RESOURCE_STATE_RENDER_TARGET);
      outputTarget = GetRTVHandle(PostProcessRTVIndex);
    }
    s_Data.CommandList->OMSetRenderTargets(1, &outputTarget, FALSE, nullptr);
    s_Data.CommandList->RSSetViewports(1, &s_Data.Viewport);
    s_Data.CommandList->RSSetScissorRects(1, &s_Data.ScissorRect);
    DrawFullscreenPass(s_Data.CompositePipeline.Get(), s_Data.SceneColor.DescriptorIndex,
                       s_Data.BloomA.DescriptorIndex,
                       glm::vec4(bloomEnabled ? effects.BloomStrength : 0.0f,
                         static_cast<float>(s_Data.Width), static_cast<float>(s_Data.Height),
                         effects.PS1Enabled ? 1.0f : 0.0f),
                       glm::vec4(effects.BloomExposure, effects.PS1VirtualHeight,
                         effects.PS1ColorLevels, effects.Gamma));
    if (renderForEditor)
    {
      TransitionResource(s_Data.PostProcessColor.Resource.Get(),
                         D3D12_RESOURCE_STATE_RENDER_TARGET,
                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    const D3D12_CPU_DESCRIPTOR_HANDLE dsv = GetDSVHandle(0);
    s_Data.CommandList->OMSetRenderTargets(1, &backBuffer, FALSE, &dsv);
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

#ifdef DEBUG
    s_Data.Device.As(&s_Data.InfoQueue);
#endif

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
    rtvDesc.NumDescriptors = PointShadowRTVBaseIndex
      + MaxShadowedPointLights * PointShadowFaceCount;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    CheckHRESULT(s_Data.Device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&s_Data.RTVHeap)),
                 "ID3D12Device::CreateDescriptorHeap (RTV)");
    s_Data.RTVDescriptorSize = s_Data.Device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
    dsvDesc.NumDescriptors = 3;
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
    CreateShadowPipeline();
    CreatePointShadowPipeline();
    CreateSkyboxPipeline();
    CreateParticlePipeline();
    CreateDebugLinePipeline();
    CreatePhysicsDebugPipeline();
    CreatePostProcessPipelines();
    CreateUIPipeline();
    constexpr std::array<uint8_t, 4> whitePixel = {255, 255, 255, 255};
    s_Data.WhiteTexture = UploadTextureBytes(whitePixel.data(), 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 4);
    CreateFontAtlas();
    CreatePostProcessResources(s_Data.Width, s_Data.Height);
    CreateShadowMap(RenderBackend::GetEffectSettings().DirectionalShadowResolution());
    CreatePointShadowMap(RenderBackend::GetEffectSettings().PointShadowResolution());
    s_Data.PendingUIVertices.reserve(32768);
    s_Data.PendingDebugLines.reserve(8192);
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
  ShutdownImGui();
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
  s_Data.SkyboxTexture = {};
  s_Data.SceneColor = {};
  s_Data.BloomA = {};
  s_Data.BloomB = {};
  s_Data.PostProcessColor = {};
  s_Data.ShadowMap = {};
  s_Data.PointShadowMap = {};
  s_Data.PointShadowDepth.Reset();
  s_Data.SkyboxVertexBuffer.Reset();
  s_Data.SceneRootSignature.Reset();
  s_Data.ScenePipeline.Reset();
  s_Data.ShadowRootSignature.Reset();
  s_Data.ShadowPipeline.Reset();
  s_Data.PointShadowRootSignature.Reset();
  s_Data.PointShadowPipeline.Reset();
  s_Data.SkyboxRootSignature.Reset();
  s_Data.SkyboxPipeline.Reset();
  s_Data.ParticleRootSignature.Reset();
  s_Data.ParticlePipeline.Reset();
  s_Data.DebugLineRootSignature.Reset();
  s_Data.DebugLinePipeline.Reset();
  s_Data.PhysicsDebugRootSignature.Reset();
  s_Data.PhysicsDebugPipeline.Reset();
  s_Data.PostRootSignature.Reset();
  s_Data.BloomExtractPipeline.Reset();
  s_Data.BloomBlurPipeline.Reset();
  s_Data.CompositePipeline.Reset();
  s_Data.UIRootSignature.Reset();
  s_Data.UIPipeline.Reset();
  s_Data.SceneUIPipeline.Reset();
  s_Data.SRVHeap.Reset();
  s_Data.PendingUIVertices.clear();
  s_Data.PendingDebugLines.clear();
  s_Data.PointShadowLightIndices.clear();
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
      gpuMesh.DiffuseDescriptorIndex = s_Data.WhiteTexture.DescriptorIndex;
      gpuMesh.NormalDescriptorIndex = s_Data.WhiteTexture.DescriptorIndex;
      gpuMesh.SpecularDescriptorIndex = s_Data.WhiteTexture.DescriptorIndex;
      for (const auto& texture : mesh.m_Textures)
      {
        if (!texture) continue;
        const std::string& type = texture->GetType();
        const uint32_t descriptor = GetOrCreateModelTexture(texture);
        if (type.find("normal") != std::string::npos)
        {
          gpuMesh.NormalDescriptorIndex = descriptor;
          gpuMesh.HasNormalMap = true;
        }
        else if (type.find("specular") != std::string::npos)
        {
          gpuMesh.SpecularDescriptorIndex = descriptor;
          gpuMesh.HasSpecularMap = true;
        }
        else if (type.find("diffuse") != std::string::npos ||
                 gpuMesh.DiffuseDescriptorIndex == s_Data.WhiteTexture.DescriptorIndex)
        {
          gpuMesh.DiffuseDescriptorIndex = descriptor;
        }
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

bool DirectX12Renderer::UploadSkybox(const std::shared_ptr<Texture>& cubemap)
{
  if (!s_Data.SceneRendererInitialized || !cubemap) return false;
  try
  {
    if (s_Data.SkyboxTexture.Resource)
      s_Data.RetiredResources.push_back(std::move(s_Data.SkyboxTexture.Resource));
    s_Data.SkyboxTexture = UploadCubeTexture(cubemap);
    return s_Data.SkyboxTexture.Resource != nullptr;
  }
  catch (const std::exception& error)
  {
    GABGL_ERROR("DX12 skybox upload failed: {}", error.what());
    return false;
  }
}

bool DirectX12Renderer::InitImGui()
{
  if (!s_Data.SceneRendererInitialized || !s_Data.Device || !s_Data.SRVHeap ||
      ImGui::GetCurrentContext() == nullptr)
    return false;
  if (s_Data.ImGuiInitialized) return true;

  try
  {
    s_Data.ImGuiFontDescriptor = AllocateDescriptor();
    const bool initialized = ImGui_ImplDX12_Init(
      s_Data.Device.Get(), static_cast<int>(FrameCount), BackBufferFormat,
      s_Data.SRVHeap.Get(), GetSRVCPUHandle(s_Data.ImGuiFontDescriptor),
      GetSRVGPUHandle(s_Data.ImGuiFontDescriptor));
    if (!initialized)
    {
      GABGL_ERROR("Could not initialize the Dear ImGui DirectX 12 backend");
      return false;
    }
    s_Data.ImGuiInitialized = true;
    return true;
  }
  catch (const std::exception& error)
  {
    GABGL_ERROR("Dear ImGui DirectX 12 initialization failed: {}", error.what());
    return false;
  }
}

void DirectX12Renderer::ShutdownImGui()
{
  if (!s_Data.ImGuiInitialized) return;
  try { WaitForGPU(); } catch (...) {}
  ImGui_ImplDX12_Shutdown();
  s_Data.ImGuiInitialized = false;
  s_Data.ImGuiFontDescriptor = std::numeric_limits<uint32_t>::max();
}

void DirectX12Renderer::BeginImGuiFrame()
{
  if (s_Data.ImGuiInitialized) ImGui_ImplDX12_NewFrame();
}

void DirectX12Renderer::RenderImGuiDrawData()
{
  if (!s_Data.ImGuiInitialized || !s_Data.FrameStarted || !ImGui::GetDrawData()) return;
  const D3D12_CPU_DESCRIPTOR_HANDLE backBuffer = GetRTVHandle(s_Data.FrameIndex);
  s_Data.CommandList->OMSetRenderTargets(1, &backBuffer, FALSE, nullptr);
  s_Data.CommandList->RSSetViewports(1, &s_Data.Viewport);
  s_Data.CommandList->RSSetScissorRects(1, &s_Data.ScissorRect);
  ID3D12DescriptorHeap* heaps[] = {s_Data.SRVHeap.Get()};
  s_Data.CommandList->SetDescriptorHeaps(1, heaps);
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), s_Data.CommandList.Get());
}

uint64_t DirectX12Renderer::GetEditorTextureID()
{
  if (!s_Data.PostProcessColor.Resource || !s_Data.SRVHeap) return 0;
  return GetSRVGPUHandle(s_Data.PostProcessColor.DescriptorIndex).ptr;
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
    if (s_Data.SceneRendererInitialized)
      CreatePostProcessResources(width, height);
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
    LogD3D12Messages();
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
                                  bool advanceSimulation, bool renderForEditor,
                                  const RenderEffectSettings& effects)
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
  try
  {
    const uint32_t shadowResolution = effects.DirectionalShadowResolution();
    const uint32_t pointShadowResolution = effects.PointShadowResolution();
    const bool recreateDirectionalShadow = shadowResolution != s_Data.ShadowMapSize;
    const bool recreatePointShadow = pointShadowResolution != s_Data.PointShadowMapSize;
    if (recreateDirectionalShadow || recreatePointShadow)
      WaitForGPU();
    if (recreateDirectionalShadow)
    {
      s_Data.ShadowMap.Resource.Reset();
      s_Data.ShadowMapReadable = false;
      CreateShadowMap(shadowResolution);
    }
    if (recreatePointShadow)
    {
      s_Data.PointShadowMap.Resource.Reset();
      s_Data.PointShadowDepth.Reset();
      s_Data.PointShadowMapReadable = false;
      CreatePointShadowMap(pointShadowResolution);
    }
    DrawShadowMap(effects);
    DrawPointShadowMaps(effects);
    BeginSceneColorPass();
    DrawModels(effects);
    DrawSkybox();
    ParticleRenderer::UpdateAndRender(dt);
    if (RenderBackend::DebugSettings().Physics) DrawPhysicsDebug();
    Renderer::DrawBackendDebugVisualizations();
    s_Data.UIToSceneColor = true;
    Renderer::DrawBackendDebug2D();
    s_Data.UIToSceneColor = false;
    PostProcessScene(renderForEditor, effects);
  }
  catch (const std::exception& error)
  {
    GABGL_ERROR("DirectX 12 scene rendering failed: {}", error.what());
  }
}

void DirectX12Renderer::DrawParticles(const std::vector<ParticleRenderInstance>& instances)
{
  if (!s_Data.FrameStarted || instances.empty()) return;
  std::vector<ParticleVertex> vertices;
  vertices.reserve(instances.size() * 6);
  static constexpr std::array<glm::vec2, 6> corners = {
    glm::vec2(-0.5f,-0.5f), glm::vec2(0.5f,-0.5f), glm::vec2(0.5f,0.5f),
    glm::vec2(0.5f,0.5f), glm::vec2(-0.5f,0.5f), glm::vec2(-0.5f,-0.5f)
  };
  for (const ParticleRenderInstance& instance : instances)
  {
    const float cosine = std::cos(instance.Rotation);
    const float sine = std::sin(instance.Rotation);
    for (const glm::vec2 corner : corners)
    {
      const glm::vec2 rotated(cosine * corner.x - sine * corner.y,
                              sine * corner.x + cosine * corner.y);
      const glm::vec3 world = glm::vec3(instance.PositionAndSize)
        + glm::vec3(instance.RightAndStyle) * rotated.x * instance.PositionAndSize.w
        + glm::vec3(instance.Up) * rotated.y * instance.PositionAndSize.w;
      vertices.push_back({world, instance.Color, corner * 2.0f, instance.RightAndStyle.w});
    }
  }
  auto& frame = s_Data.FrameUploads[s_Data.FrameIndex];
  const uint64_t byteCount = vertices.size() * sizeof(ParticleVertex);
  const uint64_t offset = AlignUp(frame.UIVerticesOffset, 16);
  if (offset + byteCount > UIVertexBufferBytes) return;
  std::memcpy(frame.UIVerticesMapped + offset, vertices.data(), static_cast<size_t>(byteCount));
  frame.UIVerticesOffset = offset + byteCount;
  const D3D12_VERTEX_BUFFER_VIEW view{
    frame.UIVertices->GetGPUVirtualAddress() + offset,
    static_cast<UINT>(byteCount), sizeof(ParticleVertex)};
  SkyboxConstants constants;
  constants.ViewProjection = DirectXViewProjection();
  s_Data.CommandList->SetPipelineState(s_Data.ParticlePipeline.Get());
  s_Data.CommandList->SetGraphicsRootSignature(s_Data.ParticleRootSignature.Get());
  s_Data.CommandList->SetGraphicsRootConstantBufferView(0, UploadConstants(constants));
  s_Data.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  s_Data.CommandList->IASetVertexBuffers(0, 1, &view);
  s_Data.CommandList->DrawInstanced(static_cast<UINT>(vertices.size()), 1, 0, 0);
}

void DirectX12Renderer::BeginDebugLines()
{
  s_Data.PendingDebugLines.clear();
}

void DirectX12Renderer::DrawDebugLine(const glm::vec3& start, const glm::vec3& end,
                                      const glm::vec4& color)
{
  s_Data.PendingDebugLines.push_back({start, color});
  s_Data.PendingDebugLines.push_back({end, color});
}

void DirectX12Renderer::EndDebugLines()
{
  if (!s_Data.FrameStarted || s_Data.PendingDebugLines.empty()) return;
  auto& frame = s_Data.FrameUploads[s_Data.FrameIndex];
  const uint64_t byteCount = s_Data.PendingDebugLines.size() * sizeof(DebugLineVertex);
  const uint64_t offset = AlignUp(frame.UIVerticesOffset, 16);
  if (offset + byteCount > UIVertexBufferBytes) return;
  std::memcpy(frame.UIVerticesMapped + offset, s_Data.PendingDebugLines.data(), static_cast<size_t>(byteCount));
  frame.UIVerticesOffset = offset + byteCount;
  const D3D12_VERTEX_BUFFER_VIEW view{
    frame.UIVertices->GetGPUVirtualAddress() + offset,
    static_cast<UINT>(byteCount), sizeof(DebugLineVertex)};
  SkyboxConstants constants;
  constants.ViewProjection = DirectXViewProjection();
  s_Data.CommandList->SetPipelineState(s_Data.DebugLinePipeline.Get());
  s_Data.CommandList->SetGraphicsRootSignature(s_Data.DebugLineRootSignature.Get());
  s_Data.CommandList->SetGraphicsRootConstantBufferView(0, UploadConstants(constants));
  s_Data.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
  s_Data.CommandList->IASetVertexBuffers(0, 1, &view);
  s_Data.CommandList->DrawInstanced(static_cast<UINT>(s_Data.PendingDebugLines.size()), 1, 0, 0);
  s_Data.PendingDebugLines.clear();
}

void DirectX12Renderer::DrawPhysicsDebug()
{
  if (!s_Data.FrameStarted || !s_Data.PhysicsDebugPipeline) return;

  const D3D12_CPU_DESCRIPTOR_HANDLE rtv = GetRTVHandle(SceneRTVIndex);
  const D3D12_CPU_DESCRIPTOR_HANDLE dsv = GetDSVHandle(0);
  s_Data.CommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
  s_Data.CommandList->RSSetViewports(1, &s_Data.Viewport);
  s_Data.CommandList->RSSetScissorRects(1, &s_Data.ScissorRect);
  s_Data.CommandList->SetPipelineState(s_Data.PhysicsDebugPipeline.Get());
  s_Data.CommandList->SetGraphicsRootSignature(s_Data.PhysicsDebugRootSignature.Get());
  s_Data.CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  PhysicsDebugConstants constants;
  constants.ViewProjection = DirectXViewProjection();
  for (const std::string& modelName : ModelManager::GetModelNames())
  {
    const auto model = ModelManager::GetModel(modelName);
    if (!model || model->GetPhysXMeshType() != MeshType::CONVEXMESH) continue;
    for (const glm::mat4& transform : model->m_InstanceTransforms)
    {
      constants.Model = transform;
      for (const Mesh& mesh : model->GetMeshes())
      {
        const auto found = s_Data.Meshes.find(&mesh);
        if (found == s_Data.Meshes.end()) continue;
        const GPUMesh& gpu = found->second;
        s_Data.CommandList->SetGraphicsRootConstantBufferView(0, UploadConstants(constants));
        s_Data.CommandList->IASetVertexBuffers(0, 1, &gpu.VertexView);
        s_Data.CommandList->IASetIndexBuffer(&gpu.IndexView);
        s_Data.CommandList->DrawIndexedInstanced(gpu.IndexCount, 1, 0, 0, 0);
      }
    }
  }
}

void DirectX12Renderer::BeginScene()
{
  s_Data.PendingUIVertices.clear();
}

void DirectX12Renderer::PrepareScreenUI(const glm::vec4& clearColor, bool clear)
{
  if (!s_Data.FrameStarted) return;
  s_Data.UIToSceneColor = false;
  const D3D12_CPU_DESCRIPTOR_HANDLE rtv = GetRTVHandle(s_Data.FrameIndex);
  const D3D12_CPU_DESCRIPTOR_HANDLE dsv = GetDSVHandle(0);
  if (clear)
  {
    s_Data.CommandList->ClearRenderTargetView(rtv, &clearColor.x, 0, nullptr);
    s_Data.CommandList->ClearDepthStencilView(
      dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
  }
  s_Data.CommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
  s_Data.CommandList->RSSetViewports(1, &s_Data.Viewport);
  s_Data.CommandList->RSSetScissorRects(1, &s_Data.ScissorRect);
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
    if (s_Data.UIToSceneColor)
    {
      const D3D12_CPU_DESCRIPTOR_HANDLE sceneRTV = GetRTVHandle(SceneRTVIndex);
      const D3D12_CPU_DESCRIPTOR_HANDLE dsv = GetDSVHandle(0);
      s_Data.CommandList->OMSetRenderTargets(1, &sceneRTV, FALSE, &dsv);
    }
    s_Data.CommandList->SetPipelineState(
      s_Data.UIToSceneColor ? s_Data.SceneUIPipeline.Get() : s_Data.UIPipeline.Get());
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
bool DirectX12Renderer::UploadSkybox(const std::shared_ptr<Texture>&) { return false; }
bool DirectX12Renderer::InitImGui() { return false; }
void DirectX12Renderer::ShutdownImGui() {}
void DirectX12Renderer::BeginImGuiFrame() {}
void DirectX12Renderer::RenderImGuiDrawData() {}
uint64_t DirectX12Renderer::GetEditorTextureID() { return 0; }
void DirectX12Renderer::DrawScene(DeltaTime&, const std::function<void()>&, bool, bool,
                                  const RenderEffectSettings&) {}
void DirectX12Renderer::DrawParticles(const std::vector<ParticleRenderInstance>&) {}
void DirectX12Renderer::BeginDebugLines() {}
void DirectX12Renderer::DrawDebugLine(const glm::vec3&, const glm::vec3&, const glm::vec4&) {}
void DirectX12Renderer::EndDebugLines() {}
void DirectX12Renderer::DrawPhysicsDebug() {}
void DirectX12Renderer::PrepareScreenUI(const glm::vec4&, bool) {}
void DirectX12Renderer::BeginScene() {}
void DirectX12Renderer::EndScene() {}
void DirectX12Renderer::DrawQuad(const glm::mat4&, const glm::vec4&) {}
void DirectX12Renderer::DrawText(const std::string&, const glm::vec2&, float, const glm::vec4&) {}

#endif
