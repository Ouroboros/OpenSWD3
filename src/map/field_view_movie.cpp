#include "map/field_view_internal.h"

namespace swd3
{

// The fields of a movie header, little-endian.
MovieHeader MovieHeader::Read(std::span<const u8, MovieHeader::Bytes> bytes)
{
    ByteReader reader(bytes);
    MovieHeader header;
    header.unk00 = reader.Word32();
    header.lastFrame = reader.Word32();
    header.bitsPerPixel = reader.Word16();
    header.unk0A = reader.Word16();
    header.unk0C = reader.Word16();
    header.active = reader.Word16();
    header.height = reader.Word16();
    header.unk12 = reader.Word16();
    header.paletteCount = reader.Word16();
    header.unk16 = reader.Word16();
    header.nextUnk0C = reader.Word32();
    header.nextRows = reader.Word32();
    header.nextSize = reader.Word32();
    return header;
}

u16 MapMovie::Rgb555(u8 red, u8 green, u8 blue)
{
    constexpr int kChannelBits = 5;
    constexpr int kDropped = CHAR_BIT - kChannelBits;
    return static_cast<u16>(
        (red >> kDropped) << (2 * kChannelBits) | (green >> kDropped) << kChannelBits | blue >> kDropped
    );
}

// swd32.exe 0x004157D0: the rows of the movie frame, centred vertically: 8-bit rows through the palette, 16-bit rows
// converted to the display in the frame itself (so a frame drawn again is converted again) and copied
void FieldView::DrawMovieFrame()
{
    Game& owner = this->game;
    MapMovie& film = this->movie;
    const int pitch = owner.painter.ScreenPitch();
    auto* screen = reinterpret_cast<u8*>(owner.drawSurface);
    const std::size_t screenBytes = sizeof(Pixel) * static_cast<std::size_t>(pitch) * Display::Height;
    const std::ptrdiff_t base =
        static_cast<std::ptrdiff_t>(sizeof(Pixel)) * pitch * ((Display::Height - film.height) >> 1);
    const u32 rows = film.current != nullptr ? film.current->rows : 0;
    std::size_t at = 0;

    auto dword = [&film](std::size_t offset) -> u32
    {
        return ByteReader::Word32At(film.frame, offset);
    };

    for (u32 row = 0; row < rows && static_cast<i32>(row) >= 0; ++row)
    {
        const u32 length = dword(at);
        const std::ptrdiff_t target = base + static_cast<std::ptrdiff_t>(dword(at + kRowTargetAt));

        if (length < kRowHeaderBytes || at + length > film.frame.size())
        {
            break;
        }

        const std::size_t count = length - kRowHeaderBytes;
        u8* pixels = film.frame.data() + at + kRowHeaderBytes;

        if (!film.palette.empty())
        {
            for (std::size_t index = 0; index < count; ++index)
            {
                const std::ptrdiff_t out = target + static_cast<std::ptrdiff_t>(sizeof(Pixel) * index);

                if (out >= 0 && static_cast<std::size_t>(out) + sizeof(Pixel) <= screenBytes)
                {
                    const u16 color = film.palette[pixels[index]];
                    std::memcpy(screen + out, &color, sizeof(Pixel));
                }
            }
        }
        else
        {
            std::vector<u16> converted(count / sizeof(Pixel));
            std::memcpy(converted.data(), pixels, converted.size() * sizeof(Pixel));
            owner.painter.ConvertPixels(converted);
            std::memcpy(pixels, converted.data(), converted.size() * sizeof(Pixel));

            if (target >= 0 && static_cast<std::size_t>(target) + count <= screenBytes)
            {
                std::memcpy(screen + target, pixels, count);
            }
        }

        at += length;
    }
}

// swd32.exe 0x004158C0: the chunk of the current frame: from the chunks read before, or read on from the last one
// (keeping every chunk; a palette due at this frame read from the file's end); past the last frame the screen is kept
// as the frame and 1 returned
int FieldView::ReadMovieFrame()
{
    Game& owner = this->game;
    MapMovie& film = this->movie;

    auto unpack = [&film](u32 bytes)
    {
        film.buffer.resize(std::max<std::size_t>(film.buffer.size(), bytes));
        MapCache::Unpack(std::span<const u8>(film.buffer.data(), bytes), film.frame);
    };

    MovieChunk* previous = &film.chunks;

    for (MovieChunk* chunk = film.chunks.next.get(); chunk != nullptr; chunk = chunk->next.get())
    {
        if (chunk->frame == film.frameIndex)
        {
            const u32 bytes = chunk->size;
            film.file.SeekBegin(static_cast<i32>(chunk->offset + MapMovie::FileHeaderBytes));

            if (bytes != 0)
            {
                owner.audioDevice.Serve();
                u32 read = bytes;
                film.buffer.resize(std::max<std::size_t>(film.buffer.size(), bytes));
                film.file.Read(film.buffer.data(), read);
                unpack(bytes);
            }

            film.current = chunk;
            return 0;
        }

        previous = chunk;
    }

    u32 offset = previous->offset + previous->size;
    i32 frame = previous->frame + 1;
    film.file.SeekBegin(static_cast<i32>(offset + MapMovie::FileHeaderBytes));
    MovieChunk* chunk = nullptr;

    for (;;)
    {
        if (frame > film.lastFrame)
        {
            const std::size_t bytes = std::min<std::size_t>(film.frame.size(), sizeof(Pixel) * Display::Pixels);
            std::memcpy(film.frame.data(), owner.drawSurface, bytes);
            return 1;
        }

        auto fresh = std::make_unique<MovieChunk>();
        fresh->offset = offset;
        fresh->unk0C = previous->nextUnk0C;
        fresh->rows = previous->nextRows;
        fresh->size = previous->nextSize;
        fresh->frame = frame;
        owner.audioDevice.Serve();
        u32 read = fresh->size;
        film.buffer.resize(std::max<std::size_t>(film.buffer.size(), fresh->size));
        film.file.Read(film.buffer.data(), read);

        if (film.paletteCount != 0 && film.paletteIndex < film.paletteFrames.size() &&
            film.frameIndex == film.paletteFrames[film.paletteIndex])
        {
            owner.audioDevice.Serve();
            const auto back = static_cast<i32>(
                MapMovie::PaletteBytes * (film.paletteCount - film.paletteIndex++) + MapMovie::PaletteTailBytes
            );
            std::array<u8, MapMovie::PaletteBytes> colors = {};
            film.file.SeekEnd(-back);
            u32 bytes = MapMovie::PaletteBytes;
            film.file.Read(colors.data(), bytes);
            film.palette.assign(MapMovie::PaletteColors, 0);

            for (std::size_t index = 0; index < MapMovie::PaletteColors; ++index)
            {
                const u8* color = colors.data() + MapMovie::PaletteBytes / MapMovie::PaletteColors * index;
                film.palette[index] =
                    MapMovie::Rgb555(color[MapMovie::RedAt], color[MapMovie::GreenAt], color[MapMovie::BlueAt]);
            }

            owner.painter.ConvertPixels(film.palette);
        }

        auto tail = [&film, &fresh](u32 back) -> u32
        {
            const std::size_t at = static_cast<std::size_t>(fresh->size) - back;
            return fresh->size >= back ? ByteReader::Word32At(film.buffer, at) : 0;
        };

        fresh->nextUnk0C = tail(MovieChunk::TrailerBytes);
        fresh->nextRows = tail(MovieChunk::TrailerBytes - sizeof(u32));
        fresh->nextSize = tail(sizeof(u32));
        offset += fresh->size;
        chunk = fresh.get();
        previous->next = std::move(fresh);
        previous = chunk;

        if (frame == film.frameIndex)
        {
            break;
        }

        ++frame;
    }

    const u32 packed = chunk->size - MovieChunk::TrailerBytes;

    if (packed != 0)
    {
        unpack(packed);
    }

    film.current = chunk;
    return 0;
}

// swd32.exe 0x004154A0: the movie over the map: while talk windows, lines or role heads show (or it is paused) the
// screen is kept as it was; it fades in with black bars closing, plays a frame a call, and fades out (darkening, or the
// map coming back between closing bars); at the end everything is freed and the frame rate restored. 1 while fading
// out.
int FieldView::UpdateMovie()
{
    Game& owner = this->game;
    MapMovie& film = this->movie;
    const i32 active = film.active;

    if (active == 0)
    {
        return 0;
    }

    auto* screen = reinterpret_cast<u8*>(owner.drawSurface);
    constexpr std::size_t kScreenBytes = sizeof(Pixel) * Display::Pixels;

    if (owner.scene.talkWindows != nullptr || owner.scene.effects != nullptr || owner.scene.spritesB != nullptr ||
        (film.options & MapMovie::Paused) != 0)
    {
        film.buffer.resize(std::max(film.buffer.size(), kScreenBytes));

        if (film.saved == 0)
        {
            std::memcpy(film.buffer.data(), screen, kScreenBytes);
            film.saved = 1;
            return 0;
        }

        std::memcpy(screen, film.buffer.data(), kScreenBytes);
        return 0;
    }

    if (film.saved == 1)
    {
        std::memcpy(screen, film.buffer.data(), std::min(film.buffer.size(), kScreenBytes));
        film.saved = 0;
        return 0;
    }

    const int pitch = owner.painter.ScreenPitch();

    if (film.frameIndex <= 0)
    {
        this->DrawMovieFrame();
        const int bar = MapMovie::BarStep * (film.frameIndex - MapMovie::FadeInStart);
        const auto bytes = static_cast<std::size_t>(std::max(bar, 0)) * sizeof(Pixel) * static_cast<std::size_t>(pitch);
        std::memset(screen, 0, std::min(bytes, kScreenBytes));

        if (bytes <= kScreenBytes)
        {
            std::memset(
                screen +
                    sizeof(Pixel) * static_cast<std::size_t>(pitch) * static_cast<std::size_t>(Display::Height - bar),
                0,
                bytes
            );
        }

        ++film.frameIndex;
        return 0;
    }

    if (film.frameIndex >= MapMovie::FadeOutStart)
    {
        if ((film.options & MapMovie::DarkenOut) != 0)
        {
            const int level = kDarkenStep * (MapMovie::FadeOutStart - film.frameIndex);
            owner.painter.TintPixels(std::span<u16>(owner.drawSurface, Display::Pixels), level, level, level);
        }
        else
        {
            film.active = 0;
            owner.scene.screenFlags &= ~Scene::BlackScreen;
            this->Paint();
            owner.scene.screenFlags |= Scene::BlackScreen;
            film.active = active;
            const auto bytes = static_cast<std::size_t>(std::max(
                0,
                kCloseRowsPerFrame * static_cast<int>(sizeof(Pixel)) * pitch *
                    (MapMovie::FadeOutStart + MapMovie::CloseFrames - film.frameIndex)
            ));
            std::memset(screen, 0, std::min(bytes, kScreenBytes));

            if (bytes <= kScreenBytes)
            {
                std::memset(screen + kScreenBytes - bytes, 0, bytes);
            }
        }

        ++film.frameIndex;
        const bool ended = (film.options & MapMovie::DarkenOut) != 0
                               ? film.frameIndex > MapMovie::FadeOutStart + MapMovie::DarkenFrames
                               : film.frameIndex > MapMovie::FadeOutStart + MapMovie::CloseFrames;

        if (ended)
        {
            film.chunks.next.reset();
            film.frame.clear();
            film.buffer.clear();
            film.palette.clear();
            film.paletteFrames.clear();
            film.chunks.offset = 0;
            film.chunks.size = 0;
            film.chunks.nextSize = 0;
            film.chunks.frame = 0;
            film.current = &film.chunks;
            film.height = 0;
            film.active = 0;
            film.frameIndex = 0;
            film.file.Close();
            owner.systemFlags &= ~Game::MovieOpened;
            owner.state.SetFrameInterval(Game::FrameInterval);
        }

        return 1;
    }

    if (this->ReadMovieFrame() != 0)
    {
        film.frameIndex = MapMovie::FadeOutStart;
    }
    else
    {
        this->DrawMovieFrame();
        ++film.frameIndex;
    }

    return 0;
}

}  // namespace swd3
