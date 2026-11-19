#include "AnimGraph.h"


void AnimGraph::PlayAnimation(const unsigned int animation, float transition) {
	AnimInstance* node = new AnimInstance(animation);
	TransitionTo(node, transition);
}

void AnimGraph::PlayMixedAnimation(const unsigned int anim1, const unsigned int anim2, float weight, float transition) {
	BlendNode* node = new BlendNode(new AnimInstance(anim1), new AnimInstance(anim2));
	node->weight = weight;
	TransitionTo(node, transition);
}

void AnimGraph::TransitionTo(AnimNode* node, float transition) {

	if (transition <= 0 || root == nullptr) {
		if (root != nullptr)
			delete root;
		root = node;
	}
	else {
		BlendNode* blend = new BlendNode(root, node, transition);
		root = new BlendNode(root, node, transition);
	}
}