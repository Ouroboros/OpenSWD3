// Golden generator for the projector and the picture operations: runs tests/golden/projector_scenarios.h through the
// legacy transliteration and writes "name hash" lines to $GOLDEN_OUT (see README.md). Buffers are padded where the
// original reads or writes past a picture (see projector_scenarios.h).

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "codec/lzo.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "ddraw_stub.h"
#include "draw/draw.h"
#include "golden/projector_scenarios.h"
#include "pic/pic_ops.h"
#include "pic/pixel_format.h"
#include "pic/projector.h"
#include "test.h"

using namespace swd3;

namespace
{

std::vector<std::uint16_t> g_screen(640 * 480);

struct Screen565 : swd3test::StubSurface
{
    HRESULT Lock(RECT*, DDSURFACEDESC2* desc, DWORD, HANDLE) override
    {
        desc->dwWidth = 640;
        desc->dwHeight = 480;
        desc->lPitch = 1280;
        desc->lpSurface = g_screen.data();
        return DD_OK;
    }

    HRESULT Unlock(LPVOID) override
    {
        return DD_OK;
    }

    HRESULT GetPixelFormat(DDPIXELFORMAT* format) override
    {
        format->dwRBitMask = 0xF800;
        format->dwGBitMask = 0x7E0;
        format->dwBBitMask = 0x1F;
        return DD_OK;
    }
};

// Zero-initialised static storage, like the original's global at 0x0053B0B8.
Projector g_projector;

class LegacyProjector : public golden::ProjectorBackend
{
public:
    void Reset(golden::u32 seed) override
    {
        for (int y = 0; y < 480; ++y)
        {
            for (int x = 0; x < 640; ++x)
            {
                g_screen[static_cast<std::size_t>(y) * 640 + x] = golden::Background(seed, x, y);
            }
        }

        G.particleRange = 0;
    }

    void DrawScaled(const std::vector<golden::u16>& pixels, int width, int height,
                    const golden::ScaledScenario& scenario) override
    {
        std::vector<std::uint16_t> padded = pixels;
        padded.push_back(golden::kSkipKey);
        ProjectedPicture picture{};
        picture.pixels = padded.data();
        picture.width = static_cast<std::uint16_t>(width);
        picture.height = static_cast<std::uint16_t>(height);
        picture.x = scenario.x;
        picture.y = scenario.y;
        picture.scaleX = scenario.scaleX;
        picture.scaleY = scenario.scaleY;
        picture.flags = scenario.flags;
        picture.angle = scenario.angle;
        g_projector.DrawScaled(g_screen.data(), &picture);
    }

    std::vector<golden::u8> Explosion(const std::vector<golden::u16>& pixels, int width, int height,
                                      const golden::ExplosionScenario& scenario) override
    {
        std::vector<std::uint16_t> data = pixels;
        data.resize(pixels.size() + static_cast<std::size_t>(width) + 2, golden::kFillKey);
        ParticleEffect effect{};
        effect.pixels = data.data();
        effect.width = static_cast<std::uint16_t>(width);
        effect.height = static_cast<std::uint16_t>(height);
        effect.x = scenario.x;
        effect.y = scenario.y;
        effect.targetLeft = scenario.targetLeft;
        effect.targetWidth = scenario.targetWidth;
        effect.targetTop = scenario.targetTop;
        effect.targetHeight = scenario.targetHeight;
        effect.lifeBase = scenario.lifeBase;
        effect.speed = scenario.speed;
        effect.bursts = scenario.bursts;
        effect.divisor = scenario.divisor;
        effect.flags = scenario.flags;
        effect.started = 1;
        effect.pixelCount = width * height;

        for (std::size_t i = 0; i < pixels.size(); ++i)
        {
            if (pixels[i] != golden::kFillKey && pixels[i] != golden::kSkipKey)
            {
                ++effect.opaque;
            }
        }

        effect.slowLimit = effect.opaque / 20;
        effect.burstStep = effect.pixelCount / effect.bursts;
        crt::srand(scenario.seed);
        std::vector<golden::u8> out;

        for (int frame = 0; frame < scenario.frames; ++frame)
        {
            const int result = g_projector.DrawExplosion(g_screen.data(), &effect);
            golden::Append32(out, static_cast<golden::u32>(result));
            golden::Append32(out, static_cast<golden::u32>(effect.particles));
            golden::Append32(out, static_cast<golden::u32>(effect.opaque));
            golden::Append32(out, effect.bursts);
            const golden::u64 hash = golden::Hash(g_screen.data(), 640 * 480 * 2);
            golden::Append32(out, static_cast<golden::u32>(hash));
            golden::Append32(out, static_cast<golden::u32>(hash >> 32));

            if (result == 1)
            {
                break;
            }
        }

        while (effect.head != nullptr)
        {
            Particle* next = effect.head->next;
            crt::free(effect.head);
            effect.head = next;
        }

        const golden::u64 picture = golden::Hash(data.data(), pixels.size() * 2);
        golden::Append32(out, static_cast<golden::u32>(picture));
        return out;
    }

    int HitTest(const golden::Frame& frame, int x, int y, int originX, int originY) override
    {
        std::vector<std::uint8_t> padded = frame.rle;
        padded.resize(frame.rle.size() + 0x10000, 0);
        Picture picture{};
        picture.pixels = reinterpret_cast<std::uint16_t*>(padded.data());
        picture.width = static_cast<std::uint16_t>(frame.width);
        picture.height = static_cast<std::uint16_t>(frame.height);
        return PictureHitTest(&picture, x, y, originX, originY);
    }

    std::vector<golden::u8> Shift(const std::vector<golden::u8>& data, int mode, int amount) override
    {
        std::vector<golden::u8> copy = data;
        ShiftPicture(reinterpret_cast<std::uint16_t*>(copy.data()), mode, amount);
        return copy;
    }

    std::vector<golden::i32> Directions() override
    {
        std::vector<golden::i32> out(g_projector.m_dirX, g_projector.m_dirX + 360);
        out.insert(out.end(), g_projector.m_dirY, g_projector.m_dirY + 360);
        return out;
    }

    std::vector<golden::i32> Steps(int angle, int steps) override
    {
        DirectionWalker walker{};
        walker.angle = angle;
        std::vector<golden::i32> out;

        for (int i = 0; i < steps; ++i)
        {
            g_projector.Step(&walker);
            out.push_back(walker.x);
            out.push_back(walker.y);
        }

        return out;
    }

    std::vector<golden::i32> LineSteps(int fromX, int fromY, int toX, int toY, int steps) override
    {
        LineWalker walker{};
        walker.fromX = fromX;
        walker.fromY = fromY;
        walker.toX = toX;
        walker.toY = toY;
        walker.x = fromX;
        walker.y = fromY;
        std::vector<golden::i32> out;

        for (int i = 0; i < steps; ++i)
        {
            const bool reached = LineStep(&walker);
            out.push_back(walker.x);
            out.push_back(walker.y);
            out.push_back(reached ? 1 : 0);

            if (reached)
            {
                break;
            }
        }

        return out;
    }

    const golden::u16* Screen() const override
    {
        return g_screen.data();
    }
};

}  // namespace

TEST_CASE("golden: legacy projector")
{
    const char* output = std::getenv("GOLDEN_OUT");
    const char* gameDir = std::getenv("SWD3_GAME_DIR");

    if (output == nullptr || gameDir == nullptr)
    {
        std::printf("  (GOLDEN_OUT / SWD3_GAME_DIR not set, skipped)\n");
        return;
    }

    static Screen565 surface;
    DrawInit(&surface);
    auto decompress = [](const std::uint8_t* input, std::size_t size, std::vector<std::uint8_t>& out)
    {
        std::vector<std::uint8_t> buffer(out.size() + 0x10000);
        std::uint32_t length = 0;
        const int status = Lzo1xDecompress(input, static_cast<std::uint32_t>(size), buffer.data(), &length);

        if (status != 0 || length != out.size())
        {
            return false;
        }

        std::copy_n(buffer.begin(), out.size(), out.begin());
        return true;
    };
    auto gameFile = [gameDir](const std::string& name)
    {
        return std::string(gameDir) + "/" + name;
    };
    const std::vector<golden::Frame> frames = golden::LoadGoldenFrames(gameFile, decompress);
    std::printf("  fill %04x skip %04x\n", static_cast<unsigned>(G.rleFillColor16), static_cast<unsigned>(G.rleSkipColor16));
    std::FILE* file = std::fopen(output, "w");
    LegacyProjector backend;
    golden::RunProjector(
        backend,
        frames,
        [file](const std::string& name, golden::u64 hash)
        {
            std::fprintf(file, "%s %016llx\n", name.c_str(), static_cast<unsigned long long>(hash));
            std::fflush(file);
        }
    );
    std::fclose(file);
    PixelFormatInit(nullptr);
    std::printf("  frames: %zu\n", frames.size());
}
