#pragma once

#include <cassert>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "Bone.h"
#include "Animdata.h"
#include "DAEloader.h"

struct AssimpNodeData
{
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
    // Animation-specific data for each node.
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale> scales;
};

struct AnimationData {
    std::string name;
    float duration;
    float ticksPerSecond;
    std::vector<Bone> bones;  // Preprocessed bone data for the animation.
    AssimpNodeData hierarchy; // Precomputed node hierarchy for the animation.
};

struct Animation {

    Animation() = default;
    ~Animation() = default;

    Animation(const std::string& animationPath, DAE* model) : model(model), m_CurrentAnimationIndex(0)
    {
        assert(model);

        Assimp::Importer importer;
        scene = importer.ReadFile(animationPath, aiProcess_Triangulate);

        assert(scene && scene->mRootNode);

        for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
        {
            auto animation = scene->mAnimations[i];

            AnimationData animData;
            animData.name = animation->mName.C_Str();
            animData.duration = animation->mDuration;
            animData.ticksPerSecond = animation->mTicksPerSecond;

            // Read bone and hierarchy data.
            ReadHierarchyData(animData.hierarchy, scene->mRootNode);
            ReadMissingBones(animation, *model);

            // Optionally, copy bone data or preprocess for faster runtime evaluation.
            animData.bones = m_Bones;

            m_ProcessedAnimations.push_back(animData);
        }

        assert(!m_ProcessedAnimations.empty());
        SetAnimation(m_CurrentAnimationIndex);
    }

    void SetAnimation(int animationIndex)
    {
        assert(animationIndex >= 0 && animationIndex < m_ProcessedAnimations.size());
        m_CurrentAnimationIndex = animationIndex;

        const auto& animData = m_ProcessedAnimations[m_CurrentAnimationIndex];

        // Update current animation details.
        m_Duration = animData.duration;
        m_TicksPerSecond = animData.ticksPerSecond;

        // Update hierarchy and bone data.
        m_RootNode = animData.hierarchy;
        m_Bones = animData.bones;

        std::cout << "Current animation: " << animData.name << '\n';
    }

    Bone* FindBone(const std::string& name)
    {
        auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
            [&](const Bone& Bone) {
                return Bone.GetBoneName() == name;
            }
        );
        if (iter == m_Bones.end()) return nullptr;
        else return &(*iter);
    }

    inline float GetTicksPerSecond() { return m_TicksPerSecond; }
    inline float GetDuration() { return m_Duration; }
    inline const AssimpNodeData& GetRootNode() { return m_RootNode; }
    inline const std::map<std::string,BoneInfo>& GetBoneIDMap() 
    { 
        return m_BoneInfoMap;
    }

private:

    std::vector<AnimationData> m_ProcessedAnimations;
    DAE* model;
    const aiScene* scene = nullptr;
    int m_CurrentAnimationIndex;
    float m_Duration;
    int m_TicksPerSecond;
    std::vector<Bone> m_Bones;
    std::vector<const aiAnimation*> m_Animations;
    AssimpNodeData m_RootNode;
    std::map<std::string, BoneInfo> m_BoneInfoMap;

    void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
    {
        assert(src);  

        dest.name = "";
        dest.transformation = glm::mat4(1.0f);  // Reset to identity matrix
        dest.children.clear();  // Clear previous children

        dest.name = src->mName.data;
        dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
        dest.childrenCount = src->mNumChildren;

        for (int i = 0; i < src->mNumChildren; i++)
        {
            if (src->mChildren[i] == nullptr) 
            {
                std::cerr << "Null child node found at index " << i << std::endl;
                continue;  // Skip if child node is null
            }

            AssimpNodeData newData;
            ReadHierarchyData(newData, src->mChildren[i]);
            dest.children.push_back(newData);
        }
    }

    void ReadMissingBones(const aiAnimation* animation, DAE& model)
    {
        assert(animation);  
        assert(&model);
        auto& boneInfoMap = model.GetBoneInfoMap();
        int& boneCount = model.GetBoneCount();

        m_Bones.clear();
        boneCount = 0;

        for (int i = 0; i < animation->mNumChannels; i++)
        {
            auto channel = animation->mChannels[i];
            std::string boneName = channel->mNodeName.data;

            if (boneInfoMap.find(boneName) == boneInfoMap.end())
            {
                boneInfoMap[boneName].id = boneCount;
                boneCount++;
            }

            m_Bones.push_back(Bone(channel->mNodeName.data, boneInfoMap[boneName].id, channel));
        }

        m_BoneInfoMap = boneInfoMap;
    }
};


