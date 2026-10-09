#include "openswd3/battle/legacy_battle_single_effect_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/resource_io/legacy_lzo1x.hpp"
#include "openswd3/rendering/legacy_image_command_stream.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

namespace {

using namespace openswd3;
using compat::u8;
using compat::u16;
using compat::u32;
using compat::i32;

void put_word(std::vector<u8>& bytes, std::size_t offset, u16 value) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void put_dword(std::vector<u8>& bytes, std::size_t offset, u32 value) {
    put_word(bytes, offset, static_cast<u16>(value));
    put_word(bytes, offset + 2U, static_cast<u16>(value >> 16U));
}

struct Assets {
    std::filesystem::path root =
        std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
        ("single-effect-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()
         ));

    Assets() {
        std::filesystem::create_directories(root);
        constexpr std::size_t block = 0x1CU + 3000U * 0x2CU;
        constexpr std::size_t descriptor = block + 12U + 512U;
        constexpr std::size_t payload = descriptor + 36U;
        const std::array<u8, 2> pixels{2U, 4U};
        const auto encoded =
            rendering::encode_legacy_image_command_stream(pixels, 2U, 1U, 8U);
        std::vector<u8> compressed(128U);
        const auto compression =
            resource_io::compress_legacy_lzo1x_14(encoded.bytes, compressed);
        compressed.resize(compression.bytes_written);
        std::vector<u8> image(payload + compressed.size());
        put_dword(image, 0x1CU + 0x14U, static_cast<u32>(image.size() - block));
        put_dword(image, 0x1CU + 0x18U, static_cast<u32>(block));
        put_word(image, block + 4U, 0xABCDU);
        put_word(image, block + 6U, 1U);
        put_word(image, block + 8U, 8U);
        put_word(image, block + 10U, 12U);
        put_word(image, block + 12U + 4U, 0x001FU);
        put_word(image, block + 12U + 8U, 0x03E0U);
        put_dword(image, descriptor, static_cast<u32>(payload - block));
        put_dword(image, descriptor + 4U, static_cast<u32>(compressed.size()));
        put_dword(
            image, descriptor + 8U, static_cast<u32>(encoded.bytes.size())
        );
        put_word(image, descriptor + 0x20U, 2U);
        put_word(image, descriptor + 0x22U, 1U);
        std::copy(
            compressed.begin(), compressed.end(), image.begin() + payload
        );
        for (const auto* name :
             {"all_char.tsw",
              "all_item.tsw",
              "all_magic.tsw",
              "all_sys.tsw",
              "all_map1.tsw",
              "all_map2.tsw"}) {
            write(name, image);
        }

        std::vector<u8> sound(block + 48U);
        put_dword(sound, 0x1CU + 0x14U, 48U);
        put_dword(sound, 0x1CU + 0x18U, static_cast<u32>(block));
        write("all.snd", sound);
    }

    ~Assets() {
        std::error_code ignored;
        std::filesystem::remove_all(root, ignored);
    }

    void write(const char* name, const std::vector<u8>& bytes) {
        std::ofstream output(root / name, std::ios::binary);
        output.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size())
        );
    }
};

class Stream final : public asset_runtime::LegacyActionStreamProvider {
public:
    asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        ++loads;
        return {
            .status = fail
                ? asset_runtime::LegacyActionStreamStatus::load_failed
                : asset_runtime::LegacyActionStreamStatus::ready,
            .stream = {}
        };
    }

    bool fail{};
    u32 loads{};
};

class Audio final : public audio_video::LegacySampleBackend {
public:
    u32 driver_token() const override {
        return 1U;
    }

    u32 allocate_sample_handle() override {
        return 1U;
    }

    void initialize_sample(u32) override {}

    void release_sample_handle(u32) override {}

    bool set_sample_file(u32, std::span<const u8> bytes) override {
        return !bytes.empty();
    }

    bool set_named_sample_file(
        u32, std::string_view, std::span<const u8> bytes, u32
    ) override {
        return !bytes.empty();
    }

    void set_sample_user_data(u32, u32 slot, u32 value) override {
        data.at(slot) = value;
    }

    u32 sample_user_data(u32, u32 slot) override {
        return data.at(slot);
    }

    void set_sample_volume(u32, i32 value) override {
        volume = value;
    }

    void set_sample_pan(u32, i32 value) override {
        pans.push_back(value);
    }

    void set_sample_loop_count(u32, i32) override {}

    void start_sample(u32) override {
        ++starts;
    }

    void end_sample(u32) override {}

    u32 sample_status(u32) override {
        return 4U;
    }

    void close_output() override {}

    std::array<u32, 16> data{};
    std::vector<i32> pans;
    i32 volume{};
    u32 starts{};
};

struct Fixture {
    Assets assets;
    Stream stream;
    asset_runtime::LegacyActionUpdater updater{stream};
    asset_runtime::LegacyTswRuntime images{assets.root};
    Audio audio;
    audio_video::LegacySndArchive sounds;
    audio_video::LegacySampleManager samples{audio, sounds};
    rendering::LegacyFramebuffer framebuffer;
    rendering::LegacyRasterGeometryState raster{};
    rendering::LegacyBlitRequest request{};
    rendering::LegacyBlitEffectState effects{};
    rendering::LegacyRleRowJitterState jitter{};
    battle::LegacyBattleSingleEffectFrameState state;
    std::unique_ptr<battle::LegacyBattleStartupState> startup =
        std::make_unique<battle::LegacyBattleStartupState>();
    u32 message{};

    Fixture() {
        static_cast<void>(sounds.open(assets.root / "all.snd"));
        static_cast<void>(samples.initialize_pool(1));
        static_cast<void>(rendering::initialize_legacy_raster_geometry(
            raster, framebuffer.geometry().surface
        ));
        startup->group_b_lifecycle = std::make_shared<
            std::array<battle::LegacyBattleActorGroupBElementState, 8>>();
        auto& actor = (*startup->group_b_lifecycle)[0].action_execution;
        actor.position_x = 100U;
        actor.position_y = 200U;
        auto& record = state.primary[0];
        record.action_id = 1U;
        record.cached_action_id = 1U;
        record.wait_remaining = 1U;
        record.field_4a = 1U;
        record.field_58 = 1U;
        state.sample_level = 11;
    }

    battle::LegacyBattleSingleEffectFrameContext context() {
        return {
            updater,
            &images,
            &samples,
            framebuffer,
            raster,
            request,
            effects,
            jitter
        };
    }

    battle::LegacyBattleSingleEffectFrameResult run(u32 slot = 0U) {
        return battle::advance_legacy_battle_single_effect_frame(
            state,
            context(),
            message,
            battle::kLegacyBattleActorCoordinatesGroupBBaseToken,
            1U,
            slot,
            {.startup = startup.get()}
        );
    }
};

}

void test_battle_single_effect_frame(openswd3::test::Context& test) {
    using battle::LegacyBattleSingleEffectFrameStatus;
    {
        Fixture fixture;
        const auto result = fixture.run(8U);
        test.expect_true(
            result.status ==
                    LegacyBattleSingleEffectFrameStatus::
                        slot_index_typed_stop &&
                fixture.stream.loads == 0U,
            "invalid slot stops before action update"
        );
    }

    {
        Fixture fixture;
        fixture.state.primary[0].field_8c = 1U;
        fixture.state.primary[0].field_5a = 0x8000U;
        fixture.state.battle_gate = 9U;
        const auto result = fixture.run();
        test.expect_true(
            result.finished && fixture.state.battle_gate == 0U &&
                fixture.message == 1U &&
                fixture.state.primary[0].action_id == 0U &&
                fixture.stream.loads == 0U,
            "signed status publishes the shared message before clearing the completed action record"
        );
    }

    {
        Fixture fixture;
        fixture.stream.fail = true;
        fixture.state.alternate[0].action_id = 8U;
        fixture.state.alternate_active[0] = 1U;
        const auto result = fixture.run();
        test.expect_true(
            result.status == LegacyBattleSingleEffectFrameStatus::completed &&
                result.finished && fixture.state.alternate[0].action_id == 0U &&
                fixture.state.alternate_active[0] == 0U &&
                fixture.audio.starts == 0U,
            "actual action stream load failure clears the alternate record and suppresses resource and sound work"
        );
    }

    {
        Fixture fixture;
        fixture.state.primary[0].field_4a = 0xFFFFU;
        const auto result = fixture.run();
        test.expect_true(
            result.status ==
                    LegacyBattleSingleEffectFrameStatus::
                        resource_owner_typed_stop &&
                fixture.request.source_token == 0U &&
                fixture.audio.starts == 0U &&
                fixture.state.retained_frames.size() == 1U,
            "stopped owned load retains its allocated record without publishing an image or playing sound"
        );
    }

    for (const bool flip : {false, true}) {
        Fixture fixture;
        fixture.state.global_flip_mode = flip ? 1U : 0U;
        const auto result = fixture.run();
        const auto pixels = fixture.framebuffer.row_pixels(200U);
        const auto left = flip ? 98U : 100U;
        test.expect_true(
            result.status == LegacyBattleSingleEffectFrameStatus::completed &&
                !result.finished && fixture.stream.loads == 1U &&
                fixture.state.primary[0].wait_remaining == 0U &&
                fixture.state.primary[0].field_58 == 0U &&
                fixture.state.retained_frames.empty() &&
                fixture.audio.starts == 1U && fixture.audio.volume == 127 &&
                !fixture.audio.pans.empty() &&
                fixture.audio.pans.back() == 47 &&
                pixels[left] == (flip ? 0x03E0U : 0x001FU) &&
                pixels[left + 1U] == (flip ? 0x001FU : 0x03E0U),
            "real action update, owned image load, left-panned audio, pixel drawing and resource destruction complete together"
        );
    }

    {
        Fixture fixture;
        auto& actor = (*fixture.startup->group_b_lifecycle)[0].action_execution;
        actor.render_x_base = 2U;
        actor.render_y_base = 3U;
        actor.position_x = 410U;
        actor.source_y_offset = 10U;
        actor.position_y = 25U;
        actor.target_phase_y_adjustment = 5;
        const auto result = fixture.run();
        test.expect_true(
            result.status == LegacyBattleSingleEffectFrameStatus::completed &&
                fixture.request.destination_x == 402 &&
                fixture.request.destination_y == 23 &&
                fixture.audio.pans.back() == 79 &&
                fixture.state.retained_frames.empty(),
            "nonzero render offsets use base coordinates and pan right at the original screen threshold"
        );
    }

    for (const u32 failure : {0U, 1U, 2U}) {
        Fixture fixture;
        auto& actor = (*fixture.startup->group_b_lifecycle)[0].action_execution;
        if (failure == 0U) {
            actor.render_x_base = 0x1234U;
            actor.render_y_base_read_accessible = false;
        } else if (failure == 1U) {
            actor.render_x_base = 2U;
            actor.render_y_base = 3U;
            actor.target_phase_y_adjustment_read_accessible = false;
        } else {
            actor.position_y_read_accessible = false;
        }

        const auto result = fixture.run();
        const std::array expected{
            LegacyBattleSingleEffectFrameStatus::actor_render_offset_typed_stop,
            LegacyBattleSingleEffectFrameStatus::
                actor_base_coordinate_typed_stop,
            LegacyBattleSingleEffectFrameStatus::actor_coordinate_typed_stop,
        };
        test.expect_true(
            result.status == expected[failure] && fixture.audio.starts == 0U &&
                fixture.state.primary[0].field_58 == 1U &&
                fixture.request.source_token != 0U &&
                fixture.state.retained_frames.size() == 1U &&
                !fixture.state.retained_frames[0]->primary_stream.empty(),
            "coordinate failure retains the loaded image and suppresses sound, drawing and release"
        );
    }
}
