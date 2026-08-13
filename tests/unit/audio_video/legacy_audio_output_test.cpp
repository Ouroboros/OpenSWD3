#include "test.hpp"

#include "openswd3/audio_video/legacy_audio_output.hpp"

#include <deque>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using openswd3::audio_video::kLegacySampleCountPreference;
using openswd3::audio_video::kLegacyWavePreference;
using openswd3::audio_video::LegacyAudioOutputBackend;
using openswd3::audio_video::LegacyAudioOutputStatus;
using openswd3::audio_video::LegacyPcmOutputFormat;
using openswd3::compat::i32;

enum class Call {
    set_preference,
    preference,
    open,
    configuration,
    last_error,
    close,
};

struct Event {
    Call call{};
    i32 index{};
    i32 value{};
    LegacyPcmOutputFormat format{};

    friend bool operator==(const Event&, const Event&) = default;
};

class RecordingOutput final : public LegacyAudioOutputBackend {
public:
    void set_preference(const i32 index, const i32 value) override {
        events.push_back({Call::set_preference, index, value});
        if (index == kLegacyWavePreference) {
            wave_preference = value;
        }
    }

    i32 preference(const i32 index) override {
        events.push_back({Call::preference, index});
        if (index == kLegacyWavePreference) {
            return wave_preference;
        }
        return sample_count_preference;
    }

    bool open_output(const LegacyPcmOutputFormat& format) override {
        events.push_back({Call::open, 0, 0, format});
        if (open_results.empty()) {
            return false;
        }
        const bool result = open_results.front();
        open_results.pop_front();
        return result;
    }

    std::string_view output_configuration() override {
        events.push_back({Call::configuration});
        return configuration;
    }

    std::string_view last_error() const override {
        const_cast<RecordingOutput*>(this)->events.push_back(
            {Call::last_error}
        );
        return error;
    }

    void close_output() override {
        events.push_back({Call::close});
    }

    std::deque<bool> open_results;
    std::vector<Event> events;
    std::string configuration{"SDL default output"};
    std::string error{"open failed"};
    i32 wave_preference{};
    i32 sample_count_preference{32};
};

[[nodiscard]] LegacyPcmOutputFormat
format(const i32 sample_rate, const i32 bits) {
    const i32 bytes_per_sample = bits / 8;
    const i32 block_align = bytes_per_sample * 2;
    return LegacyPcmOutputFormat{
        1U,
        2U,
        static_cast<openswd3::compat::u32>(sample_rate),
        static_cast<openswd3::compat::u32>(sample_rate * block_align),
        static_cast<openswd3::compat::u16>(block_align),
        static_cast<openswd3::compat::u16>(bits),
    };
}

void test_initial_success(openswd3::test::Context& test) {
    RecordingOutput backend;
    backend.open_results.push_back(true);
    const auto result =
        openswd3::audio_video::initialize_legacy_audio_output(backend);

    test.expect_equal(
        result.status, LegacyAudioOutputStatus::ready, "initial output opens"
    );
    test.expect_equal(
        result.selected_format, format(44'100, 16), "initial PCM format"
    );
    test.expect_equal(
        result.sample_handle_count,
        16,
        "preference one minus eight is capped at sixteen"
    );
    const std::vector<Event> expected{
        {Call::set_preference, 15, 0},
        {Call::open, 0, 0, format(44'100, 16)},
        {Call::configuration},
        {Call::preference, 15},
        {Call::preference, 1},
    };
    test.expect_equal(
        backend.events, expected, "initial success follows the LST call order"
    );
}

void test_emulated_retry(openswd3::test::Context& test) {
    RecordingOutput backend;
    backend.configuration = "Primary Sound Driver (Emulated)";
    backend.sample_count_preference = 5;
    backend.open_results = {true, true};
    const auto result =
        openswd3::audio_video::initialize_legacy_audio_output(backend);

    test.expect_equal(
        result.status,
        LegacyAudioOutputStatus::ready,
        "emulated output succeeds on forced retry"
    );
    test.expect_equal(
        result.sample_handle_count,
        -3,
        "negative handle count is not lower-clamped"
    );
    const std::vector<Event> expected{
        {Call::set_preference, 15, 0},
        {Call::open, 0, 0, format(44'100, 16)},
        {Call::configuration},
        {Call::preference, 15},
        {Call::close},
        {Call::set_preference, 15, 1},
        {Call::set_preference, 15, 1},
        {Call::open, 0, 0, format(44'100, 16)},
        {Call::configuration},
        {Call::preference, 15},
        {Call::preference, 1},
    };
    test.expect_equal(
        backend.events,
        expected,
        "Emulated path closes and writes preference twice"
    );
}

void test_failure_retry_and_recovery(openswd3::test::Context& test) {
    RecordingOutput backend;
    backend.sample_count_preference = 20;
    backend.error = "first open failed";
    backend.open_results = {false, true};
    const auto result =
        openswd3::audio_video::initialize_legacy_audio_output(backend);

    test.expect_equal(
        result.status,
        LegacyAudioOutputStatus::ready,
        "preference retry recovers"
    );
    test.expect_equal(
        result.selected_format,
        format(44'100, 16),
        "first retry keeps the same format"
    );
    test.expect_equal(
        result.sample_handle_count,
        12,
        "successful preference one is reduced by eight"
    );
    test.expect_equal(
        result.last_error,
        std::string{"first open failed"},
        "earlier error text remains in the legacy object"
    );
}

void test_complete_fallback_ladder(openswd3::test::Context& test) {
    RecordingOutput backend;
    backend.open_results = {
        false,
        false,
        false,
        false,
        false,
        false,
        false,
    };
    const auto result =
        openswd3::audio_video::initialize_legacy_audio_output(backend);

    test.expect_equal(
        result.status,
        LegacyAudioOutputStatus::output_open_failed,
        "exhausted ladder fails"
    );
    std::vector<LegacyPcmOutputFormat> attempts;
    for (const Event& event : backend.events) {
        if (event.call == Call::open) {
            attempts.push_back(event.format);
        }
    }
    const std::vector<LegacyPcmOutputFormat> expected{
        format(44'100, 16),
        format(44'100, 16),
        format(22'050, 16),
        format(11'025, 16),
        format(44'100, 8),
        format(22'050, 8),
        format(11'025, 8),
    };
    test.expect_equal(
        attempts,
        expected,
        "fallback ladder exactly matches 0x00485A20-0x00485B51"
    );
    test.expect_equal(
        result.last_error,
        std::string{"open failed"},
        "last backend error is retained"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_initial_success(test);
    test_emulated_retry(test);
    test_failure_retry_and_recovery(test);
    test_complete_fallback_ladder(test);
    return test.exit_code();
}
