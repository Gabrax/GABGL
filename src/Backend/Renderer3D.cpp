#include "Renderer3D.h"

#include "BackendLogger.h"
#include "Buffer.h"
#include <array>
#include "RendererAPI.h"
#include "glm/fwd.hpp"
#include "Shader.h"

struct MeshVertex
{
    glm::vec3 Position;   // loc=0
    glm::vec3 Normal;     // loc=1
    glm::vec4 Color;      // loc=3
    int EntityID;         // loc=5
};

struct Renderer3DData
{
    static const uint32_t MaxVertices = 100000;
    static const uint32_t MaxIndices = MaxVertices * 6 / 4; // cube has 6 indices per 4 vertices
    static const uint32_t MaxTextureSlots = 32;

    std::shared_ptr<VertexArray> CubeVertexArray;
    std::shared_ptr<VertexBuffer> CubeVertexBuffer;
    std::shared_ptr<IndexBuffer> CubeIndexBuffer;

    std::array<std::shared_ptr<Texture>, MaxTextureSlots> TextureSlots;
    uint32_t TextureSlotIndex = 1;

    struct Shaders3D
    {
        std::shared_ptr<Shader> modelShader;
        std::shared_ptr<Shader> skyboxShader;
    } _shaders3D;

    Renderer3D::Statistics Stats;

    struct CameraData
    {
        glm::mat4 ViewProjection;
    } CameraBuffer;
    std::shared_ptr<UniformBuffer> CameraUniformBuffer;

    uint32_t CubeIndexCount = 0;
    uint32_t CubeVertexCount = 0;

    MeshVertex* CubeVertexBufferBase = nullptr;
    MeshVertex* CubeVertexBufferPtr = nullptr;

} s_Data;


void Renderer3D::LoadShaders()
{
  s_Data._shaders3D.modelShader = Shader::Create("res/shaders/Renderer3D_static.glsl");
}

void Renderer3D::Init()
{
    // Allocate dynamic vertex buffer memory on CPU for batching
    s_Data.CubeVertexBufferBase = new MeshVertex[s_Data.MaxVertices];

    // Create empty GPU vertex buffer, large enough for max vertices, dynamic usage
    s_Data.CubeVertexBuffer = VertexBuffer::Create(nullptr, s_Data.MaxVertices * sizeof(MeshVertex));
    s_Data.CubeVertexBuffer->SetLayout({
        { ShaderDataType::Float3, "a_Position" },   // 0
        { ShaderDataType::Float3, "a_Normal" },     // 1
        { ShaderDataType::Float4, "a_Color" },      // 3
        { ShaderDataType::Int,    "a_EntityID" }    // 5
    });

    // Precompute large index buffer for max cubes
    std::vector<uint32_t> indices;
    indices.reserve(s_Data.MaxIndices);

    // Each cube has 36 indices (6 faces * 2 triangles * 3 vertices)
    // Your cube uses 8 vertices and 36 indices, but indexing method can be adjusted.
    // Let's define a cube with 36 indices (12 triangles):
    // We'll replicate the cube indices for each cube, offsetting by 8 vertices.

    uint32_t offset = 0;
    const uint32_t cubeIndicesCount = 36;

    // Cube indices (36) for the standard cube with 8 vertices:
    uint32_t cubeIndices[36] = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        4, 0, 3, 3, 7, 4,       // Left
        1, 5, 6, 6, 2, 1,       // Right
        3, 2, 6, 6, 7, 3,       // Top
        4, 5, 1, 1, 0, 4        // Bottom
    };

    uint32_t maxCubes = s_Data.MaxVertices / 8; // 8 vertices per cube
    for (uint32_t i = 0; i < maxCubes; i++)
    {
        for (uint32_t j = 0; j < cubeIndicesCount; j++)
        {
            indices.push_back(cubeIndices[j] + i * 8);
        }
    }

    s_Data.CubeIndexBuffer = IndexBuffer::Create(indices.data(), (uint32_t)indices.size());

    // Setup Vertex Array
    s_Data.CubeVertexArray = VertexArray::Create();
    s_Data.CubeVertexArray->AddVertexBuffer(s_Data.CubeVertexBuffer);
    s_Data.CubeVertexArray->SetIndexBuffer(s_Data.CubeIndexBuffer);

    s_Data.CubeIndexCount = 0;
    s_Data.CubeVertexCount = 0;
    s_Data.CubeVertexBufferPtr = s_Data.CubeVertexBufferBase;

    LoadShaders();

    s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::CameraData), 0);
}

void Renderer3D::Shutdown()
{
    delete[] s_Data.CubeVertexBufferBase;
    s_Data.CubeVertexBufferBase = nullptr;

    s_Data.CubeVertexArray.reset();
    s_Data.CubeVertexBuffer.reset();
    s_Data.CubeIndexBuffer.reset();
}

void Renderer3D::StartBatch()
{
    s_Data.CubeVertexCount = 0;
    s_Data.CubeIndexCount = 0;
    s_Data.CubeVertexBufferPtr = s_Data.CubeVertexBufferBase;
    s_Data.TextureSlotIndex = 1;
}

void Renderer3D::Flush()
{
    if (s_Data.CubeIndexCount == 0) return; // Nothing to draw

    // Upload vertex data to GPU
    uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CubeVertexBufferPtr - (uint8_t*)s_Data.CubeVertexBufferBase);
    s_Data.CubeVertexBuffer->SetData(s_Data.CubeVertexBufferBase, dataSize);

    s_Data._shaders3D.modelShader->Use();
    s_Data.CubeVertexArray->Bind();

    RendererAPI::DrawIndexed(s_Data.CubeVertexArray, s_Data.CubeIndexCount);

    s_Data.Stats.DrawCalls++;
}

void Renderer3D::EndScene()
{
    Flush();
}

void Renderer3D::NextBatch()
{
    Flush();
    StartBatch();
}

// Cube vertices in object space (static, reused for all cubes)
static MeshVertex cubeVertices[8] = {
    {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
    {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
    {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
    {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},

    {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
    {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
    {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
    {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1.0f, 1.0f, 1.0f, 1.0f}, 0},
};

// Call this to batch a cube instance
void Renderer3D::DrawCube(const TransformComponent& transform, int entityID)
{
    if (s_Data.CubeIndexCount >= Renderer3DData::MaxIndices)
        NextBatch(); // Flush batch if full

    glm::mat4 model = transform.GetTransform();
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

    for (int i = 0; i < 8; i++)
    {
        MeshVertex vertex = cubeVertices[i];

        // Transform position
        glm::vec4 transformedPos = model * glm::vec4(vertex.Position, 1.0f);
        vertex.Position = glm::vec3(transformedPos);
        vertex.Normal = glm::normalize(normalMatrix * vertex.Normal);
        vertex.Color = glm::vec4(2.0f);
        vertex.EntityID = entityID;

        *s_Data.CubeVertexBufferPtr = vertex;
        s_Data.CubeVertexBufferPtr++;
    }

    s_Data.CubeVertexCount += 8;
    s_Data.CubeIndexCount += 36; // 6 faces * 2 triangles * 3 indices = 36 indices per cube
}

void Renderer3D::BeginScene(const Camera& camera, const glm::mat4& transform)
{
    s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
    s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));

    StartBatch();
}

void Renderer3D::BeginScene(const Camera& camera)
{
    s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
    s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));

    StartBatch();
}
