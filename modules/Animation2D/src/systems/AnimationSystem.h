#ifndef ANIMATION2D_ANIMATIONSYSTEM_H
#define ANIMATION2D_ANIMATIONSYSTEM_H

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "components/AnimationSettings.h"
#include "components/Animator.h"
#include "components/Sprite.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "ecs/Zipper.h"

namespace RType::Animation {

class AnimationSystem {
public:
    static AnimationClip createClip(
        std::vector<AnimationFrame> frames, float frameDuration);
    static AnimationClip createSpriteSheetClip(AnimationFrame region,
        int frameWidth, int frameHeight, std::size_t frameCount,
        float frameDuration);
    static bool addClip(Animator &animator, std::string name, AnimationClip clip);
    static bool play(Animator &animator, const std::string &name,
        bool loop = true, bool restart = true);
    static std::size_t frameCount(const AnimationClip &clip);
    static std::optional<AnimationFrame> frameAt(
        const AnimationClip &clip, std::size_t index);
    static bool isValid(const AnimationClip &clip);
    static void advance(Animator &animator, Sprite &sprite, float dt);
    static void update(SparseArray<Animator> &animators,
        SparseArray<Sprite> &sprites, SparseArray<AnimationSettings> &settings);
    static void addSystem(Registry &registry);

private:
    static std::size_t sheetFrameCount(const AnimationClip &clip);
    static float deltaTime(SparseArray<AnimationSettings> &settings);
    static void setSpriteFrame(Sprite &sprite, const AnimationFrame &frame);
    static void advanceFrame(Animator &animator, const AnimationClip &clip,
        std::size_t count, float dt);
    static void advanceNonLooping(Animator &animator, double elapsed,
        double steps, float duration, std::size_t count);
    static void updateOne(Animator &animator, Sprite &sprite, float dt);
};

} // namespace RType::Animation

#endif // ANIMATION2D_ANIMATIONSYSTEM_H
