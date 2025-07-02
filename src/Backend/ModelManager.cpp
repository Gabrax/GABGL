#include "ModelManager.h"
#include "BackendLogger.h"
#include "meshoptimizer.h"
#include <filesystem>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "Buffer.h"

static glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
{
  glm::mat4 to;
  //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
  to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
  to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
  to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
  to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
  return to;
}

static glm::vec3 GetGLMVec(const aiVector3D& vec) 
{ 
  return glm::vec3(vec.x, vec.y, vec.z); 
}

static glm::quat GetGLMQuat(const aiQuaternion& pOrientation)
{
  return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
}

struct ModelsData
{
  std::shared_ptr<StorageBuffer> InPositionsBuffer;
  std::shared_ptr<StorageBuffer> InNormalsBuffer;
  std::shared_ptr<StorageBuffer> BoneIDBuffer;
  std::shared_ptr<StorageBuffer> BoneWeightBuffer;
  std::shared_ptr<StorageBuffer> OutPositionsBuffer;
  std::shared_ptr<StorageBuffer> OutNormalsBuffer;

  std::unordered_map<std::string, std::shared_ptr<Model>> m_Models;

} s_Data; 

void ModelManager::BakeModelInstancedBuffers(Mesh& mesh, const std::vector<Transform>& transforms)
{
  if (transforms.empty()) return;

  std::vector<glm::mat4> instanceMatrices;
  instanceMatrices.reserve(transforms.size());

  for (const auto& t : transforms) instanceMatrices.push_back(t.GetTransform());

  if (mesh.instanceVBO == 0) glGenBuffers(1, &mesh.instanceVBO);

  glBindVertexArray(mesh.VAO);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.instanceVBO);
  glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4), instanceMatrices.data(), GL_STREAM_DRAW);

  if (!mesh.instanceAttribsConfigured)
  {
    std::size_t vec4Size = sizeof(glm::vec4);
    for (int i = 0; i < 4; ++i)
    {
        GLuint loc = 7 + i;
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));
        glVertexAttribDivisor(loc, 1);
    }

    mesh.instanceAttribsConfigured = true;
  }

  glBindVertexArray(0);
}

void ModelManager::BakeModel(const std::string& path, const std::shared_ptr<Model>& model)
{
  Timer timer;

  constexpr int NUM_BUFFERS = 2;
  std::array<std::unique_ptr<PixelBuffer>, NUM_BUFFERS> pboBuffers;
  int currentPBO = 0;

  for (auto& mesh : model->GetMeshes())
  {
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
      glGenTextures(1, &id);
      glBindTexture(GL_TEXTURE_2D, id);
      texture->SetRendererID(id);

      pbo->Bind();
      glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);
      pbo->Unbind();

      glGenerateMipmap(GL_TEXTURE_2D);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      currentPBO = (currentPBO + 1) % NUM_BUFFERS;
    }

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.m_Vertices.size() * sizeof(Vertex), &mesh.m_Vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.m_Indices.size() * sizeof(unsigned int), &mesh.m_Indices[0], GL_STATIC_DRAW);

    struct Attribute {
        GLint size;
        GLenum type;
        GLboolean normalized;
        size_t offset;
    };

    std::array<Attribute, 7> attributes = { {
        {3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Position)},
        {3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Normal)},
        {2, GL_FLOAT, GL_FALSE, offsetof(Vertex, TexCoords)},
        {3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Tangent)},
        {3, GL_FLOAT, GL_FALSE, offsetof(Vertex, Bitangent)},
        {4, GL_INT, GL_FALSE, offsetof(Vertex, m_BoneIDs)},
        {4, GL_FLOAT, GL_FALSE, offsetof(Vertex, m_Weights)}
    } };

    for (size_t i = 0; i < attributes.size(); ++i) {
        glEnableVertexAttribArray(static_cast<GLuint>(i));
        if (attributes[i].type == GL_INT) {
            glVertexAttribIPointer(static_cast<GLuint>(i), attributes[i].size, attributes[i].type, sizeof(Vertex), (void*)attributes[i].offset);
        } else {
            glVertexAttribPointer(static_cast<GLuint>(i), attributes[i].size, attributes[i].type, attributes[i].normalized, sizeof(Vertex), (void*)attributes[i].offset);
        }
    }

    glBindVertexArray(0);

    for(auto& tex : mesh.m_Textures) tex->ClearRawData();

    if(model->GetPhysXMeshType() == MeshType::TRIANGLEMESH) model->CreatePhysXStaticMesh(mesh.m_Vertices, mesh.m_Indices);
    else if(model->GetPhysXMeshType() == MeshType::CONVEXMESH) model->CreatePhysXDynamicMesh(mesh.m_Vertices);
  }

  std::string name = std::filesystem::path(path).stem().string();
  s_Data.m_Models[name] = std::move(model);

  GABGL_WARN("Model: {0} baking took {1} ms", name, timer.ElapsedMillis());
}

std::shared_ptr<Model> ModelManager::GetModel(const std::string& name)
{
  auto it = s_Data.m_Models.find(name);
  if (it != s_Data.m_Models.end())
      return it->second;
  return nullptr;
}

void ModelManager::UpdateAnimations(const DeltaTime& dt)
{
  for(auto i : s_Data.m_Models)
  {
    if(i.second->IsAnimated()) i.second->UpdateAnimation(dt);
  }
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

  for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
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
  for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
      const aiFace& face = mesh->mFaces[i];
      indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
  }

  std::vector<std::shared_ptr<Texture>> textures;
  if (mesh->mMaterialIndex >= 0) {
      aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

      loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", textures);
      loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", textures);
      loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", textures);
      loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height", textures);
  }

  if(m_isAnimated) ExtractBoneWeightForVertices(vertices, mesh);
  OptimizeMesh(vertices, indices);

  return Mesh(vertices, indices, textures);
}

void Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, std::vector<std::shared_ptr<Texture>>& textures)
{
  for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
      aiString str;
      mat->GetTexture(type, i, &str);
      std::string texturePath = str.C_Str();

      if (m_TexturesLoaded.find(texturePath) != m_TexturesLoaded.end()) {
          textures.emplace_back(m_TexturesLoaded[texturePath]);
          continue;
      }

      if (texturePath[0] == '*') {
        const aiTexture* aitexture = m_Scene->GetEmbeddedTexture(str.C_Str());
        if (aitexture) {
            std::shared_ptr<Texture> texture = Texture::CreateRAWEMBEDDED(aitexture,texturePath);
            texture->SetType(typeName);

            textures.emplace_back(texture);
            m_TexturesLoaded[texturePath] = texture;
        }
      }
      else
      {
        std::shared_ptr<Texture> texture = Texture::CreateRAW(texturePath, m_Directory);
        texture->SetType(typeName);

        textures.emplace_back(texture);
        m_TexturesLoaded[texturePath] = texture;
      }
  }
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

void Model::SetPosition(const glm::mat4& transform)
{
  if(m_meshType == MeshType::CONTROLLER)
  {
    GABGL_ERROR("This function is for non Controller model!");
    return;
  }

  PxTransform pxTransform = PxTransform(PhysX::GlmMat4ToPxTransform(transform));

  if(m_meshType == MeshType::TRIANGLEMESH) m_StaticMeshActor->setGlobalPose(pxTransform);
  else if(m_meshType == MeshType::CONVEXMESH)
  {
    if (m_isKinematic) m_DynamicMeshActor->setKinematicTarget(pxTransform);
    else m_DynamicMeshActor->setGlobalPose(pxTransform);
  }
}

void Model::SetPosition(const Transform& transform, float radius, float height, bool slopeLimit)
{
  if(m_meshType != MeshType::CONTROLLER)
  {
    GABGL_ERROR("This function is for Controller model!");
    return;
  }

  m_ControllerTransform = transform;

  CreateCharacterController(PhysX::GlmVec3ToPxVec3(m_ControllerTransform.GetPosition()), radius, height, slopeLimit);
}

void Model::Move(const Movement& movement, float speed, const DeltaTime& dt)
{
    if (m_meshType != MeshType::CONTROLLER)
    {
        GABGL_ERROR("This function is for Controller model!");
        return;
    }

    // Constants
    const float gravity = -9.81f;
    const float damping = 0.9f;
    const float jumpSpeed = 5.5f;

    // Apply gravity
    m_ControllerVelocity.y += gravity * dt;

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
        m_ControllerVelocity.x = direction.x * speed;
        m_ControllerVelocity.z = direction.z * speed;
    }
    else
    {
        // Apply damping when no input
        m_ControllerVelocity.x *= damping;
        m_ControllerVelocity.z *= damping;
    }

    // Move controller
    PxVec3 displacement = m_ControllerVelocity * dt;
    PxControllerCollisionFlags flags = m_ActorController->move(displacement, 0.001f, dt, PxControllerFilters());

    // Ground check and vertical reset
    if (flags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN))
    {
        m_ControllerVelocity.y = 0.0f;
        m_ControllerIsGrounded = true;
    }
    else
    {
        m_ControllerIsGrounded = false;
    }

    // Update internal transform
    PxExtendedVec3 footPos = m_ActorController->getFootPosition();
    m_ControllerTransform.SetPosition(glm::vec3(footPos.x, footPos.y, footPos.z));
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
          m_BoneInfoMap[boneName] = { boneID, ConvertMatrixToGLMFormat(mesh->mBones[i]->mOffsetMatrix) };
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
  dest.transformation = ConvertMatrixToGLMFormat(src->mTransformation);
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
      KeyPosition data = { GetGLMVec(aiPosition), timeStamp };
      m_Positions.emplace_back(data);
  }

  // Extract rotation keyframes
  m_NumRotations = channel->mNumRotationKeys;
  for (unsigned int i = 0; i < m_NumRotations; ++i) {
      aiQuaternion aiOrientation = channel->mRotationKeys[i].mValue;
      float timeStamp = static_cast<float>(channel->mRotationKeys[i].mTime);
      KeyRotation data = { GetGLMQuat(aiOrientation), timeStamp };
      m_Rotations.emplace_back(data);
  }

  // Extract scaling keyframes
  m_NumScalings = channel->mNumScalingKeys;
  for (unsigned int i = 0; i < m_NumScalings; ++i) {
      aiVector3D aiScale = channel->mScalingKeys[i].mValue;
      float timeStamp = static_cast<float>(channel->mScalingKeys[i].mTime);
      KeyScale data = { GetGLMVec(aiScale), timeStamp };
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



