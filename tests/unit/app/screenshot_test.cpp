#include "test.hpp"

#include "openswd3/app/screenshot.hpp"

#include <vector>

namespace {

using openswd3::compat::u32;

enum class ScreenshotCall {
    beep,
    maintain_audio,
    open_existing,
    close_existing,
    save,
};

struct ScreenshotEvent {
    ScreenshotCall call{};
    u32 sequence{};

    bool operator==(const ScreenshotEvent&) const = default;
};

class RecordingScreenshotPorts final : public openswd3::app::ScreenshotPorts {
public:
    void beep() override {
        events.push_back({ScreenshotCall::beep, 0U});
    }

    void maintain_audio() override {
        events.push_back({ScreenshotCall::maintain_audio, 0U});
    }

    bool open_existing_numbered_bmp(const u32 sequence) override {
        events.push_back({ScreenshotCall::open_existing, sequence});
        return sequence < first_missing;
    }

    void close_existing_numbered_bmp() override {
        events.push_back({ScreenshotCall::close_existing, 0U});
    }

    void save_framebuffer_as_numbered_bmp(const u32 sequence) override {
        events.push_back({ScreenshotCall::save, sequence});
        saved_sequence = sequence;
    }

    u32 first_missing{};
    u32 saved_sequence{};
    std::vector<ScreenshotEvent> events;
};

void test_filename(openswd3::test::Context& test) {
    test.expect_equal(
        openswd3::app::make_legacy_screenshot_filename(0U),
        std::string{"00000.bmp"},
        "screenshot zero has five digits"
    );
    test.expect_equal(
        openswd3::app::make_legacy_screenshot_filename(42U),
        std::string{"00042.bmp"},
        "screenshot sequence is zero padded"
    );
    test.expect_equal(
        openswd3::app::make_legacy_screenshot_filename(99997U),
        std::string{"99997.bmp"},
        "last reachable sequence retains five digits"
    );
}

void test_first_slot(openswd3::test::Context& test) {
    RecordingScreenshotPorts ports;
    openswd3::app::capture_legacy_screenshot(ports);

    const std::vector<ScreenshotEvent> expected{
        {ScreenshotCall::beep, 0U},
        {ScreenshotCall::maintain_audio, 0U},
        {ScreenshotCall::open_existing, 0U},
        {ScreenshotCall::save, 0U},
    };
    test.expect_equal(ports.events, expected, "first screenshot call order");
}

void test_scan_existing_slots(openswd3::test::Context& test) {
    RecordingScreenshotPorts ports;
    ports.first_missing = 2U;
    openswd3::app::capture_legacy_screenshot(ports);

    const std::vector<ScreenshotEvent> expected{
        {ScreenshotCall::beep, 0U},
        {ScreenshotCall::maintain_audio, 0U},
        {ScreenshotCall::open_existing, 0U},
        {ScreenshotCall::close_existing, 0U},
        {ScreenshotCall::maintain_audio, 0U},
        {ScreenshotCall::open_existing, 1U},
        {ScreenshotCall::close_existing, 0U},
        {ScreenshotCall::maintain_audio, 0U},
        {ScreenshotCall::open_existing, 2U},
        {ScreenshotCall::save, 2U},
    };
    test.expect_equal(
        ports.events,
        expected,
        "each existing screenshot is closed before the next audio service"
    );
}

void test_probe_limit_bug(openswd3::test::Context& test) {
    RecordingScreenshotPorts ports;
    ports.first_missing = openswd3::app::kLegacyScreenshotProbeLimit;
    openswd3::app::capture_legacy_screenshot(ports);

    test.expect_equal(
        ports.saved_sequence,
        openswd3::app::kLegacyScreenshotProbeLimit - 1U,
        "full legacy range overwrites the last probed filename"
    );
    test.expect_equal(
        ports.events.size(),
        static_cast<std::size_t>(
            2U + 3U * openswd3::app::kLegacyScreenshotProbeLimit
        ),
        "probe limit preserves beep, per-slot service/open/close, and final save"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_filename(test);
    test_first_slot(test);
    test_scan_existing_slots(test);
    test_probe_limit_bug(test);
    return test.exit_code();
}
