#include <string>
#include <string_view>

#include <SDL3/SDL_log.h>

#include "core/log.h"
#include "test.h"
#include "text/big5.h"

namespace
{

void Capture(void* data, int /*category*/, SDL_LogPriority /*priority*/, const char* message)
{
    *static_cast<std::string*>(data) = message;
}

// The line the log gets for `write`, with the SDL output caught.
template <typename Write>
std::string Logged(Write write)
{
    SDL_LogOutputFunction previous = nullptr;
    void* previousData = nullptr;
    SDL_GetLogOutputFunction(&previous, &previousData);
    std::string captured;
    SDL_SetLogOutputFunction(&Capture, &captured);
    SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);
    write();
    SDL_SetLogOutputFunction(previous, previousData);
    return captured;
}

bool Holds(const std::string& line, std::u8string_view text)
{
    return line.find(std::string_view(reinterpret_cast<const char*>(text.data()), text.size())) != std::string::npos;
}

}  // namespace

TEST_CASE("log: messages and game text arguments are written in UTF-8")
{
    const std::string plain = Logged(
        []
        {
            swd3::LogInfo(u8"載入地圖");
        }
    );
    CHECK(Holds(plain, u8"載入地圖"));

    const std::string formatted = Logged(
        []
        {
            swd3::LogInfo(u8"地圖 %s (%d)", swd3::Big5(u8"賽特"), 7);
        }
    );
    CHECK(Holds(formatted, u8"地圖 賽特 (7)"));

    const std::u8string directory = u8"/遊戲/swd3";
    const std::string path = Logged(
        [&directory]
        {
            const std::string_view bytes(reinterpret_cast<const char*>(directory.data()), directory.size());
            swd3::LogInfo(u8"路徑 %s", swd3::AsUtf8(bytes));
        }
    );
    CHECK(Holds(path, u8"路徑 /遊戲/swd3"));
}
