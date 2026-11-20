#include "Skeleton.h"
#include "Animation.h"

#include <map>

using namespace DirectX;


Skeleton::Skeleton() : m_animationCount(0)
{
    XMStoreFloat4x4(&m_rootTransform, XMMatrixIdentity());
}





int Skeleton::AddJoint(int parentIndex, const DirectX::XMFLOAT4X4& localBindTransform)
{
    Joint newJoint;
    newJoint.localBindTransform = localBindTransform;
    // The inverse bind matrix is for skinning, so we'll just use identity for now.
    DirectX::XMStoreFloat4x4(&newJoint.inverseBindMatrix, DirectX::XMMatrixIdentity());

    const int newJointIndex = m_joints.size();

    if (parentIndex < 0) { // This is a root joint
        m_rootJointIndices.push_back(newJointIndex);
    }
    else { // This is a child joint
        m_joints[parentIndex].children.push_back(newJointIndex);
    }

    m_joints.push_back(newJoint);
    m_skinningMatrices.push_back({}); // Add a placeholder skinning matrix
    m_isLoaded = true;
    return newJointIndex;
}


// Helper function to get a node's local transform.
// This avoids code duplication.
DirectX::XMFLOAT4X4 GetNodeLocalTransform(const tinygltf::Node& node)
{
    DirectX::XMFLOAT4X4 ret;

    if (node.matrix.size() == 16) {
        const double* m = node.matrix.data();
        XMStoreFloat4x4(&ret, XMMATRIX(
            (float)m[0], (float)m[1], (float)m[2], (float)m[3],
            (float)m[4], (float)m[5], (float)m[6], (float)m[7],
            (float)m[8], (float)m[9], (float)m[10], (float)m[11],
            (float)m[12], (float)m[13], (float)m[14], (float)m[15]
        ));
    }
    else {
        DirectX::XMVECTOR s = { 1.0f, 1.0f, 1.0f, 0.0f };
        if (node.scale.size() == 3) {
            s = DirectX::XMVectorSet((float)node.scale[0], (float)node.scale[1], (float)node.scale[2], 0.0f);
        }
        DirectX::XMVECTOR r = { 0.0f, 0.0f, 0.0f, 1.0f };
        if (node.rotation.size() == 4) {
            r = DirectX::XMVectorSet((float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3]);
        }
        DirectX::XMVECTOR t = { 0.0f, 0.0f, 0.0f, 0.0f };
        if (node.translation.size() == 3) {
            t = DirectX::XMVectorSet((float)node.translation[0], (float)node.translation[1], (float)node.translation[2], 1.0f);
        }
        XMStoreFloat4x4(&ret, XMMatrixScalingFromVector(s) *
            DirectX::XMMatrixRotationQuaternion(r) *
            DirectX::XMMatrixTranslationFromVector(t));
    }

    return ret;
}

const void Skeleton::GetSkinningMatrices(XMFLOAT4X4* matrixlist, unsigned int arraylength) const
{
    DirectX::XMMATRIX m;

    size_t n = std::min<size_t>(m_skinningMatrices.size(), arraylength);

    for (size_t i = 0; i < n; i += 1)
    {
        m = DirectX::XMLoadFloat4x4(&m_skinningMatrices[i]);
        m = DirectX::XMMatrixTranspose(m); // transpose for the GPU

        XMStoreFloat4x4(&matrixlist[i], m);
    }
}

bool Skeleton::LoadFromGltf(const tinygltf::Model& model)
{
    // --- PART 1 load the joints
    if (model.skins.empty()) {
        return false;
    }

    const tinygltf::Skin& skin = model.skins[0];
    m_skinningMatrices.resize(skin.joints.size());
    std::map<int, int> nodeToJointMap;
    for (size_t i = 0; i < skin.joints.size(); ++i) {
        m_joints.emplace_back(); // Use emplace_back for efficiency
        nodeToJointMap[skin.joints[i]] = static_cast<int>(i);
    }
    for (size_t i = 0; i < skin.joints.size(); ++i) {
        int nodeIndex = skin.joints[i];
        const tinygltf::Node& node = model.nodes[nodeIndex];
        Joint& joint = m_joints[i];
        joint.name = node.name;
        joint.localBindTransform = GetNodeLocalTransform(node); // Use helper
        const tinygltf::Accessor& ibmAccessor = model.accessors[skin.inverseBindMatrices];
        const tinygltf::BufferView& ibmBufferView = model.bufferViews[ibmAccessor.bufferView];
        const tinygltf::Buffer& ibmBuffer = model.buffers[ibmBufferView.buffer];
        const float* ibmPtr = reinterpret_cast<const float*>(&ibmBuffer.data[ibmBufferView.byteOffset + ibmAccessor.byteOffset]);
        XMStoreFloat4x4(&joint.inverseBindMatrix, XMMATRIX(&ibmPtr[i * 16]));
        
    }
    for (size_t i = 0; i < m_joints.size(); ++i) {
        int nodeIndex = skin.joints[i];
        const tinygltf::Node& node = model.nodes[nodeIndex];
        for (int childNodeIndex : node.children) {
            auto it = nodeToJointMap.find(childNodeIndex);
            if (it != nodeToJointMap.end()) {
                m_joints[i].children.push_back(it->second);
            }
        }
        bool isRoot = true;
        for (size_t j = 0; j < m_joints.size(); ++j) {
            if (i == j) continue;
            int parentNodeIndex = skin.joints[j];
            const tinygltf::Node& parentNode = model.nodes[parentNodeIndex];
            for (int childNodeIndex : parentNode.children) {
                if (childNodeIndex == nodeIndex) {
                    isRoot = false;
                    break;
                }
            }
            if (!isRoot) break;
        }
        if (isRoot) {
            m_rootJointIndices.push_back(static_cast<int>(i));
        }
    }

    // now load the animations
    m_animationCount = model.animations.size();

    for (unsigned int i = 0; i < m_animationCount; i++)
    {
        Animation a;
        a.LoadFromGltf(model, nodeToJointMap, i);
        m_animations.push_back(a);
    }

    if (m_animationCount > 0) {
        m_animGraph.PlayAnimation(0);
        m_animGraph.PlayAnimation(2, 10);
    }
    m_isLoaded = true;
    return true;
}

void Skeleton::Update(float deltaTime)
{
    m_lifetime += deltaTime;

    // Load the root transform we found during loading.
    DirectX::XMMATRIX rootTransform = DirectX::XMLoadFloat4x4(&m_rootTransform);

    if (m_animGraph.root != nullptr) {
        m_animGraph.Update(deltaTime);
        for (auto& pair : ApplyAnimNode(m_animGraph.root)) {
            Joint& joint = m_joints[pair.first];
            XMStoreFloat4x4(&joint.finalTransform, pair.second.Matrix());
        }
    }

    UpdateJointGlobalTransform(0, XMMatrixIdentity());
 

    for (size_t i = 0; i < m_joints.size(); ++i) {
        XMMATRIX inv = XMLoadFloat4x4(&m_joints[i].inverseBindMatrix);
        
        XMMATRIX finalTransform = XMLoadFloat4x4(&m_joints[i].finalTransform);
        XMMATRIX out = inv * finalTransform;
        XMStoreFloat4x4(&m_skinningMatrices[i], out);
    }
}

ComponentTransform Skeleton::GetLocalAnimatedTransformForJoint(
    int jointIndex,
    const Animation* animation, // Use a pointer to allow for nullptr
    float timeInSeconds
)
{
    const Joint& joint = m_joints[jointIndex];
    // If there is no animation, return the joint's default local transform.
    ComponentTransform out;

    // Decompose the joint's local bind transform into its default TRS.
    // These will be our starting values, which we'll overwrite if a channel animates them.
    DirectX::XMVECTOR scale, rotation, translation;
    DirectX::XMMatrixDecompose(&scale, &rotation, &translation, DirectX::XMLoadFloat4x4(&joint.localBindTransform));

    if (!animation)
    {
        XMStoreFloat3(&out.scale, scale);
        XMStoreFloat4(&out.rotation, rotation);
        XMStoreFloat3(&out.translation, translation);
        // Assumes localBindTransform is now an XMFLOAT4X4
        return out;
    }

    // Iterate through all animation channels to find ones that affect this joint
    for (const auto& channel : animation->m_channels)
    {
        if (channel.jointIndex != jointIndex) {
            continue;
        }

        const AnimationSampler& sampler = animation->m_samplers[channel.samplerIndex];

        // Find the two keyframes to interpolate between
        size_t prevFrame = 0;
        // Use std::upper_bound for a fast search (assumes timestamps are sorted)
        auto it = std::upper_bound(sampler.timestamps.begin(), sampler.timestamps.end(), timeInSeconds);
        if (it != sampler.timestamps.begin()) {
            prevFrame = std::distance(sampler.timestamps.begin(), it) - 1;
        }

        // Ensure we don't go past the end of the animation
        size_t nextFrame = std::min(prevFrame + 1, sampler.timestamps.size() - 1);

        // Get the interpolation factor (t), handling division by zero
        float frameDuration = sampler.timestamps[nextFrame] - sampler.timestamps[prevFrame];
        float t = (frameDuration > 0.0f) ? ((timeInSeconds - sampler.timestamps[prevFrame]) / frameDuration) : 0.0f;

        // Apply the interpolated value based on the channel's path
        if (channel.path == AnimationChannel::TRANSLATION) {
            DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&sampler.vec3_values[prevFrame]);
            DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&sampler.vec3_values[nextFrame]);
            translation = DirectX::XMVectorLerp(v1, v2, t);
        }
        else if (channel.path == AnimationChannel::ROTATION) {
            DirectX::XMVECTOR q1 = DirectX::XMLoadFloat4(&sampler.vec4_values[prevFrame]);
            DirectX::XMVECTOR q2 = DirectX::XMLoadFloat4(&sampler.vec4_values[nextFrame]);

            // Ensure we take the shortest path for rotation
            if (XMVectorGetX(XMVector4Dot(q1, q2)) < 0.0f) {
                q2 = XMVectorNegate(q2);
            }
            
            rotation = DirectX::XMQuaternionSlerp(q1, q2, t);
        }
        else if (channel.path == AnimationChannel::SCALE) {
            DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&sampler.vec3_values[prevFrame]);
            DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&sampler.vec3_values[nextFrame]);
            scale = DirectX::XMVectorLerp(v1, v2, t);
        }
    }

    XMStoreFloat3(&out.scale, scale);
    XMStoreFloat4(&out.rotation, rotation);
    XMStoreFloat3(&out.translation, translation);

    // Re-compose the final local matrix from the (potentially animated) TRS values
    return out;
}

std::map<int, ComponentTransform> Skeleton::ApplyAnimNode(std::shared_ptr<BaseAnimNode> node) {
    if (node->IsBlendNode()) {
        auto blendNode = std::static_pointer_cast<BlendNode>(node);
        std::map<int, ComponentTransform> out = {};

        auto transform1 = ApplyAnimNode(blendNode->nodes.first);
        auto transform2 = ApplyAnimNode(blendNode->nodes.second);
        for (auto& pair : transform1) {
            Joint& joint = m_joints[pair.first];
            XMVECTOR scale = XMVectorLerp(transform1[pair.first].Scale(), transform2[pair.first].Scale(), blendNode->weight);

            XMVECTOR q1 = transform1[pair.first].Rotation();
            XMVECTOR q2 = transform2[pair.first].Rotation();

            if (XMVectorGetX(XMVector4Dot(q1, q2)) < 0.0f) {
                q2 = XMVectorNegate(q2);
            }
            XMVECTOR rotation = XMQuaternionSlerp(q1, q2, blendNode->weight);
            XMVECTOR translation = XMVectorLerp(transform1[pair.first].Translation(), transform2[pair.first].Translation(), blendNode->weight);
            out[pair.first] = {};
            XMStoreFloat3(&out[pair.first].scale, scale);
            XMStoreFloat4(&out[pair.first].rotation, rotation);
            XMStoreFloat3(&out[pair.first].translation, translation);
        }
        return out;
    }
    else {
        auto animNode = std::static_pointer_cast<AnimNode>(node);
        Animation& anim = m_animations[animNode->anim_idx];
        float animation_time = std::fmod(m_lifetime, anim.GetEndTime() - anim.GetStartTime()) + anim.GetStartTime();
        std::map<int, ComponentTransform> out = {};

        for (int jointIndex = 0; jointIndex < m_joints.size(); ++jointIndex) {
            out[jointIndex] = GetLocalAnimatedTransformForJoint(jointIndex, &anim, animation_time);
        }
        return out;
    }
}

void Skeleton::UpdateJointGlobalTransform(int jointIndex, const DirectX::XMMATRIX& parentTransform)
{
    // run through all of these in a flat way, then update
    Joint& currentJoint = m_joints[jointIndex];

    DirectX::XMMATRIX finalTransform = XMLoadFloat4x4(&currentJoint.finalTransform) * parentTransform;

    XMStoreFloat4x4(&currentJoint.finalTransform, finalTransform);

    for (int childIndex : currentJoint.children) {
        UpdateJointGlobalTransform(childIndex, finalTransform);
    }
}

void Skeleton::PlayAnimation(int index, float transitionTime) {
    m_animGraph.PlayAnimation(index, transitionTime);
}

