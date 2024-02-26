#pragma once

#include <memory>
#include <string>

struct ImFont;

class GUI {
public:
    GUI() noexcept;
    void render() noexcept;
    ImFont* getTahoma28Font() const noexcept;
    ImFont* getUnicodeFont() const noexcept;
    void handleToggle() noexcept;
    bool isOpen() noexcept { return open; }
private:
    bool open = true;

    void renderGuiStyle() noexcept;
    void renderAimbotWindow() noexcept;
    void renderTriggerbotWindow() noexcept;
    void renderAutoWindow() noexcept;
    void renderFakelagWindow() noexcept;
    void renderAntiAimWindow() noexcept;
    void renderBacktrackWindow() noexcept;
    void renderChamsWindow() noexcept;
    void renderGlowWindow() noexcept;
    void renderStreamProofESPWindow() noexcept;
    void renderVisualsWindow() noexcept;
    void renderMiscWindow() noexcept;
    void renderSkinChangerWindow() noexcept;
    void renderConfigWindow() noexcept;

    struct {
        ImFont* normal15px = nullptr;
        ImFont* tahoma28 = nullptr;
        ImFont* tahoma34 = nullptr;
        ImFont* unicodeFont = nullptr;
    } fonts;

    float timeToNextConfigRefresh = 0.1f;
};

inline std::unique_ptr<GUI> gui;
