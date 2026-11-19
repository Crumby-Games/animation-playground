#pragma once

#include <memory>
#include <iostream>

class AnimNode
{
public:
	virtual bool IsBranch() = 0;
	virtual void Update(float delta) {};
	
	virtual ~AnimNode() = default;
protected:
	AnimNode() {};
};

struct AnimInstance : public AnimNode
{
	unsigned int anim_idx;
	bool IsBranch() override { return false; };
	AnimInstance(const unsigned int animation) : anim_idx(animation) {
	};
};

struct BlendNode : public AnimNode {
	AnimNode* from = nullptr;
	AnimNode* to = nullptr;
	float weight = 0;
	bool fixed = true;
	float time_factor = 1;

	bool IsBranch() override {
		return true;
	};

	BlendNode(AnimNode* fromNode, AnimNode* toNode, float transition = -1) {
		from = fromNode;
		to = toNode;
		if (transition > 0) {
			fixed = false;
			time_factor = 1 / transition;
		}

	}

	~BlendNode() override {
		if (from)
			delete from;
		if (to)
			delete to;
	}

	void Update(float delta) override {
		if (!fixed)
			weight += delta * time_factor;
		from->Update(delta);
		to->Update(delta);
	}

};

class AnimGraph
{
public:
	void PlayMixedAnimation(const unsigned int anim1, const unsigned int anim2, float weight = 0.5, float transition_time = 0);
	void PlayAnimation(const unsigned int animation, float transition_time = 0);
	void Update(float delta) { if (root) root->Update(delta); };
	//~AnimGraph() {
	//	if (root)
	//		delete root;
	//}
	AnimNode* root = nullptr;
private:
	void TransitionTo(AnimNode* node, float transition);

};