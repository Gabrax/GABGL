#pragma once

#include <cassert>
#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "Animation.h"
#include "Bone.h"


struct Animator {

    Animator() = default;

    Animator(Animation* animation) : m_CurrentAnimation(animation), m_CurrentTime(0.0f)
    {
        ResizeFinalBoneMatrices();
    }

    void UpdateAnimation(float dt)
    {
        m_DeltaTime = dt;
        if (m_CurrentAnimation)
        {
            m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
            m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
            CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
        }
    }

    void PlayAnimation(int animationIndex)
    {
        assert(m_CurrentAnimation);
        
        m_CurrentAnimation->SetAnimation(animationIndex);
        m_CurrentTime = 0.0f;
        /*m_CurrentAnimation->SetAnimation(animationIndex);*/
        ResizeFinalBoneMatrices();
    }

    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
    {
        std::string nodeName = node->name;
        glm::mat4 nodeTransform = node->transformation;

        // Check if this node has a corresponding bone in the animation
        Bone* bone = m_CurrentAnimation->FindBone(nodeName);
        if (bone)
        {
            bone->Update(m_CurrentTime);
            nodeTransform = bone->GetLocalTransform();
        }

        // Calculate the global transformation for this node
        glm::mat4 globalTransformation = parentTransform * nodeTransform;

        // Look up the bone in the boneInfoMap to get the offset matrix
        const auto& boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
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

private:

    void ResizeFinalBoneMatrices()
    {
      assert(m_CurrentAnimation);
      const auto& boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
      m_FinalBoneMatrices.resize(boneInfoMap.size(), glm::mat4(1.0f));
    }

    std::vector<glm::mat4> m_FinalBoneMatrices;
    Animation* m_CurrentAnimation = nullptr;
    float m_CurrentTime = 0.0f;
    float m_DeltaTime = 0.0f;
};

