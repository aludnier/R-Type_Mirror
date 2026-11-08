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
    static bool addClip(
        Animator &animator, std::string name, AnimationClip clip);
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

// Defined here, not in the module: a game that only handles Animator and
// AnimationClip uses them without linking the module's shared library.
inline AnimationClip AnimationSystem::createClip(
    std::vector<AnimationFrame> frames, float frameDuration)
{
    return AnimationClip {
        .frames = std::move(frames), .frameDuration = frameDuration};
}

inline AnimationClip AnimationSystem::createSpriteSheetClip(
    AnimationFrame region, int frameWidth, int frameHeight,
    std::size_t frameCount, float frameDuration)
{
    return AnimationClip {.frameDuration = frameDuration,
        .sheetRegion = region,
        .frameWidth = frameWidth,
        .frameHeight = frameHeight,
        .sheetFrameCount = frameCount};
}

inline bool AnimationSystem::addClip(
    Animator &animator, std::string name, AnimationClip clip)
{
    if (name.empty() || !isValid(clip)) {
        return false;
    }
    animator.clips.insert_or_assign(std::move(name), std::move(clip));
    return true;
}

inline bool AnimationSystem::play(
    Animator &animator, const std::string &name, bool loop, bool restart)
{
    const auto clip = animator.clips.find(name);
    if (clip == animator.clips.end()) {
        return false;
    }
    const bool changed = animator.currentClip != clip->first;
    animator.currentClip = clip->first;
    animator.loop = loop;
    if (changed || restart) {
        animator.frameIndex = 0;
        animator.timer = 0.0f;
    }
    return true;
}

inline std::size_t AnimationSystem::frameCount(const AnimationClip &clip)
{
    return clip.frames.empty() ? sheetFrameCount(clip) : clip.frames.size();
}

inline std::optional<AnimationFrame> AnimationSystem::frameAt(
    const AnimationClip &clip, std::size_t index)
{
    if (!clip.frames.empty()) {
        return index < clip.frames.size() ? std::optional {clip.frames[index]}
                                          : std::nullopt;
    }
    if (index >= sheetFrameCount(clip)) {
        return std::nullopt;
    }
    const auto columns = clip.sheetRegion.width / clip.frameWidth;
    return AnimationFrame {clip.sheetRegion.left +
            static_cast<int>(index % columns) * clip.frameWidth,
        clip.sheetRegion.top +
            static_cast<int>(index / columns) * clip.frameHeight,
        clip.frameWidth,
        clip.frameHeight};
}

inline bool AnimationSystem::isValid(const AnimationClip &clip)
{
    if (!std::isfinite(clip.frameDuration) || clip.frameDuration <= 0.0f ||
        frameCount(clip) == 0) {
        return false;
    }
    return std::ranges::all_of(clip.frames, [](const AnimationFrame &frame) {
        return frame.width > 0 && frame.height > 0;
    });
}

inline void AnimationSystem::advance(
    Animator &animator, Sprite &sprite, float dt)
{
    updateOne(animator, sprite, dt);
}

inline void AnimationSystem::update(SparseArray<Animator> &animators,
    SparseArray<Sprite> &sprites, SparseArray<AnimationSettings> &settings)
{
    const auto dt = deltaTime(settings);
    for (auto [animator, sprite] : Zipper(animators, sprites)) {
        updateOne(animator, sprite, dt);
    }
}

inline void AnimationSystem::addSystem(Registry &registry)
{
    registry.addSystem<Animator, Sprite, AnimationSettings>(
        [](auto &animators, auto &sprites, auto &settings) {
            AnimationSystem::update(animators, sprites, settings);
        });
}

inline std::size_t AnimationSystem::sheetFrameCount(const AnimationClip &clip)
{
    if (clip.frameWidth <= 0 || clip.frameHeight <= 0 ||
        clip.sheetRegion.width <= 0 || clip.sheetRegion.height <= 0) {
        return 0;
    }
    const auto columns =
        static_cast<std::size_t>(clip.sheetRegion.width / clip.frameWidth);
    const auto rows =
        static_cast<std::size_t>(clip.sheetRegion.height / clip.frameHeight);
    return std::min(clip.sheetFrameCount, columns * rows);
}

inline float AnimationSystem::deltaTime(
    SparseArray<AnimationSettings> &settings)
{
    for (const auto &setting : settings) {
        if (setting) {
            return setting->dt;
        }
    }
    return AnimationSettings {}.dt;
}

inline void AnimationSystem::setSpriteFrame(
    Sprite &sprite, const AnimationFrame &frame)
{
    sprite.rectLeft = frame.left;
    sprite.rectTop = frame.top;
    sprite.rectWidth = frame.width;
    sprite.rectHeight = frame.height;
}

inline void AnimationSystem::advanceNonLooping(Animator &animator,
    double elapsed, double steps, float duration, std::size_t count)
{
    const auto remaining = count - animator.frameIndex - 1;
    if (steps >= static_cast<double>(remaining)) {
        animator.frameIndex = count - 1;
        animator.timer = 0.0f;
        return;
    }
    animator.frameIndex += static_cast<std::size_t>(steps);
    animator.timer = static_cast<float>(std::fmod(elapsed, duration));
}

inline void AnimationSystem::advanceFrame(
    Animator &animator, const AnimationClip &clip, std::size_t count, float dt)
{
    if (!std::isfinite(dt) || dt <= 0.0f || !std::isfinite(animator.speed) ||
        animator.speed <= 0.0f) {
        return;
    }
    const double elapsed =
        animator.timer + static_cast<double>(dt) * animator.speed;
    if (!std::isfinite(elapsed)) {
        animator.timer = 0.0f;
        return;
    }
    const double steps = std::floor(elapsed / clip.frameDuration);
    if (steps < 1.0) {
        animator.timer = static_cast<float>(elapsed);
        return;
    }
    if (animator.loop) {
        const auto advance = static_cast<std::size_t>(
            std::fmod(steps, static_cast<double>(count)));
        animator.frameIndex = (animator.frameIndex + advance) % count;
        animator.timer =
            static_cast<float>(std::fmod(elapsed, clip.frameDuration));
        return;
    }
    advanceNonLooping(animator, elapsed, steps, clip.frameDuration, count);
}

inline void AnimationSystem::updateOne(
    Animator &animator, Sprite &sprite, float dt)
{
    const auto clip = animator.clips.find(animator.currentClip);
    if (clip == animator.clips.end() || !isValid(clip->second)) {
        return;
    }
    const auto count = frameCount(clip->second);
    if (animator.frameIndex >= count) {
        animator.frameIndex = 0;
        animator.timer = 0.0f;
    }
    if (animator.loop || animator.frameIndex + 1 < count) {
        advanceFrame(animator, clip->second, count, dt);
    }
    if (const auto frame = frameAt(clip->second, animator.frameIndex)) {
        setSpriteFrame(sprite, *frame);
    }
}
} // namespace RType::Animation

#endif // ANIMATION2D_ANIMATIONSYSTEM_H
