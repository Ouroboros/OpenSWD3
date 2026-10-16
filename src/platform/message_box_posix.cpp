#include "platform/message_box.h"

#include <cstdio>

namespace swd3
{

void ShowErrorMessage(std::u8string_view title, std::u8string_view text)
{
    std::fprintf(
        stderr,
        "swd3: %.*s: %.*s\n",
        static_cast<int>(title.size()),
        reinterpret_cast<const char*>(title.data()),
        static_cast<int>(text.size()),
        reinterpret_cast<const char*>(text.data())
    );
}

void DialogErrorReporter::ShowError(std::u8string_view title, std::u8string_view text)
{
    ShowErrorMessage(title, text);
}

AssertChoice DialogErrorReporter::AskAssertion(std::u8string_view title, std::u8string_view text)
{
    ShowErrorMessage(title, text);
    return AssertChoice::Ignore;
}

void PlaySystemBeep()
{
}

}  // namespace swd3
