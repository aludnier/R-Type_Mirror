/*
** EPITECH PROJECT, 2026
** r-type
** File description:
** Keys.h
*/

#ifndef KEYS_H_
#define KEYS_H_

/** @brief Key codes used with InputManager. Values match GLFW. */
namespace RType::Key {
inline constexpr int Space = 32;
inline constexpr int Num0 = 48;
inline constexpr int Num9 = 57;
inline constexpr int A = 65;
inline constexpr int D = 68;
inline constexpr int S = 83;
inline constexpr int W = 87;
inline constexpr int Z = 90;
inline constexpr int Escape = 256;
inline constexpr int Enter = 257;
inline constexpr int Tab = 258;
inline constexpr int Backspace = 259;
inline constexpr int Delete = 261;
inline constexpr int Right = 262;
inline constexpr int Left = 263;
inline constexpr int Down = 264;
inline constexpr int Up = 265;
inline constexpr int F1 = 290;
inline constexpr int F12 = 301;
inline constexpr int LeftShift = 340;
inline constexpr int LeftControl = 341;
inline constexpr int LeftAlt = 342;
} // namespace RType::Key

#endif /* !KEYS_H_ */
