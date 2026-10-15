#pragma once

#include "swd3/ddraw.h"

namespace swd3test {

using namespace w32;

// IDirectDrawSurface4 whose methods all fail; tests override what they need.
struct StubSurface : IDirectDrawSurface4 {
    HRESULT QueryInterface(REFIID, void**) override { return E_NOINTERFACE; }
    DWORD AddRef() override { return 1; }
    DWORD Release() override { return 0; }
    HRESULT AddAttachedSurface(IDirectDrawSurface4*) override { return DDERR_GENERIC; }
    HRESULT AddOverlayDirtyRect(RECT*) override { return DDERR_GENERIC; }
    HRESULT Blt(RECT*, IDirectDrawSurface4*, RECT*, DWORD, DDBLTFX*) override { return DDERR_GENERIC; }
    HRESULT BltBatch(void*, DWORD, DWORD) override { return DDERR_GENERIC; }
    HRESULT BltFast(DWORD, DWORD, IDirectDrawSurface4*, RECT*, DWORD) override { return DDERR_GENERIC; }
    HRESULT DeleteAttachedSurface(DWORD, IDirectDrawSurface4*) override { return DDERR_GENERIC; }
    HRESULT EnumAttachedSurfaces(void*, void*) override { return DDERR_GENERIC; }
    HRESULT EnumOverlayZOrders(DWORD, void*, void*) override { return DDERR_GENERIC; }
    HRESULT Flip(IDirectDrawSurface4*, DWORD) override { return DDERR_GENERIC; }
    HRESULT GetAttachedSurface(DDSCAPS2*, IDirectDrawSurface4**) override { return DDERR_GENERIC; }
    HRESULT GetBltStatus(DWORD) override { return DDERR_GENERIC; }
    HRESULT GetCaps(DDSCAPS2*) override { return DDERR_GENERIC; }
    HRESULT GetClipper(IDirectDrawClipper**) override { return DDERR_GENERIC; }
    HRESULT GetColorKey(DWORD, DDCOLORKEY*) override { return DDERR_GENERIC; }
    HRESULT GetDC(HDC*) override { return DDERR_GENERIC; }
    HRESULT GetFlipStatus(DWORD) override { return DDERR_GENERIC; }
    HRESULT GetOverlayPosition(LONG*, LONG*) override { return DDERR_GENERIC; }
    HRESULT GetPalette(IDirectDrawPalette**) override { return DDERR_GENERIC; }
    HRESULT GetPixelFormat(DDPIXELFORMAT*) override { return DDERR_GENERIC; }
    HRESULT GetSurfaceDesc(DDSURFACEDESC2*) override { return DDERR_GENERIC; }
    HRESULT Initialize(void*, DDSURFACEDESC2*) override { return DDERR_GENERIC; }
    HRESULT IsLost() override { return DD_OK; }
    HRESULT Lock(RECT*, DDSURFACEDESC2*, DWORD, HANDLE) override { return DDERR_GENERIC; }
    HRESULT ReleaseDC(HDC) override { return DDERR_GENERIC; }
    HRESULT Restore() override { return DDERR_GENERIC; }
    HRESULT SetClipper(IDirectDrawClipper*) override { return DDERR_GENERIC; }
    HRESULT SetColorKey(DWORD, DDCOLORKEY*) override { return DDERR_GENERIC; }
    HRESULT SetOverlayPosition(LONG, LONG) override { return DDERR_GENERIC; }
    HRESULT SetPalette(IDirectDrawPalette*) override { return DDERR_GENERIC; }
    HRESULT Unlock(LPVOID) override { return DDERR_GENERIC; }
    HRESULT UpdateOverlay(RECT*, IDirectDrawSurface4*, RECT*, DWORD, void*) override { return DDERR_GENERIC; }
    HRESULT UpdateOverlayDisplay(DWORD) override { return DDERR_GENERIC; }
    HRESULT UpdateOverlayZOrder(DWORD, IDirectDrawSurface4*) override { return DDERR_GENERIC; }
    HRESULT GetDDInterface(void**) override { return DDERR_GENERIC; }
    HRESULT PageLock(DWORD) override { return DDERR_GENERIC; }
    HRESULT PageUnlock(DWORD) override { return DDERR_GENERIC; }
    HRESULT SetSurfaceDesc(DDSURFACEDESC2*, DWORD) override { return DDERR_GENERIC; }
    HRESULT SetPrivateData(REFIID, void*, DWORD, DWORD) override { return DDERR_GENERIC; }
    HRESULT GetPrivateData(REFIID, void*, DWORD*) override { return DDERR_GENERIC; }
    HRESULT FreePrivateData(REFIID) override { return DDERR_GENERIC; }
    HRESULT GetUniquenessValue(DWORD*) override { return DDERR_GENERIC; }
    HRESULT ChangeUniquenessValue() override { return DDERR_GENERIC; }
};

}  // namespace swd3test
