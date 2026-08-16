#include "test.hpp"
#include "world_role_runtime_adapter.hpp"

#include "openswd3/audio_video/legacy_sample_manager.hpp"
#include "openswd3/audio_video/legacy_snd_archive.hpp"
#include "openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/rendering/legacy_text_renderer_runtime.hpp"
#include "openswd3/world_map/legacy_world_path_script.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::asset_runtime::LegacyAniRoleParticleEffect;
using openswd3::asset_runtime::LegacyAniRoleParticlePorts;
using openswd3::asset_runtime::LegacyAniRoleParticlePositionPort;
using openswd3::asset_runtime::LegacyAniRoleParticleViewport;
using openswd3::audio_video::LegacySampleBackend;
using openswd3::audio_video::LegacySampleHandle;
using openswd3::audio_video::LegacySampleManager;
using openswd3::audio_video::LegacySampleManagerInitializeStatus;
using openswd3::audio_video::LegacySndArchive;
using openswd3::audio_video::LegacySndOpenStatus;
using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::input_time_rng::LegacySecondaryRng;
using openswd3::platform_sdl3::WorldRoleRuntimeAdapter;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyGlyphProvider;
using openswd3::rendering::LegacyGlyphProviderStatus;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacyRawCharacter;
using openswd3::rendering::LegacyTextRendererRuntime;
using openswd3::rendering::LegacyTextRendererRuntimeStatus;
using openswd3::world_map::LegacyWorldCameraRect;
using openswd3::world_map::LegacyWorldPathScriptState;
using openswd3::world_map::LegacyWorldRoleRecord;

constexpr std::size_t kSndIndexOffset = 0x1CU;
constexpr std::size_t kSndRecordSize = 0x2CU;
constexpr std::size_t kSndSlotCount = 3000U;
constexpr std::size_t kSndPayloadOffset =
    kSndIndexOffset + kSndSlotCount * kSndRecordSize;
constexpr std::size_t kSndPayloadSize = 48U;

void write_u32(
    const std::span<u8> bytes, const std::size_t offset, const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] std::vector<u8> synthetic_snd_archive() {
    std::vector<u8> bytes(kSndPayloadOffset + kSndPayloadSize, 0U);
    write_u32(
        bytes, kSndIndexOffset + 0x14U, static_cast<u32>(kSndPayloadSize)
    );
    write_u32(
        bytes, kSndIndexOffset + 0x18U, static_cast<u32>(kSndPayloadOffset)
    );
    for (std::size_t index = 0U; index < kSndPayloadSize; ++index) {
        bytes[kSndPayloadOffset + index] = static_cast<u8>(index + 1U);
    }
    return bytes;
}

class TestTree final {
public:
    TestTree() {
        const auto unique =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("world-role-runtime-adapter-" + std::to_string(unique));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] std::filesystem::path write_archive() const {
        const std::filesystem::path path = root_ / "all.snd";
        const std::vector<u8> bytes = synthetic_snd_archive();
        std::ofstream output{path, std::ios::binary | std::ios::trunc};
        for (const u8 byte : bytes) {
            output.put(static_cast<char>(byte));
        }
        return path;
    }

private:
    std::filesystem::path root_;
};

class RecordingSampleBackend final : public LegacySampleBackend {
public:
    [[nodiscard]] u32 driver_token() const override {
        return 1U;
    }
    [[nodiscard]] LegacySampleHandle allocate_sample_handle() override {
        return ++allocated_count;
    }
    void initialize_sample(LegacySampleHandle) override {}
    void release_sample_handle(LegacySampleHandle) override {}
    [[nodiscard]] bool
    set_sample_file(LegacySampleHandle, std::span<const u8>) override {
        return true;
    }
    [[nodiscard]] bool set_named_sample_file(
        LegacySampleHandle, std::string_view, std::span<const u8>, u32
    ) override {
        return true;
    }
    void set_sample_user_data(
        const LegacySampleHandle handle, u32, const u32 value
    ) override {
        user_data[handle] = value;
        last_sound_id = value;
    }
    [[nodiscard]] u32
    sample_user_data(const LegacySampleHandle handle, u32) override {
        return user_data[handle];
    }
    void set_sample_volume(LegacySampleHandle, const i32 value) override {
        volumes.push_back(value);
    }
    void set_sample_pan(LegacySampleHandle, const i32 value) override {
        pans.push_back(value);
    }
    void set_sample_loop_count(LegacySampleHandle, i32) override {}
    void start_sample(LegacySampleHandle) override {
        ++start_count;
    }
    void end_sample(LegacySampleHandle) override {}
    [[nodiscard]] u32 sample_status(LegacySampleHandle) override {
        return 4U;
    }
    void close_output() override {}

    u32 allocated_count{};
    std::array<u32, 8U> user_data{};
    u32 last_sound_id{};
    u32 start_count{};
    std::vector<i32> volumes;
    std::vector<i32> pans;
};

struct SampleFixture {
    SampleFixture() : manager(backend, archive) {
        open_status = archive.open(tree.write_archive());
        initialize_status = manager.initialize_pool(2);
    }

    TestTree tree;
    RecordingSampleBackend backend;
    LegacySndArchive archive;
    LegacySampleManager manager;
    LegacySndOpenStatus open_status{};
    LegacySampleManagerInitializeStatus initialize_status{};
};

class ParticlePorts final : public LegacyAniRoleParticlePorts {
public:
    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override {
        record.draw_offset_x = 3U;
        record.draw_offset_y = 4U;
        record.mode_flags = 5U;
        record.field_4a = 1U;
        record.field_4c = 2U;
        return LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool load_frame_piece(u16, u16, LegacyFramePiece&) override {
        return true;
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame_piece(
        const LegacyFramePiece&, i32, i32, u32, i32, i32, i32
    ) noexcept override {
        ++draw_count;
        return LegacyBlitExecutionStatus::completed;
    }

    u32 draw_count{};
};

class DirectPositions final : public LegacyAniRoleParticlePositionPort {
public:
    [[nodiscard]] bool resolve_role_position(
        const u16 selector, i16& world_x, i16& world_y
    ) override {
        if (selector != 9U) {
            return false;
        }
        world_x = 100;
        world_y = 200;
        return true;
    }
};

class SolidGlyphProvider final : public LegacyGlyphProvider {
public:
    [[nodiscard]] LegacyGlyphProviderStatus provide_glyph_mask(
        const LegacyRawCharacter&, i32, i32, const std::span<u8> destination
    ) noexcept override {
        std::ranges::fill(destination, 0xFFU);
        return LegacyGlyphProviderStatus::completed;
    }
};

void seed_particle_effect(LegacyAniRoleParticleEffect& effect) {
    auto& emitter = effect.emitters()[0U];
    emitter.field_08 = 24;
    emitter.role_selector = 9;
    const u32 token = effect.nodes().allocate_zeroed();
    emitter.head_token = token;
    auto* const node = effect.nodes().node(token);
    node->fixed_x_1_16 = 1600;
    node->world_y = 200;
    node->horizontal_step_1_16 = 1;
    node->vertical_step = -1;
    node->lifetime = 10;
}

[[nodiscard]] bool matching_particle_state(
    const LegacyAniRoleParticleEffect& left,
    const LegacyAniRoleParticleEffect& right
) {
    const auto& left_emitter = left.emitters()[0U];
    const auto& right_emitter = right.emitters()[0U];
    if (left_emitter.head_token != right_emitter.head_token ||
        left_emitter.world_x != right_emitter.world_x ||
        left_emitter.world_y != right_emitter.world_y ||
        left_emitter.flags != right_emitter.flags ||
        left.nodes().active_count() != right.nodes().active_count()) {
        return false;
    }
    const auto* const left_node = left.nodes().node(left_emitter.head_token);
    const auto* const right_node = right.nodes().node(right_emitter.head_token);
    return left_node != nullptr && right_node != nullptr &&
        left_node->fixed_x_1_16 == right_node->fixed_x_1_16 &&
        left_node->world_y == right_node->world_y &&
        left_node->horizontal_step_1_16 == right_node->horizontal_step_1_16 &&
        left_node->vertical_step == right_node->vertical_step &&
        left_node->lifetime == right_node->lifetime;
}

void expect_sample_fixture(
    openswd3::test::Context& test, const SampleFixture& fixture
) {
    test.expect_true(
        fixture.open_status == LegacySndOpenStatus::ready &&
            fixture.initialize_status ==
                LegacySampleManagerInitializeStatus::ready,
        "runtime adapter sample fixture is ready"
    );
}

void test_one_shot_sample_reaches_manager(openswd3::test::Context& test) {
    SampleFixture samples;
    expect_sample_fixture(test, samples);
    LegacyAniRoleParticleEffect effect;
    ParticlePorts particle_ports;
    LegacySecondaryRng random;
    LegacyActionRecord action{};
    LegacyTextRendererRuntime text;
    LegacyPixelConversionState conversion;
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[0U].world_x = 1000U;
    roles[0U].world_y = 1000U;
    LegacyWorldCameraRect camera{};
    WorldRoleRuntimeAdapter adapter{
        samples.manager,
        11,
        effect,
        7,
        camera,
        random,
        roles.data(),
        roles.size(),
        0U,
        action,
        particle_ports,
        nullptr,
        nullptr,
        nullptr,
        text,
        conversion,
    };

    roles[0U].world_x = 3U;
    roles[0U].world_y = 4U;
    static_cast<void>(adapter.play_positional_sample(1U, 3, 4));
    test.expect_true(
        samples.backend.start_count == 1U &&
            samples.backend.last_sound_id == 1U &&
            samples.manager.active_sample_count() == 1U,
        "ordinary-role one-shot positional sample reaches sample manager"
    );
}

void test_particle_adapter_matches_direct_update(
    openswd3::test::Context& test
) {
    SampleFixture samples;
    expect_sample_fixture(test, samples);
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1U].guid = 9U;
    roles[1U].world_x = 100U;
    roles[1U].world_y = 200U;
    LegacyAniRoleParticleEffect adapted_effect;
    LegacyAniRoleParticleEffect direct_effect;
    seed_particle_effect(adapted_effect);
    seed_particle_effect(direct_effect);
    ParticlePorts adapted_ports;
    ParticlePorts direct_ports;
    LegacySecondaryRng adapted_random;
    LegacySecondaryRng direct_random;
    adapted_random.seed(0x12345678U);
    direct_random.seed(0x12345678U);
    LegacyActionRecord adapted_action{};
    LegacyActionRecord direct_action{};
    LegacyTextRendererRuntime text;
    LegacyPixelConversionState conversion;
    constexpr LegacyAniRoleParticleViewport kViewport{
        .left = -1000, .top = -1000, .right = 1000, .bottom = 1000
    };
    LegacyWorldCameraRect camera{
        .left = 5000U, .top = 5000U, .right = 6000U, .bottom = 6000U
    };
    WorldRoleRuntimeAdapter adapter{
        samples.manager,
        11,
        adapted_effect,
        7,
        camera,
        adapted_random,
        roles.data(),
        roles.size(),
        0U,
        adapted_action,
        adapted_ports,
        nullptr,
        nullptr,
        nullptr,
        text,
        conversion,
    };
    DirectPositions positions;

    camera = {
        .left = static_cast<u32>(kViewport.left),
        .top = static_cast<u32>(kViewport.top),
        .right = static_cast<u32>(kViewport.right),
        .bottom = static_cast<u32>(kViewport.bottom),
    };
    const auto adapted = adapter.update_role_particles(100, 200, 9U);
    const auto direct = direct_effect.update(
        100,
        200,
        9U,
        7,
        kViewport,
        direct_random,
        positions,
        direct_action,
        direct_ports
    );
    test.expect_true(
        adapted.status == direct.status &&
            adapted.random_call_count == direct.random_call_count &&
            adapted.matching_emitter_count == direct.matching_emitter_count &&
            adapted.role_query_count == direct.role_query_count &&
            adapted.updated_node_count == direct.updated_node_count &&
            adapted.draw_count == direct.draw_count &&
            adapted_random.index() == direct_random.index() &&
            adapted_ports.draw_count == direct_ports.draw_count,
        "production particle adapter result matches direct seeded update"
    );
    test.expect_true(
        matching_particle_state(adapted_effect, direct_effect),
        "production particle adapter preserves direct seeded effect state"
    );
}

void test_label_color_and_12_point_framebuffer(openswd3::test::Context& test) {
    SampleFixture samples;
    expect_sample_fixture(test, samples);
    LegacyFramebuffer framebuffer;
    SolidGlyphProvider glyphs;
    LegacyTextRendererRuntime text;
    test.expect_equal(
        text.rebuild(12U, framebuffer, glyphs),
        LegacyTextRendererRuntimeStatus::completed,
        "12-point renderer is ready"
    );
    LegacyWorldPathScriptState paths;
    paths.role_label_payloads[0U] = {'A', 0U};
    LegacyAniRoleParticleEffect effect;
    ParticlePorts particle_ports;
    LegacySecondaryRng random;
    LegacyActionRecord action{};
    LegacyPixelConversionState conversion;
    std::array<LegacyWorldRoleRecord, 1U> roles{};
    LegacyWorldCameraRect camera{};
    WorldRoleRuntimeAdapter adapter{
        samples.manager,
        11,
        effect,
        7,
        camera,
        random,
        roles.data(),
        roles.size(),
        0U,
        action,
        particle_ports,
        nullptr,
        &paths,
        nullptr,
        text,
        conversion,
    };

    const auto label = adapter.resolve_label_bytes(1U);
    const u16 color = adapter.label_color(3U);
    adapter.draw_label(label.data, label.size - 1U, 10, 10, color, 4U);
    const std::span<const u16> pixels = framebuffer.physical_pixels();
    test.expect_true(
        color != 0U &&
            std::find(pixels.begin(), pixels.end(), color) != pixels.end(),
        "built-in label color and 12-point renderer reach framebuffer"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_one_shot_sample_reaches_manager(test);
    test_particle_adapter_matches_direct_update(test);
    test_label_color_and_12_point_framebuffer(test);
    return test.exit_code();
}
