#pragma once

#include <memory>
#include <iostream>

struct AnimNode
{
	virtual bool IsBranch() { std::cout << "Hold up anim node referenced?"; return false; };
	virtual void Update(float delta) {};
	
	virtual ~AnimNode() = default;

};

struct AnimInstance : public AnimNode
{
	unsigned int anim_idx;
	AnimInstance(const unsigned int animation) : anim_idx(animation){
	};
};

struct BlendNode : public AnimNode {
	std::shared_ptr<AnimNode> from = nullptr;
	std::shared_ptr<AnimNode> to = nullptr;
	float weight = 0;
	bool paused;
	float time_factor = 1;

	bool IsBranch() override {
		return true;
	};

	BlendNode(AnimNode* from, AnimNode* to, float duration = -1) : from(from), to(to) {
		paused = duration > 0;
		if (duration > 0)
			time_factor = 1 / duration;
	}

	void Update(float delta) override {
		if (paused)
			return;
		//weight ;
		from->Update(delta);

		//if (from->IsBranch()) {
		//	if (std::static_pointer_cast<BlendNode>(from)->weight)
		//}

		to->Update(delta);
		
	}

};

class AnimGraph
{
public:
	BlendNode* PlayMixedAnimation(const unsigned int anim1, const unsigned int anim2, float weight = 0.5, float transition_time = 0);
	AnimInstance* PlayAnimation(const unsigned int animation, float transition_time = 0);
	void Update(float delta) { root->Update(delta); };
	std::shared_ptr<AnimNode> root = nullptr;
private:
	void TransitionTo(AnimNode* node, float transition);

};