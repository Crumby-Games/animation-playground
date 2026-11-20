#pragma once

#include <string>
#include <vector>
#include <DirectXMath.h>
#include "tiny_gltf.h"
#include "AnimGraph.h"
#include "Animation.h"


struct ComponentTransform {
    DirectX::XMFLOAT3 translation;
    DirectX::XMFLOAT4 rotation;
    DirectX::XMFLOAT3 scale;

    DirectX::XMVECTOR Scale() {
        return DirectX::XMLoadFloat3(&scale);
    }
    DirectX::XMVECTOR Rotation() {
        return DirectX::XMLoadFloat4(&rotation);
    }
    DirectX::XMVECTOR Translation() {
        return DirectX::XMLoadFloat3(&translation);
    }

    DirectX::XMMATRIX Matrix() {
        return DirectX::XMMatrixScalingFromVector(Scale()) * DirectX::XMMatrixRotationQuaternion(Rotation()) * DirectX::XMMatrixTranslationFromVector(Translation());
    }
};

struct Joint
{
    // The name of the joint, useful for debugging.
    std::string name = "unknown";

    // The transform of the bone in its parent's space at the time of binding.
    DirectX::XMFLOAT4X4  localBindTransform{};

    // The inverse of the bone's transform in model space at the time of binding.
    DirectX::XMFLOAT4X4  inverseBindMatrix{};

    // Indices of the children of this joint in the skeleton's main joint list.
    std::vector<int> children;

    ComponentTransform componentTransform{};

    DirectX::XMFLOAT4X4 finalTransform{};

    // The final transform of the bone in model space for the current animation frame.
    // This is calculated at runtime by combining the local animated pose with the parent's final transform.

};

class Skeleton
{
public:

    Skeleton();

    // Loads the skeleton hierarchy and matrices from a glTF model.
    // Returns true on success.
    bool LoadFromGltf(const tinygltf::Model& model);

    // Updates the pose of the skeleton based on the animation time.
    void Update(float deltaTime);

    // Returns the final skinning matrices ready to be sent to the GPU.
    const void GetSkinningMatrices(DirectX::XMFLOAT4X4* matrixlist, unsigned int arraylength) const;
    const unsigned int GetBoneCount() { return m_skinningMatrices.size(); }
    const DirectX::XMMATRIX& GetRootTransform() const { return XMLoadFloat4x4(&m_rootTransform); }

    AnimGraph& GetAnimGraph() { return m_animGraph; }
    unsigned int GetAnimationCount() { return m_animationCount; }
    std::vector<Animation>& GetAnimations() { return m_animations; }
    bool IsLoaded() { return m_isLoaded; }

    int AddJoint(int parentIndex, const DirectX::XMFLOAT4X4& localBindTransform);
    Joint* GetJoint(unsigned int joint) {
        return &m_joints[joint];
    }

    void PlayAnimation(int index, float transitionTime = 0);

private:
    // Change the signature to accept a pointer.
    void UpdateJointGlobalTransform(int jointIndex, const DirectX::XMMATRIX& parentTransform);

    std::map<int, ComponentTransform> ApplyAnimNode(std::shared_ptr<BaseAnimNode> node);
    ComponentTransform GetLocalAnimatedTransformForJoint(
        int jointIndex,
        const Animation* animation, // Use a pointer to allow for nullptr
        float timeInSeconds
    );

    // The flat list of all joints that make up this skeleton.
    std::vector<Joint> m_joints;

    // A list of indices for the root joints (those without a parent in the skeleton).
    std::vector<int> m_rootJointIndices;

    // The final matrices sent to the shader, calculated by multiplying the
    // inverse bind matrix by the final animated transform for each joint.
    std::vector<DirectX::XMFLOAT4X4> m_skinningMatrices;

    DirectX::XMFLOAT4X4 m_rootTransform;
   
    AnimGraph               m_animGraph;
    unsigned int            m_animationCount;
    std::vector<Animation>  m_animations;
    float                   m_lifetime = 0.0f;
    bool                    m_isLoaded = false;
};


