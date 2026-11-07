/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ImGui renderer for UI documents
*/

#include "ui/ImGuiUiRenderer.h"

#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include <exception>
#include <iostream>
#include <string>
#include <utility>

#include "library/AssetManager.h"

namespace {
float extent(float value, float parent)
{
    return value >= 0.0f && value <= 1.0f ? value * parent : value;
}

std::string widgetKey(const std::string &prefix, const std::string &id)
{
    return prefix + "/" + id;
}

ImVec2 position(const RType::UI::UiRect &rect, ImVec2 origin, ImVec2 size)
{
    return {
        origin.x + extent(rect.x, size.x), origin.y + extent(rect.y, size.y)};
}

ImVec2 dimensions(const RType::UI::UiRect &rect, ImVec2 parentSize)
{
    return {
        extent(rect.width, parentSize.x), extent(rect.height, parentSize.y)};
}

std::string displayText(const RType::UI::UiElement &element)
{
    return element.text.empty() ? element.id : element.text;
}
} // namespace

void RType::UI::ImGuiUiRenderer::render(UiManager &manager,
    const AssetManager &assets, const sf::RenderWindow &window)
{
    manager.clearClickedEvents();
    const auto windowSize = window.getSize();
    const ImVec2 size(
        static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    for (const auto &[name, document] : manager.documents()) {
        if (!manager.isVisible(name))
            continue;
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(size);
        const auto flags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoBackground;
        const std::string windowId = "##ui_" + name;
        ImGui::Begin(windowId.c_str(), nullptr, flags);
        ImGui::BringWindowToDisplayBack(ImGui::GetCurrentWindow());
        renderElements(document.elements,
            name,
            ImGui::GetWindowPos(),
            size,
            manager,
            assets);
        ImGui::End();
    }
    ImGui::PopStyleVar();

    std::vector<std::string> pendingClicks;
    pendingClicks.swap(_pendingClicks);
    for (const auto &key : pendingClicks) {
        try {
            manager.click(key);
        } catch (const std::exception &error) {
            std::cerr << "[UI] Click handler for '" << key
                      << "' failed: " << error.what() << std::endl;
        } catch (...) {
            std::cerr << "[UI] Click handler for '" << key
                      << "' failed with an unknown error" << std::endl;
        }
    }
}

void RType::UI::ImGuiUiRenderer::renderElements(
    const std::vector<UiElement> &elements, const std::string &prefix,
    ImVec2 origin, ImVec2 parentSize, UiManager &manager,
    const AssetManager &assets)
{
    for (const auto &element : elements) {
        const std::string key = element.type == UiElementType::Include
            ? prefix + "/" + element.includeName
            : widgetKey(prefix, element.id);
        const ImVec2 itemPosition = position(element.rect, origin, parentSize);
        const ImVec2 itemSize = dimensions(element.rect, parentSize);
        ImGui::SetCursorScreenPos(itemPosition);
        ImGui::PushID(key.c_str());
        try {
            const RenderContext context {
                key, prefix, itemSize, manager, assets};
            (this->*elementRenderers.at(element.type))(element, context);
        } catch (const std::exception &error) {
            if (_reportedErrors.insert(key).second)
                std::cerr << "[UI] Failed to render '" << key
                          << "': " << error.what() << std::endl;
        } catch (...) {
            if (_reportedErrors.insert(key).second)
                std::cerr << "[UI] Failed to render '" << key
                          << "' with an unknown error" << std::endl;
        }
        ImGui::PopID();
    }
}

void RType::UI::ImGuiUiRenderer::renderContainer(
    const UiElement &element, const RenderContext &context)
{
    const bool isInclude = element.type == UiElementType::Include;
    if (ImGui::BeginChild(
            isInclude ? "##included" : "##panel", context.size, !isInclude)) {
        const auto childPosition = ImGui::GetWindowPos();
        const auto childSize = ImGui::GetWindowSize();
        renderElements(element.children,
            isInclude ? context.key : context.prefix,
            childPosition,
            childSize,
            context.manager,
            context.assets);
    }
    ImGui::EndChild();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void RType::UI::ImGuiUiRenderer::renderText(
    const UiElement &, const RenderContext &context)
{
    const auto text = context.manager.getText(context.key);
    ImGui::TextUnformatted(text.c_str());
}

void RType::UI::ImGuiUiRenderer::renderImage(
    const UiElement &element, const RenderContext &context)
{
    const std::string assetKey = "asset:" + element.asset;
    if (!element.asset.empty() && _missingImages.contains(assetKey))
        return;
    std::string imagePath = element.path;
    if (!element.asset.empty()) {
        try {
            const auto &asset = context.assets.getAsset(element.asset);
            if (asset.type != AssetManager::AssetType::Texture) {
                if (_missingImages.insert(assetKey).second)
                    std::cerr << "[UI] UI image asset '" << element.asset
                              << "' is not a texture" << std::endl;
                return;
            }
            imagePath = asset.path;
        } catch (const std::exception &error) {
            if (_missingImages.insert(assetKey).second)
                std::cerr << "[UI] Failed to load UI image asset '"
                          << element.asset << "': " << error.what()
                          << std::endl;
            return;
        }
    }
    const std::string pathKey = "path:" + imagePath;
    if (_missingImages.contains(pathKey))
        return;
    auto texture = _textures.find(imagePath);
    if (texture == _textures.end()) {
        sf::Texture loaded;
        try {
            if (!loaded.loadFromFile(imagePath)) {
                if (_missingImages.insert(pathKey).second)
                    std::cerr << "[UI] Failed to load UI image '" << imagePath
                              << "' for '" << context.key << "'" << std::endl;
                return;
            }
        } catch (const std::exception &error) {
            if (_missingImages.insert(pathKey).second)
                std::cerr << "[UI] Failed to load UI image '" << imagePath
                          << "' for '" << context.key << "': " << error.what()
                          << std::endl;
            return;
        } catch (...) {
            if (_missingImages.insert(pathKey).second)
                std::cerr << "[UI] Failed to load UI image '" << imagePath
                          << "' for '" << context.key
                          << "' with an unknown error" << std::endl;
            return;
        }
        texture = _textures.emplace(imagePath, std::move(loaded)).first;
    }
    ImGui::Image(texture->second, sf::Vector2f(context.size.x, context.size.y));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void RType::UI::ImGuiUiRenderer::renderButton(
    const UiElement &element, const RenderContext &context)
{
    if (ImGui::Button(displayText(element).c_str(), context.size))
        _pendingClicks.push_back(context.key);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void RType::UI::ImGuiUiRenderer::renderInputText(
    const UiElement &element, const RenderContext &context)
{
    auto value = context.manager.getText(context.key);
    const std::string label = "##" + element.id;
    const bool changed = element.hint.empty()
        ? ImGui::InputText(label.c_str(), &value)
        : ImGui::InputTextWithHint(label.c_str(), element.hint.c_str(), &value);
    if (value.size() > element.maxLength)
        value.resize(element.maxLength);
    if (changed || value != context.manager.getText(context.key))
        context.manager.setText(context.key, std::move(value));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void RType::UI::ImGuiUiRenderer::renderCheckbox(
    const UiElement &element, const RenderContext &context)
{
    auto value = std::get<bool>(context.manager.getValue(context.key));
    if (ImGui::Checkbox(displayText(element).c_str(), &value))
        context.manager.setValue(context.key, value);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void RType::UI::ImGuiUiRenderer::renderSlider(
    const UiElement &element, const RenderContext &context)
{
    auto value = std::get<float>(context.manager.getValue(context.key));
    if (ImGui::SliderFloat(displayText(element).c_str(),
            &value,
            element.minimum,
            element.maximum))
        context.manager.setValue(context.key, value);
}
