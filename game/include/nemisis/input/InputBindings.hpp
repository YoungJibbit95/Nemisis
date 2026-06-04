#pragma once

#include "novacore/platform/InputAction.hpp"

#include <cstdint>
#include <string_view>

namespace nemisis::input {

namespace actions {

inline constexpr std::string_view MoveForward = "move_forward";
inline constexpr std::string_view MoveBackward = "move_backward";
inline constexpr std::string_view MoveLeft = "move_left";
inline constexpr std::string_view MoveRight = "move_right";
inline constexpr std::string_view Jump = "jump";
inline constexpr std::string_view DoubleJump = "double_jump";
inline constexpr std::string_view Dash = "dash";
inline constexpr std::string_view Slide = "slide";
inline constexpr std::string_view Sprint = "sprint";
inline constexpr std::string_view TacticalSprint = "tactical_sprint";
inline constexpr std::string_view Mantle = "mantle";
inline constexpr std::string_view Fire = "fire";
inline constexpr std::string_view Ads = "ads";
inline constexpr std::string_view Reload = "reload";

} // namespace actions

namespace key_codes {

inline constexpr std::uint16_t W = 87;
inline constexpr std::uint16_t A = 65;
inline constexpr std::uint16_t S = 83;
inline constexpr std::uint16_t D = 68;
inline constexpr std::uint16_t R = 82;
inline constexpr std::uint16_t C = 67;
inline constexpr std::uint16_t Space = 32;
inline constexpr std::uint16_t LeftShift = 160;
inline constexpr std::uint16_t LeftAlt = 164;

} // namespace key_codes

namespace mouse_codes {

inline constexpr std::uint16_t Left = 1;
inline constexpr std::uint16_t Right = 2;

} // namespace mouse_codes

namespace gamepad_buttons {

inline constexpr std::uint16_t A = 0;
inline constexpr std::uint16_t B = 1;
inline constexpr std::uint16_t X = 2;
inline constexpr std::uint16_t LeftStick = 8;

} // namespace gamepad_buttons

namespace gamepad_axes {

inline constexpr std::uint16_t LeftX = 0;
inline constexpr std::uint16_t LeftY = 1;
inline constexpr std::uint16_t LeftTrigger = 4;
inline constexpr std::uint16_t RightTrigger = 5;

} // namespace gamepad_axes

[[nodiscard]] novacore::platform::InputActionMap createDefaultActionMap();

} // namespace nemisis::input
