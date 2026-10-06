#include "openswd3/battle/legacy_battle_action_rotation_resources.hpp"

#include <algorithm>
#include <exception>
#include <utility>
#include <vector>

namespace openswd3::battle {

LegacyBattleActionRotationResources::LegacyBattleActionRotationResources(
    asset_runtime::LegacyTswRuntime& runtime
) noexcept
    : runtime_(runtime) {}

LegacyBattleMutableFrameImage
LegacyBattleActionRotationResources::query_frame_image(
    const compat::u32 resource_id, const compat::u32 frame_index
) {
    auto loaded = runtime_.load_owned(resource_id, frame_index);
    last_load_status_ = loaded.status;
    // Preserve a record allocated before a stopped physical load. It must
    // not be published into the caller's cache as a normal null image.
    auto* const frame = loaded.frame.get();
    if (frame != nullptr) {
        owners_.emplace(frame->record_token, std::move(loaded.frame));
    }

    if (loaded.status != asset_runtime::LegacyTswRuntimeStatus::ready) {
        return {.typed_stop = true};
    }

    return {
        .owner_token = loaded.return_record_token,
        .image_token = frame->primary_stream_token,
        .pointer_valid = true,
        .bytes = frame->primary_stream,
        .frame = {
            .source =
                {
                    .bytes = frame->primary_stream,
                    .layout = rendering::LegacyBlitSourceLayout::direct_16,
                },
            .legacy_source_token = frame->primary_stream_token,
            .width = frame->width,
            .height = frame->height,
        },
    };
}

void LegacyBattleActionRotationResources::release_image(
    const compat::u32 image_token
) noexcept {
    const auto found = std::ranges::find_if(owners_, [image_token](auto& item) {
        return item.second->primary_stream_token == image_token;
    });
    if (found == owners_.end() || found->second->primary_stream.empty()) {
        std::terminate();
    }

    // Retain the record until the following owner release. Its allocation
    // identity is immutable; the caller clears its stored image pointer.
    std::vector<compat::u8>{}.swap(found->second->primary_stream);
}

void LegacyBattleActionRotationResources::release_owner(
    const compat::u32 owner_token
) noexcept {
    const auto found = owners_.find(owner_token);
    if (found == owners_.end() || !found->second->primary_stream.empty()) {
        std::terminate();
    }

    owners_.erase(found);
}

std::size_t
LegacyBattleActionRotationResources::live_owner_count() const noexcept {
    return owners_.size();
}

std::size_t
LegacyBattleActionRotationResources::live_image_count() const noexcept {
    return static_cast<std::size_t>(
        std::ranges::count_if(owners_, [](const auto& item) {
            return !item.second->primary_stream.empty();
        })
    );
}

std::optional<asset_runtime::LegacyTswRuntimeStatus>
LegacyBattleActionRotationResources::last_load_status() const noexcept {
    return last_load_status_;
}

}  // namespace openswd3::battle
