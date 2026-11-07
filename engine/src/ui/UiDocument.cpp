/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** UI document parsing and include resolution
*/

#include "ui/UiDocument.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

namespace {
using Json = nlohmann::json;
using RType::UI::UiDocument;
using RType::UI::UiElement;
using RType::UI::UiElementType;

std::filesystem::path normalizePath(const std::filesystem::path &path)
{
    std::error_code error;
    const auto absolute = std::filesystem::absolute(path, error);
    if (error)
        return path.lexically_normal();
    const auto canonical = std::filesystem::weakly_canonical(absolute, error);
    return error ? absolute.lexically_normal() : canonical;
}

std::string requiredString(const Json &object, const char *key)
{
    if (!object.contains(key) || !object.at(key).is_string())
        throw std::invalid_argument(
            std::string("expected string field '") + key + "'");
    return object.at(key).get<std::string>();
}

float number(const Json &value, const char *description)
{
    if (!value.is_number())
        throw std::invalid_argument(
            std::string(description) + " must be a number");
    const float result = value.get<float>();
    if (!std::isfinite(result))
        throw std::invalid_argument(
            std::string(description) + " must be finite");
    return result;
}

RType::UI::UiRect parseRect(const Json &object)
{
    if (!object.contains("rect") || !object.at("rect").is_array() ||
        object.at("rect").size() != 4)
        throw std::invalid_argument("field 'rect' must contain four numbers");
    const auto &rect = object.at("rect");
    RType::UI::UiRect result {
        number(rect.at(0), "rect x"),
        number(rect.at(1), "rect y"),
        number(rect.at(2), "rect width"),
        number(rect.at(3), "rect height"),
    };
    if (result.width < 0.0f || result.height < 0.0f)
        throw std::invalid_argument("rect width and height cannot be negative");
    return result;
}

} // namespace

namespace RType::UI {

UiElementType UiLoader::parseType(const std::string &type)
{
    const auto found = elementTypes.find(type);
    if (found == elementTypes.end())
        throw std::invalid_argument("unknown UI element type '" + type + "'");
    return found->second;
}

void UiLoader::parseWidgetCommon(const Json &json, UiElement &element)
{
    element.id = requiredString(json, "id");
    if (element.id.empty())
        throw std::invalid_argument("field 'id' cannot be empty");
    if (json.contains("text")) {
        if (!json.at("text").is_string())
            throw std::invalid_argument("field 'text' must be a string");
        element.text = json.at("text").get<std::string>();
    }
}

void UiLoader::parseInclude(const Json &json, UiElement &element,
    const std::filesystem::path &file, LoadingStack &loading)
{
    element.includeName = requiredString(json, "name");
    if (element.includeName.empty())
        throw std::invalid_argument("include field 'name' cannot be empty");
    const auto includeFile =
        std::filesystem::path(requiredString(json, "file"));
    if (includeFile.empty())
        throw std::invalid_argument("include field 'file' cannot be empty");
    const auto childPath = includeFile.is_absolute()
        ? includeFile
        : file.parent_path() / includeFile;
    element.children = loadRecursive(childPath, loading).elements;
}

void UiLoader::parsePanel(const Json &json, UiElement &element,
    const std::filesystem::path &file, LoadingStack &loading)
{
    if (json.contains("children"))
        element.children = parseElements(json.at("children"), file, loading);
}

void UiLoader::parseText(const Json &json, UiElement &,
    const std::filesystem::path &, LoadingStack &)
{
    if (!json.contains("text"))
        throw std::invalid_argument("text element requires 'text'");
}

void UiLoader::parseImage(const Json &json, UiElement &element,
    const std::filesystem::path &file, LoadingStack &)
{
    const bool hasAsset = json.contains("asset");
    const bool hasPath = json.contains("path");
    if (hasAsset == hasPath)
        throw std::invalid_argument(
            "image element requires exactly one of 'asset' or 'path'");
    if (hasAsset) {
        element.asset = requiredString(json, "asset");
        return;
    }
    const auto imagePath = requiredString(json, "path");
    if (imagePath.empty())
        throw std::invalid_argument("image field 'path' cannot be empty");
    element.path = (file.parent_path() / imagePath).lexically_normal().string();
}

void UiLoader::parseButton(
    const Json &, UiElement &, const std::filesystem::path &, LoadingStack &)
{
}

void UiLoader::parseInputText(const Json &json, UiElement &element,
    const std::filesystem::path &, LoadingStack &)
{
    if (json.contains("hint"))
        element.hint = requiredString(json, "hint");
    if (json.contains("max_length")) {
        if (!json.at("max_length").is_number_unsigned() &&
            !json.at("max_length").is_number_integer())
            throw std::invalid_argument(
                "field 'max_length' must be a positive integer");
        const auto maxLength = json.at("max_length").get<long long>();
        if (maxLength <= 0)
            throw std::invalid_argument(
                "field 'max_length' must be a positive integer");
        element.maxLength = static_cast<std::size_t>(maxLength);
    }
    if (json.contains("value"))
        element.text = requiredString(json, "value");
    if (element.text.size() > element.maxLength)
        element.text.resize(element.maxLength);
}

void UiLoader::parseCheckbox(const Json &json, UiElement &element,
    const std::filesystem::path &, LoadingStack &)
{
    if (!json.contains("checked"))
        return;
    if (!json.at("checked").is_boolean())
        throw std::invalid_argument("field 'checked' must be boolean");
    element.checked = json.at("checked").get<bool>();
}

void UiLoader::parseSlider(const Json &json, UiElement &element,
    const std::filesystem::path &, LoadingStack &)
{
    element.minimum = number(json.at("min"), "slider min");
    element.maximum = number(json.at("max"), "slider max");
    if (element.minimum >= element.maximum)
        throw std::invalid_argument("slider min must be less than max");
    element.value = json.contains("value")
        ? number(json.at("value"), "slider value")
        : element.minimum;
    if (element.value < element.minimum || element.value > element.maximum)
        throw std::invalid_argument(
            "slider value must be within its min and max");
}

std::vector<UiElement> UiLoader::parseElements(const Json &elements,
    const std::filesystem::path &file, LoadingStack &loading)
{
    if (!elements.is_array())
        throw std::invalid_argument("field 'elements' must be an array");
    std::vector<UiElement> result;
    result.reserve(elements.size());
    for (const auto &json : elements) {
        if (!json.is_object())
            throw std::invalid_argument("each UI element must be an object");
        UiElement element;
        element.type = parseType(requiredString(json, "type"));
        element.rect = parseRect(json);
        if (element.type != UiElementType::Include)
            parseWidgetCommon(json, element);
        elementParsers.at(element.type)(json, element, file, loading);
        result.push_back(std::move(element));
    }
    return result;
}

UiDocument UiLoader::loadRecursive(
    const std::filesystem::path &file, LoadingStack &loading)
{
    const auto normalized = normalizePath(file);
    if (std::find(loading.begin(), loading.end(), normalized) != loading.end())
        throw std::runtime_error("include cycle detected");
    loading.push_back(normalized);
    try {
        std::ifstream input(normalized);
        if (!input)
            throw std::runtime_error("cannot open file");
        const Json json = Json::parse(input);
        if (!json.is_object())
            throw std::invalid_argument("UI document must be an object");
        UiDocument document;
        document.name = requiredString(json, "name");
        if (document.name.empty())
            throw std::invalid_argument("field 'name' cannot be empty");
        document.source = normalized;
        if (!json.contains("elements"))
            throw std::invalid_argument("missing field 'elements'");
        document.elements =
            parseElements(json.at("elements"), normalized, loading);
        loading.pop_back();
        return document;
    } catch (const std::exception &error) {
        loading.pop_back();
        throw std::runtime_error(
            "UI file '" + normalized.string() + "': " + error.what());
    }
}

UiDocument UiLoader::loadFile(const std::filesystem::path &file)
{
    LoadingStack loading;
    return loadRecursive(file, loading);
}

} // namespace RType::UI
