#pragma once

#include "openswd3/audio_video/legacy_sample_manager.hpp"

#include <functional>
#include <map>
#include <memory>
#include <utility>

namespace openswd3::test {

class BattleSampleBackend final : public audio_video::LegacySampleBackend {
public:
    compat::u32 driver_token() const override {
        return 1U;
    }

    compat::u32 allocate_sample_handle() override {
        return ++last_handle;
    }

    void initialize_sample(compat::u32) override {}
    void release_sample_handle(compat::u32) override {}

    bool
    set_sample_file(compat::u32, std::span<const compat::u8> bytes) override {
        return !bytes.empty();
    }

    bool set_named_sample_file(
        compat::u32 handle,
        std::string_view,
        std::span<const compat::u8> bytes,
        compat::u32
    ) override {
        return set_sample_file(handle, bytes);
    }

    void set_sample_user_data(
        compat::u32 handle, compat::u32 slot, compat::u32 value
    ) override {
        user_data[{handle, slot}] = value;
    }

    compat::u32
    sample_user_data(compat::u32 handle, compat::u32 slot) override {
        return user_data.at({handle, slot});
    }

    void set_sample_volume(compat::u32, compat::i32 value) override {
        last_volume = value;
    }

    void set_sample_pan(compat::u32, compat::i32 value) override {
        last_pan = value;
    }

    void set_sample_loop_count(compat::u32, compat::i32 value) override {
        last_loop_count = value;
    }

    void start_sample(compat::u32 handle) override {
        started_ids.push_back(sample_user_data(handle, 0U));
        if (on_start) {
            on_start();
        }
    }

    void end_sample(compat::u32) override {}

    compat::u32 sample_status(compat::u32) override {
        return 4U;
    }

    void close_output() override {}

    compat::u32 last_handle{};
    compat::i32 last_volume{};
    compat::i32 last_pan{};
    compat::i32 last_loop_count{};
    std::map<std::pair<compat::u32, compat::u32>, compat::u32> user_data;
    std::vector<compat::u32> started_ids;
    std::function<void()> on_start;
};

struct LegacyBattleSamplesFixture {
    BattleSampleBackend backend;
    std::unique_ptr<audio_video::LegacySndArchive> archive =
        std::make_unique<audio_video::LegacySndArchive>();
    audio_video::LegacySampleManager samples{backend, *archive};
};

}
