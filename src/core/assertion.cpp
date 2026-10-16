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
constexpr char kLayout[] = "\n\300\311\256\327: %s\n\246\346\274\306: \262\304%u\246\346\n\277\371\273~: %s";
constexpr char kTitle[] = "\272\373\305@\277\371\273~";

}  // namespace

// swd32.exe 0x00431150
void MaintenanceAssert(ErrorReporter& reporter, std::string_view message, i32 value, std::string_view file, u32 line)
{
    const std::string text = WinFormat(message, {value});
    const std::string box = WinFormat(kLayout, {file, line, std::string_view(text)});

    if (reporter.AskAssertion(kTitle, box) != AssertChoice::Ignore)
    {
        std::abort();
    }
}

}  // namespace swd3
