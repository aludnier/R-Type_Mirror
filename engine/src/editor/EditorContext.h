/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** EditorContext.h
*/

#ifndef EDITORCONTEXT_H_
#define EDITORCONTEXT_H_

#include <optional>

#include "ecs/Registry.h"

namespace RType::Editor {
/** @brief State shared by every editor panel. */
struct EditorContext {
    Registry &registry;
    /** @brief The entity shown in the inspector, if any. */
    std::optional<Entity> selected;
};
} // namespace RType::Editor

#endif /* !EDITORCONTEXT_H_ */
