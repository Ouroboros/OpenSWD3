#include "gfx/graphics.h"

#include <algorithm>

#include "platform/display.h"
#include "platform/message_box.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

// DirectDraw results the error dialog names.
constexpr u32 kGenericError = 0x80004005u;
constexpr u32 kUnsupportedMode = 0x8876024Eu;
// The video memory the device reports (the original asks the driver for texture memory).
constexpr u32 kVideoMemory = 64u << 20;

// GDI's colour of a COLORREF on a 16-bit RGB565 surface (SetPixel keeps the high bits of each channel).
Pixel DevicePixel(ColorRef color)
{
    const u32 red = color & 0xFF;
    const u32 green = (color >> 8) & 0xFF;
    const u32 blue = (color >> 16) & 0xFF;
    return static_cast<Pixel>(((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3));
}

}  // namespace

// swd32.exe 0x004311C0: a message box naming the DirectDraw result (nothing for an unknown one). The original
// returns a leftover value that no caller reads.
void ShowDeviceError(ErrorReporter& errors, std::string_view caption, u32 result)
{
    std::string_view text;

    switch (result)
    {
        case 0x88760232u:
            text = "DDERR_DIRECTDRAWALREADYCREATED ";
            break;

        case 0x88760233u:
            text = "DDERR_NODIRECTDRAWHW ";
            break;

        case 0x88760234u:
            text = "DDERR_PRIMARYSURFACEALREADYEXISTS  ";
            break;

        case 0x88760235u:
            text = "DDERR_NOEMULATION  ";
            break;

        case 0x88760238u:
            text = "DDERR_NOCLIPPERATTACHED  ";
            break;

        case 0x8876023Au:
            text = "DDERR_HWNDSUBCLASSED  ";
            break;

        case 0x8876023Bu:
            text = "DDERR_HWNDALREADYSET  ";
            break;

        case 0x88760245u:
            text = "DDERR_EXCLUSIVEMODEALREADYSET  ";
            break;

        case 0x8876024Eu:
            text = "DDERR_UNSUPPORTEDMODE  ";
            break;

        case 0x88760250u:
            text = "DDERR_INVALIDSURFACETYPE  ";
            break;

        case 0x88760231u:
            text = "DDERR_INVALIDDIRECTDRAWGUID ";
            break;

        case 0x88760082u:
            text = "DDERR_INVALIDOBJECT  ";
            break;

        case 0x887600D4u:
            text = "DDERR_NOCOOPERATIVELEVELSET  ";
            break;

        case 0x887600E1u:
            text = "DDERR_NOEXCLUSIVEMODE ";
            break;

        case 0x887600E6u:
            text = "DDERR_NOFLIPHW ";
            break;

        case 0x8876017Cu:
            text = "DDERR_OUTOFVIDEOMEMORY   ";
            break;

        case 0x8876006Eu:
            text = "DDERR_INVALIDCLIPLIST ";
            break;

        case 0x8876005Fu:
            text = "DDERR_INCOMPATIBLEPRIMARY  ";
            break;

        case 0x88760064u:
            text = "DDERR_INVALIDCAPS ";
            break;

        case 0x80070057u:
            text = "DDERR_INVALIDPARAMS  ";
            break;

        case 0x80004005u:
            text = "DDERR_GENERIC ";
            break;

        case 0x8007000Eu:
            text = "DDERR_OUTOFMEMORY ";
            break;

        default:
            break;
    }

    if (!text.empty())
    {
        errors.ShowError(caption, text);
    }
}

// swd32.exe 0x00437560: no device and no surfaces
Graphics::Graphics(Display& display, ErrorReporter& errors)
    : display(display),
      errors(errors)
{
}

Graphics::~Graphics() = default;

int Graphics::Width() const
{
    return this->width;
}

int Graphics::Height() const
{
    return this->height;
}

// swd32.exe 0x00437570: kWindowed uses the normal cooperative level, any other mode exclusive full screen
bool Graphics::Init(int mode, int width, int height, int bitsPerPixel)
{
    this->width = width;
    this->height = height;

    if (!this->CreateDevice())
    {
        return false;
    }

    if (!this->SetCooperativeLevel(mode == Graphics::kWindowed ? Graphics::kNormal : Graphics::kExclusive))
    {
        return false;
    }

    if (!this->SetDisplayMode(width, height, bitsPerPixel))
    {
        return false;
    }

    if (!this->CreateSurfaces())
    {
        return false;
    }

    this->DetectPixelFormat();
    return true;
}

// swd32.exe 0x00437610: DirectDrawCreate and the IDirectDraw4 interface
bool Graphics::CreateDevice()
{
    this->device = true;
    return true;
}

// swd32.exe 0x00437680
bool Graphics::SetCooperativeLevel(u32 flags)
{
    this->cooperativeFlags = flags;
    return true;
}

// swd32.exe 0x004376D0: the game window shows 16-bit pixels only; any other depth fails like an unsupported mode
bool Graphics::SetDisplayMode(int width, int height, int bitsPerPixel)
{
    if (bitsPerPixel != 16)
    {
        ShowDeviceError(this->errors, "DirectDraw SetDisplayMode Error!!", kUnsupportedMode);
        this->device = false;
        return false;
    }

    this->width = width;
    this->height = height;
    this->bitsPerPixel = bitsPerPixel;
    return true;
}

// swd32.exe 0x00437720: the clipper of the primary surface (the window clips presentation here)
bool Graphics::CreateClipper()
{
    return true;
}

// swd32.exe 0x004377D0: the primary surface; in windowed mode also a back buffer of the desktop size (the size of the
// logical screen here). Records pitch (bytes) and height of both, the primary twice when there is no back buffer.
bool Graphics::CreateSurfaces()
{
    this->QueryVideoMemory();

    if (this->cooperativeFlags == Graphics::kNormal)
    {
        this->primary = std::make_unique<Surface>(Display::kWidth, Display::kHeight);

        if (!this->CreateClipper())
        {
            return false;
        }

        this->width = Display::kWidth;
        this->height = Display::kHeight;
        this->back = this->CreateOffscreen(Display::kWidth, Display::kHeight);
        this->primaryPitch = this->primary->Width() * 2;
        this->primaryHeight = this->primary->Height();
        this->backPitch = this->back->Width() * 2;
        this->backHeight = this->back->Height();
        return true;
    }

    if (this->width <= 0 || this->height <= 0)
    {
        ShowDeviceError(this->errors, "DirectDraw Create Primary Surface Error!!", kGenericError);
        this->device = false;
        return false;
    }

    this->primary = std::make_unique<Surface>(this->width, this->height);

    if (!this->CreateClipper())
    {
        return false;
    }

    this->primaryPitch = this->primary->Width() * 2;
    this->primaryHeight = this->primary->Height();
    this->backPitch = this->primaryPitch;
    this->backHeight = this->primaryHeight;
    return true;
}

// swd32.exe 0x00437A50: the display mode returns with the device in the original
bool Graphics::ReleaseSurfaces()
{
    if (!this->device)
    {
        return false;
    }

    this->third.reset();
    this->back.reset();
    this->primary.reset();
    return true;
}

// swd32.exe 0x00437AD0: the remaining reference count of the device
u32 Graphics::ReleaseDevice()
{
    this->device = false;
    return 0;
}

// swd32.exe 0x00437AE0: full screen flips the primary surface after the vertical blank; windowed copies the back
// buffer to the primary surface first. The window shows the primary surface.
bool Graphics::Present()
{
    if (this->primary == nullptr)
    {
        return true;
    }

    if ((this->cooperativeFlags & Graphics::kFullScreenFlag) == 0 && this->back != nullptr)
    {
        std::copy_n(
            this->back->Pixels(),
            static_cast<std::size_t>(this->back->Width()) * this->back->Height(),
            this->primary->Pixels()
        );
    }

    this->display.Present(*this->primary);
    return true;
}

// swd32.exe 0x00437B60: a system memory surface; its bytes count as video memory used
std::unique_ptr<Surface> Graphics::CreateOffscreen(int width, int height)
{
    this->videoMemoryUsed += static_cast<u32>(height * width) * (this->BitDepth() >> 3);
    return std::make_unique<Surface>(width, height);
}

// swd32.exe 0x00437C40: filled with the clear colour (never set: 0)
void Graphics::ClearSurface(Surface& surface)
{
    surface.Fill(static_cast<Pixel>(this->clearColor));
}

// swd32.exe 0x00437D80: the depth of the display mode
u32 Graphics::BitDepth() const
{
    return 16;
}

// swd32.exe 0x00437DC0
bool Graphics::QueryVideoMemory()
{
    this->videoMemoryTotal = kVideoMemory;
    this->videoMemoryFree = kVideoMemory - this->videoMemoryUsed;
    return true;
}

// swd32.exe 0x00437DF0
Surface* Graphics::GetSurface(SurfaceId id)
{
    switch (id)
    {
        case SurfaceId::Primary:
            return this->primary.get();

        case SurfaceId::Back:
            return this->back.get();

        case SurfaceId::Third:
            return this->third.get();
    }

    return nullptr;
}

// swd32.exe 0x00437E20: the device (the IDirectDraw4 interface in the original)
Graphics& Graphics::Device()
{
    return *this;
}

// IDirectDraw4::GetGDISurface: the surface GDI draws on, the primary one.
Surface* Graphics::GdiSurface()
{
    return this->primary.get();
}

// swd32.exe 0x00437E30: classifies the masks of the primary surface; returns the green mask
u32 Graphics::DetectPixelFormat()
{
    this->pixelFormat = 0;

    if (Surface::kGreenMask == 0x7E0)
    {
        this->pixelFormat = 2;
    }

    if (Surface::kBlueMask == 0x3F)
    {
        this->pixelFormat = 1;
    }

    if (Surface::kRedMask == 0xFC00)
    {
        this->pixelFormat = 3;
    }

    return Surface::kGreenMask;
}

// swd32.exe 0x00437E90: pitch in bytes
i32 Graphics::BackSize(i32& pitch, i32& height) const
{
    pitch = this->backPitch;
    height = this->backHeight;
    return this->backHeight;
}

// swd32.exe 0x00437C00: filled with the device colour of `color`
void FillSurfaceColor(Surface& surface, ColorRef color)
{
    surface.Fill(static_cast<Pixel>(MatchSurfaceColor(surface, color)));
}

// swd32.exe 0x00437C80: the device colour of `color` as GDI writes it at (0, 0), read back and the pixel restored;
// CLR_INVALID gives the current top-left pixel
u32 MatchSurfaceColor(Surface& surface, ColorRef color)
{
    Pixel* corner = surface.Pixels();

    if (color == kInvalidColor)
    {
        return *corner;
    }

    const Pixel saved = *corner;
    *corner = DevicePixel(color);
    const u32 result = *corner;
    *corner = saved;
    return result;
}

}  // namespace swd3
