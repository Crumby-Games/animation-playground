#include "AnimGraph.h"


AnimInstance* AnimGraph::PlayAnimation(const unsigned int animation, float transition) {
	AnimInstance* node = new AnimInstance(animation);
	TransitionTo(node, transition);
	return node;
}

BlendNode* AnimGraph::PlayMixedAnimation(const unsigned int anim1, const unsigned int anim2, float weight, float transition) {
	BlendNode* node = new BlendNode(new AnimInstance(anim1), new AnimInstance(anim2));
	node->weight = weight;
	TransitionTo(node, transition);
	return node;
}

void AnimGraph::TransitionTo(AnimNode* node, float transition) {
	if (transition <= 0)
		root = std::shared_ptr<AnimNode>(node);
	else {
		BlendNode* blend = new BlendNode(root.get(), node, transition);
		root = std::shared_ptr<AnimNode>(blend);
	}
}