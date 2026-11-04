#ifndef TEST_ACTIONS_H
#define TEST_ACTIONS_H

#include "components/Action.h"

/** @brief Actions used by the tests only: the engine defines none. */
namespace Action {
constexpr RType::ActionId MoveUp = 0;
constexpr RType::ActionId MoveDown = 1;
constexpr RType::ActionId MoveLeft = 2;
constexpr RType::ActionId MoveRight = 3;
constexpr RType::ActionId Shoot = 4;
constexpr RType::ActionId Count = 5;
} // namespace Action

#endif
