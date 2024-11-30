#pragma once

#include <assimp/scene.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "Assimp_glm_helpers.h"
#include <string>
#include <vector>
#include <cassert>

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

class Bone {
public:
    Bone(const std::string& name, int ID, const aiNodeAnim* channel)
        : m_Name(name), m_ID(ID), m_LocalTransform(1.0f) {
        // Extract position keyframes
        m_NumPositions = channel->mNumPositionKeys;
        for (unsigned int i = 0; i < m_NumPositions; ++i) {
            aiVector3D aiPosition = channel->mPositionKeys[i].mValue;
            float timeStamp = static_cast<float>(channel->mPositionKeys[i].mTime);
            KeyPosition data = { AssimpGLMHelpers::GetGLMVec(aiPosition), timeStamp };
            m_Positions.push_back(data);
        }

        // Extract rotation keyframes
        m_NumRotations = channel->mNumRotationKeys;
        for (unsigned int i = 0; i < m_NumRotations; ++i) {
            aiQuaternion aiOrientation = channel->mRotationKeys[i].mValue;
            float timeStamp = static_cast<float>(channel->mRotationKeys[i].mTime);
            KeyRotation data = { AssimpGLMHelpers::GetGLMQuat(aiOrientation), timeStamp };
            m_Rotations.push_back(data);
        }

        // Extract scaling keyframes
        m_NumScalings = channel->mNumScalingKeys;
        for (unsigned int i = 0; i < m_NumScalings; ++i) {
            aiVector3D aiScale = channel->mScalingKeys[i].mValue;
            float timeStamp = static_cast<float>(channel->mScalingKeys[i].mTime);
            KeyScale data = { AssimpGLMHelpers::GetGLMVec(aiScale), timeStamp };
            m_Scales.push_back(data);
        }
    }

    void Update(float animationTime) {
        glm::mat4 translation = InterpolatePosition(animationTime);
        glm::mat4 rotation = InterpolateRotation(animationTime);
        glm::mat4 scale = InterpolateScaling(animationTime);
        m_LocalTransform = translation * rotation * scale;
    }

    glm::mat4 GetInterpolatedTransform(float animationTime) const
    {
        glm::mat4 translation = InterpolatePosition(animationTime);
        glm::mat4 rotation = InterpolateRotation(animationTime);
        glm::mat4 scale = InterpolateScaling(animationTime);
        return translation * rotation * scale;
    }

    void SetTransform(const glm::mat4& transform) { m_LocalTransform = transform; }
    glm::mat4 GetLocalTransform() const { return m_LocalTransform; }
    std::string GetBoneName() const { return m_Name; }
    int GetBoneID() const { return m_ID; }

private:
    // Interpolation helper functions
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const {
        float scaleFactor = (animationTime - lastTimeStamp) / (nextTimeStamp - lastTimeStamp);
        return glm::clamp(scaleFactor, 0.0f, 1.0f); // Ensure it's in the valid range
    }

    glm::mat4 InterpolatePosition(float animationTime) const {
        if (m_NumPositions == 1) {
            return glm::translate(glm::mat4(1.0f), m_Positions[0].position);
        }

        int p0Index = GetPositionIndex(animationTime);
        int p1Index = p0Index + 1;

        float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp, m_Positions[p1Index].timeStamp, animationTime);
        glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);

        return glm::translate(glm::mat4(1.0f), finalPosition);
    }

    glm::mat4 InterpolateRotation(float animationTime) const {
        if (m_NumRotations == 1) {
            return glm::toMat4(glm::normalize(m_Rotations[0].orientation));
        }

        int p0Index = GetRotationIndex(animationTime);
        int p1Index = p0Index + 1;

        float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp, m_Rotations[p1Index].timeStamp, animationTime);
        glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation, scaleFactor);

        return glm::toMat4(glm::normalize(finalRotation));
    }

    glm::mat4 InterpolateScaling(float animationTime) const {
        if (m_NumScalings == 1) {
            return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);
        }

        int p0Index = GetScaleIndex(animationTime);
        int p1Index = p0Index + 1;

        float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);
        glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);

        return glm::scale(glm::mat4(1.0f), finalScale);
    }

    int GetPositionIndex(float animationTime) const {
        for (int i = 0; i < m_NumPositions - 1; ++i) {
            if (animationTime < m_Positions[i + 1].timeStamp) {
                return i;
            }
        }
        assert(false && "Invalid position index");
        return -1;
    }

    int GetRotationIndex(float animationTime) const {
        for (int i = 0; i < m_NumRotations - 1; ++i) {
            if (animationTime < m_Rotations[i + 1].timeStamp) {
                return i;
            }
        }
        assert(false && "Invalid rotation index");
        return -1;
    }

    int GetScaleIndex(float animationTime) const {
        for (int i = 0; i < m_NumScalings - 1; ++i) {
            if (animationTime < m_Scales[i + 1].timeStamp) {
                return i;
            }
        }
        assert(false && "Invalid scaling index");
        return -1;
    }

    // Members
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
