/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** UI document model and loader
*/

#ifndef UIDOCUMENT_H_
#define UIDOCUMENT_H_

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json_fwd.hpp>

namespace RType::UI {

struct UiRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

enum class UiElementType {
    Panel,
    Text,
    Image,
    Button,
    InputText,
    Checkbox,
    Slider,
    Include,
};

struct UiElement {
    UiElementType type = UiElementType::Text;
    std::string id;
    UiRect rect;
    std::string text;
    std::string asset;
    std::string path;
    std::string hint;
    std::string includeName;
    std::size_t maxLength = 256;
    bool checked = false;
    float minimum = 0.0f;
    float maximum = 1.0f;
    float value = 0.0f;
    std::vector<UiElement> children;
};

struct UiDocument {
    std::string name;
    std::filesystem::path source;
    std::vector<UiElement> elements;
};

class UiLoader {
public:
    [[nodiscard]] static UiDocument loadFile(const std::filesystem::path &file);

private:
    using Json = nlohmann::json;
    using LoadingStack = std::vector<std::filesystem::path>;
    using ElementParser = void (*)(const Json &, UiElement &,
        const std::filesystem::path &, LoadingStack &);

    static UiElementType parseType(const std::string &type);
    static UiDocument loadRecursive(
        const std::filesystem::path &file, LoadingStack &loading);
    static std::vector<UiElement> parseElements(const Json &elements,
        const std::filesystem::path &file, LoadingStack &loading);
    static void parseWidgetCommon(const Json &json, UiElement &element);
    static void parseInclude(const Json &json, UiElement &element,
        const std::filesystem::path &file, LoadingStack &loading);
    static void parsePanel(const Json &json, UiElement &element,
        const std::filesystem::path &file, LoadingStack &loading);
    static void parseText(const Json &json, UiElement &element,
        const std::filesystem::path &file, LoadingStack &loading);
    static void parseImage(const Json &json, UiElement &element,
        const std::filesystem::path &file, LoadingStack &loading);
    static void parseButton(const Json &json, UiElement &element,
        const std::filesystem::path &file, LoadingStack &loading);
    static void parseInputText(const Json &json, UiElement &element,
        const std::filesystem::path &file, LoadingStack &loading);
    static void parseCheckbox(const Json &json, UiElement &element,
        const std::filesystem::path &file, LoadingStack &loading);
    static void parseSlider(const Json &json, UiElement &element,
        const std::filesystem::path &file, LoadingStack &loading);

    inline static const std::unordered_map<std::string, UiElementType>
        elementTypes {
            {"panel", UiElementType::Panel},
            {"text", UiElementType::Text},
            {"image", UiElementType::Image},
            {"button", UiElementType::Button},
            {"input_text", UiElementType::InputText},
            {"checkbox", UiElementType::Checkbox},
            {"slider", UiElementType::Slider},
            {"include", UiElementType::Include},
        };
    inline static const std::unordered_map<UiElementType, ElementParser>
        elementParsers {
            {UiElementType::Panel, parsePanel},
            {UiElementType::Text, parseText},
            {UiElementType::Image, parseImage},
            {UiElementType::Button, parseButton},
            {UiElementType::InputText, parseInputText},
            {UiElementType::Checkbox, parseCheckbox},
            {UiElementType::Slider, parseSlider},
            {UiElementType::Include, parseInclude},
        };
};

} // namespace RType::UI

#endif
