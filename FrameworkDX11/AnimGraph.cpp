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

	if (transition <= 0 || root.get()) {
		root.reset(node);
	}
	else {
		root.reset(new BlendNode(root.get(), node, transition));
	}
}