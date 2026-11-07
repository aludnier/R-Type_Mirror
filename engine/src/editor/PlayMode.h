
/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** PlayMode.h
*/

#ifndef PLAYMODE_H_
#define PLAYMODE_H_

#include <optional>

#include <nlohmann/json.hpp>

#include "editor/EditorContext.h"
#include "scripting/ScriptEngine.h"
#include "components/Action.h"

namespace RType::Editor {
/**
 * @brief Runs the scene in the editor, then puts it back as it was.
 *
 * Systems added with addScriptSystem() cannot be removed from a Registry, so
 * the scripts are updated from here instead.
 */
class PlayMode {
public:
    /**
     * @brief Saves the scene, starts the scripts and locks the panels.
     * @note Does nothing without an AssetManager, which the snapshot needs.
     */
    void play(EditorContext &context);
    /**
     * @brief Runs one step of the scene. Does nothing when not playing.
     * @param actions The active actions, given to every Controllable.
     */
    void update(EditorContext &context, float dt, ActionMask actions = 0);
    /** @brief Stops the scripts and restores the scene saved by play(). */
    void stop(EditorContext &context);

    [[nodiscard]] bool isPlaying() const;

private:
    nlohmann::json _snapshot;
    std::optional<ScriptEngine> _scripts;
};
} // namespace RType::Editor

#endif /* !PLAYMODE_H_ */
