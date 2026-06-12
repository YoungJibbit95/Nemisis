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
inline constexpr std::string_view LookRight = "look_right";
inline constexpr std::string_view LookUp = "look_up";
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
inline constexpr std::string_view MenuUp = "menu_up";
inline constexpr std::string_view MenuDown = "menu_down";
inline constexpr std::string_view MenuLeft = "menu_left";
inline constexpr std::string_view MenuRight = "menu_right";
inline constexpr std::string_view MenuPrevTab = "menu_prev_tab";
inline constexpr std::string_view MenuNextTab = "menu_next_tab";
inline constexpr std::string_view MenuConfirm = "menu_confirm";
inline constexpr std::string_view MenuBack = "menu_back";
inline constexpr std::string_view ToggleDebug = "toggle_debug";
inline constexpr std::string_view DebugNextPage = "debug_next_page";
inline constexpr std::string_view SelectDevRange = "select_dev_range";
inline constexpr std::string_view SelectTdm = "select_tdm";
inline constexpr std::string_view SelectControl = "select_control";
inline constexpr std::string_view SelectMainMenu = "select_main_menu";

} // namespace actions

namespace key_codes {

inline constexpr std::uint16_t W = 87;
inline constexpr std::uint16_t A = 65;
inline constexpr std::uint16_t E = 69;
inline constexpr std::uint16_t Q = 81;
inline constexpr std::uint16_t S = 83;
inline constexpr std::uint16_t D = 68;
inline constexpr std::uint16_t R = 82;
inline constexpr std::uint16_t C = 67;
inline constexpr std::uint16_t Space = 32;
inline constexpr std::uint16_t LeftShift = 160;
inline constexpr std::uint16_t LeftAlt = 164;
inline constexpr std::uint16_t Enter = 13;
inline constexpr std::uint16_t Escape = 27;
inline constexpr std::uint16_t Tab = 9;
inline constexpr std::uint16_t Digit1 = 49;
inline constexpr std::uint16_t Digit2 = 50;
inline constexpr std::uint16_t Digit3 = 51;
inline constexpr std::uint16_t F1 = 112;
inline constexpr std::uint16_t Up = 1000;
inline constexpr std::uint16_t Down = 1001;
inline constexpr std::uint16_t Left = 1002;
inline constexpr std::uint16_t Right = 1003;

} // namespace key_codes

namespace mouse_codes {

inline constexpr std::uint16_t Left = 1;
inline constexpr std::uint16_t Right = 2;

} // namespace mouse_codes

namespace mouse_axes {

inline constexpr std::uint16_t X = 0;
inline constexpr std::uint16_t Y = 1;

} // namespace mouse_axes

namespace gamepad_buttons {

inline constexpr std::uint16_t A = 0;
inline constexpr std::uint16_t B = 1;
inline constexpr std::uint16_t X = 2;
inline constexpr std::uint16_t LeftShoulder = 4;
inline constexpr std::uint16_t RightShoulder = 5;
inline constexpr std::uint16_t Start = 6;
inline constexpr std::uint16_t LeftStick = 8;
inline constexpr std::uint16_t DPadUp = 11;
inline constexpr std::uint16_t DPadDown = 12;
inline constexpr std::uint16_t DPadLeft = 13;
inline constexpr std::uint16_t DPadRight = 14;

} // namespace gamepad_buttons

namespace gamepad_axes {

inline constexpr std::uint16_t LeftX = 0;
inline constexpr std::uint16_t LeftY = 1;
inline constexpr std::uint16_t RightX = 2;
inline constexpr std::uint16_t RightY = 3;
inline constexpr std::uint16_t LeftTrigger = 4;
inline constexpr std::uint16_t RightTrigger = 5;

} // namespace gamepad_axes

[[nodiscard]] novacore::platform::InputActionMap createDefaultActionMap();

} // namespace nemisis::input
