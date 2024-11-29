#pragma once

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../OBJ/Mesh.h"
#include "../LoadShader.h"
#include "../LoadTexture.h"
#include "Assimp_glm_helpers.h"
#include "Animdata.h"

#include <string>
#include <iostream>
#include <map>
#include <vector>

struct DAE {

    const aiScene* scene; // Store scene data
    Assimp::Importer importer; // Assimp importer object

    bool gammaCorrection;
    std::map<std::string, BoneInfo> boneInfoMap; 
    int boneCounter = 0;

    std::vector<Texture> texturesLoaded;
    std::vector<Mesh> meshes;
    std::string directory;

    // Constructor
    DAE(const std::string& path, bool gamma = false) 
        : gammaCorrection(gamma), scene(nullptr) {
        loadModel(path);
    }

    void Draw(Shader& shader) {
        for (auto& mesh : meshes)
            mesh.Draw(shader);
    }

    std::map<std::string, BoneInfo>& GetBoneInfoMap() { return boneInfoMap; }
    int& GetBoneCount() { return boneCounter; }

private:
    // Load model from file
    void loadModel(const std::string& path) {
        scene = importer.ReadFile(path, 
            aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode);
    }

    // Recursively process nodes
    void processNode(aiNode* node) {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh));
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            processNode(node->mChildren[i]);
        }
    }

    // Process individual mesh
    Mesh processMesh(aiMesh* mesh) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            Vertex vertex;
            SetDefaultBoneData(vertex);

            vertex.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
            vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);
            vertex.TexCoords = mesh->mTextureCoords[0] ? 
                glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) :
                glm::vec2(0.0f, 0.0f);

            vertices.push_back(std::move(vertex));
        }

        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            aiFace face = mesh->mFaces[i];
            indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
        }

        // Process material
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            auto normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
            auto heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");

            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
            textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        }

        ExtractBoneWeightForVertices(vertices, mesh);
        return Mesh(std::move(vertices), std::move(indices), std::move(textures));
    }

    // Load material textures
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName) {
        std::vector<Texture> textures;

        for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
            aiString str;
            mat->GetTexture(type, i, &str);

            if (str.C_Str()[0] == '*') {
                const aiTexture* texture = scene->GetEmbeddedTexture(str.C_Str());
                if (texture) {
                    GLuint textureID;
                    LoadEmbeddedTexture(texture, textureID);

                    Texture embeddedTexture;
                    embeddedTexture.id = textureID;
                    embeddedTexture.type = typeName;
                    embeddedTexture.path = str.C_Str();
                    textures.push_back(embeddedTexture);
                }
            } else {
                // Handle external textures
                Texture externalTexture;
                externalTexture.id = TextureFromFile(str.C_Str(), directory);
                externalTexture.type = typeName;
                externalTexture.path = str.C_Str();
                textures.push_back(externalTexture);
            }
        }

        return textures;
    }

    // Extract bone weights
    void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh) {
        for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
            std::string boneName = mesh->mBones[i]->mName.C_Str();
            int boneID = -1;

            if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
                boneID = boneCounter++;
                boneInfoMap[boneName] = { boneID, AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[i]->mOffsetMatrix) };
            } else {
                boneID = boneInfoMap[boneName].id;
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
    void SetDefaultBoneData(Vertex& vertex) {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            vertex.m_BoneIDs[i] = -1;
            vertex.m_Weights[i] = 0.0f;
        }
    }

    // Set bone data for a vertex
    void SetBoneData(Vertex& vertex, int boneID, float weight) {
        if (weight <= 0.0f) return;

        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if (vertex.m_BoneIDs[i] < 0) {
                vertex.m_BoneIDs[i] = boneID;
                vertex.m_Weights[i] = weight;
                return;
            }
        }
    }

  void LoadEmbeddedTexture(const aiTexture* paiTexture, GLuint& textureID)
  {
      // Create and bind the texture
      glGenTextures(1, &textureID);
      glBindTexture(GL_TEXTURE_2D, textureID);

      if (paiTexture->mHeight == 0)
      {
          // Compressed texture (e.g., DDS)
          /*printf("Loading compressed embedded texture, format: '%s'\n", paiTexture->achFormatHint);*/
          int width, height, nrComponents;

          // Use stb_image or equivalent library to decode compressed texture
          unsigned char* data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(paiTexture->pcData),
                                                      paiTexture->mWidth, &width, &height, &nrComponents, 0);

          if (data)
          {
              GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;
              glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
              glGenerateMipmap(GL_TEXTURE_2D);

              stbi_image_free(data);
          }
          else
          {
              std::cerr << "Failed to load compressed embedded texture!\n";
          }
      }
      else
      {
          // Uncompressed raw data
          printf("Loading uncompressed embedded texture\n");
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, paiTexture->mWidth, paiTexture->mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, paiTexture->pcData);
          glGenerateMipmap(GL_TEXTURE_2D);
      }

      // Set texture parameters
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

};
