/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** R-Type game application
*/

#ifndef GAME_GAMEAPP_H
#define GAME_GAMEAPP_H

#include <cstdint>
#include <imgui-SFML.h>
#include <memory>
#include <optional>
#include <string>

#include "components/ParallaxBackground.h"
#include "game/GameApp.h"
#include "network/NetworkClient.h"
#include "scripting/ScriptEngine.h"
#include "systems/NetworkSyncSystem.h"
#include "systems/RenderSystem.h"
#include "systems/ShipTiltSystem.h"
#include "ui/ImGuiUiRenderer.h"
#include "ui/UiManager.h"

namespace Game {

enum class ScreenState { Connection, Game };
using ParallaxBackground = RType::Environment::ParallaxBackground;

class GameApp : public RType::Game::GameApp {
public:
    explicit GameApp(const RType::Configuration &configuration);

    ~GameApp() override = default;

    [[nodiscard]] ScreenState getScreenState() const;
    void setScreenState(ScreenState state);
    [[nodiscard]] NetworkClient &getNetworkClient();
    [[nodiscard]] RType::UI::UiManager &getUiManager();

protected:
    void registerComponents() override;
    void setupKeyBindings() override;
    void setupResources() override;
    void setupSystems() override;
    void spawnInitialEntities() override;
    void updateGame(float dt) override;
    void renderGame() override;
    void onWindowCreated() override;
    void onShutdown() override;
    void handleEvent(const sf::Event &event) override;

private:
    void renderHud();
    /** @brief Health bars of the hit enemies, and of the boss at the top. */
    void renderHealthBars();
    static void drawBar(sf::RenderWindow &window, sf::Vector2f position,
        sf::Vector2f size, float ratio);
    /** @brief Leaves and joins the same server again, with a new ship. */
    void playAgain();
    /** @brief Shows the game over screen and the LEVEL / BOSS banner. */
    void updateEndOfGameUi();
    /** @brief Bakes every printable glyph once, on the first frame. */
    void warmUpFont();
    /** @brief Shows the energy in front of the local ship while it charges. */
    void updateChargeEffect();
    /** @brief Creates the window again with the size or mode chosen. */
    void createWindow();
    void initImGui();
    /** @brief Recreates the window between two frames if asked. */
    void applyWindowChange();
    /** @brief Letterboxed view of the 1920x1080 world, for any window. */
    void updateView();
    void changeVolume(RType::AudioChannel channel, float step);
    /** @brief Shows the window size and volumes in the Escape menu. */
    void updateSettingsText();
    void setupUi();
    void handleUiClicks();
    void updateConnectionStatus();
    void updateInGameOverlay();

    void handleKeyPressed(const sf::Event::KeyPressed &keyPressed);
    void handleKeyReleased(const sf::Event::KeyReleased &keyReleased);

    bool _inGameOverlayVisible = false;
    /** @brief "LEVEL n" or "BOSS", shown for _bannerTime more seconds. */
    std::string _bannerText;
    float _bannerTime = 0.f;
    /** @brief Server of the last Connect, for Play again. */
    std::string _lastHost;
    std::uint16_t _lastPort = 0;
    std::string _lastPlayerName;
    bool _reconnect = false;
    bool _fontWarmedUp = false;
    std::optional<RType::Entity> _chargeEffect;
    std::size_t _windowSize = 0;
    bool _fullscreen = false;
    bool _windowChanged = false;
    bool _muted = false;
    std::optional<sf::Cursor> _arrowCursor;

    NetworkClient _networkClient {};
    NetworkSyncSystem _networkSyncSystem {};
    ShipTiltSystem _shipTilt {};
    /** @brief Kept between frames: it caches the textures it loaded. */
    RType::RenderSystem _renderSystem {};
    RType::UI::UiManager _uiManager {};
    RType::UI::ImGuiUiRenderer _uiRenderer {};
    ScreenState _screenState = ScreenState::Connection;
    std::unique_ptr<RType::ScriptEngine> _scripts;
};

} // namespace Game

#endif // GAME_GAMEAPP_H
