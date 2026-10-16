#include "platform/message_box.h"

#include <cstdio>

namespace swd3
{

void ShowErrorMessage(std::string_view title, std::string_view text)
{
    std::fprintf(
        stderr,
        "swd3: %.*s: %.*s\n",
        static_cast<int>(title.size()),
        title.data(),
        static_cast<int>(text.size()),
        text.data()
    );
}

}  // namespace swd3

namespace swd3
{

void DialogErrorReporter::ShowError(std::string_view title, std::string_view text)
{
    ShowErrorMessage(title, text);
}

}  // namespace swd3

namespace swd3
{

void PlaySystemBeep()
{
}

}  // namespace swd3
