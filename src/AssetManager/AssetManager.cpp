/*#define TINYOBJLOADER_IMPLEMENTATION*/
/*#include "AssetManager.h"*/
/*#include "tiny_obj_loader.h"*/
/*#include <cstdint>*/
/*#include <unordered_map>*/
/*#include <iostream>*/
/*#include <mutex>*/
/*#include <future>*/
/**/
/*namespace AssetManager{*/
/**/
/*  struct CompletedLoadingTasks {*/
/*      bool g_hardcodedModels = false;*/
/*      bool g_materials = false;*/
/*      bool g_texturesBaked = false;*/
/*      bool g_cubemapTexturesBaked = false;*/
/*      bool g_cmpTexturesBaked = false;*/
/*      bool g_all = false;*/
/*  } g_completedLoadingTasks;*/

  /*std::vector<Vertex> g_vertices;*/
  /*std::vector<WeightedVertex> g_weightedVertices;*/
  /*std::vector<uint32_t> g_indices;*/
  /*std::vector<uint32_t> g_weightedIndices;*/
  /**/
  /*std::vector<Mesh> g_meshes;*/
  /*std::vector<Model> g_models;*/
  /*// Used to new data insert into the vectors above*/
  /*int _nextVertexInsert = 0;*/
  /*int _nextIndexInsert = 0;*/
  /*int _nextWeightedVertexInsert = 0;*/
  /*int _nextWeightedIndexInsert = 0;*/
  /**/
  /*std::mutex _modelsMutex;*/
  /*std::vector<std::future<void>> _futures;*/
/*}*/


/*void AssetManager::LoadModel(Model* model) {*/
/**/
/*    tinyobj::attrib_t attrib;*/
/*    std::vector<tinyobj::shape_t> shapes;*/
/*    std::vector<tinyobj::material_t> materials;*/
/*    std::string warn;*/
/*    std::string err;*/
/*    glm::vec3 modelAabbMin = glm::vec3(std::numeric_limits<float>::max());*/
/*    glm::vec3 modelAabbMax = glm::vec3(-std::numeric_limits<float>::max());*/
/**/
/*    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model->m_fullPath.c_str())) {*/
/*        std::cout << "LoadModel() failed to load: '" << model->m_fullPath << '\n';*/
/*        return;*/
/*    }*/
/**/
/*    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};*/
/**/
/*    for (const auto& shape : shapes) {*/
/**/
/*        std::vector<Vertex> vertices;*/
/*        std::vector<uint32_t> indices;*/
/*        glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());*/
/*        glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());*/
/**/
/*        for (int i = 0; i < shape.mesh.indices.size(); i++) {*/
/*            Vertex vertex = {};*/
/*            const auto& index = shape.mesh.indices[i];*/
/*            vertex.position = {*/
/*                attrib.vertices[3 * index.vertex_index + 0],*/
/*                attrib.vertices[3 * index.vertex_index + 1],*/
/*                attrib.vertices[3 * index.vertex_index + 2]*/
/*            };*/
/*            // Check if `normal_index` is zero or positive. negative = no normal data*/
/*            if (index.normal_index >= 0) {*/
/*                vertex.normal.x = attrib.normals[3 * size_t(index.normal_index) + 0];*/
/*                vertex.normal.y = attrib.normals[3 * size_t(index.normal_index) + 1];*/
/*                vertex.normal.z = attrib.normals[3 * size_t(index.normal_index) + 2];*/
/*            }*/
/*            if (attrib.texcoords.size() && index.texcoord_index != -1) { // should only be 1 or 2, there is some bug here where in debug where there were over 1000 on the sphere lines model...*/
/*                vertex.uv = { attrib.texcoords[2 * index.texcoord_index + 0],	1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };*/
/*            }*/
/**/
/*            if (uniqueVertices.count(vertex) == 0) {*/
/*                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());*/
/*                vertices.push_back(vertex);*/
/*            }*/
/**/
/*            // store bounding box shit*/
/*            aabbMin.x = std::min(aabbMin.x, vertex.position.x);*/
/*            aabbMin.y = std::min(aabbMin.y, vertex.position.y);*/
/*            aabbMin.z = std::min(aabbMin.z, vertex.position.z);*/
/*            aabbMax.x = std::max(aabbMax.x, vertex.position.x);*/
/*            aabbMax.y = std::max(aabbMax.y, vertex.position.y);*/
/*            aabbMax.z = std::max(aabbMax.z, vertex.position.z);*/
/**/
/*            indices.push_back(uniqueVertices[vertex]);*/
/*        }*/
/**/
/*        // Tangents*/
/*        for (int i = 0; i < indices.size(); i += 3) {*/
/*            Vertex* vert0 = &vertices[indices[i]];*/
/*            Vertex* vert1 = &vertices[indices[i + 1]];*/
/*            Vertex* vert2 = &vertices[indices[i + 2]];*/
/*            glm::vec3 deltaPos1 = vert1->position - vert0->position;*/
/*            glm::vec3 deltaPos2 = vert2->position - vert0->position;*/
/*            glm::vec2 deltaUV1 = vert1->uv - vert0->uv;*/
/*            glm::vec2 deltaUV2 = vert2->uv - vert0->uv;*/
/*            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);*/
/*            glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;*/
/*            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;*/
/*            vert0->tangent = tangent;*/
/*            vert1->tangent = tangent;*/
/*            vert2->tangent = tangent;*/
/*        }*/
/**/
/*        modelAabbMin = Vec3Min(modelAabbMin, aabbMin);*/
/*        modelAabbMax = Vec3Max(modelAabbMax, aabbMax);*/
/**/
/*        std::lock_guard<std::mutex> lock(_modelsMutex);*/
/*        model->AddMeshIndex(AssetManager::CreateMesh(shape.name, vertices, indices, aabbMin, aabbMax));*/
/*    }*/
/**/
/*    // Build the bounding box*/
/*    float width = std::abs(modelAabbMax.x - modelAabbMin.x);*/
/*    float height = std::abs(modelAabbMax.y - modelAabbMin.y);*/
/*    float depth = std::abs(modelAabbMax.z - modelAabbMin.z);*/
/*    BoundingBox boundingBox;*/
/*    boundingBox.size = glm::vec3(width, height, depth);*/
/*    boundingBox.offsetFromModelOrigin = modelAabbMin;*/
/*    model->SetBoundingBox(boundingBox);*/
/*    model->m_aabbMin = modelAabbMin;*/
/*    model->m_aabbMax = modelAabbMax;*/
/**/
/*    // Done*/
/*    model->m_loadedFromDisk = true;*/
/*}*/
/**/
/*int AssetManager::CreateMesh(std::string name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3 aabbMin, glm::vec3 aabbMax) {*/
/*    Mesh& mesh = g_meshes.emplace_back();*/
/*    mesh.baseVertex = _nextVertexInsert;*/
/*    mesh.baseIndex = _nextIndexInsert;*/
/*    mesh.vertexCount = (uint32_t)vertices.size();*/
/*    mesh.indexCount = (uint32_t)indices.size();*/
/*    mesh.name = name;*/
/*    mesh.aabbMin = aabbMin;*/
/*    mesh.aabbMax = aabbMax;*/
/*    mesh.extents = aabbMax - aabbMin;*/
/*    mesh.boundingSphereRadius = std::max(mesh.extents.x, std::max(mesh.extents.y, mesh.extents.z)) * 0.5f;*/
/**/
/*    g_vertices.reserve(g_vertices.size() + vertices.size());*/
/*    g_vertices.insert(std::end(g_vertices), std::begin(vertices), std::end(vertices));*/
/*    g_indices.reserve(g_indices.size() + indices.size());*/
/*    g_indices.insert(std::end(g_indices), std::begin(indices), std::end(indices));*/
/*    _nextVertexInsert += mesh.vertexCount;*/
/*    _nextIndexInsert += mesh.indexCount;*/
/*    return g_meshes.size() - 1;*/
/*}*/
