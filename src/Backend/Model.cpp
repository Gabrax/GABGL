#include "Model.h"
#include "BackendLogger.h"
#include "meshoptimizer.h"
#include <filesystem>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


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

Model::Model(const char* path, float optimizerStrength, bool isAnimated, bool isKinematic, PhysXMeshType type) :  m_isKinematic(isKinematic), m_OptimizerStrength(optimizerStrength), m_isAnimated(isAnimated)
{
  Timer timer;

  Assimp::Importer importer;
  if(m_isAnimated) m_Scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
  else m_Scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

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

        GABGL_INFO("Animation at index: " + std::to_string(i) + " " + animData.name);

        m_ProcessedAnimations.emplace_back(animData);
    }

    GABGL_ASSERT(!m_ProcessedAnimations.empty(),"[MODEL]: Model doesnt contain animations");

    SetAnimationByName("IDLE");

    ResizeFinalBoneMatrices();
  }

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
      loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", textures);
      loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height", textures);
  }

  if(m_isAnimated) ExtractBoneWeightForVertices(vertices, mesh);
  OptimizeMesh(vertices, indices);
  CreatePhysXStaticMesh(vertices, indices);

  return Mesh(vertices, indices, textures);
}

void Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, std::vector<std::shared_ptr<Texture>>& textures)
{
  for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
      aiString str;
      mat->GetTexture(type, i, &str);
      std::string texturePath = str.C_Str();

      // Check if texture is already loaded
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

void Model::UpdatePhysXActor(const glm::mat4& transform)
{
  const PxU32 nbShapes = m_MeshActor->getNbShapes();
  PxShape* shapes[32];
  m_MeshActor->getShapes(shapes, nbShapes);

  for (PxU32 j = 0; j < nbShapes; j++) {
      const PxMat44 shapePose(PxShapeExt::getGlobalPose(*shapes[j], *m_MeshActor));
      const PxGeometry& geom = shapes[j]->getGeometry();

      if (geom.getType() == PxGeometryType::eTRIANGLEMESH) {
          if(m_isKinematic) m_MeshActor->setKinematicTarget(PxTransform(PhysX::GlmMat4ToPxTransform(transform)));
          else m_MeshActor->setGlobalPose(PxTransform(PhysX::GlmMat4ToPxTransform(transform)));
      }
  }
}

void Model::CreatePhysXStaticMesh(std::vector<Vertex>& m_Vertices, std::vector<GLuint>& m_Indices)
{
  std::vector<PxVec3> physxVertices(m_Vertices.size());
  for (size_t i = 0; i < m_Vertices.size(); ++i) {
      physxVertices[i] = PxVec3(m_Vertices[i].Position.x, m_Vertices[i].Position.y, m_Vertices[i].Position.z);
  }

  PxTriangleMesh* physxMesh = PhysX::CreateTriangleMesh(static_cast<PxU32>(physxVertices.size()), physxVertices.data(), static_cast<PxU32>(m_Indices.size() / 3), m_Indices.data());

  if (!physxMesh) GABGL_ERROR("Failed to create PhysX triangle mesh");

  PxScene* scene = PhysX::getScene();
  PxPhysics* physics = PhysX::getPhysics();
  PxMaterial* material = PhysX::getMaterial();

  PxTriangleMeshGeometry geom;
  PxTransform pose = PxTransform(PxVec3(0));
  geom.triangleMesh = physxMesh;

  m_MeshActor = physics->createRigidDynamic(pose);
  PxShape* meshShape = nullptr;
  if(m_MeshActor){
      m_MeshActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, m_isKinematic);

      PxTriangleMeshGeometry triGeom;
      triGeom.triangleMesh = physxMesh;
      meshShape = PxRigidActorExt::createExclusiveShape(*m_MeshActor, triGeom, *material);
      scene->addActor(*m_MeshActor);
  }
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

void Model::UpdateAnimation(DeltaTime& dt)
{
    m_DeltaTime = dt;
    m_CurrentTime += GetTicksPerSecond() * dt;
    m_CurrentTime = fmod(m_CurrentTime, GetDuration());
    CalculateBoneTransform(&GetRootNode(), glm::mat4(1.0f));
}

void Model::SetAnimationbyIndex(int animationIndex)
{
    assert(animationIndex >= 0 && animationIndex < m_ProcessedAnimations.size());
    
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


std::shared_ptr<Model> Model::CreateSTATIC(const char* path, float optimizerStrength, bool isKinematic, PhysXMeshType type)
{
	return std::make_shared<Model>(path,optimizerStrength,false,isKinematic,type);
}

std::shared_ptr<Model> Model::CreateANIMATED(const char* path, float optimizerStrength, bool isKinematic, PhysXMeshType type)
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



