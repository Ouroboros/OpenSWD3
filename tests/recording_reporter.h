#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "platform/message_box.h"

namespace swd3test
{

// Records the error dialogs a module would show instead of opening them.
class RecordingReporter : public swd3::ErrorReporter
{
public:
    void ShowError(std::string_view title, std::string_view text) override
    {
        this->messages.emplace_back(std::string(title) + ": " + std::string(text));
    }

    swd3::AssertChoice AskAssertion(std::string_view title, std::string_view text) override
    {
        this->messages.emplace_back(std::string(title) + ": " + std::string(text));
        return swd3::AssertChoice::Ignore;
    }

    std::vector<std::string> messages;
};

}  // namespace swd3test
