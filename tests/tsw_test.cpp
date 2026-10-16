#include <cstdio>
#include <fstream>
#include <map>
#include <string>

#include "core/types.h"
#include "draw/painter.h"
#include "game_data.h"
#include "golden/tsw_scenarios.h"
#include "pic/rle.h"
#include "platform/surface.h"
#include "recording_reporter.h"
#include "res/tsw_archive.h"
#include "test.h"

using swd3::u32;
using swd3::u64;

namespace
{

golden::PictureResult Result(const swd3::Picture* picture)
{
    golden::PictureResult result;

    if (picture == nullptr)
    {
        return result;
    }

    result.present = true;
    result.data = picture->data;
    result.width = picture->width;
    result.height = picture->height;
    result.paletteEntries = picture->palette.size();
    return result;
}

class ArchiveBackend : public golden::TswBackend
{
public:
    explicit ArchiveBackend(swd3::TswArchive& archive) : archive(archive)
    {
    }

    void SetCacheLimit(int bytes) override
    {
        this->archive.SetCacheLimit(bytes);
    }

    golden::PictureResult Get(u32 id, u32 frame) override
    {
        return Result(this->archive.GetPicture(id, frame));
    }

    golden::PictureResult Find(u32 id, u32 frame) override
    {
        return Result(this->archive.FindPicture(id, frame));
    }

    golden::PictureResult Direct(u32 id, u32 frame) override
    {
        const auto picture = this->archive.LoadPictureDirect(id, frame);
        return Result(picture.get());
    }

    int CacheBytes() override
    {
        return this->archive.CacheBytes();
    }

    void CloseAll() override
    {
        this->archive.CloseAll();
    }

private:
    swd3::TswArchive& archive;
};

std::map<std::string, std::string> LoadGolden(const std::string& path)
{
    std::map<std::string, std::string> entries;
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line))
    {
        const std::size_t space = line.rfind(' ');

        if (space != std::string::npos)
        {
            entries[line.substr(0, space)] = line.substr(space + 1);
        }
    }

    return entries;
}

}  // namespace

TEST_CASE("tsw: matches the golden results of the legacy archives")
{
    const std::map<std::string, std::string> golden = LoadGolden(SWD3_SOURCE_DIR "/tests/golden/tsw_golden.txt");

    if (golden.empty() || swd3test::ReadGameFile("all_sys.act").empty())
    {
        std::printf("  (game data or golden file not found, skipped)\n");
        return;
    }

    swd3::Surface screen(640, 480);
    swd3::Painter painter;
    painter.Init(screen);
    const swd3::RleCodec codec(painter);
    swd3test::RecordingReporter errors;
    swd3::TswArchive archive(codec, errors);
    archive.SetGameDirectory(swd3test::GameDirectory());
    ArchiveBackend backend(archive);
    int compared = 0;
    int mismatches = 0;
    golden::RunTsw(
        backend,
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

TEST_CASE("tsw: a frame past the end of an entry reports an error instead of failing the allocation")
{
    if (swd3test::ReadGameFile("all_sys.tsw").empty())
    {
        std::printf("  (game data not found, skipped)\n");
        return;
    }

    swd3::Surface screen(640, 480);
    swd3::Painter painter;
    painter.Init(screen);
    const swd3::RleCodec codec(painter);
    swd3test::RecordingReporter errors;
    swd3::TswArchive archive(codec, errors);
    archive.SetGameDirectory(swd3test::GameDirectory());
    CHECK(archive.GetPicture(9024, 0) != nullptr);
    CHECK(archive.GetPicture(9024, 1) == nullptr);
    CHECK(!errors.messages.empty());
}
