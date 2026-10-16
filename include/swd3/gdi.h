#pragma once

#include "swd3/ddraw.h"

// GDI text output on surface device contexts (IDirectDrawSurface4::GetDC). Implemented with real GDI on
// Windows; elsewhere surfaces have no DC, so only object creation and deletion are emulated.
namespace w32 {

using HGDIOBJ = void*;
using HFONT = void*;

constexpr int kBkTransparent = 1;
constexpr int kBkOpaque = 2;
constexpr BYTE kChineseBig5Charset = 0x88;

struct LOGFONTA {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    CHAR lfFaceName[32];
};

static_assert(sizeof(LOGFONTA) == 0x3C);

HFONT CreateFontIndirectA(const LOGFONTA* font);
BOOL DeleteObject(HGDIOBJ object);
HGDIOBJ SelectObject(HDC dc, HGDIOBJ object);
COLORREF SetBkColor(HDC dc, COLORREF color);
int SetBkMode(HDC dc, int mode);
COLORREF SetTextColor(HDC dc, COLORREF color);
BOOL TextOutA(HDC dc, int x, int y, LPCSTR text, int length);

}  // namespace w32
