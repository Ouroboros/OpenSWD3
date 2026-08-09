#include "test.hpp"

#include "openswd3/audio_video/legacy_video.hpp"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace {

using openswd3::audio_video::ImmediateCompleteLegacyVideoBackend;
using openswd3::audio_video::LegacyVideoBackend;
using openswd3::audio_video::LegacyVideoBeginStatus;
using openswd3::audio_video::LegacyVideoCopyRequest;
using openswd3::audio_video::LegacyVideoFramePorts;
using openswd3::audio_video::LegacyVideoHandle;
using openswd3::audio_video::LegacyVideoOpenDisposition;
using openswd3::audio_video::LegacyVideoOpenResult;
using openswd3::audio_video::LegacyVideoPixelFormat;
using openswd3::audio_video::LegacyVideoPlayer;
using openswd3::audio_video::LegacyVideoStepStatus;
using openswd3::audio_video::build_legacy_video_path;
using openswd3::audio_video::legacy_bink_filename;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;

enum class BackendCall {
    open,
    set_volume,
    wait,
    decode,
    copy,
    frame_count,
    frame_number,
    advance,
    service,
    close,
};

struct BackendEvent {
    BackendCall call{};
    u32 handle{};
    i32 value{};

    bool operator==(const BackendEvent&) const = default;
};

class RecordingBackend final : public LegacyVideoBackend {
public:
    LegacyVideoOpenResult open_video(
        const std::string_view filename
    ) override {
        last_filename = filename;
        events.push_back({BackendCall::open, open_result.handle});
        return open_result;
    }

    std::string_view last_error() const override {
        return error_text;
    }

    void close_video(const LegacyVideoHandle handle) override {
        events.push_back({BackendCall::close, handle});
    }

    void set_video_volume(
        const LegacyVideoHandle handle,
        const i32 volume
    ) override {
        events.push_back({BackendCall::set_volume, handle, volume});
    }

    bool wait_for_video_frame(const LegacyVideoHandle handle) override {
        events.push_back({BackendCall::wait, handle});
        return wait_result;
    }

    void decode_video_frame(const LegacyVideoHandle handle) override {
        events.push_back({BackendCall::decode, handle});
    }

    i32 copy_video_frame(
        const LegacyVideoHandle handle,
        const LegacyVideoCopyRequest& request
    ) override {
        events.push_back({BackendCall::copy, handle, copy_result});
        last_copy_request = request;
        return copy_result;
    }

    u32 video_frame_count(const LegacyVideoHandle handle) override {
        events.push_back({BackendCall::frame_count, handle});
        return frame_count;
    }

    u32 video_frame_number(const LegacyVideoHandle handle) override {
        events.push_back({BackendCall::frame_number, handle});
        return frame_number;
    }

    void advance_video_frame(const LegacyVideoHandle handle) override {
        events.push_back({BackendCall::advance, handle});
    }

    void service_video(const LegacyVideoHandle handle) override {
        events.push_back({BackendCall::service, handle});
    }

    void clear_events() {
        events.clear();
    }

    LegacyVideoOpenResult open_result{
        .disposition = LegacyVideoOpenDisposition::opened,
        .handle = 7U,
        .summary = {320, 200},
    };
    bool wait_result{};
    i32 copy_result{1};
    u32 frame_count{10U};
    u32 frame_number{3U};
    std::string error_text{"decoder open failure"};
    std::string last_filename;
    LegacyVideoCopyRequest last_copy_request{};
    std::vector<BackendEvent> events;
};

class RecordingFramePorts final : public LegacyVideoFramePorts {
public:
    std::span<u16> video_destination_pixels() override {
        return pixels;
    }

    i32 video_destination_pitch_bytes() override {
        return 1280;
    }

    LegacyVideoPixelFormat video_pixel_format() override {
        return format;
    }

    void report_video_copy_failure() override {
        ++copy_failure_count;
    }

    bool present_video_frame() override {
        ++present_count;
        return present_result;
    }

    std::array<u16, 640U * 480U> pixels{};
    LegacyVideoPixelFormat format{LegacyVideoPixelFormat::rgb565};
    bool present_result{true};
    u32 copy_failure_count{};
    u32 present_count{};
};

void test_open_volume_and_failure(openswd3::test::Context& test) {
    RecordingBackend backend;
    LegacyVideoPlayer player(backend);

    test.expect_equal(
        player.begin("swd3\\video\\title.bik", -1),
        LegacyVideoBeginStatus::playing,
        "negative legacy volume still opens and mutes"
    );
    test.expect_equal(
        backend.last_filename,
        std::string{"swd3\\video\\title.bik"},
        "filename reaches decoder"
    );
    test.expect_equal(
        backend.events,
        std::vector<BackendEvent>{
            {BackendCall::open, 7U},
            {BackendCall::set_volume, 7U, 0},
        },
        "open precedes clamped volume"
    );
    test.expect_equal(
        player.summary(),
        openswd3::audio_video::LegacyVideoSummary{320, 200},
        "decoder summary is retained"
    );
    test.expect_true(player.active(), "successful open is active");
    test.expect_equal(
        player.begin("second.bik", 1),
        LegacyVideoBeginStatus::failed,
        "active global video is not overwritten"
    );
    test.expect_true(player.close(), "first close releases active handle");
    test.expect_false(player.close(), "second close returns zero");

    backend.clear_events();
    test.expect_equal(
        player.begin("volume.bik", 0x9000),
        LegacyVideoBeginStatus::playing,
        "second open succeeds after close"
    );
    test.expect_equal(
        backend.events,
        std::vector<BackendEvent>{
            {BackendCall::open, 7U},
            {BackendCall::set_volume, 7U, 0x8000},
        },
        "positive volume clamps at 0x8000"
    );
    static_cast<void>(player.close());

    backend.clear_events();
    backend.open_result = {
        .disposition = LegacyVideoOpenDisposition::failed,
    };
    test.expect_equal(
        player.begin("missing.bik", 10),
        LegacyVideoBeginStatus::failed,
        "decoder failure remains a failure"
    );
    test.expect_equal(
        player.last_error(),
        std::string_view{"decoder open failure"},
        "decoder error is retained"
    );
    test.expect_equal(
        backend.events,
        std::vector<BackendEvent>{{BackendCall::open, 0U}},
        "failed open does not set volume"
    );
}

void test_legacy_video_path(openswd3::test::Context& test) {
    test.expect_equal(
        legacy_bink_filename("opening.avi"),
        std::string{"opening.bik"},
        "avi suffix is rewritten to bik"
    );
    test.expect_equal(
        legacy_bink_filename("scene.mpg"),
        std::string{"scene.bik"},
        "mpg suffix is rewritten to bik"
    );
    test.expect_equal(
        legacy_bink_filename("a.avi.b.mpg"),
        std::string{"a.bik.b.bik"},
        "legacy code rewrites the first occurrence of both old extensions"
    );
    test.expect_equal(
        legacy_bink_filename("OPENING.AVI"),
        std::string{"OPENING.AVI"},
        "extension search remains case sensitive"
    );
    test.expect_equal(
        build_legacy_video_path("E:/Game/swd3", "opening.avi"),
        std::filesystem::path{"E:/Game/swd3"} / "Video" / "opening.bik",
        "configured data root replaces the old root plus swd3 prefix"
    );
}

void test_immediate_completion_placeholder(openswd3::test::Context& test) {
    ImmediateCompleteLegacyVideoBackend backend;
    LegacyVideoPlayer player(backend);
    RecordingFramePorts ports;

    test.expect_equal(
        player.begin("swd3\\video\\opening.bik", 0x4000),
        LegacyVideoBeginStatus::completed,
        "deferred decoder completes request immediately"
    );
    test.expect_false(player.active(), "placeholder never sets active video");
    test.expect_equal(
        player.legacy_progress(),
        -1,
        "placeholder has no active progress"
    );
    const auto step = player.step(ports);
    test.expect_equal(
        step.status,
        LegacyVideoStepStatus::inactive,
        "placeholder does not enter frame service"
    );
    test.expect_equal(ports.present_count, 0U, "placeholder presents nothing");
}

void test_wait_and_active_progress(openswd3::test::Context& test) {
    RecordingBackend backend;
    LegacyVideoPlayer player(backend);
    RecordingFramePorts ports;
    static_cast<void>(player.begin("movie.bik", 100));
    backend.clear_events();

    backend.wait_result = true;
    const auto waiting = player.step(ports);
    test.expect_equal(
        waiting.status,
        LegacyVideoStepStatus::waiting,
        "nonzero wait skips decode and presentation"
    );
    test.expect_equal(
        backend.events,
        std::vector<BackendEvent>{{BackendCall::wait, 7U}},
        "wait path returns without decoder or progress calls"
    );
    test.expect_equal(
        waiting.legacy_progress,
        -1,
        "wait path does not synthesize a progress query"
    );
    test.expect_equal(ports.present_count, 0U, "wait path does not present");

    backend.clear_events();
    backend.frame_number = 11U;
    test.expect_equal(
        player.legacy_progress(),
        -11,
        "frame beyond count negates with legacy sign"
    );
    test.expect_equal(
        backend.events,
        std::vector<BackendEvent>{
            {BackendCall::frame_number, 7U},
            {BackendCall::frame_count, 7U},
        },
        "progress reads frame then count"
    );
    static_cast<void>(player.close());
}

void test_decode_copy_advance_and_complete(
    openswd3::test::Context& test
) {
    RecordingBackend backend;
    LegacyVideoPlayer player(backend);
    RecordingFramePorts ports;
    static_cast<void>(player.begin("movie.bik", 100));
    backend.clear_events();
    backend.copy_result = 0;

    const auto frame = player.step(ports);
    test.expect_equal(
        frame.status,
        LegacyVideoStepStatus::frame_presented,
        "frame count above current advances and remains active"
    );
    test.expect_equal(frame.legacy_progress, 10, "positive count continues");
    test.expect_equal(frame.copy_result, 0, "raw copy result is preserved");
    test.expect_true(
        frame.presentation_succeeded,
        "copy result does not suppress the legacy presentation"
    );
    test.expect_equal(
        backend.events,
        std::vector<BackendEvent>{
            {BackendCall::wait, 7U},
            {BackendCall::decode, 7U},
            {BackendCall::copy, 7U, 0},
            {BackendCall::frame_count, 7U},
            {BackendCall::frame_number, 7U},
            {BackendCall::advance, 7U},
            {BackendCall::service, 7U},
        },
        "decode-copy-count-frame-advance-service order matches LST"
    );
    test.expect_equal(
        backend.last_copy_request.destination.data(),
        ports.pixels.data(),
        "copy targets the provided software surface"
    );
    test.expect_equal(
        backend.last_copy_request.destination.size(),
        ports.pixels.size(),
        "copy receives the complete physical destination"
    );
    test.expect_equal(
        backend.last_copy_request.pitch_bytes,
        1280,
        "copy receives the legacy surface pitch"
    );
    test.expect_equal(
        backend.last_copy_request.destination_height,
        480,
        "copy height remains the fixed legacy canvas"
    );
    test.expect_equal(
        backend.last_copy_request.destination_x,
        160,
        "video is horizontally centered"
    );
    test.expect_equal(
        backend.last_copy_request.destination_y,
        140,
        "video is vertically centered"
    );
    test.expect_equal(
        backend.last_copy_request.pixel_format,
        LegacyVideoPixelFormat::rgb565,
        "pixel format is supplied by the surface port"
    );
    test.expect_equal(ports.present_count, 1U, "one frame is presented");
    test.expect_equal(
        ports.copy_failure_count,
        1U,
        "zero copy result reaches the legacy failure branch"
    );
    test.expect_true(player.active(), "positive progress keeps handle active");

    backend.clear_events();
    backend.frame_number = 10U;
    const auto completed = player.step(ports);
    test.expect_equal(
        completed.status,
        LegacyVideoStepStatus::completed,
        "count not above current completes after presentation"
    );
    test.expect_equal(completed.legacy_progress, -10, "count is negated");
    test.expect_equal(
        backend.events,
        std::vector<BackendEvent>{
            {BackendCall::wait, 7U},
            {BackendCall::decode, 7U},
            {BackendCall::copy, 7U, 0},
            {BackendCall::frame_count, 7U},
            {BackendCall::frame_number, 7U},
            {BackendCall::service, 7U},
            {BackendCall::close, 7U},
        },
        "terminal frame services, presents, then closes without advance"
    );
    test.expect_equal(ports.present_count, 2U, "terminal frame is presented");
    test.expect_equal(
        ports.copy_failure_count,
        2U,
        "terminal zero copy result also reports failure"
    );
    test.expect_false(player.active(), "terminal frame clears active handle");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_legacy_video_path(test);
    test_open_volume_and_failure(test);
    test_immediate_completion_placeholder(test);
    test_wait_and_active_progress(test);
    test_decode_copy_advance_and_complete(test);
    return test.exit_code();
}
