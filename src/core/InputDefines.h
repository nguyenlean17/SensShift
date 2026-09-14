#pragma once

#include <windows.h>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>

// Magic identifier placed in MOUSEINPUT::dwExtraInfo so our hook distinguishes
// injected events from physical hardware input and avoids infinite feedback loops.
constexpr ULONG_PTR SENS_INJECTED_MAGIC = 0x53454E53ULL; // "SENS" in ASCII

enum class MouseButton : uint8_t {
    None = 0,
    Left,
    Right,
    Middle,
    XButton1, // Mouse 4
    XButton2  // Mouse 5
};

enum ModifierFlags : uint32_t {
    ModNone  = 0,
    ModShift = 1 << 0,
    ModCtrl  = 1 << 1,
    ModAlt   = 1 << 2,
    ModWin   = 1 << 3
};

enum class ActivationMode : uint8_t {
    Hold = 0,        // Active only while held
    Toggle,          // Press once to activate, press again to deactivate
    HoldOverride     // While held, temporarily overrides other active profiles
};

enum class TransformationOrder : uint8_t {
    SensThenAccel = 0, // Sensitivity scaling first, then acceleration curve
    AccelThenSens      // Acceleration curve first based on raw velocity, then sensitivity scaling
};

enum class CurveType : uint8_t {
    Linear = 0,
    Polynomial,
    Exponential,
    Power,
    CustomLUT
};

struct Hotkey {
    uint32_t vkCode = 0;                  // Virtual key code (0 if pure mouse button)
    MouseButton mouseButton = MouseButton::None; // Mouse button if bound to mouse
    uint32_t modifiers = ModNone;         // Combination of ModifierFlags

    bool IsEmpty() const {
        return vkCode == 0 && mouseButton == MouseButton::None;
    }

    bool operator==(const Hotkey& other) const {
        return vkCode == other.vkCode &&
               mouseButton == other.mouseButton &&
               modifiers == other.modifiers;
    }

    bool operator!=(const Hotkey& other) const {
        return !(*this == other);
    }

    std::string ToString() const {
        if (IsEmpty()) {
            return "None";
        }
        std::string result;
        if (modifiers & ModCtrl)  result += "Ctrl + ";
        if (modifiers & ModAlt)   result += "Alt + ";
        if (modifiers & ModShift) result += "Shift + ";
        if (modifiers & ModWin)   result += "Win + ";

        if (mouseButton != MouseButton::None) {
            switch (mouseButton) {
                case MouseButton::Left:     result += "LMB"; break;
                case MouseButton::Right:    result += "RMB"; break;
                case MouseButton::Middle:   result += "MMB"; break;
                case MouseButton::XButton1: result += "Mouse 4"; break;
                case MouseButton::XButton2: result += "Mouse 5"; break;
                default: result += "Mouse"; break;
            }
        } else if (vkCode != 0) {
            switch (vkCode) {
                case VK_SPACE: result += "Space"; break;
                case VK_CAPITAL: result += "Caps Lock"; break;
                case VK_TAB: result += "Tab"; break;
                case VK_RETURN: result += "Enter"; break;
                case VK_ESCAPE: result += "Esc"; break;
                case VK_BACK: result += "Backspace"; break;
                case VK_DELETE: result += "Delete"; break;
                case VK_INSERT: result += "Insert"; break;
                case VK_HOME: result += "Home"; break;
                case VK_END: result += "End"; break;
                case VK_PRIOR: result += "Page Up"; break;
                case VK_NEXT: result += "Page Down"; break;
                case VK_UP: result += "Up Arrow"; break;
                case VK_DOWN: result += "Down Arrow"; break;
                case VK_LEFT: result += "Left Arrow"; break;
                case VK_RIGHT: result += "Right Arrow"; break;
                case VK_SHIFT:
                case VK_LSHIFT:
                case VK_RSHIFT:
                    if (!(modifiers & ModShift)) result += "Shift";
                    else if (!result.empty() && result.ends_with(" + ")) result.resize(result.size() - 3);
                    break;
                case VK_CONTROL:
                case VK_LCONTROL:
                case VK_RCONTROL:
                    if (!(modifiers & ModCtrl)) result += "Ctrl";
                    else if (!result.empty() && result.ends_with(" + ")) result.resize(result.size() - 3);
                    break;
                case VK_MENU:
                case VK_LMENU:
                case VK_RMENU:
                    if (!(modifiers & ModAlt)) result += "Alt";
                    else if (!result.empty() && result.ends_with(" + ")) result.resize(result.size() - 3);
                    break;
                default: {
                    if (vkCode >= VK_F1 && vkCode <= VK_F24) {
                        result += "F" + std::to_string(vkCode - VK_F1 + 1);
                    } else if ((vkCode >= 'A' && vkCode <= 'Z') || (vkCode >= '0' && vkCode <= '9')) {
                        result += static_cast<char>(vkCode);
                    } else {
                        char name[64] = {0};
                        UINT scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
                        if (GetKeyNameTextA(scanCode << 16, name, sizeof(name)) > 0) {
                            result += name;
                        } else {
                            result += "Key 0x" + std::to_string(vkCode);
                        }
                    }
                    break;
                }
            }
        }
        return result;
    }
};

// Emergency Killswitch Hotkey: Ctrl + Shift + F12
inline bool IsEmergencyFailsafeCombo(uint32_t vkCode, uint32_t modifiers) {
    return vkCode == VK_F12 && (modifiers & ModCtrl) && (modifiers & ModShift);
}
