#include "AnimGraph.h"

void AnimGraph::Update(float delta) {
	if (!root)
		return;
	if (root->Expired()) {
		if (!root->Replacement())
			return;
		root = root->Replacement();
	}
	root->Update(delta);

};

void AnimGraph::PlayAnimation(const unsigned int animation, float transition) {
	if (root && root->IsTransitionNode()) {
		auto node = std::static_pointer_cast<TransitionNode>(root);
		if (node->RightmostLeaf()->anim_idx == animation)
			return;
	}
	auto node = std::make_shared<AnimNode>(animation);
	TransitionTo(node, transition);
}

void AnimGraph::PlayBlendedAnimation(const unsigned int anim1, const unsigned int anim2, float weight, float transition) {
	auto node = std::make_shared<BlendNode>(std::make_shared<AnimNode>(anim1), std::make_shared<AnimNode>(anim2), weight);
	TransitionTo(node, transition);
}

void AnimGraph::TransitionTo(std::shared_ptr<BaseAnimNode> node, float transition) {

	if (transition <= 0 || !root.get() || root->Expired()) {
		root = node;
	}
	else {
		root = std::make_shared<TransitionNode>(root, node, transition);
	}
}

void BlendNode::Update(float delta) {
	nodes.first->Update(delta);
	nodes.second->Update(delta);

	if (nodes.first->Expired()) {
		if (nodes.first->Replacement())
			nodes.first = nodes.first->Replacement();
	}

	if (nodes.second->Expired()) {
		if (nodes.second->Replacement())
			nodes.second = nodes.second->Replacement();
	}
}

void TransitionNode::Update(float delta) {
	weight += delta * time_factor;
	if (weight > 1) {
		weight = 1.0f;
		expired = true;
	}
	BlendNode::Update(delta);
}