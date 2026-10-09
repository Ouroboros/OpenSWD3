#include "openswd3/battle/legacy_battle_frame_coordinator.hpp"
#include "openswd3/battle/legacy_battle_frame_surface.hpp"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "test.hpp"

namespace {

using namespace openswd3::battle;
using openswd3::compat::u32;

struct SurfacePort final : LegacyBattleFrameSurfacePort {
    explicit SurfacePort(LegacyBattleFrameCoordinatorState& owner)
        : state(owner) {}

    LegacyBattleFrameCoordinatorState& state;
    std::optional<u32> locked_pixels{0xCAFE1234U};
    bool unlock_succeeds{true};
    std::function<void()> during_lock;
    std::function<void()> during_unlock;
    std::vector<u32> locked_surfaces;
    std::vector<std::pair<u32, u32>> unlocked_surfaces;
    bool publication_seen{};

    std::optional<u32> lock_frame_surface(const u32 surface) override {
        locked_surfaces.push_back(surface);
        if (during_lock) {
            during_lock();
        }

        return locked_pixels;
    }

    bool unlock_frame_surface(const u32 surface, const u32 pixels) override {
        unlocked_surfaces.emplace_back(surface, pixels);
        publication_seen = state.current_target_pointer_token == pixels;
        if (during_unlock) {
            during_unlock();
        }

        return unlock_succeeds;
    }
};

}  // namespace

void test_battle_frame_surface(openswd3::test::Context& test) {
    for (const auto pointer : {0U, 0xCAFE1234U, 0xFFFFFFFFU}) {
        for (const auto latch : {0U, 1U, 2U, 0xFFFFFFFFU}) {
            auto owner = std::make_unique<LegacyBattleFrameCoordinatorState>();
            auto& state = *owner;
            state.target_surface_token = 0x1000U;
            state.current_target_pointer_token = 0x1357U;
            state.render_abort_latch = 1U;
            state.active = 1U;
            SurfacePort port(state);
            port.locked_pixels = pointer;
            port.during_lock = [&] { state.target_surface_token = 0x2000U; };
            port.during_unlock = [&] {
                state.render_abort_latch = latch;
                state.active = 0x81234567U;
            };

            const auto result =
                prepare_legacy_battle_frame_surface(state, port);
            test.expect_true(
                port.locked_surfaces == std::vector<u32>{0x1000U} &&
                    port.unlocked_surfaces ==
                        std::vector<std::pair<u32, u32>>{{0x2000U, pointer}} &&
                    port.publication_seen &&
                    state.current_target_pointer_token == pointer &&
                    result.lock_calls == 1U && result.unlock_calls == 1U,
                "surface prefix reloads the surface and publishes even a zero pixel address before unlock"
            );
            test.expect_true(
                result.status ==
                        (latch == 1U
                             ? LegacyBattleFrameSurfaceStatus::render_aborted
                             : LegacyBattleFrameSurfaceStatus::
                                   continue_frame) &&
                    (latch != 1U || result.return_value == 0x81234567U),
                "surface prefix rereads exact-one abort and current active DWORD after unlock"
            );
        }
    }

    for (const bool stop_lock : {false, true}) {
        auto owner = std::make_unique<LegacyBattleFrameCoordinatorState>();
        auto& state = *owner;
        state.current_target_pointer_token = 0x1357U;
        state.render_abort_latch = 1U;
        state.active = 0xFFFFFFFFU;
        SurfacePort port(state);
        if (stop_lock) {
            port.locked_pixels.reset();
        }

        port.unlock_succeeds = false;
        const auto result = prepare_legacy_battle_frame_surface(state, port);
        test.expect_true(
            result.status ==
                    (stop_lock
                         ? LegacyBattleFrameSurfaceStatus::lock_stopped
                         : LegacyBattleFrameSurfaceStatus::unlock_stopped) &&
                result.lock_calls == 1U &&
                result.unlock_calls == (stop_lock ? 0U : 1U) &&
                result.return_value == 0U &&
                state.current_target_pointer_token ==
                    (stop_lock ? 0x1357U : *port.locked_pixels),
            "stopped surface calls retain only completed publication and never report the abort return"
        );
    }

    {
        auto owner = std::make_unique<LegacyBattleFrameCoordinatorState>();
        auto& state = *owner;
        SurfacePort port(state);
        port.during_unlock = [&] {
            state.current_target_pointer_token = 0xDEADBEEFU;
        };

        const auto result = prepare_legacy_battle_frame_surface(state, port);
        test.expect_true(
            result.status == LegacyBattleFrameSurfaceStatus::continue_frame &&
                state.current_target_pointer_token == 0xDEADBEEFU,
            "surface publication does not overwrite later callee writes"
        );
    }

    for (const auto pitch : {1280, 1344}) {
        openswd3::rendering::LegacyFramebuffer framebuffer(
            {.pitch_bytes = pitch, .width = 640, .height = 480}
        );
        LegacyBattleFramebufferSurface binding(framebuffer, 0x1234U);
        test.expect_true(
            binding.pixel_bytes(0U).empty() && binding.pitch_shadow() == 0 &&
                !binding.lock_frame_surface(0x9999U).has_value(),
            "unbound pixel addresses and unknown surfaces are not fabricated mappings"
        );
        const auto locked = binding.lock_frame_surface(0x1234U);
        test.expect_true(locked.has_value(), "known software surface locks");
        if (!locked.has_value()) {
            continue;
        }

        const auto bytes = binding.pixel_bytes(*locked);
        const bool unlocked = binding.unlock_frame_surface(0x1234U, *locked);
        test.expect_true(
            *locked != 0U && unlocked && binding.pitch_shadow() == pitch / 2 &&
                bytes.data() ==
                    std::as_writable_bytes(
                        framebuffer.physical_pixels_with_read_guard()
                    )
                        .data() &&
                bytes.size() == framebuffer.physical_byte_size() + 2U,
            "software surface publishes real framebuffer storage and measured pitch including its existing read guard"
        );
        if (!bytes.empty()) {
            bytes.front() = std::byte{0x5AU};
            test.expect_true(
                std::as_writable_bytes(framebuffer.physical_pixels()).front() ==
                        std::byte{0x5AU} &&
                    binding.lock_frame_surface(0x1234U) == locked &&
                    binding.pixel_bytes(*locked + 1U).data() ==
                        bytes.data() + 1U &&
                    binding.pixel_bytes(*locked - 1U).empty() &&
                    binding
                        .pixel_bytes(*locked + static_cast<u32>(bytes.size()))
                        .empty(),
                "unlock keeps the same writable framebuffer and bounded guest identity"
            );
        }

        test.expect_true(
            !binding.unlock_frame_surface(0x9999U, *locked) &&
                binding.unlock_frame_surface(0x1234U, 0U) &&
                binding.pitch_shadow() == pitch / 2,
            "software unlock accepts the original zero pointer boundary but rejects unknown surface identity"
        );

        auto owner = std::make_unique<LegacyBattleFrameCoordinatorState>();
        auto& state = *owner;
        state.target_surface_token = 0x1234U;
        state.render_abort_latch = 1U;
        state.active = 0xFEDCBA98U;
        const auto prepared =
            prepare_legacy_battle_frame_surface(state, binding);
        test.expect_true(
            prepared.status == LegacyBattleFrameSurfaceStatus::render_aborted &&
                prepared.return_value == 0xFEDCBA98U &&
                prepared.lock_calls == 1U && prepared.unlock_calls == 1U &&
                state.current_target_pointer_token == *locked &&
                binding.pixel_bytes(state.current_target_pointer_token)
                        .data() == bytes.data(),
            "production software binding and shared prefix publish the same actual framebuffer before abort return"
        );
        state.target_surface_token = 0x9999U;
        const auto missing =
            prepare_legacy_battle_frame_surface(state, binding);
        test.expect_true(
            missing.status == LegacyBattleFrameSurfaceStatus::lock_stopped &&
                missing.unlock_calls == 0U &&
                state.current_target_pointer_token == *locked,
            "unknown production surface stops before replacing the previous framebuffer publication"
        );
    }
}
