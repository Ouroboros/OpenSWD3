// DirectInput emulation on SDL3: the system keyboard reports DIK scan codes from the SDL keyboard state; the
// system mouse reports accumulated relative motion as absolute axes and the button states, at the offsets of
// the data format set by the game.
#include <SDL3/SDL.h>

#include <cstring>
#include <vector>

#include "platform/video.h"
#include "swd3/dinput.h"

namespace w32 {
namespace {

constexpr GUID kGuidSysKeyboard{0x6F1D2B61u, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
constexpr GUID kGuidSysMouse{0x6F1D2B60u, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
constexpr GUID kGuidXAxis{0xA36D02E0u, 0xC9F3, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
constexpr GUID kGuidYAxis{0xA36D02E1u, 0xC9F3, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
constexpr GUID kGuidButton{0xA36D02F0u, 0xC9F3, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
constexpr HRESULT kNoInterface = static_cast<HRESULT>(0x80004002u);
constexpr HRESULT kDeviceNotRegistered = static_cast<HRESULT>(0x80040154u);

bool sameGuid(const GUID& a, const GUID& b) { return std::memcmp(&a, &b, sizeof(GUID)) == 0; }

struct DikMapping {
    SDL_Scancode scancode;
    std::uint8_t dik;
};

const DikMapping kDikMap[] = {
    {SDL_SCANCODE_ESCAPE, 0x01}, {SDL_SCANCODE_1, 0x02}, {SDL_SCANCODE_2, 0x03}, {SDL_SCANCODE_3, 0x04},
    {SDL_SCANCODE_4, 0x05}, {SDL_SCANCODE_5, 0x06}, {SDL_SCANCODE_6, 0x07}, {SDL_SCANCODE_7, 0x08},
    {SDL_SCANCODE_8, 0x09}, {SDL_SCANCODE_9, 0x0A}, {SDL_SCANCODE_0, 0x0B}, {SDL_SCANCODE_MINUS, 0x0C},
    {SDL_SCANCODE_EQUALS, 0x0D}, {SDL_SCANCODE_BACKSPACE, 0x0E}, {SDL_SCANCODE_TAB, 0x0F}, {SDL_SCANCODE_Q, 0x10},
    {SDL_SCANCODE_W, 0x11}, {SDL_SCANCODE_E, 0x12}, {SDL_SCANCODE_R, 0x13}, {SDL_SCANCODE_T, 0x14},
    {SDL_SCANCODE_Y, 0x15}, {SDL_SCANCODE_U, 0x16}, {SDL_SCANCODE_I, 0x17}, {SDL_SCANCODE_O, 0x18},
    {SDL_SCANCODE_P, 0x19}, {SDL_SCANCODE_LEFTBRACKET, 0x1A}, {SDL_SCANCODE_RIGHTBRACKET, 0x1B},
    {SDL_SCANCODE_RETURN, 0x1C}, {SDL_SCANCODE_LCTRL, 0x1D}, {SDL_SCANCODE_A, 0x1E}, {SDL_SCANCODE_S, 0x1F},
    {SDL_SCANCODE_D, 0x20}, {SDL_SCANCODE_F, 0x21}, {SDL_SCANCODE_G, 0x22}, {SDL_SCANCODE_H, 0x23},
    {SDL_SCANCODE_J, 0x24}, {SDL_SCANCODE_K, 0x25}, {SDL_SCANCODE_L, 0x26}, {SDL_SCANCODE_SEMICOLON, 0x27},
    {SDL_SCANCODE_APOSTROPHE, 0x28}, {SDL_SCANCODE_GRAVE, 0x29}, {SDL_SCANCODE_LSHIFT, 0x2A},
    {SDL_SCANCODE_BACKSLASH, 0x2B}, {SDL_SCANCODE_Z, 0x2C}, {SDL_SCANCODE_X, 0x2D}, {SDL_SCANCODE_C, 0x2E},
    {SDL_SCANCODE_V, 0x2F}, {SDL_SCANCODE_B, 0x30}, {SDL_SCANCODE_N, 0x31}, {SDL_SCANCODE_M, 0x32},
    {SDL_SCANCODE_COMMA, 0x33}, {SDL_SCANCODE_PERIOD, 0x34}, {SDL_SCANCODE_SLASH, 0x35},
    {SDL_SCANCODE_RSHIFT, 0x36}, {SDL_SCANCODE_KP_MULTIPLY, 0x37}, {SDL_SCANCODE_LALT, 0x38},
    {SDL_SCANCODE_SPACE, 0x39}, {SDL_SCANCODE_CAPSLOCK, 0x3A}, {SDL_SCANCODE_F1, 0x3B}, {SDL_SCANCODE_F2, 0x3C},
    {SDL_SCANCODE_F3, 0x3D}, {SDL_SCANCODE_F4, 0x3E}, {SDL_SCANCODE_F5, 0x3F}, {SDL_SCANCODE_F6, 0x40},
    {SDL_SCANCODE_F7, 0x41}, {SDL_SCANCODE_F8, 0x42}, {SDL_SCANCODE_F9, 0x43}, {SDL_SCANCODE_F10, 0x44},
    {SDL_SCANCODE_NUMLOCKCLEAR, 0x45}, {SDL_SCANCODE_SCROLLLOCK, 0x46}, {SDL_SCANCODE_KP_7, 0x47},
    {SDL_SCANCODE_KP_8, 0x48}, {SDL_SCANCODE_KP_9, 0x49}, {SDL_SCANCODE_KP_MINUS, 0x4A}, {SDL_SCANCODE_KP_4, 0x4B},
    {SDL_SCANCODE_KP_5, 0x4C}, {SDL_SCANCODE_KP_6, 0x4D}, {SDL_SCANCODE_KP_PLUS, 0x4E}, {SDL_SCANCODE_KP_1, 0x4F},
    {SDL_SCANCODE_KP_2, 0x50}, {SDL_SCANCODE_KP_3, 0x51}, {SDL_SCANCODE_KP_0, 0x52},
    {SDL_SCANCODE_KP_PERIOD, 0x53}, {SDL_SCANCODE_F11, 0x57}, {SDL_SCANCODE_F12, 0x58},
    {SDL_SCANCODE_KP_ENTER, 0x9C}, {SDL_SCANCODE_RCTRL, 0x9D}, {SDL_SCANCODE_KP_DIVIDE, 0xB5},
    {SDL_SCANCODE_PRINTSCREEN, 0xB7}, {SDL_SCANCODE_RALT, 0xB8}, {SDL_SCANCODE_PAUSE, 0xC5},
    {SDL_SCANCODE_HOME, 0xC7}, {SDL_SCANCODE_UP, 0xC8}, {SDL_SCANCODE_PAGEUP, 0xC9}, {SDL_SCANCODE_LEFT, 0xCB},
    {SDL_SCANCODE_RIGHT, 0xCD}, {SDL_SCANCODE_END, 0xCF}, {SDL_SCANCODE_DOWN, 0xD0},
    {SDL_SCANCODE_PAGEDOWN, 0xD1}, {SDL_SCANCODE_INSERT, 0xD2}, {SDL_SCANCODE_DELETE, 0xD3},
    {SDL_SCANCODE_LGUI, 0xDB}, {SDL_SCANCODE_RGUI, 0xDC}, {SDL_SCANCODE_APPLICATION, 0xDD},
};

class Device : public IDirectInputDeviceA {
public:
    virtual ~Device() = default;
    HRESULT QueryInterface(REFIID, void** object) override {
        *object = nullptr;
        return kNoInterface;
    }
    DWORD AddRef() override { return ++m_refs; }
    DWORD Release() override {
        const DWORD refs = --m_refs;
        if (refs == 0) {
            delete this;
        }
        return refs;
    }
    HRESULT GetCapabilities(void*) override { return DIERR_INVALIDPARAM; }
    HRESULT EnumObjects(void*, void*, DWORD) override { return DIERR_INVALIDPARAM; }
    HRESULT GetProperty(const GUID*, DIPROPHEADER*) override { return DIERR_INVALIDPARAM; }
    HRESULT SetProperty(const GUID*, const DIPROPHEADER*) override { return DI_OK; }
    HRESULT Acquire() override {
        m_acquired = true;
        return DI_OK;
    }
    HRESULT Unacquire() override {
        m_acquired = false;
        return DI_OK;
    }
    HRESULT GetDeviceData(DWORD, void*, DWORD* count, DWORD) override {
        if (count != nullptr) {
            *count = 0;
        }
        return DI_OK;
    }
    HRESULT SetDataFormat(const DIDATAFORMAT* format) override {
        m_dataSize = format->dwDataSize;
        m_objects.assign(format->rgodf, format->rgodf == nullptr ? format->rgodf : format->rgodf + format->dwNumObjs);
        return DI_OK;
    }
    HRESULT SetEventNotification(HANDLE) override { return DI_OK; }
    HRESULT SetCooperativeLevel(HWND, DWORD) override { return DI_OK; }
    HRESULT GetObjectInfo(void*, DWORD, DWORD) override { return DIERR_INVALIDPARAM; }
    HRESULT GetDeviceInfo(void*) override { return DIERR_INVALIDPARAM; }
    HRESULT RunControlPanel(HWND, DWORD) override { return DI_OK; }
    HRESULT Initialize(HANDLE, DWORD, REFIID) override { return DI_OK; }

protected:
    DWORD m_refs = 1;
    bool m_acquired = false;
    DWORD m_dataSize = 0;
    std::vector<DIOBJECTDATAFORMAT> m_objects;
};

class Keyboard final : public Device {
public:
    HRESULT GetDeviceState(DWORD size, LPVOID data) override {
        if (size != m_dataSize) {
            return DIERR_INVALIDPARAM;
        }
        if (!m_acquired) {
            return DIERR_NOTACQUIRED;
        }
        auto* keys = static_cast<std::uint8_t*>(data);
        std::memset(keys, 0, size);
        SDL_PumpEvents();
        int count = 0;
        const bool* state = SDL_GetKeyboardState(&count);
        for (const DikMapping& m : kDikMap) {
            if (m.scancode < count && state[m.scancode] && m.dik < size) {
                keys[m.dik] = 0x80;
            }
        }
        return DI_OK;
    }
};

class Mouse final : public Device {
public:
    HRESULT Acquire() override {
        if (SDL_Window* window = swd3::platform::gameWindow()) {
            SDL_SetWindowRelativeMouseMode(window, true);
        }
        float dx;
        float dy;
        SDL_GetRelativeMouseState(&dx, &dy);
        return Device::Acquire();
    }
    HRESULT Unacquire() override {
        if (SDL_Window* window = swd3::platform::gameWindow()) {
            SDL_SetWindowRelativeMouseMode(window, false);
        }
        return Device::Unacquire();
    }
    HRESULT GetDeviceState(DWORD size, LPVOID data) override {
        if (size != m_dataSize) {
            return DIERR_INVALIDPARAM;
        }
        if (!m_acquired) {
            return DIERR_NOTACQUIRED;
        }
        auto* out = static_cast<std::uint8_t*>(data);
        std::memset(out, 0, size);
        SDL_PumpEvents();
        float dx = 0;
        float dy = 0;
        SDL_GetRelativeMouseState(&dx, &dy);
        m_remainderX += dx;
        m_remainderY += dy;
        const auto stepX = static_cast<std::int32_t>(m_remainderX);
        const auto stepY = static_cast<std::int32_t>(m_remainderY);
        m_remainderX -= static_cast<float>(stepX);
        m_remainderY -= static_cast<float>(stepY);
        m_axisX += stepX;
        m_axisY += stepY;
        const SDL_MouseButtonFlags buttons = SDL_GetMouseState(nullptr, nullptr);
        int buttonIndex = 0;
        for (const DIOBJECTDATAFORMAT& object : m_objects) {
            if (object.pguid != nullptr && sameGuid(*object.pguid, kGuidXAxis) && object.dwOfs + 4 <= size) {
                std::memcpy(out + object.dwOfs, &m_axisX, 4);
            } else if (object.pguid != nullptr && sameGuid(*object.pguid, kGuidYAxis) && object.dwOfs + 4 <= size) {
                std::memcpy(out + object.dwOfs, &m_axisY, 4);
            } else if (object.pguid != nullptr && sameGuid(*object.pguid, kGuidButton) && object.dwOfs < size) {
                const SDL_MouseButtonFlags mask =
                    buttonIndex == 0 ? SDL_BUTTON_LMASK : buttonIndex == 1 ? SDL_BUTTON_RMASK : SDL_BUTTON_MMASK;
                out[object.dwOfs] = (buttons & mask) ? 0x80 : 0;
                ++buttonIndex;
            }
        }
        return DI_OK;
    }

private:
    std::int32_t m_axisX = 0;
    std::int32_t m_axisY = 0;
    float m_remainderX = 0;
    float m_remainderY = 0;
};

class DirectInput final : public IDirectInputA {
public:
    HRESULT QueryInterface(REFIID, void** object) override {
        *object = nullptr;
        return kNoInterface;
    }
    DWORD AddRef() override { return ++m_refs; }
    DWORD Release() override {
        const DWORD refs = --m_refs;
        if (refs == 0) {
            delete this;
        }
        return refs;
    }
    HRESULT CreateDevice(REFIID guid, IDirectInputDeviceA** device, void*) override {
        if (sameGuid(guid, kGuidSysKeyboard)) {
            *device = new Keyboard();
            return DI_OK;
        }
        if (sameGuid(guid, kGuidSysMouse)) {
            *device = new Mouse();
            return DI_OK;
        }
        *device = nullptr;
        return kDeviceNotRegistered;
    }
    HRESULT EnumDevices(DWORD, void*, void*, DWORD) override { return DI_OK; }
    HRESULT GetDeviceStatus(REFIID) override { return DI_OK; }
    HRESULT RunControlPanel(HWND, DWORD) override { return DI_OK; }
    HRESULT Initialize(HANDLE, DWORD) override { return DI_OK; }

private:
    DWORD m_refs = 1;
};

}  // namespace

HRESULT DirectInputCreateA(HANDLE, DWORD, IDirectInputA** directInput, void*) {
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_InitSubSystem(SDL_INIT_VIDEO);
    }
    *directInput = new DirectInput();
    return DI_OK;
}

}  // namespace w32
