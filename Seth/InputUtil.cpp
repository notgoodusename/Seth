#include <algorithm>
#include <array>
#include <string_view>
#include <Windows.h>

#include "imgui/imgui.h"

#include "Hacks/Misc.h"

#include "InputUtil.h"
#include "GUI.h"

#include "SDK/Platform.h"

struct Key {
    template <std::size_t N>
    constexpr Key(const char(&name)[N], int code) : name{ name }, code{ code } {  }

    std::string_view name;
    int code;
};

// indices must match KeyBind::KeyCode enum, and has to be sorted alphabetically
static constexpr auto keyMap = std::to_array<Key>({
    { "'", VK_OEM_7 },
    { ",", VK_OEM_COMMA },
    { "-", VK_OEM_MINUS },
    { ".", VK_OEM_PERIOD },
    { "/", VK_OEM_2 },
    { "0", '0' },
    { "1", '1' },
    { "2", '2' },
    { "3", '3' },
    { "4", '4' },
    { "5", '5' },
    { "6", '6' },
    { "7", '7' },
    { "8", '8' },
    { "9", '9' },
    { ";", VK_OEM_1 },
    { "=", VK_OEM_PLUS },
    { "A", 'A' },
    { "ADD", VK_ADD },
    { "B", 'B' },
    { "BACKSPACE", VK_BACK },
    { "C", 'C' },
    { "CAPSLOCK", VK_CAPITAL },
    { "D", 'D' },
    { "DECIMAL", VK_DECIMAL },
    { "DELETE", VK_DELETE },
    { "DIVIDE", VK_DIVIDE },
    { "DOWN", VK_DOWN },
    { "E", 'E' },
    { "END", VK_END },
    { "ENTER", VK_RETURN },
    { "F", 'F' },
    { "F1", VK_F1 },
    { "F10", VK_F10 },
    { "F11", VK_F11 },
    { "F12", VK_F12 },
    { "F2", VK_F2 },
    { "F3", VK_F3 },
    { "F4", VK_F4 },
    { "F5", VK_F5 },
    { "F6", VK_F6 },
    { "F7", VK_F7 },
    { "F8", VK_F8 },
    { "F9", VK_F9 },
    { "G", 'G' },
    { "H", 'H' },
    { "HOME", VK_HOME },
    { "I", 'I' },
    { "INSERT", VK_INSERT },
    { "J", 'J' },
    { "K", 'K' },
    { "L", 'L' },
    { "LALT", VK_LMENU },
    { "LCTRL", VK_LCONTROL },
    { "LEFT", VK_LEFT },
    { "LSHIFT", VK_LSHIFT },
    { "M", 'M' },
    { "MOUSE1", 0 },
    { "MOUSE2", 1 },
    { "MOUSE3", 2 },
    { "MOUSE4", 3 },
    { "MOUSE5", 4 },
    { "MULTIPLY", VK_MULTIPLY },
    { "MWHEEL_DOWN", 0 },
    { "MWHEEL_UP", 0 },
    { "N", 'N' },
    { "NONE", 0 },
    { "NUMPAD_0", VK_NUMPAD0 },
    { "NUMPAD_1", VK_NUMPAD1 },
    { "NUMPAD_2", VK_NUMPAD2 },
    { "NUMPAD_3", VK_NUMPAD3 },
    { "NUMPAD_4", VK_NUMPAD4 },
    { "NUMPAD_5", VK_NUMPAD5 },
    { "NUMPAD_6", VK_NUMPAD6 },
    { "NUMPAD_7", VK_NUMPAD7 },
    { "NUMPAD_8", VK_NUMPAD8 },
    { "NUMPAD_9", VK_NUMPAD9 },
    { "O", 'O' },
    { "P", 'P' },
    { "PAGE_DOWN", VK_NEXT },
    { "PAGE_UP", VK_PRIOR },
    { "Q", 'Q' },
    { "R", 'R' },
    { "RALT", VK_RMENU },
    { "RCTRL", VK_RCONTROL },
    { "RIGHT", VK_RIGHT },
    { "RSHIFT", VK_RSHIFT },
    { "S", 'S' },
    { "SPACE", VK_SPACE },
    { "SUBTRACT", VK_SUBTRACT },
    { "T", 'T' },
    { "TAB", VK_TAB },
    { "U", 'U' },
    { "UP", VK_UP },
    { "V", 'V' },
    { "W", 'W', },
    { "X", 'X' },
    { "Y", 'Y' },
    { "Z", 'Z' },
    { "[", VK_OEM_4 },
    { "\\", VK_OEM_5 },
    { "]", VK_OEM_6 },
    { "`", VK_OEM_3 }
});

static_assert(keyMap.size() == KeyBind::MAX);

KeyBind::KeyBind(KeyCode keyCode) noexcept
{
    this->keyCode = static_cast<std::size_t>(keyCode) < keyMap.size() ? keyCode : KeyCode::NONE;
}

KeyBind::KeyBind(KeyCode keyCode, KeyMode keyMode) noexcept
{
    this->keyCode = static_cast<std::size_t>(keyCode) < keyMap.size() ? keyCode : KeyCode::NONE;
    this->originalKeyCode = this->keyCode;
    this->keyMode = keyMode;
    this->originalKeyMode = keyMode;
}

KeyBind::KeyBind(const char* keyName) noexcept
{
    auto it = std::lower_bound(keyMap.begin(), keyMap.end(), keyName, [](const Key& key, const char* keyName) { return key.name < keyName; });
    if (it != keyMap.end() && it->name == keyName)
        keyCode = static_cast<KeyCode>(std::distance(keyMap.begin(), it));
    else
        keyCode = KeyCode::NONE;
}

KeyBind::KeyBind(const std::string name, KeyMode keyMode) noexcept
{
    this->keyCode = KeyCode::NONE;
    this->keyMode = keyMode;
    this->originalKeyMode = keyMode;
    this->activeName = name;
}

const char* KeyBind::toString() const noexcept
{
    return keyMap[static_cast<std::size_t>(keyCode) < keyMap.size() ? keyCode : KeyCode::NONE].name.data();
}

bool KeyBind::isPressed() const noexcept
{
    if (keyCode == KeyCode::NONE)
        return false;

    if (keyCode == KeyCode::MOUSEWHEEL_DOWN)
        return ImGui::GetIO().MouseWheel < 0.0f;

    if (keyCode == KeyCode::MOUSEWHEEL_UP)
        return ImGui::GetIO().MouseWheel > 0.0f;

    if (keyCode >= KeyCode::MOUSE1 && keyCode <= KeyCode::MOUSE5)
        return ImGui::IsMouseClicked(keyMap[keyCode].code);

    return static_cast<std::size_t>(keyCode) < keyMap.size() && ImGui::IsKeyPressed(keyMap[keyCode].code, false);
}

bool KeyBind::isDown() const noexcept
{
    if (keyCode == KeyCode::NONE)
        return false;

    if (keyCode == KeyCode::MOUSEWHEEL_DOWN)
        return ImGui::GetIO().MouseWheel < 0.0f;

    if (keyCode == KeyCode::MOUSEWHEEL_UP)
        return ImGui::GetIO().MouseWheel > 0.0f;

    if (keyCode >= KeyCode::MOUSE1 && keyCode <= KeyCode::MOUSE5)
        return ImGui::IsMouseDown(keyMap[keyCode].code);

    return static_cast<std::size_t>(keyCode) < keyMap.size() && ImGui::IsKeyDown(keyMap[keyCode].code);
}

bool KeyBind::setToPressedKey() noexcept
{
    if (ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Escape])) {
        keyCode = KeyCode::NONE;
        return true;
    } else if (ImGui::GetIO().MouseWheel < 0.0f) {
        keyCode = KeyCode::MOUSEWHEEL_DOWN;
        return true;
    } else if (ImGui::GetIO().MouseWheel > 0.0f) {
        keyCode = KeyCode::MOUSEWHEEL_UP;
        return true;
    } else {
        for (int i = 0; i < IM_ARRAYSIZE(ImGui::GetIO().MouseDown); ++i) {
            if (ImGui::IsMouseClicked(i)) {
                keyCode = KeyCode(KeyCode::MOUSE1 + i);
                return true;
            }
        }

        for (int i = 0; i < IM_ARRAYSIZE(ImGui::GetIO().KeysDown); ++i) {
            if (ImGui::IsKeyPressed(i)) {
                auto it = std::find_if(keyMap.begin(), keyMap.end(), [i](const Key& key) { return key.code == i; });
                if (it != keyMap.end()) {
                    keyCode = static_cast<KeyCode>(std::distance(keyMap.begin(), it));
                    // Treat AltGr as RALT
                    if (keyCode == KeyCode::LCTRL && ImGui::IsKeyPressed(keyMap[KeyCode::RALT].code))
                        keyCode = KeyCode::RALT;
                    return true;
                }
            }
        }
    }
    return false;
}

void KeyBind::handleToggle() noexcept
{
    if (keyMode != KeyMode::Toggle)
        return;

    if (isPressed())
        toggledOn = !toggledOn;
}

bool KeyBind::isActive() const noexcept
{
    switch (keyMode)
    {
    case KeyMode::Off:
        return false;
    case KeyMode::Always:
        return true;
    case KeyMode::Hold:
        return isDown();
    case KeyMode::Toggle:
        return isToggled();
    default:
        break;
    }
    return false;
}

bool KeyBind::canShowKeybind() noexcept
{
    if (!isActive())
        return false;

    if (keyMode != KeyMode::Hold && keyMode != KeyMode::Toggle)
        return false;

    return true;
}

void KeyBind::showKeybind() noexcept
{
    if (!isActive())
        return;

    switch (keyMode)
    {
    case KeyMode::Hold:
        ImGui::TextWrapped("%s %s", "[hold]", this->activeName.c_str());
        break;
    case KeyMode::Toggle:
        ImGui::TextWrapped("%s %s", "[toggled]", this->activeName.c_str());
        break;
    default:
        break;
    }
}

void KeyBind::reset() noexcept
{
   keyCode = originalKeyCode;
   keyMode = originalKeyMode;
}