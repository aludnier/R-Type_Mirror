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
#include <unordered_set>
#include <vector>

enum class Action : std::uint8_t
{
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Shoot,
    Count
};

class InputManager
{
public:
    using KeyCode = int;

    InputManager() = default;
    ~InputManager() = default;

    /**
     * @brief Binds a key to an action.
     * @param action Action triggered by the key.
     * @param key Key to bind.
     * @note An action can have several keys (e.g. arrows and WASD).
     *       Binding the same key twice to the same action has no effect.
     */
    void bind(Action action, KeyCode key);

    /**
     * @brief Removes one key from an action.
     * @param action Action to modify.
     * @param key Key to unbind.
     * @note Does nothing if the key was not bound to this action.
     */
    void unbind(Action action, KeyCode key);

    /**
     * @brief Removes every key bound to an action.
     * @param action Action to clear.
     * @note Typically used before rebinding an action to a new key.
     */
    void clearBindings(Action action);

    /**
     * @brief Records that a key was pressed or released.
     * @param key Key whose state changed.
     * @param pressed true if the key is now down, false if released.
     * @note Called by the client's window/event layer, not by game logic.
     */
    void setKeyState(KeyCode key, bool pressed);

    /**
     * @brief Checks whether an action is currently active.
     * @param action Action to check.
     * @return true if at least one key bound to the action is down.
     */
    bool isActionDown(Action action) const;

    /**
     * @brief Packs all active actions into a single byte.
     * @return Bitmask where bit N is set if the action with value N is active.
     * @note Compact form meant for the network protocol: the client sends this
     *       byte, and the server decodes it without knowing about keyboards.
     */
    std::uint8_t actionMask() const;

private:
    std::size_t actionCount = static_cast<std::size_t>(Action::Count);

    std::vector<std::vector<KeyCode>> _bindings;
    std::unordered_set<KeyCode> _pressed;
};

#endif
