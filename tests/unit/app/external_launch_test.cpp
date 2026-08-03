#include "test.hpp"

#include "openswd3/app/external_launch.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace {

enum class LaunchKind {
    url,
    document,
};

struct LaunchEvent {
    LaunchKind kind{};
    std::string value;

    bool operator==(const LaunchEvent&) const = default;
};

class RecordingPorts final : public openswd3::app::ExternalLaunchPorts {
public:
    bool open_url(const std::string_view target) override {
        events.push_back({LaunchKind::url, std::string{target}});
        return result;
    }

    bool open_document(const std::string_view path) override {
        events.push_back({LaunchKind::document, std::string{path}});
        return result;
    }

    bool result{};
    std::vector<LaunchEvent> events;
};

void test_ignored_host_result(openswd3::test::Context& test) {
    for (const bool host_result : {false, true}) {
        RecordingPorts ports;
        ports.result = host_result;
        test.expect_equal(
            openswd3::app::open_url_with_legacy_result(
                "www.softstar.com.tw",
                ports
            ),
            0,
            "URL wrapper returns zero for either host result"
        );
        test.expect_equal(
            openswd3::app::open_document_with_legacy_result(
                "Readme.txt",
                ports
            ),
            0,
            "document wrapper returns zero for either host result"
        );
        const std::vector<LaunchEvent> expected{
            {LaunchKind::url, "www.softstar.com.tw"},
            {LaunchKind::document, "Readme.txt"},
        };
        test.expect_equal(
            ports.events,
            expected,
            "legacy wrappers forward each input exactly once"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_ignored_host_result(test);
    return test.exit_code();
}
