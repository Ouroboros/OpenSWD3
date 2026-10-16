#include "data/image.h"
#include "core/static_init.h"
#include "pic/pixel_format.h"

namespace swd3::image {

using swd3::PixelConvertNone;
namespace {

struct CodeTarget {
    std::uint32_t va;
    void* host;
};

// Functions whose original address is stored in the data image (function pointer tables, callbacks).
const CodeTarget kTargets[] = {
    {0x0044A240u, reinterpret_cast<void*>(&PixelConvertNone)},
    {0x00401000u, reinterpret_cast<void*>(&InitMemoryManager)},
    {0x00401040u, reinterpret_cast<void*>(&InitAudioSystem)},
    {0x00401080u, reinterpret_cast<void*>(&InitSoundEffects)},
    {0x004010C0u, reinterpret_cast<void*>(&InitMidiPlayer)},
    {0x00401100u, reinterpret_cast<void*>(&InitStreamPlayer)},
    {0x00401140u, reinterpret_cast<void*>(&InitMusicQueue)},
    {0x00401290u, reinterpret_cast<void*>(&InitFile4B74A0)},
    {0x004012D0u, reinterpret_cast<void*>(&InitMapCacheFile)},
    {0x00401310u, reinterpret_cast<void*>(&InitFile4CAD50)},
    {0x00401350u, reinterpret_cast<void*>(&InitFile4B9F18)},
    {0x00401390u, reinterpret_cast<void*>(&InitFile4A98C8)},
    {0x004013D0u, reinterpret_cast<void*>(&InitFile4CAA28)},
    {0x00401410u, reinterpret_cast<void*>(&InitFile4B7AD0)},
    {0x00401450u, reinterpret_cast<void*>(&InitFile4A9380)},
    {0x004272C0u, reinterpret_cast<void*>(&InitFile4CF6E0)},
    {0x00451860u, reinterpret_cast<void*>(&InitBattleUnit)},
    {0x004518A0u, reinterpret_cast<void*>(&InitProjector)},
    {0x00451900u, reinterpret_cast<void*>(&InitFile521538)},
    {0x004011D0u, reinterpret_cast<void*>(&InitTextFont4AB998)},
    {0x00401210u, reinterpret_cast<void*>(&InitTextFont4C9A28)},
    {0x00401250u, reinterpret_cast<void*>(&InitTextFont4A9ED0)},
    {0x00401490u, reinterpret_cast<void*>(&InitGraphics)},
    {0x004014B0u, reinterpret_cast<void*>(&InitInput)},
};

[[noreturn]] void trap() { unimplementedCode(0); }

}  // namespace

void* codeTarget(std::uint32_t target) {
    for (const CodeTarget& t : kTargets) {
        if (t.va == target && t.host) {
            return t.host;
        }
    }
    return reinterpret_cast<void*>(&trap);
}

}  // namespace swd3::image
