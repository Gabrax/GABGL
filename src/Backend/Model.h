#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>
#include <assimp/vector3.h>
#include <assimp/matrix4x4.h>
#include <meshoptimizer.h>

#include <unordered_map>
#include <map>

#include "Texture.h"
#include "DeltaTime.h"

#define MAX_BONE_INFLUENCE 4

struct KeyPosition {
    glm::vec3 position;
    float timeStamp;
};

struct KeyRotation {
    glm::quat orientation;
    float timeStamp;
};

struct KeyScale {
    glm::vec3 scale;
    float timeStamp;
};

struct Bone
{
  Bone(const std::string& name, int ID, const aiNodeAnim* channel);

  void Update(float animationTime);

  glm::mat4 GetInterpolatedTransform(float animationTime) const;

  inline void SetTransform(const glm::mat4& transform) { m_LocalTransform = transform; }
  inline glm::mat4 GetLocalTransform() const { return m_LocalTransform; }
  inline std::string GetBoneName() const { return m_Name; }
  inline int GetBoneID() const { return m_ID; }

private:
  float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const;
  glm::mat4 InterpolatePosition(float animationTime) const;
  glm::mat4 InterpolateRotation(float animationTime) const;
  glm::mat4 InterpolateScaling(float animationTime) const;
  int GetPositionIndex(float animationTime) const;
  int GetRotationIndex(float animationTime) const;
  int GetScaleIndex(float animationTime) const;

  std::vector<KeyPosition> m_Positions;
  std::vector<KeyRotation> m_Rotations;
  std::vector<KeyScale> m_Scales;

  int m_NumPositions;
  int m_NumRotations;
  int m_NumScalings;

  std::string m_Name;
  int m_ID;
  glm::mat4 m_LocalTransform;
};

struct BoneInfo
{
  int id;
  glm::mat4 offset;
};

struct AssimpNodeData
{
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

struct AnimationData {
    std::string name;
    float duration;
    float ticksPerSecond;
    std::vector<Bone> bones;  // Preprocessed bone data for the animation.
    AssimpNodeData hierarchy; // Precomputed node hierarchy for the animation.
};

static glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from);
static glm::vec3 GetGLMVec(const aiVector3D& vec); 
static glm::quat GetGLMQuat(const aiQuaternion& pOrientation);

struct Vertex
{
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TexCoords;
  glm::vec3 Tangent;
  glm::vec3 Bitangent;
  glm::vec4 TexIndices;
  int m_BoneIDs[MAX_BONE_INFLUENCE];
  float m_Weights[MAX_BONE_INFLUENCE];

  int EntityID;
};

struct Mesh
{
  std::vector<Vertex> m_Vertices;
  std::vector<GLuint> m_Indices;
  std::vector<std::shared_ptr<Texture>> m_Textures;

  GLuint VAO, VBO, EBO;
};


struct Model
{
  Model(const char* path, bool isAnimated);

  static std::shared_ptr<Model> CreateSTATIC(const char* path);
  static std::shared_ptr<Model> CreateANIMATED(const char* path);

  void UpdateAnimation(DeltaTime& dt);
  void SetAnimationbyIndex(int animationIndex);
  void SetAnimationByName(const std::string& animationName);

  inline std::vector<Mesh>& GetMeshes() { return meshes; }
  inline std::map<std::string,BoneInfo>& GetBoneIDMap() { return m_BoneInfoMap; }
  inline const std::map<std::string, BoneInfo>& GetBoneInfoMap() { return boneInfoMap; }
  inline int& GetBoneCount() { return boneCounter; }
  inline float GetTicksPerSecond() { return m_TicksPerSecond; }
  inline float GetDuration() { return m_Duration; }
  inline const AssimpNodeData& GetRootNode() { return m_RootNode; }
  inline bool IsAnimated() { return m_isAnimated; }
  inline const std::vector<glm::mat4>& GetFinalBoneMatrices() { return m_FinalBoneMatrices; }

private:

  std::unordered_map<std::string, std::shared_ptr<Texture>> textures_loaded; 
  std::vector<Mesh> meshes;
  std::map<std::string, BoneInfo> boneInfoMap; 
  int boneCounter = 0;
  std::vector<Bone> m_Bones;
  std::vector<const aiAnimation*> m_Animations;
  std::vector<AnimationData> m_ProcessedAnimations;
  AssimpNodeData m_RootNode;
  std::map<std::string, BoneInfo> m_BoneInfoMap;
  std::vector<glm::mat4> m_FinalBoneMatrices;

  std::string directory;

  float m_BlendFactor = 0.0f; // 0 -> 1 
  bool m_IsBlending = false;
  int m_NextAnimationIndex = -1;
  int m_CurrentAnimationIndex;
  float m_Duration;
  int m_TicksPerSecond;
  float m_CurrentTime = 0.0f;
  float m_DeltaTime = 0.0f;

  bool m_isAnimated;
  const aiScene* scene;

private:

  void processNode(aiNode* node, const aiScene* scene);
  Mesh processMesh(aiMesh* mesh, const aiScene* scene);
  void loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, std::vector<std::shared_ptr<Texture>>& textures);
  void OptimizeMesh(std::vector<Vertex>& m_Vertices, std::vector<GLuint>& m_Indices);
  void CreatePhysXStaticMesh(std::vector<Vertex>& m_Vertices, std::vector<GLuint>& m_Indices);
  void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh);
  void SetDefaultBoneData(Vertex& vertex);
  void SetBoneData(Vertex& vertex, int boneID, float weight);
  void CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform);
  Bone* FindBone(const std::string& name);
  void ResizeFinalBoneMatrices();
  void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src);
  void ReadMissingBones(const aiAnimation* animation);
};

