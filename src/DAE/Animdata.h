#pragma once

#include<glm/glm.hpp>

struct BoneInfo {
    int id;
    glm::mat4 offset;

    /*// Default constructor*/
    /*BoneInfo() : id(-1), offset(glm::mat4(1.0f)) {}*/
    /**/
    /*// Constructor with ID*/
    /*BoneInfo(int id) : id(id), offset(glm::mat4(1.0f)) {}*/
    /**/
    /*// Copy constructor*/
    /*BoneInfo(const BoneInfo& other) : id(other.id), offset(other.offset) {}*/
    /**/
    /*// Move constructor*/
    /*BoneInfo(BoneInfo&& other) noexcept : id(other.id), offset(std::move(other.offset)) {}*/
    /**/
    /*// Copy assignment operator*/
    /*BoneInfo& operator=(const BoneInfo& other) {*/
    /*    if (this != &other) {*/
    /*        id = other.id;*/
    /*        offset = other.offset;*/
    /*    }*/
    /*    return *this;*/
    /*}*/
    /**/
    /*// Move assignment operator*/
    /*BoneInfo& operator=(BoneInfo&& other) noexcept {*/
    /*    if (this != &other) {*/
    /*        id = other.id;*/
    /*        offset = std::move(other.offset);*/
    /*    }*/
    /*    return *this;*/
    /*}*/
};
#pragma once
