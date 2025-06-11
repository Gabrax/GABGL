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

  glm::mat4 m_LocalTransform;
  std::string m_Name;
  int m_ID;
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
  Model(const char* path);

  static std::shared_ptr<Model> CreateSTATIC(const char* path);
  static std::shared_ptr<Model> CreateANIMATED(const char* path);

  inline std::vector<Mesh>& GetMeshes() { return meshes; }

private:

  std::unordered_map<std::string, std::shared_ptr<Texture>> textures_loaded; 
  std::vector<Mesh> meshes;
  std::map<std::string, BoneInfo> boneInfoMap; 
  int boneCounter = 0;

  std::string directory;

private:

  void processNode(aiNode* node, const aiScene* scene);
  Mesh processMesh(aiMesh* mesh, const aiScene* scene);
  std::vector<Vertex> processVertices(aiMesh* mesh);
  std::vector<GLuint> processIndices(aiMesh* mesh);
  std::vector<std::shared_ptr<Texture>> processTextures(aiMesh* mesh, const aiScene* scene);
  void loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, std::vector<std::shared_ptr<Texture>>& textures);
  void OptimizeMesh(std::vector<Vertex>& m_Vertices, std::vector<GLuint>& m_Indices);
  void CreatePhysXStaticMesh(std::vector<Vertex>& m_Vertices, std::vector<GLuint>& m_Indices);
  void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh);
  void SetDefaultBoneData(Vertex& vertex);
  void SetBoneData(Vertex& vertex, int boneID, float weight);
};

