#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "platform/message_box.h"

namespace swd3test
{

// Records the error dialogs a module would show instead of opening them (as UTF-8 bytes in a std::string).
class RecordingReporter : public swd3::ErrorReporter
{
public:
    void ShowError(std::u8string_view title, std::u8string_view text) override
    {
        this->messages.emplace_back(Bytes(title) + ": " + Bytes(text));
    }

    swd3::AssertChoice AskAssertion(std::u8string_view title, std::u8string_view text) override
    {
        this->messages.emplace_back(Bytes(title) + ": " + Bytes(text));
        return swd3::AssertChoice::Ignore;
    }

    std::vector<std::string> messages;

private:
    static std::string Bytes(std::u8string_view text)
    {
        return std::string(reinterpret_cast<const char*>(text.data()), text.size());
    }
};

}  // namespace swd3test
