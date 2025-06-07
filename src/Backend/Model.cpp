#include "Model.h"
#include "PhysX.h"
#include "BackendLogger.h"
#include <filesystem>

Model::Model(const char* path)
{
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(
      path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
  {
    GABGL_ERROR("ERROR::ASSIMP:: " + (std::string)importer.GetErrorString());
    return;
  }
  std::string dirStr = std::filesystem::path(path).parent_path().string();
  directory = dirStr.c_str();  
  processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.emplace_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices = processVertices(mesh);
    std::vector<GLuint> indices = processIndices(mesh);
    std::vector<std::shared_ptr<Texture>> textures = processTextures(mesh, scene);

    return Mesh(vertices, indices, textures);
}

std::vector<Vertex> Model::processVertices(aiMesh* mesh)
{
  std::vector<Vertex> vertices;
  vertices.reserve(mesh->mNumVertices);

  for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
      Vertex vertex;
      vertex.Position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
      vertex.Normal = mesh->HasNormals()
                          ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
                          : glm::vec3(0.0f);
      if (mesh->mTextureCoords[0]) {
          vertex.TexCoords = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
          vertex.Tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
          vertex.Bitangent = {mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
      } else {
          vertex.TexCoords = glm::vec2(0.0f);
      }
      vertices.emplace_back(std::move(vertex));
  }
  return vertices;
}

std::vector<GLuint> Model::processIndices(aiMesh* mesh)
{
  std::vector<GLuint> indices;
  for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
      const aiFace& face = mesh->mFaces[i];
      indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
  }
  return indices;
}

std::vector<std::shared_ptr<Texture>> Model::processTextures(aiMesh* mesh, const aiScene* scene)
{
    std::vector<std::shared_ptr<Texture>> textures;
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // Load textures of each type
        loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", textures);
        loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", textures);
        loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", textures);
        loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height", textures);
    }
    return textures;
}

void Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, std::vector<std::shared_ptr<Texture>>& textures)
{
  for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
      aiString str;
      mat->GetTexture(type, i, &str);
      std::string texturePath = str.C_Str();

      // Check if texture is already loaded
      /*if (textures_loaded.find(texturePath) != textures_loaded.end()) {*/
      /*    textures.emplace_back(textures_loaded[texturePath]);*/
      /*    continue;*/
      /*}*/
      /**/
      /*// Load texture*/
      /*Texture texture;*/
      /*texture.id = TextureFromFile(texturePath.c_str(), directory);*/
      /*texture.type = typeName;*/
      /*texture.path = texturePath;*/
      /**/
      /*textures.emplace_back(texture);*/
      /*textures_loaded[texturePath] = texture;*/
  }
}

void Model::OptimizeMesh(Mesh& mesh)
{
  meshopt_optimizeVertexCache(mesh.m_Indices.data(), mesh.m_Indices.data(), mesh.m_Indices.size(), mesh.m_Vertices.size());

  meshopt_optimizeOverdraw(
      mesh.m_Indices.data(), 
      mesh.m_Indices.data(), 
      mesh.m_Indices.size(), 
      &mesh.m_Vertices[0].Position.x, 
      mesh.m_Vertices.size(), 
      sizeof(Vertex), 
      1.05f // Overdraw threshold (1.0 = minimal overdraw)
  );

  std::vector<Vertex> optimizedVertices(mesh.m_Vertices.size());
  meshopt_optimizeVertexFetch(
      optimizedVertices.data(), 
      mesh.m_Indices.data(), 
      mesh.m_Indices.size(), 
      mesh.m_Vertices.data(), 
      mesh.m_Vertices.size(), 
      sizeof(Vertex)
  );

  mesh.m_Vertices = std::move(optimizedVertices);
}

void Model::CreatePhysXStaticMesh(Mesh& mesh)
{
  std::vector<PxVec3> physxVertices(mesh.m_Vertices.size());
  for (size_t i = 0; i < mesh.m_Vertices.size(); ++i) {
      physxVertices[i] = PxVec3(mesh.m_Vertices[i].Position.x, mesh.m_Vertices[i].Position.y, mesh.m_Vertices[i].Position.z);
  }

  PxTriangleMesh* physxMesh = PhysX::CreateTriangleMesh(static_cast<PxU32>(physxVertices.size()), physxVertices.data(), static_cast<PxU32>(mesh.m_Indices.size() / 3), mesh.m_Indices.data());

  if (!physxMesh) GABGL_ERROR("Failed to create PhysX triangle mesh");

  PxScene* scene = PhysX::getScene();
  PxPhysics* physics = PhysX::getPhysics();
  PxMaterial* material = PhysX::getMaterial();

  PxTriangleMeshGeometry geom;
  PxTransform pose = PxTransform(PxVec3(0));
  geom.triangleMesh = physxMesh;

  PxRigidDynamic* meshActor = physics->createRigidDynamic(pose);
  PxShape* meshShape;
  if(meshActor){
      meshActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

      PxTriangleMeshGeometry triGeom;
      triGeom.triangleMesh = physxMesh;
      meshShape = PxRigidActorExt::createExclusiveShape(*meshActor, triGeom, *material);
      scene->addActor(*meshActor);
  }
}

glm::mat4 Model::ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
{
  glm::mat4 to;
  //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
  to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
  to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
  to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
  to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
  return to;
}

glm::vec3 Model::GetGLMVec(const aiVector3D& vec) 
{ 
  return glm::vec3(vec.x, vec.y, vec.z); 
}
glm::quat Model::GetGLMQuat(const aiQuaternion& pOrientation)
{
  return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
}



