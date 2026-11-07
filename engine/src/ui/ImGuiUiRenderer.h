/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ImGui renderer for UI documents
*/

#ifndef IMGUIUIRENDERER_H_
#define IMGUIUIRENDERER_H_

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ui/UiDocument.h"
#include "ui/UiManager.h"

class AssetManager;

namespace RType::UI {

class ImGuiUiRenderer {
public:
    void render(UiManager &manager, const AssetManager &assets,
        const sf::RenderWindow &window);

private:
    struct RenderContext {
        const std::string &key;
        const std::string &prefix;
        ImVec2 size;
        UiManager &manager;
        const AssetManager &assets;
    };

    using ElementRenderer = void (ImGuiUiRenderer::*)(
        const UiElement &, const RenderContext &);

    void renderElements(const std::vector<UiElement> &elements,
        const std::string &prefix, ImVec2 origin, ImVec2 parentSize,
        UiManager &manager, const AssetManager &assets);
    void renderContainer(
        const UiElement &element, const RenderContext &context);
    void renderText(const UiElement &element, const RenderContext &context);
    void renderImage(const UiElement &element, const RenderContext &context);
    void renderButton(const UiElement &element, const RenderContext &context);
    void renderInputText(
        const UiElement &element, const RenderContext &context);
    void renderCheckbox(const UiElement &element, const RenderContext &context);
    void renderSlider(const UiElement &element, const RenderContext &context);

    inline static const std::unordered_map<UiElementType, ElementRenderer>
        elementRenderers {
            {UiElementType::Panel, &ImGuiUiRenderer::renderContainer},
            {UiElementType::Include, &ImGuiUiRenderer::renderContainer},
            {UiElementType::Text, &ImGuiUiRenderer::renderText},
            {UiElementType::Image, &ImGuiUiRenderer::renderImage},
            {UiElementType::Button, &ImGuiUiRenderer::renderButton},
            {UiElementType::InputText, &ImGuiUiRenderer::renderInputText},
            {UiElementType::Checkbox, &ImGuiUiRenderer::renderCheckbox},
            {UiElementType::Slider, &ImGuiUiRenderer::renderSlider},
        };

    std::unordered_map<std::string, sf::Texture> _textures;
    std::unordered_set<std::string> _missingImages;
    std::unordered_set<std::string> _reportedErrors;
};

} // namespace RType::UI

#endif
