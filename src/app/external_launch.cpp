#include "openswd3/app/external_launch.hpp"

namespace openswd3::app {

compat::i32 open_url_with_legacy_result(
    const std::string_view target, ExternalLaunchPorts& ports
) {
    static_cast<void>(ports.open_url(target));
    return 0;
}

compat::i32 open_document_with_legacy_result(
    const std::string_view path, ExternalLaunchPorts& ports
) {
    static_cast<void>(ports.open_document(path));
    return 0;
}

}  // namespace openswd3::app
