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
#include "DeltaTime.hpp"
#include "PhysX.h"
#include "Transform.hpp"

#define MAX_BONE_INFLUENCE 4
#define MAX_BONES 100

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

struct AnimationData
{
  std::string name;
  float duration;
  float ticksPerSecond;
  std::vector<Bone> bones;  // Preprocessed bone data for the animation.
  AssimpNodeData hierarchy; // Precomputed node hierarchy for the animation.
};

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
  std::vector<GLuint64> m_TexturesBindlessHandles;

  GLuint VAO, VBO, EBO;
  GLuint instanceVBO = 0;
  bool instanceAttribsConfigured = false;
  bool instanceDataDirty = true;
  bool hasNormalMap;
  bool hasSpecularMap;
};

enum class MeshType : int32_t
{
  NONE = 0,
  CONTROLLER = 1,
  TRIANGLEMESH = 2,
  CONVEXMESH = 3
};

struct Model
{
  Model(const char* path, float optimizerStrength, bool isAnimated, bool isKinematic, const MeshType& type);

  static std::shared_ptr<Model> CreateSTATIC(const char* path, float optimizerStrength, bool isKinematic, MeshType type);
  static std::shared_ptr<Model> CreateANIMATED(const char* path, float optimizerStrength, bool isKinematic, MeshType type);

  void UpdateAnimation(const DeltaTime& dt);
  void SetAnimationbyIndex(int animationIndex);
  void SetAnimationByName(const std::string& animationName);
  void StartBlendToAnimation(int32_t nextAnimationIndex, float blendDuration);
  bool IsInAnimation(int index) const;
  void CreatePhysXStaticMesh(std::vector<Vertex>& m_Vertices, std::vector<GLuint>& m_Indices);
  void CreatePhysXDynamicMesh(std::vector<Vertex>& m_Vertices);
  void CreateCharacterController(const PxVec3& position, float radius, float height, bool slopeLimit);

  inline std::vector<Mesh>& GetMeshes() { return m_Meshes; }
  inline std::map<std::string,BoneInfo>& GetBoneInfoMap() { return m_BoneInfoMap; }
  inline int& GetBoneCount() { return m_BoneCounter; }
  inline float GetTicksPerSecond() { return m_TicksPerSecond; }
  inline float GetDuration() { return m_Duration; }
  inline const AssimpNodeData& GetRootNode() { return m_RootNode; }
  inline bool IsAnimated() { return m_isAnimated; }
  inline const std::vector<glm::mat4>& GetFinalBoneMatrices() { return m_FinalBoneMatrices; }
  inline const MeshType& GetPhysXMeshType() { return m_meshType; }
  inline const PxRigidStatic* GetStaticActor() { return m_StaticMeshActor; }
  inline const PxRigidDynamic* GetDynamicActor() { return m_DynamicMeshActor; }
  inline PxController* GetController() { return m_ActorController; }
  inline Transform& GetControllerTransform() { return m_ControllerTransform; }

  Transform m_ControllerTransform;
  PxVec3 m_ControllerPosition;
  PxVec3 m_ControllerVelocity = PxVec3(0.0f);
  float m_ControllerRadius; 
  float m_ControllerHeight; 
  bool m_ControllerSlopeLimit;
  bool m_ControllerIsGrounded = false;

  std::string m_Name;
  bool m_IsRendered = true;

  std::unordered_map<std::string, std::shared_ptr<Texture>> m_TexturesLoaded; 
  std::vector<Mesh> m_Meshes;
  std::vector<Bone> m_Bones;
  std::vector<const aiAnimation*> m_Animations;
  std::vector<AnimationData> m_ProcessedAnimations;
  std::map<std::string, BoneInfo> m_BoneInfoMap;
  std::vector<glm::mat4> m_FinalBoneMatrices;
  std::vector<glm::mat4> m_FinalBoneMatricesCurrent;
  std::vector<glm::mat4> m_FinalBoneMatricesNext;

  AssimpNodeData m_RootNodeNext;
  std::vector<Bone> m_BonesNext;
  float m_TicksPerSecondNext = 0.0f;
  float m_DurationNext = 0.0f;

  AssimpNodeData m_RootNode;
  int m_BoneCounter = 0;
  std::string m_Directory;
  float m_BlendTime = 0.0f;
  float m_BlendDuration = 0.5f; // Blend duration in seconds
  bool m_IsBlending = false;
  int m_NextAnimationIndex = -1;
  int m_CurrentAnimationIndex;
  float m_Duration;
  int m_TicksPerSecond;
  float m_CurrentTime = 0.0f;
  float m_NextTime = 0.0f;
  float m_DeltaTime = 0.0f;

  bool m_isKinematic;
  float m_OptimizerStrength;
  bool m_isAnimated;
  const aiScene* m_Scene;
  PxRigidStatic* m_StaticMeshActor = nullptr;
  PxRigidDynamic* m_DynamicMeshActor = nullptr;
  PxController* m_ActorController = nullptr;
  MeshType m_meshType;

private:

  void processNode(aiNode* node, const aiScene* scene);
  Mesh processMesh(aiMesh* mesh, const aiScene* scene);
  bool loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, std::vector<std::shared_ptr<Texture>>& textures);
  void OptimizeMesh(std::vector<Vertex>& m_Vertices, std::vector<GLuint>& m_Indices);
  void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh);
  void SetDefaultBoneData(Vertex& vertex);
  void SetBoneData(Vertex& vertex, int boneID, float weight);
  void CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform);
  void CalculateBoneTransform(const AssimpNodeData* node, const glm::mat4& parentTransform, std::vector<glm::mat4>& outMatrices, std::vector<Bone>& bones, float time);
  void CalculateBlendedBoneTransform(const AssimpNodeData* node,const AssimpNodeData* nodeNext,float timeCurrent,float timeNext,const glm::mat4& parentTransform,float blendFactor);

  Bone* FindBone(const std::string& name);
  Bone* FindBoneInList(const std::string& name, std::vector<Bone>& bones);
  bool ValidateBoneConsistency();
  void ResizeFinalBoneMatrices();
  void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src);
  void ReadMissingBones(const aiAnimation* animation);
};

enum class Movement : int32_t
{
  FORWARD = 0,
  BACKWARD = 1,
  LEFT = 2,
  RIGHT = 3
};

struct ModelManager
{
  static void Init();
  static void BakeModel(const std::string& path, const std::shared_ptr<Model>& model);
  static void BakeModelInstancedBuffers(Mesh& mesh, const std::vector<Transform>& instances);
  static void UploadToGPU();
  static std::shared_ptr<Model> GetModel(const std::string& name);
  static std::vector<glm::mat4> GetTransforms();
  static GLsizei GetModelsQuantity();
  static GLuint GetModelsVAO();
  static void SetRender(const std::string& name ,bool render);
  static void SetInitialModelTransform(const std::string& name, const glm::mat4& transform);
  static void SetInitialControllerTransform(const std::string& name, const Transform& transform, float radius, float height, bool slopeLimit);
  static void UpdateTransforms(const DeltaTime& dt);
  static void MoveController(const std::string& name, const Movement& movement, float speed, const DeltaTime& dt);
};

