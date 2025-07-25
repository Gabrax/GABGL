#include "ModelManager.h"
#include "BackendLogger.h"
#include "glad/glad.h"
#include "meshoptimizer.h"
#include <filesystem>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Renderer.h"

static glm::mat4 AssimpMatToGLMMat(const aiMatrix4x4& from)
{
  glm::mat4 to;
  //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
  to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
  to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
  to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
  to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
  return to;
}

static glm::vec3 AssimpVecToGLMVec(const aiVector3D& vec) 
{ 
  return glm::vec3(vec.x, vec.y, vec.z); 
}

static glm::quat AssimpQuatToGLMQuat(const aiQuaternion& pOrientation)
{
  return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
}

struct MeshTextureRange
{
  uint32_t StartIndex; // Offset in the textureHandles array
  uint32_t Count;      // How many textures this mesh has
};

struct ModelsData
{
  std::unordered_map<std::string, std::shared_ptr<Model>> m_Models;
  std::vector<std::string> m_ModelsNames;

  std::shared_ptr<StorageBuffer> m_ModelsTransforms;
  std::shared_ptr<StorageBuffer> m_MeshToTransformSSBO;
  std::shared_ptr<StorageBuffer> m_BindlessTextureSSBO;
  std::shared_ptr<StorageBuffer> m_NormalMapFlagsSSBO;
  std::shared_ptr<StorageBuffer> m_SpecularMapFlagsSSBO;
  std::shared_ptr<StorageBuffer> m_MeshToTextureRangeSSBO;
  std::shared_ptr<StorageBuffer> m_FinalBoneMatricesSSBO;
  std::shared_ptr<StorageBuffer> m_ModelIsAnimatedSSBO; 

  GLuint sharedVBO, sharedEBO, sharedVAO;

  std::vector<Vertex> allVertices;
  std::vector<uint32_t> allIndices;

} s_Data; 

void ModelManager::Init()
{
  if (s_Data.sharedVBO == 0)
    glCreateBuffers(1, &s_Data.sharedVBO);
  if (s_Data.sharedEBO == 0)
    glCreateBuffers(1, &s_Data.sharedEBO);
  if (s_Data.sharedVAO == 0)
    glCreateVertexArrays(1, &s_Data.sharedVAO);

  glVertexArrayVertexBuffer(s_Data.sharedVAO, 0, s_Data.sharedVBO, 0, sizeof(Vertex));
  glVertexArrayElementBuffer(s_Data.sharedVAO, s_Data.sharedEBO);

  struct Attribute
  {
    GLint size;
    GLenum type;
    GLboolean normalized;
    size_t offset;
  };

  std::array<Attribute, 7> attributes =
  {{
    {3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Position)},
    {3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Normal)},
    {2, GL_FLOAT, GL_FALSE, offsetof(Vertex, TexCoords)},
    {3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Tangent)},
    {3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Bitangent)},
    {4, GL_INT,   GL_FALSE, offsetof(Vertex, m_BoneIDs)},
    {4, GL_FLOAT, GL_FALSE, offsetof(Vertex, m_Weights)}
  }};

  for (GLuint i = 0; i < attributes.size(); ++i)
  {
    glEnableVertexArrayAttrib(s_Data.sharedVAO, i);
    if (attributes[i].type == GL_INT)
      glVertexArrayAttribIFormat(s_Data.sharedVAO, i, attributes[i].size, attributes[i].type, attributes[i].offset);
    else
      glVertexArrayAttribFormat(s_Data.sharedVAO, i, attributes[i].size, attributes[i].type, attributes[i].normalized, attributes[i].offset);
    glVertexArrayAttribBinding(s_Data.sharedVAO, i, 0);
  }
}

void ModelManager::BakeModel(const std::string& path, const std::shared_ptr<Model>& model)
{
  Timer timer;

  constexpr int NUM_BUFFERS = 2;
  std::array<std::unique_ptr<PixelBuffer>, NUM_BUFFERS> pboBuffers;
  int currentPBO = 0;

  // Check if model has exactly one texture total shared by all meshes
  bool singleTextureModel = false;
  GLuint64 sharedTextureHandle = 0;

  if (model->GetMeshes().size() > 0)
  {
    auto& firstMesh = model->GetMeshes()[0];
    if (firstMesh.m_Textures.size() == 1)
    {
      auto sharedTexture = firstMesh.m_Textures[0];
      singleTextureModel = true;

      for (size_t i = 1; i < model->GetMeshes().size(); ++i)
      {
        auto& mesh = model->GetMeshes()[i];
        if (mesh.m_Textures.size() != 1 || mesh.m_Textures[0] != sharedTexture)
        {
          singleTextureModel = false;
          break;
        }
      }
    }
  }

  if (singleTextureModel)
  {
    // Bake the texture only for the first mesh
    auto& firstMesh = model->GetMeshes()[0];
    firstMesh.m_TexturesBindlessHandles.clear();

    auto& texture = firstMesh.m_Textures[0];
    if (texture)
    {
      int width, height;
      GLenum format;
      const void* srcData;
      GLsizei dataSize;

      if (texture->IsUnCompressed())
      {
        auto* embeddedTex = texture->GetEmbeddedTexture();
        if (embeddedTex && embeddedTex->pcData)
        {
          width = embeddedTex->mWidth;
          height = embeddedTex->mHeight;
          format = GL_RGBA;
          dataSize = width * height * 4;
          srcData = embeddedTex->pcData;
        }
        else
          srcData = nullptr;
      }
      else
      {
        width = texture->GetWidth();
        height = texture->GetHeight();
        format = texture->GetDataFormat();
        if (format != GL_RGB && format != GL_RGBA)
          format = GL_RGBA;

        int bytesPerPixel = (format == GL_RGBA) ? 4 : 3;
        dataSize = width * height * bytesPerPixel;
        srcData = texture->GetRawData();
      }

      if (srcData && width > 0 && height > 0)
      {
        if (!pboBuffers[currentPBO] || pboBuffers[currentPBO]->GetSize() != dataSize)
          pboBuffers[currentPBO] = std::make_unique<PixelBuffer>(dataSize);

        auto& pbo = pboBuffers[currentPBO];
        pbo->WaitForCompletion();

        void* ptr = pbo->Map();
        if (ptr)
        {
          memcpy(ptr, srcData, dataSize);
          pbo->Unmap(); // Also inserts a sync
        }
        else
        {
          GABGL_ERROR("Failed to map PixelBuffer for texture upload.");
          // fallback - don't assign texture
        }

        GLuint id;
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        texture->SetRendererID(id);

        glTextureStorage2D(id, 1, GL_RGBA8, width, height);
        pbo->Bind();
        glTextureSubImage2D(id, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, nullptr);
        pbo->Unbind();

        glGenerateTextureMipmap(id);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        sharedTextureHandle = glGetTextureHandleARB(id);
        glMakeTextureHandleResidentARB(sharedTextureHandle);

        firstMesh.m_TexturesBindlessHandles.push_back(sharedTextureHandle);

        currentPBO = (currentPBO + 1) % NUM_BUFFERS;
      }
    }

    // Now assign the same handle to all other meshes
    for (size_t i = 1; i < model->GetMeshes().size(); ++i)
    {
      auto& mesh = model->GetMeshes()[i];
      mesh.m_TexturesBindlessHandles.clear();
      if (sharedTextureHandle != 0)
        mesh.m_TexturesBindlessHandles.push_back(sharedTextureHandle);
    }
  }
  else
  {
    // Normal per-mesh texture baking
    for (auto& mesh : model->GetMeshes())
    {
      mesh.m_TexturesBindlessHandles.clear();

      for (auto& texture : mesh.m_Textures)
      {
        if (!texture)
            continue;

        int width, height;
        GLenum format;
        const void* srcData;
        GLsizei dataSize;

        if (texture->IsUnCompressed())
        {
            auto* embeddedTex = texture->GetEmbeddedTexture();
            if (!embeddedTex || !embeddedTex->pcData)
                continue;

            width = embeddedTex->mWidth;
            height = embeddedTex->mHeight;
            format = GL_RGBA;
            dataSize = width * height * 4;
            srcData = embeddedTex->pcData;
        }
        else
        {
            width = texture->GetWidth();
            height = texture->GetHeight();
            format = texture->GetDataFormat();
            if (format != GL_RGB && format != GL_RGBA)
                format = GL_RGBA;

            int bytesPerPixel = (format == GL_RGBA) ? 4 : 3;
            dataSize = width * height * bytesPerPixel;
            srcData = texture->GetRawData();
        }

        if (!srcData || width <= 0 || height <= 0)
            continue;

        if (!pboBuffers[currentPBO] || pboBuffers[currentPBO]->GetSize() != dataSize)
            pboBuffers[currentPBO] = std::make_unique<PixelBuffer>(dataSize);

        auto& pbo = pboBuffers[currentPBO];

        pbo->WaitForCompletion();

        void* ptr = pbo->Map();
        if (ptr)
        {
            memcpy(ptr, srcData, dataSize);
            pbo->Unmap(); // Also inserts a sync
        }
        else
        {
            GABGL_ERROR("Failed to map PixelBuffer for texture upload.");
            continue;
        }

        GLuint id;
        glCreateTextures(GL_TEXTURE_2D, 1, &id);
        texture->SetRendererID(id);

        glTextureStorage2D(id, 1, GL_RGBA8, width, height);
        pbo->Bind();
        glTextureSubImage2D(id, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, nullptr);
        pbo->Unbind();

        glGenerateTextureMipmap(id);
        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT );
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLuint64 handle = glGetTextureHandleARB(id);
        glMakeTextureHandleResidentARB(handle);

        mesh.m_TexturesBindlessHandles.push_back(handle);

        currentPBO = (currentPBO + 1) % NUM_BUFFERS;
      }
    }
  }

  std::string name = std::filesystem::path(path).stem().string();

  for (auto& mesh : model->GetMeshes())
  {
    s_Data.allVertices.insert(s_Data.allVertices.end(), mesh.m_Vertices.begin(), mesh.m_Vertices.end());
    s_Data.allIndices.insert(s_Data.allIndices.end(), mesh.m_Indices.begin(), mesh.m_Indices.end());

    Renderer::AddDrawCommand(name, static_cast<uint32_t>(mesh.m_Vertices.size()), static_cast<uint32_t>(mesh.m_Indices.size()));

    for(auto& tex : mesh.m_Textures) tex->ClearRawData();

    if(model->GetPhysXMeshType() == MeshType::TRIANGLEMESH) model->CreatePhysXStaticMesh(mesh.m_Vertices, mesh.m_Indices);
    else if(model->GetPhysXMeshType() == MeshType::CONVEXMESH) model->CreatePhysXDynamicMesh(mesh.m_Vertices);
  }

  model->m_Name = name;
  s_Data.m_Models[name] = std::move(model);
  s_Data.m_ModelsNames.emplace_back(name);

  GABGL_WARN("Model: {0} baking took {1} ms", name, timer.ElapsedMillis());
}

void ModelManager::SetInitialControllerTransform(const std::string& name, const Transform& transform, float radius, float height, bool slopeLimit)
{
  auto it = s_Data.m_Models.find(name);
  if (it == s_Data.m_Models.end())
  {
      GABGL_WARN("Model '{}' not found in ModelManager!", name);
      return;
  }

  std::shared_ptr<Model> model = it->second;

  if(model->GetPhysXMeshType() != MeshType::CONTROLLER)
  {
    GABGL_ERROR("This function is for Controller model!");
    return;
  }

  model->m_ControllerTransform = transform;

  model->CreateCharacterController(PhysX::GlmVec3ToPxVec3(model->m_ControllerTransform.GetPosition()), radius, height, slopeLimit);

  // Find index in the names vector
  auto vecIt = std::find(s_Data.m_ModelsNames.begin(), s_Data.m_ModelsNames.end(), name);
  if (vecIt == s_Data.m_ModelsNames.end())
  {
      GABGL_WARN("Model '{}' not found in name list for SSBO!", name);
      return;
  }

  int ssboIndex = static_cast<int>(std::distance(s_Data.m_ModelsNames.begin(), vecIt));
  s_Data.m_ModelsTransforms->SetSubData(ssboIndex * sizeof(glm::mat4),sizeof(glm::mat4),glm::value_ptr(model->m_ControllerTransform.GetTransform()));
}

void ModelManager::SetInitialModelTransform(const std::string& name, const glm::mat4& transform)
{
  auto it = s_Data.m_Models.find(name);
  if (it == s_Data.m_Models.end())
  {
      GABGL_WARN("Model '{}' not found in ModelManager!", name);
      return;
  }

  const auto& model = it->second;

  if(model->GetPhysXMeshType() == MeshType::CONTROLLER)
  {
    GABGL_ERROR("This function is for non Controller model!");
    return;
  }

  std::string convexName = name + "_convex";

  auto convexIt = s_Data.m_Models.find(convexName);
  if (convexIt != s_Data.m_Models.end())
  {
    GABGL_WARN("Convex version '{}' found for model '{}'. Applying same transform.", convexName, name);

    const auto& convex = convexIt->second;

    PxTransform pxTransform = PxTransform(PhysX::GlmMat4ToPxTransform(transform));

    if(convex->GetPhysXMeshType() == MeshType::TRIANGLEMESH) convex->m_StaticMeshActor->setGlobalPose(pxTransform);
    else if(convex->GetPhysXMeshType() == MeshType::CONVEXMESH)
    {
      if (convex->m_isKinematic) convex->m_DynamicMeshActor->setKinematicTarget(pxTransform);
      else convex->m_DynamicMeshActor->setGlobalPose(pxTransform);
    }

    glm::mat4 convexTransform = PhysX::PxMat44ToGlmMat4(convexIt->second->GetDynamicActor()->getGlobalPose());

    pxTransform = PxTransform(PhysX::GlmMat4ToPxTransform(convexTransform));

    if(model->GetPhysXMeshType() == MeshType::TRIANGLEMESH) model->m_StaticMeshActor->setGlobalPose(pxTransform);
    else if(model->GetPhysXMeshType() == MeshType::CONVEXMESH)
    {
      if (model->m_isKinematic) model->m_DynamicMeshActor->setKinematicTarget(pxTransform);
      else model->m_DynamicMeshActor->setGlobalPose(pxTransform);
    }

    auto vec2It = std::find(s_Data.m_ModelsNames.begin(), s_Data.m_ModelsNames.end(), name);
    if (vec2It != s_Data.m_ModelsNames.end())
    {
      int ssboIndex = static_cast<int>(std::distance(s_Data.m_ModelsNames.begin(), vec2It));
      s_Data.m_ModelsTransforms->SetSubData(ssboIndex * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(convexTransform));
    }
    else
    {
      GABGL_WARN("Main model '{}' not found in name list for SSBO!", name);
    }
  }
  else
  {
    // No convex model, fallback to normal update
    auto vecIt = std::find(s_Data.m_ModelsNames.begin(), s_Data.m_ModelsNames.end(), name);
    if (vecIt == s_Data.m_ModelsNames.end())
    {
      GABGL_WARN("Model '{}' not found in name list for SSBO!", name);
      return;
    }

    int ssboIndex = static_cast<int>(std::distance(s_Data.m_ModelsNames.begin(), vecIt));
    s_Data.m_ModelsTransforms->SetSubData(ssboIndex * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(transform));
  }
}

void ModelManager::UpdateTransforms(const DeltaTime& dt)
{
  for (const auto& [key, model] : s_Data.m_Models)
  {
    if(model->IsAnimated() && model->m_IsRendered)
    { 
      auto& transforms = model->GetFinalBoneMatrices();

      auto nameIt = std::find(s_Data.m_ModelsNames.begin(), s_Data.m_ModelsNames.end(), key);
      if (nameIt != s_Data.m_ModelsNames.end())
      {
        int ssboIndex = static_cast<int>(std::distance(s_Data.m_ModelsNames.begin(), nameIt));
        size_t offset = ssboIndex * MAX_BONES * sizeof(glm::mat4); // assuming MAX_BONES is defined
        size_t size = transforms.size() * sizeof(glm::mat4);

        s_Data.m_FinalBoneMatricesSSBO->SetSubData(offset, size, transforms.data());
        model->UpdateAnimation(dt);
      }
      else
      {
        GABGL_WARN("Animated model '{}' not found in name list for bone SSBO!", key);
      }
    }

    const std::string& convexName = key;
    constexpr const char* suffix = "_convex";
    if (convexName.size() < 7 || convexName.compare(convexName.size() - 7, 7, suffix) != 0) continue;

    std::string baseName = convexName.substr(0, convexName.size() - 7);

    auto baseModelIt = s_Data.m_Models.find(baseName);
    if (baseModelIt == s_Data.m_Models.end())
    {
      GABGL_WARN("Base model '{}' not found for convex '{}'", baseName, convexName);
      continue;
    }

    if(baseModelIt->second->m_IsRendered)
    {
      glm::mat4 convexTransform = PhysX::PxMat44ToGlmMat4(model->GetDynamicActor()->getGlobalPose());

      auto nameIt = std::find(s_Data.m_ModelsNames.begin(), s_Data.m_ModelsNames.end(), baseName);
      if (nameIt != s_Data.m_ModelsNames.end())
      {
        int ssboIndex = static_cast<int>(std::distance(s_Data.m_ModelsNames.begin(), nameIt));
        s_Data.m_ModelsTransforms->SetSubData(ssboIndex * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(convexTransform));
      }
      else
      {
        GABGL_WARN("Base model '{}' not found in name list for SSBO!", baseName);
      }
    }
  }
}

void ModelManager::SetRender(const std::string& name, bool render)
{
  auto it = s_Data.m_Models.find(name);

  it->second->m_IsRendered = render;

  Renderer::RebuildDrawCommandsForModel(it->second,render);
}

GLsizei ModelManager::GetModelsQuantity()
{
  return s_Data.m_Models.size();
}

GLuint ModelManager::GetModelsVAO()
{
  return s_Data.sharedVAO;
}

void ModelManager::UploadToGPU()
{
  glNamedBufferStorage(s_Data.sharedVBO, s_Data.allVertices.size() * sizeof(Vertex), s_Data.allVertices.data(), 0);
  glNamedBufferStorage(s_Data.sharedEBO, s_Data.allIndices.size() * sizeof(uint32_t), s_Data.allIndices.data(), 0);

  s_Data.m_ModelsTransforms = StorageBuffer::Create(sizeof(glm::mat4) * s_Data.m_Models.size(), 5);

  auto transform = GetTransforms();
  s_Data.m_ModelsTransforms->SetData(transform.size() * sizeof(glm::mat4), transform.data());

  std::vector<int> meshToTransformIndex;

  int currentMeshIndex = 0;
  for (int modelIndex = 0; modelIndex < s_Data.m_ModelsNames.size(); ++modelIndex)
  {
      const std::string& modelName = s_Data.m_ModelsNames[modelIndex];
      std::shared_ptr<Model> model = s_Data.m_Models[modelName];

      int meshCount = model->GetMeshes().size(); 

      for (int i = 0; i < meshCount; ++i)
      {
          meshToTransformIndex.push_back(modelIndex); 
          currentMeshIndex++;
      }
  }

  s_Data.m_MeshToTransformSSBO = StorageBuffer::Create(meshToTransformIndex.size() * sizeof(int), 6);
  s_Data.m_MeshToTransformSSBO->SetData(meshToTransformIndex.size() * sizeof(int), meshToTransformIndex.data());

  std::vector<GLuint64> textureHandles;
  std::vector<MeshTextureRange> meshTextureRanges;
  std::vector<int32_t> normalMapFlags;
  std::vector<int32_t> specularMapFlags;

  for (const auto& modelName : s_Data.m_ModelsNames)
  {
    auto& model = s_Data.m_Models[modelName];
    const auto& meshes = model->GetMeshes();

    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex)
    {
      const auto& mesh = meshes[meshIndex];

      mesh.hasNormalMap ? normalMapFlags.push_back(1) : normalMapFlags.push_back(0);  
      mesh.hasSpecularMap ? specularMapFlags.push_back(1) : specularMapFlags.push_back(0);  

      MeshTextureRange range;
      range.StartIndex = static_cast<uint32_t>(textureHandles.size());
      range.Count = static_cast<uint32_t>(mesh.m_TexturesBindlessHandles.size());

      for (GLuint64 handle : mesh.m_TexturesBindlessHandles) 
      {
       textureHandles.push_back(handle);
      }

      meshTextureRanges.push_back(range);
    }
  }

  s_Data.m_BindlessTextureSSBO = StorageBuffer::Create(textureHandles.size() * sizeof(GLuint64), 7);
  s_Data.m_BindlessTextureSSBO->SetData(textureHandles.size() * sizeof(GLuint64), textureHandles.data());

  s_Data.m_MeshToTextureRangeSSBO = StorageBuffer::Create(meshTextureRanges.size() * sizeof(MeshTextureRange), 8);
  s_Data.m_MeshToTextureRangeSSBO->SetData(meshTextureRanges.size() * sizeof(MeshTextureRange), meshTextureRanges.data());
  
  std::vector<glm::mat4> identityBones(s_Data.m_Models.size() * MAX_BONES, glm::mat4(1.0f));

  s_Data.m_FinalBoneMatricesSSBO = StorageBuffer::Create(identityBones.size() * sizeof(glm::mat4), 9);
  s_Data.m_FinalBoneMatricesSSBO->SetData(identityBones.size() * sizeof(glm::mat4), identityBones.data());

  std::vector<int> isAnimatedFlags;

  for (const auto& modelName : s_Data.m_ModelsNames)
  {
      const auto& model = s_Data.m_Models[modelName];
      isAnimatedFlags.push_back(model->IsAnimated() ? 1 : 0);
  }

  s_Data.m_ModelIsAnimatedSSBO = StorageBuffer::Create(isAnimatedFlags.size() * sizeof(int), 10); 
  s_Data.m_ModelIsAnimatedSSBO->SetData(isAnimatedFlags.size() * sizeof(int), isAnimatedFlags.data());

  s_Data.m_NormalMapFlagsSSBO = StorageBuffer::Create(normalMapFlags.size() * sizeof(int), 11); 
  s_Data.m_NormalMapFlagsSSBO->SetData(normalMapFlags.size() * sizeof(int), normalMapFlags.data());

  s_Data.m_SpecularMapFlagsSSBO = StorageBuffer::Create(specularMapFlags.size() * sizeof(int), 12); 
  s_Data.m_SpecularMapFlagsSSBO->SetData(specularMapFlags.size() * sizeof(int), specularMapFlags.data());

  for (const auto& modelName : s_Data.m_Models)
  {
    for(auto& bruh : modelName.second->m_Meshes)
    {
      for(auto& hehe : bruh.m_Textures)
      {
        GABGL_WARN("TYPE OF TEXTURE: {0}",hehe->GetType());
      }
    }
  }

  s_Data.allVertices.clear();
  s_Data.allIndices.clear();
}

void ModelManager::BakeModelInstancedBuffers(Mesh& mesh, const std::vector<Transform>& transforms)
{
  if (transforms.empty()) return;

  std::vector<glm::mat4> instanceMatrices;
  instanceMatrices.reserve(transforms.size());

  for (const auto& t : transforms) instanceMatrices.push_back(t.GetTransform());

  if (mesh.instanceVBO == 0) glCreateBuffers(1, &mesh.instanceVBO);

  static size_t lastSize = 0;
  size_t newSize = instanceMatrices.size() * sizeof(glm::mat4);
  if (lastSize != newSize) {
      glNamedBufferStorage(mesh.instanceVBO, newSize, instanceMatrices.data(), 0); // immutable
      lastSize = newSize;
  } else {
      glNamedBufferSubData(mesh.instanceVBO, 0, newSize, instanceMatrices.data());
  }

  glVertexArrayVertexBuffer(mesh.VAO, 1, mesh.instanceVBO, 0, sizeof(glm::mat4));

  if (!mesh.instanceAttribsConfigured)
  {
    for (GLuint i = 0; i < 4; ++i)
    {
      GLuint loc = 7 + i; // instance matrix starts at attribute 7
      glEnableVertexArrayAttrib(mesh.VAO, loc);
      glVertexArrayAttribFormat(mesh.VAO, loc, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4) * i);
      glVertexArrayAttribBinding(mesh.VAO, loc, 1); // bind to binding index 1
      glVertexArrayBindingDivisor(mesh.VAO, 1, 1);  // instanced per-instance
    }

    mesh.instanceAttribsConfigured = true;
  }
}

void ModelManager::MoveController(const std::string& name, const Movement& movement, float speed, const DeltaTime& dt)
{
  auto it = s_Data.m_Models.find(name);
  if (it == s_Data.m_Models.end()) GABGL_ERROR("Model doesnt exist!");

  auto vecIt = std::find(s_Data.m_ModelsNames.begin(), s_Data.m_ModelsNames.end(), name);
  if (vecIt == s_Data.m_ModelsNames.end())
  {
      GABGL_WARN("Model '{}' not found in name list for SSBO!", name);
      return;
  }

  const auto& model = it->second;

  if (model->GetPhysXMeshType() != MeshType::CONTROLLER)
  {
      GABGL_ERROR("This function is for Controller model!");
      return;
  }

  // Constants
  const float gravity = -9.81f;
  const float damping = 0.9f;
  const float jumpSpeed = 5.5f;

  // Apply gravity
  model->m_ControllerVelocity.y += gravity * dt;

  // Horizontal movement
  PxVec3 direction(0.0f);
  switch (movement)
  {
      case Movement::FORWARD:  direction.z += 1.0f; break;
      case Movement::BACKWARD: direction.z -= 1.0f; break;
      case Movement::LEFT:     direction.x -= 1.0f; break;
      case Movement::RIGHT:    direction.x += 1.0f; break;
      default: break;
  }

  if (direction.magnitudeSquared() > 0.0f)
  {
      direction = direction.getNormalized();
      model->m_ControllerVelocity.x = direction.x * speed;
      model->m_ControllerVelocity.z = direction.z * speed;
  }
  else
  {
      // Apply damping when no input
      model->m_ControllerVelocity.x *= damping;
      model->m_ControllerVelocity.z *= damping;
  }

  // Move controller
  PxVec3 displacement = model->m_ControllerVelocity * dt;
  PxControllerCollisionFlags flags = model->GetController()->move(displacement, 0.001f, dt, PxControllerFilters());

  // Ground check and vertical reset
  if (flags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN))
  {
      model->m_ControllerVelocity.y = 0.0f;
      model->m_ControllerIsGrounded = true;
  }
  else
  {
      model->m_ControllerIsGrounded = false;
  }

  PxExtendedVec3 footPos = model->GetController()->getFootPosition();
  model->m_ControllerTransform.SetPosition(glm::vec3(footPos.x, footPos.y, footPos.z));

  int ssboIndex = static_cast<int>(std::distance(s_Data.m_ModelsNames.begin(), vecIt));
  s_Data.m_ModelsTransforms->SetSubData(ssboIndex * sizeof(glm::mat4),sizeof(glm::mat4),glm::value_ptr(model->m_ControllerTransform.GetTransform()));
}

std::shared_ptr<Model> ModelManager::GetModel(const std::string& name)
{
  auto it = s_Data.m_Models.find(name);
  if (it != s_Data.m_Models.end())
      return it->second;
  return nullptr;
}

std::vector<glm::mat4> ModelManager::GetTransforms()
{
  std::vector<glm::mat4> transforms;
  transforms.reserve(s_Data.m_Models.size());

  for (const auto& [key, model] : s_Data.m_Models)
  {
    if (model->GetPhysXMeshType() == MeshType::TRIANGLEMESH)
    {
        glm::mat4 mat = PhysX::PxMat44ToGlmMat4(model->GetStaticActor()->getGlobalPose());
        transforms.push_back(mat);
    }
    else if (model->GetPhysXMeshType() == MeshType::CONVEXMESH)
    {
        glm::mat4 mat = PhysX::PxMat44ToGlmMat4(model->GetDynamicActor()->getGlobalPose());
        transforms.push_back(mat);
    }
    else if (model->GetPhysXMeshType() == MeshType::CONTROLLER)
    {
        glm::mat4 mat = model->GetControllerTransform().GetTransform();
        transforms.push_back(mat);
    }
  }

  return transforms;
}

Model::Model(const char* path, float optimizerStrength, bool isAnimated, bool isKinematic, const MeshType& type) :  m_isKinematic(isKinematic), m_OptimizerStrength(optimizerStrength), m_isAnimated(isAnimated), m_meshType(type)
{
  Timer timer;

  Assimp::Importer importer;
  if(m_isAnimated) m_Scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);
  else m_Scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);

  if (!m_Scene || m_Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_Scene->mRootNode)
  {
    GABGL_ERROR("[MODEL]: {0}", (std::string)importer.GetErrorString());
    return;
  }
  std::string dirStr = std::filesystem::path(path).parent_path().string();
  m_Directory = dirStr.c_str();  
  processNode(m_Scene->mRootNode, m_Scene);

  if(isAnimated)
  {
    for (unsigned int i = 0; i < m_Scene->mNumAnimations; ++i)
    {
      aiAnimation* animation = m_Scene->mAnimations[i];

      AnimationData animData;
      animData.name = animation->mName.C_Str();
      animData.duration = animation->mDuration;
      animData.ticksPerSecond = animation->mTicksPerSecond;

      ReadHierarchyData(animData.hierarchy, m_Scene->mRootNode);
      ReadMissingBones(animation);

      animData.bones = m_Bones;

      GABGL_INFO("Model: " + std::filesystem::path(path).stem().string() + " Animation at index: " + std::to_string(i) + " " + animData.name);

      m_ProcessedAnimations.emplace_back(animData);
    }

    GABGL_ASSERT(!m_ProcessedAnimations.empty(),"[MODEL]: Model doesnt contain animations");

    SetAnimationbyIndex(0);

    ResizeFinalBoneMatrices();
  }

  m_TexturesLoaded.clear();

  GABGL_WARN("Model loading took {0} ms", timer.ElapsedMillis());
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
  for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
      aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
      m_Meshes.emplace_back(processMesh(mesh, scene));
  }
  for (unsigned int i = 0; i < node->mNumChildren; ++i) {
      processNode(node->mChildren[i], scene);
  }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
  std::vector<Vertex> vertices;
  vertices.reserve(mesh->mNumVertices);

  for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
  {
    Vertex vertex;
    if(m_isAnimated) SetDefaultBoneData(vertex);
    vertex.Position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
    vertex.Normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f);
    if (mesh->mTextureCoords[0]) {
        vertex.TexCoords = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
        vertex.Tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
        vertex.Bitangent = {mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
    } else {
        vertex.TexCoords = glm::vec2(0.0f);
    }
    vertices.emplace_back(std::move(vertex));
  }

  std::vector<GLuint> indices;
  for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
  {
    const aiFace& face = mesh->mFaces[i];
    indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
  }

  std::vector<std::shared_ptr<Texture>> textures;
  bool hasNormalMap = false;
  bool hasSpecular = false;

  if (mesh->mMaterialIndex >= 0)
  {
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", textures);
    hasNormalMap |= loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", textures);
    hasNormalMap |= loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", textures); // OBJ fallback
    hasSpecular |= loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", textures);

    // Optional
    // loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_albedo", textures);
  }

  if (m_isAnimated) ExtractBoneWeightForVertices(vertices, mesh);
  OptimizeMesh(vertices, indices);

  Mesh result(vertices, indices, textures);
  result.hasNormalMap = hasNormalMap;
  result.hasSpecularMap = hasSpecular;

  return result;
}

bool Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, std::vector<std::shared_ptr<Texture>>& textures)
{
  bool loadedAny = false;

  for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
      aiString str;
      mat->GetTexture(type, i, &str);
      std::string texturePath = str.C_Str();

      if (m_TexturesLoaded.find(texturePath) != m_TexturesLoaded.end()) {
          textures.emplace_back(m_TexturesLoaded[texturePath]);
          loadedAny = true;
          continue;
      }

      std::shared_ptr<Texture> texture;

      if (texturePath[0] == '*') {
          const aiTexture* aitexture = m_Scene->GetEmbeddedTexture(str.C_Str());
          if (aitexture) {
              texture = Texture::CreateRAWEMBEDDED(aitexture, texturePath);
          }
      } else {
          texture = Texture::CreateRAW(texturePath, m_Directory);
      }

      if (texture) {
          texture->SetType(typeName);
          textures.emplace_back(texture);
          m_TexturesLoaded[texturePath] = texture;
          loadedAny = true;
      }
  }

  return loadedAny;
}

void Model::OptimizeMesh(std::vector<Vertex>& m_Vertices, std::vector<GLuint>& m_Indices)
{
  std::vector<GLuint> remap(m_Indices.size());

  size_t OptVertexCount = meshopt_generateVertexRemap(remap.data(),m_Indices.data(),m_Indices.size(),m_Vertices.data(),m_Vertices.size(),sizeof(Vertex));

  std::vector<Vertex> OptVertices;
  std::vector<GLuint> OptIndices;
  OptVertices.resize(OptVertexCount);
  OptIndices.resize(m_Indices.size());

  meshopt_remapIndexBuffer(OptIndices.data(),m_Indices.data(),m_Indices.size(),remap.data());
  meshopt_remapVertexBuffer(OptVertices.data(),m_Vertices.data(),m_Vertices.size(),sizeof(Vertex),remap.data());
  meshopt_optimizeVertexCache(OptIndices.data(), OptIndices.data(), m_Indices.size(), OptVertexCount);
  meshopt_optimizeOverdraw(OptIndices.data(), OptIndices.data(), m_Indices.size(), &OptVertices[0].Position.x, OptVertexCount, sizeof(Vertex), 1.05f);  // Overdraw threshold (1.0 = minimal overdraw)
  meshopt_optimizeVertexFetch(OptVertices.data(), OptIndices.data(), m_Indices.size(), OptVertices.data(), OptVertexCount, sizeof(Vertex));

  std::vector<GLuint> SimplifiedIndices(OptIndices.size());
  size_t OptIndexCount = meshopt_simplify(SimplifiedIndices.data(),OptIndices.data(),m_Indices.size(),&OptVertices[0].Position.x,OptVertexCount,sizeof(Vertex),(size_t)(m_Indices.size() * m_OptimizerStrength),0.2f);
  SimplifiedIndices.resize(OptIndexCount);

  m_Indices = std::move(SimplifiedIndices);
  m_Vertices = std::move(OptVertices);
}

void Model::CreatePhysXStaticMesh(std::vector<Vertex>& m_Vertices, std::vector<GLuint>& m_Indices)
{
  std::vector<PxVec3> physxVertices(m_Vertices.size());
  for (size_t i = 0; i < m_Vertices.size(); ++i) {
      physxVertices[i] = PxVec3(
          m_Vertices[i].Position.x,
          m_Vertices[i].Position.y,
          m_Vertices[i].Position.z
      );
  }

  PxTriangleMesh* physxMesh = PhysX::CreateTriangleMesh(
      static_cast<PxU32>(physxVertices.size()), physxVertices.data(),
      static_cast<PxU32>(m_Indices.size() / 3), m_Indices.data()
  );

  if (!physxMesh) {
      GABGL_ERROR("Failed to create PhysX triangle mesh");
      return;
  }

  PxPhysics* physics = PhysX::getPhysics();
  PxScene* scene = PhysX::getScene();
  PxMaterial* material = PhysX::getMaterial();

  PxTriangleMeshGeometry triGeom;
  triGeom.triangleMesh = physxMesh;

  PxTransform pose = PxTransform(PxVec3(0));
  m_StaticMeshActor = physics->createRigidStatic(pose);

  if (m_StaticMeshActor) {
      PxShape* meshShape = PxRigidActorExt::createExclusiveShape(
          *m_StaticMeshActor, triGeom, *material
      );
      scene->addActor(*m_StaticMeshActor);
  }
}

void Model::CreatePhysXDynamicMesh(std::vector<Vertex>& m_Vertices)
{
  std::vector<PxVec3> physxVertices(m_Vertices.size());
  for (size_t i = 0; i < m_Vertices.size(); ++i)
  {
      physxVertices[i] = PxVec3(m_Vertices[i].Position.x, m_Vertices[i].Position.y,m_Vertices[i].Position.z);
  }

  PxConvexMesh* convexMesh = PhysX::CreateConvexMesh(static_cast<PxU32>(physxVertices.size()), physxVertices.data());

  if (!convexMesh) {
      GABGL_ERROR("Failed to create PhysX convex mesh");
      return;
  }

  PxPhysics* physics = PhysX::getPhysics();
  PxScene* scene = PhysX::getScene();
  PxMaterial* material = PhysX::getMaterial();

  PxConvexMeshGeometry convexGeom;
  convexGeom.convexMesh = convexMesh;
  convexGeom.scale = PxMeshScale(PxVec3(1.0f)); // Optional scaling

  PxTransform pose = PxTransform(PxVec3(0));
  m_DynamicMeshActor = physics->createRigidDynamic(pose);

  if (m_DynamicMeshActor)
  {
      PxShape* shape = PxRigidActorExt::createExclusiveShape(*m_DynamicMeshActor, convexGeom, *material);

      material->setRestitution(0.0f);

      if (m_isKinematic) m_DynamicMeshActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

      PxRigidBodyExt::setMassAndUpdateInertia(*m_DynamicMeshActor, 1.0f);

      scene->addActor(*m_DynamicMeshActor);
  }
}

void Model::CreateCharacterController(const PxVec3& position, float radius, float height, bool slopeLimit)
{
  m_ActorController = PhysX::CreateCharacterController(position, radius, height, slopeLimit);
}

void Model::ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh)
{
  for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
      std::string boneName = mesh->mBones[i]->mName.C_Str();
      int boneID = -1;

      if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end()) {
          boneID = m_BoneCounter++;
          m_BoneInfoMap[boneName] = { boneID, AssimpMatToGLMMat(mesh->mBones[i]->mOffsetMatrix) };
      } else {
          boneID = m_BoneInfoMap[boneName].id;
      }

      for (unsigned int j = 0; j < mesh->mBones[i]->mNumWeights; ++j) {
          int vertexID = mesh->mBones[i]->mWeights[j].mVertexId;
          float weight = mesh->mBones[i]->mWeights[j].mWeight;

          if (vertexID < vertices.size()) {
              SetBoneData(vertices[vertexID], boneID, weight);
          }
      }
  }
}

// Set default bone data for a vertex
void Model::SetDefaultBoneData(Vertex& vertex)
{
  for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
      vertex.m_BoneIDs[i] = -1;
      vertex.m_Weights[i] = 0.0f;
  }
}

// Set bone data for a vertex
void Model::SetBoneData(Vertex& vertex, int boneID, float weight)
{
  if (weight <= 0.0f) return;

  for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
      if (vertex.m_BoneIDs[i] < 0) {
          vertex.m_BoneIDs[i] = boneID;
          vertex.m_Weights[i] = weight;
          return;
      }
  }
}

void Model::UpdateAnimation(const DeltaTime& dt)
{
  float delta = dt;

  if (!m_IsBlending)
  {
      m_CurrentTime += m_TicksPerSecond * delta;
      m_CurrentTime = fmod(m_CurrentTime, m_Duration);

      CalculateBoneTransform(&m_RootNode, glm::mat4(1.0f));
  }
  else
  {
    m_BlendTime += delta;
    float blendFactor = glm::clamp(m_BlendTime / m_BlendDuration, 0.0f, 1.0f);

    m_CurrentTime += m_TicksPerSecond * delta;
    m_NextTime    += m_TicksPerSecondNext * delta;

    m_CurrentTime = fmod(m_CurrentTime, m_Duration);
    m_NextTime    = fmod(m_NextTime, m_DurationNext);

    CalculateBlendedBoneTransform(&m_RootNode, &m_RootNodeNext, m_CurrentTime, m_NextTime, glm::mat4(1.0f), blendFactor);

    if (blendFactor >= 1.0f)
    {
        m_CurrentTime = m_NextTime;
        m_IsBlending = false;
        SetAnimationbyIndex(m_NextAnimationIndex);
    }
  }
}

bool Model::IsInAnimation(int index) const
{
    return (!m_IsBlending && m_CurrentAnimationIndex == index) || (m_IsBlending && m_NextAnimationIndex == index);
}

void Model::StartBlendToAnimation(int32_t nextAnimationIndex, float blendDuration)
{
  assert(nextAnimationIndex >= 0 && nextAnimationIndex < m_ProcessedAnimations.size());

  if (IsInAnimation(nextAnimationIndex)) return; 

  m_BlendTime = 0.0f;
  m_BlendDuration = blendDuration;
  m_IsBlending = true;
  m_NextAnimationIndex = nextAnimationIndex;

  const AnimationData& animData = m_ProcessedAnimations[nextAnimationIndex];
  m_RootNodeNext        = animData.hierarchy;
  m_BonesNext           = animData.bones;
  m_TicksPerSecondNext  = animData.ticksPerSecond;
  m_DurationNext        = animData.duration;
}

void Model::SetAnimationbyIndex(int animationIndex)
{
  assert(animationIndex >= 0 && animationIndex < m_ProcessedAnimations.size());

  m_CurrentAnimationIndex = animationIndex;
  
  const AnimationData& animData = m_ProcessedAnimations[animationIndex];

  m_Duration = animData.duration;
  m_TicksPerSecond = animData.ticksPerSecond;

  m_RootNode = animData.hierarchy;
  m_Bones = animData.bones;

  ResizeFinalBoneMatrices();
}

void Model::SetAnimationByName(const std::string& animationName)
{
  auto it = std::find_if(m_ProcessedAnimations.begin(), m_ProcessedAnimations.end(),
      [&animationName](const AnimationData& animData) {
          return animData.name == animationName;
      });

  if (it != m_ProcessedAnimations.end()) {
      // Get the index of the found animation
      int animationIndex = std::distance(m_ProcessedAnimations.begin(), it);
      SetAnimationbyIndex(animationIndex); 
  } else {
      GABGL_ERROR("Animation not found: " + animationName);
  }
}

void Model::CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform)
{
  std::string nodeName = node->name;
  glm::mat4 nodeTransform = node->transformation;

  // Check if this node has a corresponding bone in the animation
  Bone* bone = FindBone(nodeName);
  if (bone)
  {
      bone->Update(m_CurrentTime);
      nodeTransform = bone->GetLocalTransform();
  }

  // Calculate the global transformation for this node
  glm::mat4 globalTransformation = parentTransform * nodeTransform;

  // Look up the bone in the boneInfoMap to get the offset matrix
  auto it = m_BoneInfoMap.find(nodeName);
  if (it != m_BoneInfoMap.end())
  {
      int index = it->second.id;
      glm::mat4 offset = it->second.offset;
      m_FinalBoneMatrices[index] = globalTransformation * offset;
  }

  // Recursively calculate transformations for the children
  for (size_t i = 0; i < node->children.size(); ++i)
  {
      CalculateBoneTransform(&node->children[i], globalTransformation);
  }
}

void Model::CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform, std::vector<glm::mat4>& outMatrices, std::vector<Bone>& bones, float animationTime)
{
  std::string nodeName = node->name;
  glm::mat4 nodeTransform = node->transformation;

  Bone* bone = FindBoneInList(nodeName, bones);
  if (bone)
  {
      bone->Update(animationTime); // Pass the time explicitly
      nodeTransform = bone->GetLocalTransform();
  }

  glm::mat4 globalTransformation = parentTransform * nodeTransform;

  auto it = m_BoneInfoMap.find(nodeName);
  if (it != m_BoneInfoMap.end())
  {
      int index = it->second.id;
      glm::mat4 offset = it->second.offset;
      outMatrices[index] = globalTransformation * offset;
  }

  for (const auto& child : node->children)
  {
      CalculateBoneTransform(&child, globalTransformation, outMatrices, bones, animationTime);
  }
}

void Model::CalculateBlendedBoneTransform(const AssimpNodeData* node, const AssimpNodeData* nodeNext, float timeCurrent, float timeNext, const glm::mat4& parentTransform, float blendFactor)
{
  const std::string& nodeName = node->name;

  glm::mat4 transformCurrent = node->transformation;
  Bone* boneCurrent = FindBoneInList(nodeName, m_Bones);
  if (boneCurrent) {
      boneCurrent->Update(timeCurrent);
      transformCurrent = boneCurrent->GetLocalTransform();
  }

  glm::mat4 transformNext = nodeNext->transformation;
  Bone* boneNext = FindBoneInList(nodeName, m_BonesNext);
  if (boneNext) {
      boneNext->Update(timeNext);
      transformNext = boneNext->GetLocalTransform();
  }

  // Decompose matrices
  glm::vec3 scaleCurrent, translationCurrent, skew1;
  glm::quat rotationCurrent;
  glm::vec4 perspective1;
  glm::decompose(transformCurrent, scaleCurrent, rotationCurrent, translationCurrent, skew1, perspective1);
  rotationCurrent = glm::normalize(rotationCurrent);

  glm::vec3 scaleNext, translationNext, skew2;
  glm::quat rotationNext;
  glm::vec4 perspective2;
  glm::decompose(transformNext, scaleNext, rotationNext, translationNext, skew2, perspective2);
  rotationNext = glm::normalize(rotationNext);

  // Blend all components
  glm::vec3 blendedScale       = glm::mix(scaleCurrent, scaleNext, blendFactor);
  glm::quat blendedRotation    = glm::normalize(glm::slerp(rotationCurrent, rotationNext, blendFactor));
  glm::vec3 blendedTranslation = glm::mix(translationCurrent, translationNext, blendFactor);

  // Compose blended matrix
  glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), blendedTranslation);
  glm::mat4 rotationMat    = glm::toMat4(blendedRotation);
  glm::mat4 scaleMat       = glm::scale(glm::mat4(1.0f), blendedScale);

  glm::mat4 blendedTransform = translationMat * rotationMat * scaleMat;
  glm::mat4 globalTransform = parentTransform * blendedTransform;

  if (m_BoneInfoMap.count(nodeName)) {
      int index = m_BoneInfoMap[nodeName].id;
      glm::mat4 offset = m_BoneInfoMap[nodeName].offset;
      m_FinalBoneMatrices[index] = globalTransform * offset;
  }

  for (size_t i = 0; i < node->children.size(); ++i)
  {
      CalculateBlendedBoneTransform(&node->children[i], &nodeNext->children[i], timeCurrent, timeNext, globalTransform, blendFactor);
  }
}

bool Model::ValidateBoneConsistency()
{
  // Step 1: Validate BoneInfoMap contains same bones
  for (const auto& [boneName, info] : m_BoneInfoMap)
  {
      bool foundInCurrent = FindBoneInList(boneName, m_Bones) != nullptr;
      bool foundInNext    = FindBoneInList(boneName, m_BonesNext) != nullptr;

      if (!foundInCurrent || !foundInNext)
      {
          GABGL_ERROR("Bone '{}' missing in {} animation", 
              boneName, 
              !foundInCurrent ? "current" : "next");
          return false;
      }
  }

  // Step 2: Validate output matrix vector sizes
  if (m_FinalBoneMatricesCurrent.size() != m_FinalBoneMatricesNext.size())
  {
      GABGL_ERROR("Final bone matrix size mismatch: {} vs {}", 
                  m_FinalBoneMatricesCurrent.size(), 
                  m_FinalBoneMatricesNext.size());
      return false;
  }

  GABGL_INFO("Bone consistency validated between current and next animations.");
  return true;
}

Bone* Model::FindBone(const std::string& name)
{
  auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
      [&](const Bone& Bone) {
          return Bone.GetBoneName() == name;
      }
  );
  if (iter == m_Bones.end()) return nullptr;
  else return &(*iter);
}

Bone* Model::FindBoneInList(const std::string& name, std::vector<Bone>& bones)
{
  auto iter = std::find_if(bones.begin(), bones.end(),
      [&](const Bone& bone) {
          return bone.GetBoneName() == name;
      });
  return iter != bones.end() ? &(*iter) : nullptr;
}

void Model::ResizeFinalBoneMatrices()
{
  m_FinalBoneMatrices.resize(m_BoneInfoMap.size(), glm::mat4(1.0f));
}

void Model::ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
{
  assert(src);  

  dest.name = "";
  dest.transformation = glm::mat4(1.0f);  // Reset to identity matrix
  dest.children.clear();  // Clear previous children

  dest.name = src->mName.data;
  dest.transformation = AssimpMatToGLMMat(src->mTransformation);
  dest.childrenCount = src->mNumChildren;

  for (int i = 0; i < src->mNumChildren; i++)
  {
      if (src->mChildren[i] == nullptr) 
      {
          GABGL_ERROR("Null child node found at index " + std::to_string(i));
          continue;  // Skip if child node is null
      }

      AssimpNodeData newData;
      ReadHierarchyData(newData, src->mChildren[i]);
      dest.children.emplace_back(newData);
  }
}

void Model::ReadMissingBones(const aiAnimation* animation)
{
  assert(animation);  

  m_Bones.clear();
  m_BoneCounter = 0;

  for (int i = 0; i < animation->mNumChannels; i++)
  {
      auto channel = animation->mChannels[i];
      std::string boneName = channel->mNodeName.data;

      if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
      {
          m_BoneInfoMap[boneName].id = m_BoneCounter;
          m_BoneCounter++;
      }

      m_Bones.emplace_back(Bone(channel->mNodeName.data, m_BoneInfoMap[boneName].id, channel));
  }
}


std::shared_ptr<Model> Model::CreateSTATIC(const char* path, float optimizerStrength, bool isKinematic, MeshType type)
{
	return std::make_shared<Model>(path,optimizerStrength,false,isKinematic,type);
}

std::shared_ptr<Model> Model::CreateANIMATED(const char* path, float optimizerStrength, bool isKinematic, MeshType type)
{
	return std::make_shared<Model>(path,optimizerStrength,true,isKinematic,type);
}


Bone::Bone(const std::string& name, int ID, const aiNodeAnim* channel) : m_Name(name), m_ID(ID), m_LocalTransform(1.0f)
{
  // Extract position keyframes
  m_NumPositions = channel->mNumPositionKeys;
  for (unsigned int i = 0; i < m_NumPositions; ++i) {
      aiVector3D aiPosition = channel->mPositionKeys[i].mValue;
      float timeStamp = static_cast<float>(channel->mPositionKeys[i].mTime);
      KeyPosition data = { AssimpVecToGLMVec(aiPosition), timeStamp };
      m_Positions.emplace_back(data);
  }

  // Extract rotation keyframes
  m_NumRotations = channel->mNumRotationKeys;
  for (unsigned int i = 0; i < m_NumRotations; ++i) {
      aiQuaternion aiOrientation = channel->mRotationKeys[i].mValue;
      float timeStamp = static_cast<float>(channel->mRotationKeys[i].mTime);
      KeyRotation data = { AssimpQuatToGLMQuat(aiOrientation), timeStamp };
      m_Rotations.emplace_back(data);
  }

  // Extract scaling keyframes
  m_NumScalings = channel->mNumScalingKeys;
  for (unsigned int i = 0; i < m_NumScalings; ++i) {
      aiVector3D aiScale = channel->mScalingKeys[i].mValue;
      float timeStamp = static_cast<float>(channel->mScalingKeys[i].mTime);
      KeyScale data = { AssimpVecToGLMVec(aiScale), timeStamp };
      m_Scales.emplace_back(data);
  }
}

void Bone::Update(float animationTime)
{
    glm::mat4 translation = InterpolatePosition(animationTime);
    glm::mat4 rotation = InterpolateRotation(animationTime);
    glm::mat4 scale = InterpolateScaling(animationTime);
    m_LocalTransform = translation * rotation * scale;
}

glm::mat4 Bone::GetInterpolatedTransform(float animationTime) const
{
    glm::mat4 translation = InterpolatePosition(animationTime);
    glm::mat4 rotation = InterpolateRotation(animationTime);
    glm::mat4 scale = InterpolateScaling(animationTime);
    return translation * rotation * scale;
}

// Interpolation helper functions
float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const
{
    float scaleFactor = (animationTime - lastTimeStamp) / (nextTimeStamp - lastTimeStamp);
    return glm::clamp(scaleFactor, 0.0f, 1.0f); // Ensure it's in the valid range
}

glm::mat4 Bone::InterpolatePosition(float animationTime) const
{
    if (m_NumPositions == 1) {
        return glm::translate(glm::mat4(1.0f), m_Positions[0].position);
    }

    int p0Index = GetPositionIndex(animationTime);
    int p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp, m_Positions[p1Index].timeStamp, animationTime);
    glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);

    return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 Bone::InterpolateRotation(float animationTime) const
{
    if (m_NumRotations == 1) {
        return glm::toMat4(glm::normalize(m_Rotations[0].orientation));
    }

    int p0Index = GetRotationIndex(animationTime);
    int p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp, m_Rotations[p1Index].timeStamp, animationTime);
    glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, scaleFactor);

    return glm::toMat4(glm::normalize(finalRotation));
}

glm::mat4 Bone::InterpolateScaling(float animationTime) const
{
    if (m_NumScalings == 1) {
        return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);
    }

    int p0Index = GetScaleIndex(animationTime);
    int p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);
    glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);

    return glm::scale(glm::mat4(1.0f), finalScale);
}

int Bone::GetPositionIndex(float animationTime) const
{
    for (int i = 0; i < m_NumPositions - 1; ++i) {
        if (animationTime < m_Positions[i + 1].timeStamp) {
            return i;
        }
    }
    assert(false && "Invalid position index");
    return -1;
}

int Bone::GetRotationIndex(float animationTime) const
{
    for (int i = 0; i < m_NumRotations - 1; ++i) {
        if (animationTime < m_Rotations[i + 1].timeStamp) {
            return i;
        }
    }
    assert(false && "Invalid rotation index");
    return -1;
}

int Bone::GetScaleIndex(float animationTime) const
{
    for (int i = 0; i < m_NumScalings - 1; ++i) {
        if (animationTime < m_Scales[i + 1].timeStamp) {
            return i;
        }
    }
    assert(false && "Invalid scaling index");
    return -1;
}



