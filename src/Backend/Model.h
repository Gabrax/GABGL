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

#include "Texture.h"

#define MAX_BONE_INFLUENCE 4

struct Vertex
{
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TexCoords;
  glm::vec3 Tangent;
  glm::vec3 Bitangent;
  int m_BoneIDs[MAX_BONE_INFLUENCE];
  float m_Weights[MAX_BONE_INFLUENCE];
};

struct Mesh
{
  std::vector<Vertex> m_Vertices;
  std::vector<GLuint> m_Indices;
  std::vector<std::shared_ptr<Texture>> m_Textures;
};

struct Model
{
  Model(const char* path);

  static std::shared_ptr<Model> CreateSTATIC(const char* path);
  static std::shared_ptr<Model> CreateANIMATED(const char* path);

private:

  std::unordered_map<std::string, std::shared_ptr<Texture>> textures_loaded; 
  std::vector<Mesh> meshes;
  std::string directory;

private:

  void processNode(aiNode* node, const aiScene* scene);
  Mesh processMesh(aiMesh* mesh, const aiScene* scene);
  std::vector<Vertex> processVertices(aiMesh* mesh);
  std::vector<GLuint> processIndices(aiMesh* mesh);
  std::vector<std::shared_ptr<Texture>> processTextures(aiMesh* mesh, const aiScene* scene);
  void loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, std::vector<std::shared_ptr<Texture>>& textures);
  void OptimizeMesh(Mesh& mesh);
  void CreatePhysXStaticMesh(Mesh& mesh);

  glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from);
	glm::vec3 GetGLMVec(const aiVector3D& vec); 
	glm::quat GetGLMQuat(const aiQuaternion& pOrientation);
};
