#include "test.hpp"

#include "openswd3/app/platform_backend_initialization.hpp"

#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

namespace {

enum class Call {
    input,
    report_input,
    destroy,
    audio_start,
    audio_output,
    query_driver,
    midi,
    stream_nodes,
    display,
    report_display,
    source_surface,
    query_surface,
    video_audio,
};

class RecordingPorts final
    : public openswd3::app::PlatformBackendInitializationPorts {
public:
    explicit RecordingPorts(openswd3::app::PlatformBackendState& state)
        : state_(state) {}

    bool initialize_input_backend() override {
        calls.push_back(Call::input);
        return input_success;
    }

    void report_input_initialization_failure() override {
        calls.push_back(Call::report_input);
        input_flags_when_reported = state_.input_backend_flags;
    }

    void request_synchronous_destroy() override {
        calls.push_back(Call::destroy);
    }

    void start_audio_runtime(const std::string_view path) override {
        calls.push_back(Call::audio_start);
        first_path = path;
    }

    void initialize_audio_output(const std::string_view path) override {
        calls.push_back(Call::audio_output);
        second_path = path;
    }

    openswd3::app::BackendToken query_audio_driver() override {
        calls.push_back(Call::query_driver);
        return driver_results.at(driver_query_count++);
    }

    void
    initialize_midi_output(const openswd3::app::BackendToken driver) override {
        calls.push_back(Call::midi);
        midi_driver = driver;
    }

    void initialize_audio_stream_nodes(
        const openswd3::app::BackendToken driver
    ) override {
        calls.push_back(Call::stream_nodes);
        stream_driver = driver;
    }

    bool initialize_display_backend(
        const openswd3::app::DisplayInitializationRequest request
    ) override {
        calls.push_back(Call::display);
        display_request = request;
        return display_success;
    }

    void report_display_initialization_failure() override {
        calls.push_back(Call::report_display);
        process_flags_when_reported = state_.process_flags;
    }

    openswd3::app::BackendToken create_common_source_surface(
        const openswd3::compat::u32 width, const openswd3::compat::u32 height
    ) override {
        calls.push_back(Call::source_surface);
        source_width = width;
        source_height = height;
        return source_surface_result;
    }

    openswd3::app::BackendToken
    query_display_surface(const openswd3::compat::u32 selector) override {
        calls.push_back(Call::query_surface);
        display_selector = selector;
        return display_surface_result;
    }

    void configure_video_audio(
        const openswd3::app::BackendToken display_surface,
        const openswd3::compat::u8 use_direct_sound,
        const openswd3::app::BackendToken audio_driver
    ) override {
        calls.push_back(Call::video_audio);
        video_display_surface = display_surface;
        video_use_direct_sound = use_direct_sound;
        video_audio_driver = audio_driver;
    }

    openswd3::app::PlatformBackendState& state_;
    bool input_success{true};
    bool display_success{true};
    std::array<openswd3::app::BackendToken, 3> driver_results{11U, 22U, 33U};
    std::size_t driver_query_count{};
    openswd3::app::BackendToken source_surface_result{44U};
    openswd3::app::BackendToken display_surface_result{55U};
    std::vector<Call> calls;
    std::string_view first_path;
    std::string_view second_path;
    openswd3::compat::u32 input_flags_when_reported{};
    openswd3::compat::u32 process_flags_when_reported{};
    openswd3::app::BackendToken midi_driver{};
    openswd3::app::BackendToken stream_driver{};
    openswd3::app::DisplayInitializationRequest display_request{};
    openswd3::compat::u32 source_width{};
    openswd3::compat::u32 source_height{};
    openswd3::compat::u32 display_selector{};
    openswd3::app::BackendToken video_display_surface{};
    openswd3::compat::u8 video_use_direct_sound{};
    openswd3::app::BackendToken video_audio_driver{};
};

void test_success(openswd3::test::Context& test) {
    openswd3::app::PlatformBackendState state{0xFFFFFFFFU, 0x10U, 9U, 99U};
    RecordingPorts ports(state);

    test.expect_true(
        openswd3::app::run_platform_backend_initialization(
            "legacy-root", state, ports
        ),
        "all successful backends return true"
    );

    const std::vector<Call> expected{
        Call::input,
        Call::audio_start,
        Call::audio_output,
        Call::query_driver,
        Call::midi,
        Call::query_driver,
        Call::stream_nodes,
        Call::display,
        Call::source_surface,
        Call::query_driver,
        Call::query_surface,
        Call::video_audio,
    };
    test.expect_equal(ports.calls, expected, "successful assembly call order");
    test.expect_equal(state.input_backend_flags, 0U, "input flags clear first");
    test.expect_equal(
        state.process_flags, 0x10U, "success preserves process flags"
    );
    test.expect_equal(
        state.display_active, 1U, "display activates after source creation"
    );
    test.expect_equal(
        state.common_source_surface, 44U, "source token is stored even opaquely"
    );
    test.expect_equal(
        ports.first_path, std::string_view{"legacy-root"}, "audio startup path"
    );
    test.expect_equal(
        ports.second_path, std::string_view{"legacy-root"}, "audio output path"
    );
    test.expect_equal(
        ports.driver_query_count,
        std::size_t{3},
        "driver is queried three times"
    );
    test.expect_equal(ports.midi_driver, 11U, "first driver query feeds MIDI");
    test.expect_equal(
        ports.stream_driver, 22U, "second driver query feeds stream nodes"
    );
    test.expect_equal(
        ports.display_request,
        openswd3::app::DisplayInitializationRequest{0x4E22U, 640U, 480U, 16U},
        "legacy display request is exact"
    );
    test.expect_equal(ports.source_width, 640U, "source width");
    test.expect_equal(ports.source_height, 480U, "source height");
    test.expect_equal(
        ports.display_selector, 0x2711U, "primary surface selector"
    );
    test.expect_equal(
        ports.video_display_surface, 55U, "surface feeds video setup"
    );
    test.expect_equal(ports.video_use_direct_sound, 0U, "Miles path selected");
    test.expect_equal(
        ports.video_audio_driver, 33U, "third driver query feeds video"
    );
}

void test_input_failure(openswd3::test::Context& test) {
    openswd3::app::PlatformBackendState state{0x80U, 0x10U, 7U, 8U};
    RecordingPorts ports(state);
    ports.input_success = false;

    test.expect_false(
        openswd3::app::run_platform_backend_initialization(
            "root", state, ports
        ),
        "input failure returns false"
    );
    test.expect_equal(
        ports.calls,
        std::vector<Call>{Call::input, Call::report_input, Call::destroy},
        "input failure stops immediately"
    );
    test.expect_equal(
        state.input_backend_flags,
        0x02U,
        "input failure replaces old flags then sets bit two"
    );
    test.expect_equal(
        ports.input_flags_when_reported,
        0x02U,
        "input bit is visible before report"
    );
    test.expect_equal(
        state.process_flags,
        0x10U,
        "input failure does not set process bit four"
    );
    test.expect_equal(
        state.display_active, 7U, "input failure does not touch display state"
    );
    test.expect_equal(
        state.common_source_surface,
        8U,
        "input failure does not touch source token"
    );
}

void test_display_failure(openswd3::test::Context& test) {
    openswd3::app::PlatformBackendState state{9U, 0x11U, 7U, 8U};
    RecordingPorts ports(state);
    ports.display_success = false;

    test.expect_false(
        openswd3::app::run_platform_backend_initialization(
            "root", state, ports
        ),
        "display failure returns false"
    );
    const std::vector<Call> expected{
        Call::input,
        Call::audio_start,
        Call::audio_output,
        Call::query_driver,
        Call::midi,
        Call::query_driver,
        Call::stream_nodes,
        Call::display,
        Call::report_display,
        Call::destroy,
    };
    test.expect_equal(ports.calls, expected, "display failure order");
    test.expect_equal(
        state.input_backend_flags, 0U, "successful input leaves flags clear"
    );
    test.expect_equal(
        state.process_flags, 0x15U, "display failure ORs process bit four"
    );
    test.expect_equal(
        ports.process_flags_when_reported,
        0x15U,
        "process bit is visible before report"
    );
    test.expect_equal(
        state.display_active, 7U, "display failure does not overwrite activity"
    );
    test.expect_equal(
        state.common_source_surface, 8U, "display failure creates no source"
    );
    test.expect_equal(
        ports.driver_query_count,
        std::size_t{2},
        "display failure skips third driver query"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_success(test);
    test_input_failure(test);
    test_display_failure(test);
    return test.exit_code();
}
