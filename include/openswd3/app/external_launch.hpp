#pragma once

#include "openswd3/compat/types.hpp"

#include <string_view>

namespace openswd3::app {

class ExternalLaunchPorts {
public:
    virtual ~ExternalLaunchPorts() = default;

    [[nodiscard]] virtual bool open_url(std::string_view target) = 0;
    [[nodiscard]] virtual bool open_document(std::string_view path) = 0;
};

[[nodiscard]] compat::i32 open_url_with_legacy_result(
    std::string_view target,
    ExternalLaunchPorts& ports
);

[[nodiscard]] compat::i32 open_document_with_legacy_result(
    std::string_view path,
    ExternalLaunchPorts& ports
);

}  // namespace openswd3::app
