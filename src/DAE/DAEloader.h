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

    DAE(const std::string& path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    void Draw(Shader& shader)
    {
        for (auto& mesh : meshes)
            mesh.Draw(shader);
    }

    std::map<std::string, BoneInfo>& GetBoneInfoMap() { return boneInfoMap; }
    int& GetBoneCount() { return boneCounter; }

private:

    std::vector<Texture> texturesLoaded;  
    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection;

    std::map<std::string, BoneInfo> boneInfoMap; 
    int boneCounter = 0;

    // Load model from file
    void loadModel(const std::string& path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, 
            aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
    }

    // Recursively process nodes
    void processNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    // Process individual mesh
    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
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
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
        }

        // Process material
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse",scene);
            auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular",scene);
            auto normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal",scene);
            auto heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height",scene);

            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
            textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        }

        ExtractBoneWeightForVertices(vertices, mesh, scene);

        return Mesh(std::move(vertices), std::move(indices), std::move(textures));
    }

    // Initialize bone data for a vertex
    void SetDefaultBoneData(Vertex& vertex)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            vertex.m_BoneIDs[i] = -1;
            vertex.m_Weights[i] = 0.0f;
        }
    }

    // Set bone data for a vertex
    void SetBoneData(Vertex& vertex, int boneID, float weight)
    {
        if (weight <= 0.0f) return;

        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            if (vertex.m_BoneIDs[i] < 0)
            {
                vertex.m_BoneIDs[i] = boneID;
                vertex.m_Weights[i] = weight;
                return;
            }
        }
    }

    // Extract bone weights
    void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
    {
        for (unsigned int i = 0; i < mesh->mNumBones; ++i)
        {
            std::string boneName = mesh->mBones[i]->mName.C_Str();
            int boneID = -1;

            if (boneInfoMap.find(boneName) == boneInfoMap.end())
            {
                boneID = boneCounter++;
                boneInfoMap[boneName] = { boneID, AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[i]->mOffsetMatrix) };
            }
            else
            {
                boneID = boneInfoMap[boneName].id;
            }

            for (unsigned int j = 0; j < mesh->mBones[i]->mNumWeights; ++j)
            {
                int vertexID = mesh->mBones[i]->mWeights[j].mVertexId;
                float weight = mesh->mBones[i]->mWeights[j].mWeight;

                if (vertexID < vertices.size())
                {
                    SetBoneData(vertices[vertexID], boneID, weight);
                }
            }
        }
    }

    // Load material textures
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, const aiScene* scene)
    {
        std::vector<Texture> textures;

        for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            // Check if the texture is already loaded (by path)
            auto it = std::find_if(texturesLoaded.begin(), texturesLoaded.end(), [&](const Texture& tex) {
                return tex.path == str.C_Str();
            });

            if (it != texturesLoaded.end())
            {
                // Texture is already loaded
                textures.push_back(*it);
            }
            else
            {
                Texture texture;

                // Check if this texture is embedded (Assimp stores embedded textures in the scene)
                if (scene && i < scene->mNumTextures)
                {
                    aiTexture* aiTex = scene->mTextures[i];  // Access the embedded texture directly

                    if (aiTex != nullptr && aiTex->mHeight == 0)  // Check for embedded texture (mHeight == 0 means it's embedded)
                    {
                        // Convert aiTexel* data to unsigned char*
                        const aiTexel* rawData = aiTex->pcData;
                        size_t width = aiTex->mWidth;
                        size_t height = aiTex->mHeight;
                        size_t byteSize = width * height * 4;  // Assuming RGBA format for embedded textures (4 components per pixel)

                        // Load the texture from memory (assuming it's in RGBA format)
                        texture.id = TextureFromMemory(reinterpret_cast<const unsigned char*>(rawData), byteSize);
                        texture.type = typeName;
                        texture.path = str.C_Str();  // Set the texture path to the aiString path
                        textures.push_back(texture);
                        texturesLoaded.push_back(std::move(texture));  // Add the loaded texture to the list
                    }
                }

                // If not embedded, load the texture from the file system
                if (texture.id == 0)  // If it was not loaded from memory
                {
                    texture.id = TextureFromFile(str.C_Str(), directory);
                    texture.type = typeName;
                    texture.path = str.C_Str();
                    textures.push_back(texture);
                    texturesLoaded.push_back(std::move(texture));
                }
            }
        }

        return textures;
    }
};
