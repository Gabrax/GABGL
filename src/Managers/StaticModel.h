#pragma once

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "../LoadShader.h"
#include "../LoadTexture.h"
#include "../Window.h"
#include "../Utilities.hpp"

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map> 


struct StaticModel {

    StaticModel(const char* modelpath, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(modelpath);

        const char* filename = strrchr(modelpath, '/');
        filename = (filename == nullptr) ? modelpath : filename + 1;

        std::cout << filename << " loaded" << '\n';
    }

    ~StaticModel() noexcept = default;

    void Render(const glm::vec3& position, const glm::vec3& scale = glm::vec3(1.0f), const float rotation = 0.0f)
    {
        _shader.Use();
        _shader.setVec3("viewPos", this->camera.Position);

        // light properties
        _shader.setFloat("light.constant", 1.0f);
        _shader.setFloat("light.linear", 0.09f);
        _shader.setFloat("light.quadratic", 0.032f);

        // material properties
        _shader.setFloat("material.shininess", 32.0f);
        
        glm::mat4 projection = glm::perspective(glm::radians(this->camera.Zoom), Window::getAspectRatio(), 0.001f, 2000.0f);
        _shader.setMat4("projection", projection);

        _shader.setMat4("view", this->camera.GetViewMatrix());
        glm::mat4 model = glm::mat4(1.0f); 
        model = glm::translate(model, position);
        model = glm::scale(model, scale);  
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
        _shader.setMat4("model", model);

        DrawModel(_shader);
    }

    void Destroy() {
        this->~StaticModel();
    }

    void DrawModel(Shader& shader) {
        for (auto& mesh : meshes)
            mesh.Draw(shader);
    }

private:

    Camera& camera = Window::_camera;
    Shader& _shader = Utilities::g_shaders.model;

    std::unordered_map<std::string, Texture> textures_loaded; 
    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection;

    void loadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                  aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.emplace_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices = processVertices(mesh);
        std::vector<unsigned int> indices = processIndices(mesh);
        std::vector<Texture> textures = processTextures(mesh, scene);

        return Mesh(vertices, indices, textures);
    }

    std::vector<Vertex> processVertices(aiMesh* mesh) {
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

    std::vector<unsigned int> processIndices(aiMesh* mesh) {
        std::vector<unsigned int> indices;
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            const aiFace& face = mesh->mFaces[i];
            indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
        }
        return indices;
    }

    std::vector<Texture> processTextures(aiMesh* mesh, const aiScene* scene) {
        std::vector<Texture> textures;
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

    void loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName,
                              std::vector<Texture>& textures) {
        for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i) {
            aiString str;
            mat->GetTexture(type, i, &str);
            std::string texturePath = str.C_Str();

            // Check if texture is already loaded
            if (textures_loaded.find(texturePath) != textures_loaded.end()) {
                textures.emplace_back(textures_loaded[texturePath]);
                continue;
            }

            // Load texture
            Texture texture;
            texture.id = TextureFromFile(texturePath.c_str(), directory);
            texture.type = typeName;
            texture.path = texturePath;

            textures.emplace_back(texture);
            textures_loaded[texturePath] = texture;
        }
    }
};


