#pragma once

#include <memory>

class BaseAnimNode
{
public:
	virtual void Update(float delta) {};
	virtual bool Expired() { return false; }
	virtual std::shared_ptr<BaseAnimNode> Replacement() { return nullptr; }
	virtual bool IsBlendNode() { return false; };
	virtual bool IsTransitionNode() { return false; };
	virtual ~BaseAnimNode() = default;
protected:
	BaseAnimNode() {};
};

struct AnimNode : public BaseAnimNode
{
	unsigned int anim_idx;
	AnimNode(const unsigned int animation) : anim_idx(animation) {
	};
	
};

class BlendNode : public BaseAnimNode {
public:
	std::pair<std::shared_ptr<BaseAnimNode>, std::shared_ptr<BaseAnimNode>> nodes;
	float weight;
	BlendNode(std::shared_ptr<BaseAnimNode> node1, std::shared_ptr<BaseAnimNode> node2, float w) : nodes{ node1, node2 }, weight(w) {}
	bool IsBlendNode() override { return true; }
	void Update(float delta) override;
	std::shared_ptr<AnimNode> RightmostLeaf() {
		if (nodes.second->IsBlendNode()) {
			auto node = std::static_pointer_cast<BlendNode>(nodes.second);
			return node->RightmostLeaf();
		}
		else {
			return std::static_pointer_cast<AnimNode>(nodes.second);
		}
	}
private:
	std::pair<float, float> weights;
};

class TransitionNode : public BlendNode {
public:

	TransitionNode(std::shared_ptr<BaseAnimNode> node1, std::shared_ptr<BaseAnimNode> node2, float time) : BlendNode(node1, node2, 0) {
		time_factor = 1.0f / time;
	}

	bool IsTransitionNode() override { return true; }
	void Update(float delta) override;

	bool Expired() override { return weight >= 1; }
	std::shared_ptr<BaseAnimNode> Replacement() override { return nodes.second; }

private:
	float time_factor = 1;
	bool expired = false;
};



class AnimGraph
{
public:
	void PlayBlendedAnimation(const unsigned int anim1, const unsigned int anim2, float weight = 0.5, float transition_time = 0);
	void PlayAnimation(const unsigned int animation, float transition_time = 0);
	void Update(float delta);
	AnimGraph() {};
	std::shared_ptr<BaseAnimNode> root = nullptr;
private:
	void TransitionTo(std::shared_ptr<BaseAnimNode> node, float transition);

};