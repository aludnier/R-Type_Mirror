/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** UI document visibility and widget state
*/

#ifndef UIMANAGER_H_
#define UIMANAGER_H_

#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ui/UiDocument.h"

namespace RType::UI {

class UiManager {
public:
    using WidgetValue = std::variant<std::string, bool, float>;
    using ClickHandler = std::function<void()>;

    void load(const std::string &name, const std::filesystem::path &file);
    void show(const std::string &name);
    void hide(const std::string &name);
    [[nodiscard]] bool isVisible(const std::string &name) const;
    [[nodiscard]] const UiDocument &document(const std::string &name) const;
    [[nodiscard]] const std::map<std::string, UiDocument> &documents() const;

    [[nodiscard]] std::string getText(std::string_view key) const;
    void setText(std::string_view key, std::string value);
    [[nodiscard]] WidgetValue getValue(std::string_view key) const;
    void setValue(std::string_view key, WidgetValue value);

    void onClick(std::string key, ClickHandler handler);
    void click(std::string_view key);
    [[nodiscard]] std::vector<std::string> takeClickedEvents();

private:
    using Values = std::map<std::string, WidgetValue>;
    using TextLimits = std::map<std::string, std::size_t>;
    using ValueCollector = void (*)(
        const UiElement &, const std::string &, Values &, TextLimits &);

    static void collectText(const UiElement &element, const std::string &key,
        Values &values, TextLimits &textLimits);
    static void collectInputText(const UiElement &element,
        const std::string &key, Values &values, TextLimits &textLimits);
    static void collectCheckbox(const UiElement &element,
        const std::string &key, Values &values, TextLimits &textLimits);
    static void collectSlider(const UiElement &element, const std::string &key,
        Values &values, TextLimits &textLimits);
    static void collectValues(const std::vector<UiElement> &elements,
        const std::string &prefix, std::map<std::string, WidgetValue> &values,
        std::map<std::string, std::size_t> &textLimits);

    inline static const std::unordered_map<UiElementType, ValueCollector>
        valueCollectors {
            {UiElementType::Text, collectText},
            {UiElementType::InputText, collectInputText},
            {UiElementType::Checkbox, collectCheckbox},
            {UiElementType::Slider, collectSlider},
        };

    std::map<std::string, UiDocument> _documents;
    std::map<std::string, bool> _visibility;
    std::map<std::string, WidgetValue> _values;
    std::map<std::string, std::size_t> _textLimits;
    std::map<std::string, ClickHandler> _clickHandlers;
    std::vector<std::string> _clickedEvents;
};

} // namespace RType::UI

#endif
