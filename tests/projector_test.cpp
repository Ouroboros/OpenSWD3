#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "codec/lzo.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "game_data.h"
#include "golden/projector_scenarios.h"
#include "pic/picture.h"
#include "pic/picture_ops.h"
#include "pic/projector.h"
#include "platform/surface.h"
#include "test.h"

using golden::i32;
using golden::u16;
using golden::u32;
using golden::u64;
using golden::u8;

namespace
{

std::map<std::string, std::string> LoadGolden(const std::string& path)
{
    std::map<std::string, std::string> golden;
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line))
    {
        std::istringstream fields(line);
        std::string name;
        std::string hash;

        if (fields >> name >> hash)
        {
            golden[name] = hash;
        }
    }

    return golden;
}

class ProjectorBackend : public golden::ProjectorBackend
{
public:
    ProjectorBackend() : screen(640, 480)
    {
        this->painter.Init(this->screen);
    }

    void Reset(u32 seed) override
    {
        for (int y = 0; y < 480; ++y)
        {
            for (int x = 0; x < 640; ++x)
            {
                this->screen.Row(y)[x] = golden::Background(seed, x, y);
            }
        }

        this->projector.particleRange = 0;
    }

    void

    DrawScaled(const std::vector<u16>& pixels, int width, int height, const golden::ScaledScenario& scenario) override
    {
        swd3::ProjectedPicture picture;
        picture.pixels = pixels;
        picture.width = static_cast<u16>(width);
        picture.height = static_cast<u16>(height);
        picture.x = scenario.x;
        picture.y = scenario.y;
        picture.scaleX = scenario.scaleX;
        picture.scaleY = scenario.scaleY;
        picture.flags = scenario.flags;
        picture.angle = scenario.angle;
        this->projector.DrawScaled(this->screen, this->painter, picture);
    }

    std::vector<u8>

    Explosion(const std::vector<u16>& pixels, int width, int height, const golden::ExplosionScenario& scenario) override
    {
        std::vector<u16> data = pixels;
        swd3::ParticleEffect effect;
        effect.pixels = data;
        effect.width = static_cast<u16>(width);
        effect.height = static_cast<u16>(height);
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

        for (const u16 pixel : data)
        {
            if (pixel != golden::kFillKey && pixel != golden::kSkipKey)
            {
                ++effect.opaque;
            }
        }

        effect.slowLimit = effect.opaque / 20;
        effect.burstStep = effect.pixelCount / effect.bursts;
        this->random.Seed(scenario.seed);
        std::vector<u8> out;

        for (int frame = 0; frame < scenario.frames; ++frame)
        {
            const int result = this->projector.DrawExplosion(this->screen, this->painter, this->random, effect);
            golden::Append32(out, static_cast<u32>(result));
            golden::Append32(out, static_cast<u32>(effect.particles));
            golden::Append32(out, static_cast<u32>(effect.opaque));
            golden::Append32(out, effect.bursts);
            const u64 hash = golden::Hash(this->screen.Pixels(), 640 * 480 * 2);
            golden::Append32(out, static_cast<u32>(hash));
            golden::Append32(out, static_cast<u32>(hash >> 32));

            if (result == 1)
            {
                break;
            }
        }

        const u64 picture = golden::Hash(data.data(), data.size() * 2);
        golden::Append32(out, static_cast<u32>(picture));
        return out;
    }

    int HitTest(const golden::Frame& frame, int x, int y, int originX, int originY) override
    {
        swd3::Picture picture;
        picture.data = frame.rle;
        picture.width = static_cast<u16>(frame.width);
        picture.height = static_cast<u16>(frame.height);
        return swd3::PictureHitTest(picture, x, y, originX, originY);
    }

    std::vector<u8> Shift(const std::vector<u8>& data, int mode, int amount) override
    {
        std::vector<u8> copy = data;
        swd3::ShiftPicture(copy, static_cast<swd3::ShiftMode>(mode), amount);
        return copy;
    }

    std::vector<i32> Directions() override
    {
        std::vector<i32> out(this->projector.dirX.begin(), this->projector.dirX.end());
        out.insert(out.end(), this->projector.dirY.begin(), this->projector.dirY.end());
        return out;
    }

    std::vector<i32> Steps(int angle, int steps) override
    {
        swd3::DirectionWalker walker;
        walker.angle = angle;
        std::vector<i32> out;

        for (int i = 0; i < steps; ++i)
        {
            this->projector.Step(walker);
            out.push_back(walker.x);
            out.push_back(walker.y);
        }

        return out;
    }

    std::vector<i32> LineSteps(int fromX, int fromY, int toX, int toY, int steps) override
    {
        swd3::LineWalker walker;
        walker.fromX = fromX;
        walker.fromY = fromY;
        walker.toX = toX;
        walker.toY = toY;
        walker.x = fromX;
        walker.y = fromY;
        std::vector<i32> out;

        for (int i = 0; i < steps; ++i)
        {
            const bool reached = swd3::LineStep(walker);
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

    const u16* Screen() const override
    {
        return this->screen.Pixels();
    }

private:
    swd3::Surface screen;
    swd3::Painter painter;
    swd3::Projector projector;
    swd3::crt::Random random;
};

}  // namespace

TEST_CASE("projector: the colour keys are compared unconverted")
{
    swd3::Surface screen(640, 480);
    swd3::Painter painter;
    CHECK(painter.Init(screen));
    CHECK_EQ(painter.Rle().fill16, golden::kFillKey);
    CHECK_EQ(painter.Rle().skip16, golden::kSkipKey);
}

TEST_CASE("projector: the first explosion frame counts the opaque pixels")
{
    swd3::Surface screen(640, 480);
    swd3::Painter painter;
    CHECK(painter.Init(screen));
    swd3::Projector projector;
    swd3::crt::Random random;
    std::vector<u16> pixels(40 * 20, golden::kFillKey);

    for (int i = 0; i < 300; ++i)
    {
        pixels[static_cast<std::size_t>(i)] = 0x1234;
    }

    swd3::ParticleEffect effect;
    effect.pixels = pixels;
    effect.width = 40;
    effect.height = 20;
    effect.x = 100;
    effect.y = 100;
    effect.targetLeft = 400;
    effect.targetWidth = 50;
    effect.targetTop = 300;
    effect.targetHeight = 50;
    effect.lifeBase = 10;
    effect.speed = 3;
    effect.bursts = 2;
    effect.divisor = 1;
    CHECK_EQ(projector.DrawExplosion(screen, painter, random, effect), 0);
    CHECK_EQ(effect.started, 1);
    CHECK_EQ(effect.pixelCount, 800);
    CHECK_EQ(effect.slowLimit, 15);
    CHECK_EQ(effect.burstStep, 400);
    CHECK_EQ(effect.bursts, 1);
}

TEST_CASE("projector: matches the golden results of the legacy projector")
{
    const std::map<std::string, std::string> golden = LoadGolden(SWD3_SOURCE_DIR "/tests/golden/projector_golden.txt");

    auto decompress = [](const u8* input, std::size_t size, std::vector<u8>& out)
    {
        swd3::LzoDecoder decoder({input, size}, out);
        return decoder.Run() == swd3::LzoStatus::Ok && decoder.Written() == out.size();
    };

    auto gameFile = [](const std::string& name)
    {
        return swd3test::GameFile(name);
    };

    const std::vector<golden::Frame> frames = golden::LoadGoldenFrames(gameFile, decompress);

    if (frames.empty() || golden.empty())
    {
        std::printf("  (game data or golden file not found, skipped)\n");
        return;
    }

    ProjectorBackend backend;
    int compared = 0;
    int mismatches = 0;
    golden::RunProjector(
        backend,
        frames,
        [&](const std::string& name, u64 hash)
        {
            char text[17];
            std::snprintf(text, sizeof(text), "%016llx", static_cast<unsigned long long>(hash));
            const auto found = golden.find(name);
            ++compared;

            if (found == golden.end() || found->second != text)
            {
                if (++mismatches <= 20)
                {
                    std::printf("  mismatch: %s\n", name.c_str());
                }
            }
        }
    );
    std::printf("  compared %d scenarios\n", compared);
    CHECK_EQ(mismatches, 0);
    CHECK_EQ(compared, static_cast<int>(golden.size()));
}
