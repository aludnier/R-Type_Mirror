#ifndef ANIMATION2D_ANIMATIONCLIP_H
#define ANIMATION2D_ANIMATIONCLIP_H

#include <cstddef>
#include <vector>

namespace RType::Animation {

struct AnimationFrame {
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
};

struct AnimationClip {
    std::vector<AnimationFrame> frames;
    float frameDuration = 0.1f;
    AnimationFrame sheetRegion {};
    int frameWidth = 0;
    int frameHeight = 0;
    std::size_t sheetFrameCount = 0;
};

} // namespace RType::Animation

#endif // ANIMATION2D_ANIMATIONCLIP_H
