/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** InputPanel.h
*/

#ifndef INPUTPANEL_H_
#define INPUTPANEL_H_

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>

#include "editor/EditorContext.h"

namespace RType::Editor {
/**
 * @brief Edits the actions of the game and the keys bound to them: add,
 *        rename and remove actions, bind the next key pressed, unbind keys,
 *        and save them to the input config file.
 */
class InputPanel {
public:
    static constexpr const char *TITLE = "Input";

    void draw(EditorContext &context);

    /**
     * @brief Defines an action on a free bit.
     * @return An error to show, or nothing on success.
     */
    static std::optional<std::string> addAction(
        EditorContext &context, const std::string &name);

    /**
     * @brief Renames an action, keeping its bit and its keys.
     * @return An error to show, or nothing on success.
     */
    static std::optional<std::string> renameAction(
        EditorContext &context, const std::string &from, const std::string &to);

    /**
     * @brief Saves the actions to @p file, which becomes the input file.
     * @return An error to show, or nothing on success.
     */
    static std::optional<std::string> save(
        EditorContext &context, const std::filesystem::path &file);

    /**
     * @return The first input of @p now that is not in @p before, in sorted
     *         order, or nothing if no new input is down.
     */
    [[nodiscard]] static std::optional<std::string> newInput(
        const std::unordered_set<std::string> &before,
        const std::unordered_set<std::string> &now);

    /** @return A readable name for an input id, such as "Space" or "F5". */
    [[nodiscard]] static std::string inputLabel(const std::string &input);

private:
    enum class Popup { None, AddAction, RenameAction, SaveAs };

    void drawToolbar(EditorContext &context);
    void drawActions(EditorContext &context);
    void captureInput(EditorContext &context);
    void drawPopups(EditorContext &context);
    void request(Popup popup, std::string text);

    /** @brief The action waiting for a key, and the keys down before. */
    std::optional<std::string> _capturing;
    std::unordered_set<std::string> _pressedBefore;

    Popup _requested = Popup::None;
    std::string _renaming;
    std::string _text;
    std::string _error;
};
} // namespace RType::Editor

#endif /* !INPUTPANEL_H_ */
