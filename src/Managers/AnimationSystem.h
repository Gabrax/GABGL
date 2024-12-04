#pragma once

#include <cassert>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "Bone.h"
#include "Animdata.h"
#include "AnimatedMesh.h"

struct AssimpNodeData
{
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

struct AnimationData {
    std::string name;
    float duration;
    float ticksPerSecond;
    std::vector<Bone> bones;  // Preprocessed bone data for the animation.
    AssimpNodeData hierarchy; // Precomputed node hierarchy for the animation.
};

struct AnimationSystem {

    ~AnimationSystem() = default;

    AnimationSystem() = default;
    AnimationSystem(const std::string& animationPath, AnimatedMesh* model) : model(model), m_CurrentAnimationIndex(0)
    {
        assert(model);

        Assimp::Importer importer;
        scene = importer.ReadFile(animationPath, aiProcess_Triangulate);

        assert(scene && scene->mRootNode);

        for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
        {
            aiAnimation* animation = scene->mAnimations[i];

            AnimationData animData;
            animData.name = animation->mName.C_Str();
            animData.duration = animation->mDuration;
            animData.ticksPerSecond = animation->mTicksPerSecond;

            ReadHierarchyData(animData.hierarchy, scene->mRootNode);
            ReadMissingBones(animation, *model);

            animData.bones = m_Bones;

            m_ProcessedAnimations.push_back(animData);
        }

        assert(!m_ProcessedAnimations.empty());

        SetAnimationByName("IDLE");

        ResizeFinalBoneMatrices();
    }

    void UpdateAnimation(float dt)
    {
        m_DeltaTime = dt;
        m_CurrentTime += GetTicksPerSecond() * dt;
        m_CurrentTime = fmod(m_CurrentTime, GetDuration());
        CalculateBoneTransform(&GetRootNode(), glm::mat4(1.0f));
    }

    void SetAnimationbyIndex(int animationIndex)
    {
        assert(animationIndex >= 0 && animationIndex < m_ProcessedAnimations.size());
        
        const AnimationData& animData = m_ProcessedAnimations[animationIndex];
        const AnimationData& animData2 = m_ProcessedAnimations[0];

        m_Duration = animData.duration;
        m_TicksPerSecond = animData.ticksPerSecond;

        m_RootNode = animData.hierarchy;
        m_Bones = animData.bones;

        std::cout << "Current animation: " << animData.name << '\n';

        ResizeFinalBoneMatrices();
    }

    void SetAnimationByName(const std::string& animationName)
    {
        auto it = std::find_if(m_ProcessedAnimations.begin(), m_ProcessedAnimations.end(),
            [&animationName](const AnimationData& animData) {
                return animData.name == animationName;
            });

        if (it != m_ProcessedAnimations.end()) {
            // Get the index of the found animation
            int animationIndex = std::distance(m_ProcessedAnimations.begin(), it);
            SetAnimationbyIndex(animationIndex); 
        } else {
            std::cerr << "Animation not found: " << animationName << '\n';
        }
    }

    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
    {
        std::string nodeName = node->name;
        glm::mat4 nodeTransform = node->transformation;

        // Check if this node has a corresponding bone in the animation
        Bone* bone = FindBone(nodeName);
        if (bone)
        {
            bone->Update(m_CurrentTime);
            nodeTransform = bone->GetLocalTransform();
        }

        // Calculate the global transformation for this node
        glm::mat4 globalTransformation = parentTransform * nodeTransform;

        // Look up the bone in the boneInfoMap to get the offset matrix
        const auto& boneInfoMap = GetBoneIDMap();
        auto it = boneInfoMap.find(nodeName);
        if (it != boneInfoMap.end())
        {
            int index = it->second.id;
            glm::mat4 offset = it->second.offset;
            m_FinalBoneMatrices[index] = globalTransformation * offset;
        }

        // Recursively calculate transformations for the children
        for (size_t i = 0; i < node->children.size(); ++i)
        {
            CalculateBoneTransform(&node->children[i], globalTransformation);
        }
    }

    std::vector<glm::mat4> GetFinalBoneMatrices() const
    {
        return m_FinalBoneMatrices;
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
    inline const std::map<std::string,BoneInfo>& GetBoneIDMap() { return m_BoneInfoMap; }

private:

    void ResizeFinalBoneMatrices()
    {
      const auto& boneInfoMap = GetBoneIDMap();
      m_FinalBoneMatrices.resize(boneInfoMap.size(), glm::mat4(1.0f));
    }

    std::vector<glm::mat4> m_FinalBoneMatrices;
    float m_CurrentTime = 0.0f;
    float m_DeltaTime = 0.0f;

    float m_BlendFactor = 0.0f; // 0 -> 1 
    bool m_IsBlending = false;
    int m_NextAnimationIndex = -1;
    std::vector<AnimationData> m_ProcessedAnimations;
    AnimatedMesh* model;
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

    void ReadMissingBones(const aiAnimation* animation, AnimatedMesh& model)
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


