/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ConnectionDialog header
*/

#ifndef GAME_CONNECTIONDIALOG_H
#define GAME_CONNECTIONDIALOG_H

#include <string>
#include "network/NetworkClient.h"

namespace Game {

    /**
     * @brief ImGui UI dialog for entering server details and managing server connection.
     */
    class ConnectionDialog {
    public:
        ConnectionDialog();
        ~ConnectionDialog() = default;

        /**
         * @brief Renders the dedicated connection screen (full screen centered lobby).
         * @param client The NetworkClient instance.
         * @param windowWidth The width of the game window.
         * @param windowHeight The height of the game window.
         */
        void renderScreen(NetworkClient &client, float windowWidth, float windowHeight);

        /**
         * @brief Renders an in-game pause/disconnect overlay menu when Esc is pressed.
         * @param client The NetworkClient instance.
         * @param windowWidth The width of the game window.
         * @param windowHeight The height of the game window.
         */
        void renderInGameOverlay(NetworkClient &client, float windowWidth, float windowHeight);

        /**
         * @brief Renders the ImGui connection window and handles user actions.
         * @param client The NetworkClient instance to connect/disconnect with.
         */
        void render(NetworkClient &client);

        [[nodiscard]] bool isVisible() const;
        void setVisible(bool visible);
        void toggleVisible();

    private:
        bool _visible = true;
        std::string _hostBuffer;
        int _port = 4242;
        std::string _playerNameBuffer;
    };

} // namespace Game

#endif // GAME_CONNECTIONDIALOG_H
