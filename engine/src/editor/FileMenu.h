/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** FileMenu.h
*/

#ifndef FILEMENU_H_
#define FILEMENU_H_

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

#include "editor/EditorContext.h"

namespace RType::Editor {
/**
 * @brief The File menu: New, Open, Save and Save As, with Ctrl+N/O/S. Paths
 *        are typed in a popup.
 */
class FileMenu {
public:
    /** @brief Draws the menu. Call it inside the main menu bar. */
    void drawMenu(EditorContext &context);
    /** @brief Draws the Open and Save As popups, outside the menu bar. */
    void drawPopups(EditorContext &context);

private:
    enum class Dialog { None, Open, SaveAs };

    void save(EditorContext &context);
    void request(Dialog dialog, const EditorContext &context);
    using Action = std::function<std::optional<std::string>(
        const std::filesystem::path &)>;

    /** @brief Runs @p action on the typed path, keeps the popup on error. */
    void drawPathPopup(
        const char *title, const char *button, const Action &action);

    Dialog _requested = Dialog::None;
    std::string _path = "scene.json";
    std::string _error;
};
} // namespace RType::Editor

#endif /* !FILEMENU_H_ */
