#pragma once

#include "openswd3/battle/legacy_battle_frame_refresh.hpp"

#include <deque>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace openswd3::test {

struct FrameRefreshColorChange {
    compat::u32 pixels{};
    compat::u32 pixel_count{};
    compat::i32 amount{};
};

struct FrameRefreshBackgroundDraw {
    compat::u32 pixels{};
    compat::u32 source{};
    compat::u32 width{};
    compat::u32 height{};
};

class LegacyBattleFrameRefreshFixture
    : public virtual battle::LegacyBattleFrameRefreshStatePort {
public:
    std::array<compat::u32, 5> refresh_background{};
    rendering::LegacyBlitRequest refresh_blit{};
    compat::u32 refresh_pixels{};
    std::size_t refresh_source_queries{};
    bool refresh_source_available{true};
    std::optional<std::size_t> refresh_stop_ordinal;
    std::vector<std::string> refresh_events;
    std::deque<compat::u32> refresh_lock_results;
    std::vector<compat::u32> refresh_locks;
    std::vector<std::pair<compat::u32, compat::u32>> refresh_unlocks;
    std::vector<FrameRefreshBackgroundDraw> refresh_draws;
    std::vector<FrameRefreshColorChange> refresh_red;
    std::vector<FrameRefreshColorChange> refresh_green;
    std::vector<FrameRefreshColorChange> refresh_blue;
    std::function<void()> on_refresh_audio;
    std::function<void(compat::u32)> on_refresh_lock;
    std::function<void(compat::u32, compat::u32)> on_refresh_unlock;
    std::function<void(const FrameRefreshBackgroundDraw&)> on_refresh_draw;
    std::function<void(const FrameRefreshColorChange&)> on_refresh_red;
    std::function<void(const FrameRefreshColorChange&)> on_refresh_green;
    std::function<void(const FrameRefreshColorChange&)> on_refresh_blue;

    std::optional<battle::LegacyBattleFrameRefreshSource>
    frame_refresh_source() noexcept override {
        ++refresh_source_queries;
        if (!refresh_source_available) {
            return std::nullopt;
        }

        return battle::LegacyBattleFrameRefreshSource{
            refresh_background, refresh_blit, refresh_pixels
        };
    }

    bool serve_refresh_audio() override {
        if (!record_refresh_event("audio")) {
            return false;
        }

        if (on_refresh_audio) {
            on_refresh_audio();
        }

        return true;
    }

    std::optional<compat::u32>
    lock_frame_surface(compat::u32 surface) override {
        refresh_locks.push_back(surface);
        if (!record_refresh_event("lock")) {
            return std::nullopt;
        }

        if (on_refresh_lock) {
            on_refresh_lock(surface);
        }

        if (refresh_lock_results.empty()) {
            return 0U;
        }

        const auto pixels = refresh_lock_results.front();
        refresh_lock_results.pop_front();
        return pixels;
    }

    bool
    unlock_frame_surface(compat::u32 surface, compat::u32 pixels) override {
        refresh_unlocks.emplace_back(surface, pixels);
        if (!record_refresh_event("unlock")) {
            return false;
        }

        if (on_refresh_unlock) {
            on_refresh_unlock(surface, pixels);
        }

        return true;
    }

    bool draw_refresh_background(
        compat::u32 pixels,
        compat::u32 source,
        compat::u32 width,
        compat::u32 height
    ) override {
        refresh_draws.push_back({pixels, source, width, height});
        if (!record_refresh_event("background")) {
            return false;
        }

        if (on_refresh_draw) {
            on_refresh_draw(refresh_draws.back());
        }

        return true;
    }

    bool apply_refresh_red(
        compat::u32 pixels, compat::u32 count, compat::i32 amount
    ) override {
        refresh_red.push_back({pixels, count, amount});
        if (!record_refresh_event("red")) {
            return false;
        }

        if (on_refresh_red) {
            on_refresh_red(refresh_red.back());
        }

        return true;
    }

    bool apply_refresh_green(
        compat::u32 pixels, compat::u32 count, compat::i32 amount
    ) override {
        refresh_green.push_back({pixels, count, amount});
        if (!record_refresh_event("green")) {
            return false;
        }

        if (on_refresh_green) {
            on_refresh_green(refresh_green.back());
        }

        return true;
    }

    bool apply_refresh_blue(
        compat::u32 pixels, compat::u32 count, compat::i32 amount
    ) override {
        refresh_blue.push_back({pixels, count, amount});
        if (!record_refresh_event("blue")) {
            return false;
        }

        if (on_refresh_blue) {
            on_refresh_blue(refresh_blue.back());
        }

        return true;
    }

private:
    bool record_refresh_event(std::string_view name) {
        const auto ordinal = refresh_events.size();
        refresh_events.emplace_back(name);
        return refresh_stop_ordinal != ordinal;
    }
};

}  // namespace openswd3::test
