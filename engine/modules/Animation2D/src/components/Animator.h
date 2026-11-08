#ifndef ANIMATION2D_ANIMATOR_H
#define ANIMATION2D_ANIMATOR_H

#include <string>
#include <unordered_map>

#include "AnimationClip.h"

namespace RType::Animation {

struct Animator {
    std::unordered_map<std::string, AnimationClip> clips;
    std::string currentClip;
    std::size_t frameIndex = 0;
    float timer = 0.0f;
    float speed = 1.0f;
    bool loop = true;
};

} // namespace RType::Animation

#endif // ANIMATION2D_ANIMATOR_H
