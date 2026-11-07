
/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** UI document visibility and widget state
*/

#include "ui/UiManager.h"

#include <stdexcept>
#include <utility>

namespace {
std::string valueKey(std::string_view key)
{
    return std::string(key);
}
} // namespace

void RType::UI::UiManager::collectText(const UiElement &element,
    const std::string &key, Values &values,
    TextLimits &)
{
    values.insert_or_assign(key, element.text);
}

void RType::UI::UiManager::collectInputText(const UiElement &element,
    const std::string &key, Values &values,
    TextLimits &textLimits)
{
    values.insert_or_assign(key, element.text);
    textLimits.insert_or_assign(key, element.maxLength);
}

void RType::UI::UiManager::collectCheckbox(const UiElement &element,
    const std::string &key, Values &values,
    TextLimits &)
{
    values.insert_or_assign(key, element.checked);
}

void RType::UI::UiManager::collectSlider(const UiElement &element,
    const std::string &key, Values &values,
    TextLimits &)
{
    values.insert_or_assign(key, element.value);
}

void RType::UI::UiManager::load(
    const std::string &name, const std::filesystem::path &file)
{
    if (name.empty())
        throw std::invalid_argument("UI document name cannot be empty");
    auto loaded = UiLoader::loadFile(file);
    std::map<std::string, WidgetValue> values;
    std::map<std::string, std::size_t> textLimits;
    collectValues(loaded.elements, name, values, textLimits);
    const auto oldPrefix = name + "/";
    for (auto it = _values.begin(); it != _values.end();) {
        if (it->first.starts_with(oldPrefix))
            it = _values.erase(it);
        else
            ++it;
    }
    for (auto it = _textLimits.begin(); it != _textLimits.end();) {
        if (it->first.starts_with(oldPrefix))
            it = _textLimits.erase(it);
        else
            ++it;
    }
    _values.insert(values.begin(), values.end());
    _textLimits.insert(textLimits.begin(), textLimits.end());
    _documents.insert_or_assign(name, std::move(loaded));
    _visibility.try_emplace(name, false);
}

void RType::UI::UiManager::show(const std::string &name)
{
    _visibility.at(name) = true;
}

void RType::UI::UiManager::hide(const std::string &name)
{
    _visibility.at(name) = false;
}

bool RType::UI::UiManager::isVisible(const std::string &name) const
{
    return _visibility.at(name);
}

const RType::UI::UiDocument &RType::UI::UiManager::document(
    const std::string &name) const
{
    return _documents.at(name);
}

const std::map<std::string, RType::UI::UiDocument> &
RType::UI::UiManager::documents() const
{
    return _documents;
}

std::string RType::UI::UiManager::getText(std::string_view key) const
{
    return std::get<std::string>(getValue(key));
}

void RType::UI::UiManager::setText(std::string_view key, std::string value)
{
    const auto textLimit = _textLimits.find(valueKey(key));
    if (textLimit != _textLimits.end() && value.size() > textLimit->second)
        value.resize(textLimit->second);
    setValue(key, std::move(value));
}

RType::UI::UiManager::WidgetValue RType::UI::UiManager::getValue(
    std::string_view key) const
{
    return _values.at(valueKey(key));
}

void RType::UI::UiManager::setValue(std::string_view key, WidgetValue value)
{
    auto &current = _values.at(valueKey(key));
    if (current.index() != value.index())
        throw std::invalid_argument("UI widget value has the wrong type");
    current = std::move(value);
}

void RType::UI::UiManager::onClick(std::string key, ClickHandler handler)
{
    if (!handler)
        throw std::invalid_argument("UI click handler cannot be empty");
    _clickHandlers.insert_or_assign(std::move(key), std::move(handler));
}

void RType::UI::UiManager::click(std::string_view key)
{
    const auto clickedKey = valueKey(key);
    _clickedEvents.push_back(clickedKey);
    const auto exactHandler = _clickHandlers.find(clickedKey);
    ClickHandler handler;
    if (exactHandler != _clickHandlers.end()) {
        handler = exactHandler->second;
    } else {
        const auto separator = clickedKey.find_last_of('/');
        const auto id = separator == std::string::npos
            ? clickedKey
            : clickedKey.substr(separator + 1);
        const auto idHandler = _clickHandlers.find(id);
        if (idHandler != _clickHandlers.end())
            handler = idHandler->second;
    }
    if (handler)
        handler();
}

void RType::UI::UiManager::clearClickedEvents()
{
    _clickedEvents.clear();
}

std::vector<std::string> RType::UI::UiManager::takeClickedEvents()
{
    std::vector<std::string> events;
    events.swap(_clickedEvents);
    return events;
}

void RType::UI::UiManager::collectValues(const std::vector<UiElement> &elements,
    const std::string &prefix, std::map<std::string, WidgetValue> &values,
    std::map<std::string, std::size_t> &textLimits)
{
    for (const auto &element : elements) {
        if (element.type == UiElementType::Include) {
            collectValues(element.children,
                prefix + "/" + element.includeName,
                values,
                textLimits);
            continue;
        }
        const auto collector = valueCollectors.find(element.type);
        if (collector != valueCollectors.end())
            collector->second(
                element, prefix + "/" + element.id, values, textLimits);
        collectValues(element.children, prefix, values, textLimits);
    }
}
