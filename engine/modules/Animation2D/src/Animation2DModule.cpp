#include "Animation2DModule.h"

#include "components/AnimationSettings.h"
#include "components/Animator.h"
#include "components/Sprite.h"
#include "systems/AnimationSystem.h"

namespace {

RType::ModuleStatus registerAnimationModule(RType::Registry &registry)
{
    registry.registerComponent<RType::Animation::Animator>();
    registry.registerComponent<RType::Animation::AnimationSettings>();
    registry.registerComponent<RType::Sprite>();
    RType::Animation::AnimationSystem::addSystem(registry);
    return RType::ModuleStatus::Success;
}

} // namespace

extern "C" MODULE_EXPORT const RType::ModuleDescriptor *MODULE_CALL
RTypeLibEntryPoint()
{
    static const RType::ModuleDescriptor descriptor {
        .name = "animation",
        .apiVersion = {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR},
        .dependencies = {},
        .registerModule = registerAnimationModule,
    };
    return &descriptor;
}
