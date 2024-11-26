#pragma once
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/hash.hpp"
#include <string>
#include <vector>


/*struct BoundingBox {*/
/*    glm::vec3 size;*/
/*    glm::vec3 offsetFromModelOrigin;*/
/*};*/
/**/
/*struct Vertex {*/
/*    Vertex() = default;*/
/*    Vertex(glm::vec3 pos) {*/
/*        position = pos;*/
/*    }*/
/*    Vertex(glm::vec3 pos, glm::vec3 norm) {*/
/*        position = pos;*/
/*        normal = norm;*/
/*    }*/
/*   glm::vec3 position = glm::vec3(0);*/
/*   glm::vec3 normal = glm::vec3(0);*/
/*   glm::vec2 uv = glm::vec2(0);*/
/*   glm::vec3 tangent = glm::vec3(0);*/
/*    bool operator==(const Vertex& other) const {*/
/*        return position == other.position && normal == other.normal && uv == other.uv;*/
/*    }*/
/*};*/
/**/
/*struct WeightedVertex {*/
/*    glm::vec3 position = glm::vec3(0);*/
/*    glm::vec3 normal = glm::vec3(0);*/
/*    glm::vec2 uv = glm::vec2(0);*/
/*    glm::vec3 tangent = glm::vec3(0);*/
/*    glm::ivec4 boneID = glm::ivec4(0);*/
/*    glm::vec4 weight = glm::vec4(0);*/
/**/
/*    bool operator==(const Vertex& other) const {*/
/*        return position == other.position && normal == other.normal && uv == other.uv;*/
/*    }*/
/*};*/
/**/
/*struct DebugVertex {*/
/*    glm::vec3 position = glm::vec3(0);*/
/*    glm::vec3 color = glm::vec3(0);*/
/*};*/
/**/
/*inline glm::vec3 Vec3Min(const glm::vec3& a, const glm::vec3& b) {*/
/*    return glm::vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));*/
/*}*/
/**/
/*inline glm::vec3 Vec3Max(const glm::vec3& a, const glm::vec3& b) {*/
/*    return glm::vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));*/
/*}*/

/*
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}*/

/*namespace std {*/
/*    template<> struct hash<Vertex> {*/
/*        size_t operator()(Vertex const& vertex) const {*/
/*            size_t h1 = hash<glm::vec3>()(vertex.position);*/
/*            size_t h2 = hash<glm::vec3>()(vertex.normal);*/
/*            size_t h3 = hash<glm::vec2>()(vertex.uv);*/
/*            return h1 ^ (h2 << 1) ^ (h3 << 2);  // Combining the hashes*/
/*        }*/
/*    };*/
/*}*/
/**/
/*struct Mesh {*/
/**/
/*    std::string name = "undefined";*/
/**/
/*    int32_t baseVertex = 0;*/
/*    uint32_t baseIndex = 0;*/
/*    uint32_t vertexCount = 0;*/
/*    uint32_t indexCount = 0;*/
/**/
/*    glm::vec3 aabbMin = glm::vec3(0);*/
/*    glm::vec3 aabbMax = glm::vec3(0);*/
/*    glm::vec3 extents = glm::vec3(0);*/
/*    float boundingSphereRadius = 0;*/
/**/
/*    bool uploadedToGPU = false;*/
/*};*/
/**/
/*struct Model {*/
/**/
/*private:*/
/*    std::string m_name = "undefined";*/
/*    std::vector<uint32_t> m_meshIndices;*/
/*    BoundingBox m_boundingBox;*/
/*public:*/
/*    glm::vec3 m_aabbMin;*/
/*    glm::vec3 m_aabbMax;*/
/*    bool m_awaitingLoadingFromDisk = true;*/
/*    bool m_loadedFromDisk = false;*/
/*    std::string m_fullPath = "";*/
/**/
/*public:*/
/**/
/*    Model() = default;*/
/**/
/*    Model(std::string fullPath) {*/
/*        m_fullPath = fullPath;*/
/*        m_name = fullPath.substr(fullPath.rfind("/") + 1);*/
/*        m_name = m_name.substr(0, m_name.length() - 4);*/
/*    }*/
/**/
/*    void AddMeshIndex(uint32_t index) {*/
/*        m_meshIndices.push_back(index);*/
/*    }*/
/**/
/*    size_t GetMeshCount() {*/
/*        return m_meshIndices.size();*/
/*    }*/
/**/
/*    std::vector<uint32_t>& GetMeshIndices() {*/
/*        return m_meshIndices;*/
/*    }*/
/**/
/*    void SetName(std::string modelName) {*/
/*        this->m_name = modelName;*/
/*    }*/
/**/
/*    const std::string GetName() {*/
/*        return m_name;*/
/*    }*/
/**/
/*    const BoundingBox& GetBoundingBox() {*/
/*        return m_boundingBox;*/
/*    }*/
/**/
/*    void SetBoundingBox(BoundingBox& modelBoundingBox) {*/
/*        this->m_boundingBox = modelBoundingBox;*/
/*    }*/
/*};*/
