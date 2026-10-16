#include "core/assertion.h"

#include <cstdlib>
#include <string>

#include "core/format.h"
#include "platform/message_box.h"

namespace swd3
{

namespace
{

// "\n檔案: %s\n行數: 第%u行\n錯誤: %s" and "維護錯誤" (swd32.exe 0x004A5DD0, 0x004A5DC4).
constexpr std::u8string_view kLayout = u8"\n檔案: %s\n行數: 第%u行\n錯誤: %s";
constexpr std::u8string_view kTitle = u8"維護錯誤";

}  // namespace

// swd32.exe 0x00431150
void MaintenanceAssert(ErrorReporter& reporter, std::u8string_view message, i32 value, std::string_view file, u32 line)
{
    const std::u8string text = Utf8Format(message, {value});
    const std::u8string box = Utf8Format(kLayout, {file, line, std::u8string_view(text)});

    if (reporter.AskAssertion(kTitle, box) != AssertChoice::Ignore)
    {
        std::abort();
    }
}

}  // namespace swd3
