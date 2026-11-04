/*
** EPITECH PROJECT, 2026
** RType
** File description:
** input manager
*/

#ifndef INPUTMANAGER_H_
#define INPUTMANAGER_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "NlohmanReader.h"
#include "components/Action.h"

class InputManager {
public:
    using KeyCode = int;

    InputManager() = default;
    ~InputManager() = default;

    /**
     * @brief Names an action so config files can refer to it.
     * @param name Name used in the "bindings" object of a config file.
     * @param action Bit of the action in the mask.
     * @throws std::out_of_range if action is not below MAX_ACTIONS.
     */
    void defineAction(const std::string &name, RType::ActionId action);

    /**
     * @brief Loads action-to-input bindings from a JSON config file.
     * @param configFile JSON file with a "bindings" object keyed by action
     * names; each value is an array of integer key codes or named inputs.
     */
    void loadConfigFile(const std::filesystem::path &configFile);

    /**
     * @brief Binds a key to an action.
     * @param action Action triggered by the key.
     * @param key Key to bind.
     * @note An action can have several keys (e.g. arrows and WASD).
     *       Binding the same key twice to the same action has no effect.
     */
    void bind(RType::ActionId action, KeyCode key);

    /**
     * @brief Binds a named device input to an action.
     * @param input Input identifier, such as "KEYBOARD_A" or "MOUSE_LEFT".
     */
    void bind(RType::ActionId action, const std::string &input);

    /**
     * @brief Removes one key from an action.
     * @param action Action to modify.
     * @param key Key to unbind.
     * @note Does nothing if the key was not bound to this action.
     */
    void unbind(RType::ActionId action, KeyCode key);

    /**
     * @brief Removes one named device input from an action.
     */
    void unbind(RType::ActionId action, std::string_view input);

    /**
     * @brief Removes every key bound to an action.
     * @param action Action to clear.
     * @note Typically used before rebinding an action to a new key.
     */
    void clearBindings(RType::ActionId action);

    /**
     * @brief Records that a key was pressed or released.
     * @param key Key whose state changed.
     * @param pressed true if the key is now down, false if released.
     * @note Called by the client's window/event layer, not by game logic.
     */
    void setKeyState(KeyCode key, bool pressed);

    /**
     * @brief Records that a named device input was pressed or released.
     */
    void setInputState(std::string_view input, bool pressed);

    /**
     * @brief Checks whether an action is currently active.
     * @param action Action to check.
     * @return true if at least one key bound to the action is down.
     */
    bool isActionDown(RType::ActionId action) const;

    /**
     * @brief Packs all active actions into a 32-bit mask.
     * @return Bitmask where bit N is set if the action with value N is active.
     * @note Compact form meant for the network protocol: the client sends this
     *       byte, and the server decodes it without knowing about keyboards.
     */
    RType::ActionMask actionMask() const;

    static std::string keyCodeId(InputManager::KeyCode key);

private:
    using InputId = std::string;
    std::array<std::vector<InputId>, RType::MAX_ACTIONS> _bindings;
    std::map<std::string, RType::ActionId, std::less<>> _actionNames;
    std::unordered_set<InputId> _pressed;
    RType::nlohamReader _reader;
};

#endif
