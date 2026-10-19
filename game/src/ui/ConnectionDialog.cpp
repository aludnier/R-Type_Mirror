/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ConnectionDialog implementation
*/

#include "ui/ConnectionDialog.h"
#include <imgui.h>

namespace Game {

    ConnectionDialog::ConnectionDialog() : _hostBuffer("127.0.0.1"), _playerNameBuffer("Player1")
    {
    }

    void ConnectionDialog::renderScreen(NetworkClient &client, float windowWidth, float windowHeight)
    {
        constexpr ImVec2 dialogSize(500.0f, 440.0f);
        ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f, windowHeight * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(dialogSize, ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("R-Type - Server Connection", nullptr, flags)) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));
            ImGui::SetWindowFontScale(1.4f);
            ImGui::Text("    === R-TYPE MULTIPLAYER ===");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();

            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Enter server details below to enter the battle:");
            ImGui::Separator();
            ImGui::Spacing();

            const bool isConnecting = (client.getStatus() == NetworkClient::ConnectionStatus::Connecting);

            if (isConnecting) {
                ImGui::BeginDisabled();
            }

            ImGui::Text("Server Host / IP:");
            ImGui::SetNextItemWidth(480.0f);
            ImGui::InputText("##ServerAddress", _hostBuffer.data(), _hostBuffer.size());

            ImGui::Spacing();
            ImGui::Text("Server Port:");
            ImGui::SetNextItemWidth(480.0f);
            ImGui::InputInt("##ServerPort", &_port);
            if (_port < 1) _port = 1;
            if (_port > 65535) _port = 65535;

            ImGui::Spacing();
            ImGui::Text("Pilot Name:");
            ImGui::SetNextItemWidth(480.0f);
            ImGui::InputText("##PlayerName", _playerNameBuffer.data(), _playerNameBuffer.size());

            if (isConnecting) {
                ImGui::EndDisabled();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (isConnecting) {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Connecting to %s:%d...", _hostBuffer.data(), _port);
                ImGui::Spacing();
                if (ImGui::Button("Cancel Connection", ImVec2(200.0f, 40.0f))) {
                    client.disconnect();
                }
            } else {
                if (client.getStatus() == NetworkClient::ConnectionStatus::ConnectionFailed) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Connection Failed!");
                    if (!client.getLastError().empty()) {
                        ImGui::TextWrapped("Error: %s", client.getLastError().c_str());
                    }
                } else if (client.getStatus() == NetworkClient::ConnectionStatus::DisconnectedByServer) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Disconnected from server.");
                    if (!client.getLastError().empty()) {
                        ImGui::TextWrapped("Reason: %s", client.getLastError().c_str());
                    }
                }

                ImGui::Spacing();
                if (ImGui::Button("CONNECT TO SERVER", ImVec2(480.0f, 45.0f))) {
                    client.connect(_hostBuffer, static_cast<std::uint16_t>(_port), _playerNameBuffer);
                }
            }
        }
        ImGui::End();
    }

    void ConnectionDialog::renderInGameOverlay(NetworkClient &client, float windowWidth, float windowHeight)
    {
        if (!_visible) {
            return;
        }

        constexpr ImVec2 dialogSize(420.0f, 320.0f);
        ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f, windowHeight * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(dialogSize, ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("Game Menu", &_visible, flags)) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Multiplayer Session Info");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Client ID: %u", static_cast<unsigned int>(client.getClientId()));
            ImGui::Text("Linked Entity: %u", static_cast<unsigned int>(client.getLinkedEntity()));
            ImGui::Text("Server Tick: %u (Rate: %u Hz)", static_cast<unsigned int>(client.getServerTick()), static_cast<unsigned int>(client.getTickRate()));
            ImGui::Text("Ping: %u ms", static_cast<unsigned int>(client.getPingMs()));

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Resume Game", ImVec2(390.0f, 35.0f))) {
                _visible = false;
            }

            ImGui::Spacing();
            if (ImGui::Button("Disconnect & Return to Lobby", ImVec2(390.0f, 35.0f))) {
                client.disconnect();
                _visible = false;
            }

            ImGui::Spacing();
            ImGui::TextDisabled("Press Escape to resume play.");
        }
        ImGui::End();
    }

    void ConnectionDialog::render(NetworkClient &client)
    {
        renderScreen(client, 1920.0f, 1080.0f);
    }

    bool ConnectionDialog::isVisible() const
    {
        return _visible;
    }

    void ConnectionDialog::setVisible(bool visible)
    {
        _visible = visible;
    }

    void ConnectionDialog::toggleVisible()
    {
        _visible = !_visible;
    }

} // namespace Game
