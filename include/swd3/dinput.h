#pragma once

#include <cstdint>

#include "swd3/ddraw.h"

// DirectInput 5 interfaces as used by swd32.exe (keyboard and mouse). The implementation
// (src/platform/dinput_emu.cpp) emulates them on SDL3.
namespace w32 {

constexpr HRESULT DI_OK = 0;
constexpr HRESULT DIERR_INVALIDPARAM = static_cast<HRESULT>(0x80070057u);
constexpr HRESULT DIERR_NOTACQUIRED = static_cast<HRESULT>(0x8007000Cu);
constexpr HRESULT DIERR_INPUTLOST = static_cast<HRESULT>(0x8007001Eu);
constexpr DWORD DISCL_EXCLUSIVE = 0x1;
constexpr DWORD DISCL_NONEXCLUSIVE = 0x2;
constexpr DWORD DISCL_FOREGROUND = 0x4;
constexpr DWORD DISCL_BACKGROUND = 0x8;
constexpr DWORD DIDF_ABSAXIS = 0x1;

struct DIOBJECTDATAFORMAT {
    const GUID* pguid;
    DWORD dwOfs;
    DWORD dwType;
    DWORD dwFlags;
};

struct DIDATAFORMAT {
    DWORD dwSize;
    DWORD dwObjSize;
    DWORD dwFlags;
    DWORD dwDataSize;
    DWORD dwNumObjs;
    DIOBJECTDATAFORMAT* rgodf;
};

struct DIPROPHEADER {
    DWORD dwSize;
    DWORD dwHeaderSize;
    DWORD dwObj;
    DWORD dwHow;
};

struct DIPROPDWORD {
    DIPROPHEADER diph;
    DWORD dwData;
};

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(DIOBJECTDATAFORMAT) == 0x10);
static_assert(sizeof(DIDATAFORMAT) == 0x18);
static_assert(sizeof(DIPROPDWORD) == 0x14);
#endif

struct IDirectInputDeviceA : IUnknownLike {
    virtual HRESULT GetCapabilities(void* caps) = 0;
    virtual HRESULT EnumObjects(void* callback, void* context, DWORD flags) = 0;
    virtual HRESULT GetProperty(const GUID* property, DIPROPHEADER* header) = 0;
    virtual HRESULT SetProperty(const GUID* property, const DIPROPHEADER* header) = 0;
    virtual HRESULT Acquire() = 0;
    virtual HRESULT Unacquire() = 0;
    virtual HRESULT GetDeviceState(DWORD size, LPVOID data) = 0;
    virtual HRESULT GetDeviceData(DWORD objectSize, void* data, DWORD* count, DWORD flags) = 0;
    virtual HRESULT SetDataFormat(const DIDATAFORMAT* format) = 0;
    virtual HRESULT SetEventNotification(HANDLE event) = 0;
    virtual HRESULT SetCooperativeLevel(HWND window, DWORD flags) = 0;
    virtual HRESULT GetObjectInfo(void* info, DWORD object, DWORD how) = 0;
    virtual HRESULT GetDeviceInfo(void* info) = 0;
    virtual HRESULT RunControlPanel(HWND owner, DWORD flags) = 0;
    virtual HRESULT Initialize(HANDLE instance, DWORD version, REFIID guid) = 0;
};

struct IDirectInputA : IUnknownLike {
    virtual HRESULT CreateDevice(REFIID guid, IDirectInputDeviceA** device, void* outer) = 0;
    virtual HRESULT EnumDevices(DWORD type, void* callback, void* context, DWORD flags) = 0;
    virtual HRESULT GetDeviceStatus(REFIID guid) = 0;
    virtual HRESULT RunControlPanel(HWND owner, DWORD flags) = 0;
    virtual HRESULT Initialize(HANDLE instance, DWORD version) = 0;
};

HRESULT DirectInputCreateA(HANDLE instance, DWORD version, IDirectInputA** directInput, void* outer);

}  // namespace w32
