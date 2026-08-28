#include "openswd3/battle/legacy_battle_growth_actor_selection.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i32 as_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] u16 read_u16(
    const std::array<u8, world_map::kLegacyItemDefinitionSnapshotBytes>& bytes,
    const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] constexpr u32 group_a_token(const u32 actor_index) noexcept {
    return kLegacyBattleVictoryGroupABaseToken +
        actor_index * kLegacyBattleVictoryGroupAElementSize;
}

[[nodiscard]] constexpr u32 profile_offset(const u32 label) noexcept {
    return label * kLegacyBattleGrowthProfileStride;
}

class GrowthActorSelectionRunner {
public:
    GrowthActorSelectionRunner(
        LegacyBattleGrowthActorSelectionBindings bindings,
        LegacyBattleGrowthActorSelectionPort& port,
        const LegacyBattleGrowthActorSelectionRequest& request
    )
        : bindings_(bindings), port_(port), request_(request),
          state_(port.battle_growth_actor_selection_state()),
          items_(port.world_item_list_state()), eax_(request.entry_eax),
          ecx_(request.entry_ecx), edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleGrowthActorSelectionResult run() {
        eax_ = bindings_.metrics.group_a_count;
        u32 actor_index = 0U;
        maximum_item_id_ = 0U;
        if (as_i32(eax_) <= 0) {
            finish_normal();
            return result_;
        }

        while (true) {
            if (!advance_actor(actor_index)) {
                finish_stopped();
                return result_;
            }

            eax_ = bindings_.metrics.group_a_count;
            ++actor_index;
            if (as_i32(actor_index) >=
                as_i32(bindings_.metrics.group_a_count)) {
                break;
            }
        }

        finish_normal();
        return result_;
    }

private:
    [[nodiscard]] LegacyBattleGrowthActorSelectionCallReply call(
        const LegacyBattleGrowthActorSelectionCall call_kind,
        const u32 actor_token_value = 0U,
        const u32 destination_token = 0U,
        const u16 item_id = 0U,
        const std::array<u32, 4> arguments = {}
    ) {
        const auto reply = port_.invoke_growth_actor_selection({
            .call = call_kind,
            .actor_token = actor_token_value,
            .destination_token = destination_token,
            .item_id = item_id,
            .arguments = arguments,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.port_calls;
        result_.call_trace.push_back(call_kind);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        if (reply.publish_group_a_count) {
            bindings_.metrics.group_a_count = reply.group_a_count;
        }
        return reply;
    }

    void load_item_definition(
        LegacyBattleGrowthItemDefinitionState& destination,
        const u32 destination_token,
        const u16 item_id
    ) {
        destination.description.clear();
        destination.bytes.fill(0U);
        const auto reply = call(
            LegacyBattleGrowthActorSelectionCall::load_item_definition,
            0U,
            destination_token,
            item_id,
            {destination_token, static_cast<u32>(item_id), 0U, 0U}
        );
        ++result_.item_load_calls;
        if (reply.publish_definition) {
            destination.bytes = reply.definition;
            const auto length = std::min<std::size_t>(
                reply.description_length, reply.description.size()
            );
            destination.description.assign(
                reply.description.begin(),
                reply.description.begin() + static_cast<std::ptrdiff_t>(length)
            );
        }
    }

    void load_item_definition(
        world_map::LegacyWorldItemNode& destination,
        const u32 destination_token,
        const u16 item_id
    ) {
        destination.description.clear();
        destination.definition_snapshot.fill(0U);
        const auto reply = call(
            LegacyBattleGrowthActorSelectionCall::load_item_definition,
            0U,
            destination_token,
            item_id,
            {destination_token, static_cast<u32>(item_id), 0U, 0U}
        );
        ++result_.item_load_calls;
        if (reply.publish_definition) {
            destination.definition_snapshot = reply.definition;
            const auto length = std::min<std::size_t>(
                reply.description_length, reply.description.size()
            );
            destination.description.assign(
                reply.description.begin(),
                reply.description.begin() + static_cast<std::ptrdiff_t>(length)
            );
        }
    }

    void release_scratch_description() noexcept {
        state_.scratch.description.clear();
        ++result_.item_release_calls;
        eax_ = 0U;
    }

    [[nodiscard]] bool advance_actor(const u32 actor_index) {
        result_.stopped_actor_index = actor_index;
        if (actor_index >= bindings_.victory.group_a_skip_primary.size() ||
            actor_index >= bindings_.victory.group_a_skip_secondary.size()) {
            result_.status = LegacyBattleGrowthActorSelectionStatus::
                group_a_actor_typed_stop;
            return false;
        }
        if (bindings_.victory.group_a_skip_primary[actor_index] == 1U ||
            bindings_.victory.group_a_skip_secondary[actor_index] == 1U) {
            return true;
        }

        ecx_ = group_a_token(actor_index);
        const auto actor_reply = call(
            LegacyBattleGrowthActorSelectionCall::query_group_a_reward_block,
            ecx_
        );
        static_cast<void>(actor_reply);
        ++result_.actor_query_calls;
        if (eax_ == 1U) {
            return true;
        }

        if (actor_index >=
            bindings_.startup.action_mode_source.actor_label_indices.size()) {
            result_.status = LegacyBattleGrowthActorSelectionStatus::
                group_a_actor_typed_stop;
            return false;
        }
        const u32 label = bindings_.startup.action_mode_source
                              .actor_label_indices[actor_index];
        eax_ = profile_offset(label);
        if (label >= bindings_.victory.party_reward_counters.size() ||
            label >= bindings_.victory.party_growth_limits.size() ||
            label >= bindings_.victory.party_growth_item_codes.size()) {
            result_.status = LegacyBattleGrowthActorSelectionStatus::
                growth_profile_typed_stop;
            return false;
        }
        if (bindings_.victory.party_growth_item_codes[label] == 0xFFFFFFFFU) {
            return true;
        }

        if (actor_index >= items_.party_item_lists.size()) {
            result_.status = LegacyBattleGrowthActorSelectionStatus::
                party_item_list_typed_stop;
            return false;
        }
        auto& optional_list = items_.party_item_lists[actor_index];
        if (!optional_list.has_value()) {
            result_.status = LegacyBattleGrowthActorSelectionStatus::
                missing_party_item_sentinel_typed_stop;
            return false;
        }
        auto& item_list = *optional_list;

        for (auto& node : item_list.nodes) {
            if (read_u16(
                    node.definition_snapshot, kLegacyBattleGrowthItemTypeOffset
                ) != kLegacyBattleGrowthItemType) {
                continue;
            }
            ++result_.matching_item_count;
            ecx_ = (ecx_ & 0xFFFF0000U) | static_cast<u32>(node.item_id);
            load_item_definition(
                state_.scratch,
                kLegacyBattleGrowthItemScratchToken,
                node.item_id
            );
            release_scratch_description();

            eax_ = static_cast<u32>(node.item_id);
            if (as_i32(eax_) > as_i32(maximum_item_id_)) {
                maximum_item_id_ = static_cast<u16>(eax_);
            }

            eax_ = profile_offset(label);
            edx_ = static_cast<u32>(read_u16(
                state_.scratch.bytes, kLegacyBattleGrowthItemLimitOffset
            ));
            ecx_ = static_cast<u32>(read_u16(
                state_.scratch.bytes, kLegacyBattleGrowthItemCodeOffset
            ));
            ecx_ |= 0x80000000U;
            bindings_.victory.party_growth_limits[label] = edx_;
            bindings_.victory.party_growth_item_codes[label] = ecx_;
        }

        eax_ = profile_offset(label);
        edx_ = bindings_.victory.party_reward_counters[label];
        ecx_ = bindings_.victory.party_growth_limits[label];
        if (as_i32(edx_) < as_i32(ecx_)) {
            return true;
        }
        if ((bindings_.victory.party_growth_item_codes[label] & 0x7FFFFFFFU) ==
            0U) {
            return true;
        }

        const auto presence = call(
            LegacyBattleGrowthActorSelectionCall::query_item_presence,
            0U,
            0U,
            0U,
            {kLegacyBattleGrowthItemPresenceId, 0U, 0U, 0U}
        );
        static_cast<void>(presence);
        ++result_.item_presence_calls;
        if (eax_ == 1U) {
            eax_ = profile_offset(label);
            maximum_item_id_ = static_cast<u16>(
                bindings_.victory.party_growth_item_codes[label] & 0x7FFFFFFFU
            );
            if (maximum_item_id_ == 0x0665U || maximum_item_id_ == 0x0669U) {
                return true;
            }
        }

        eax_ = label;
        ecx_ = profile_offset(label);
        const u16 item_id =
            static_cast<u16>(bindings_.victory.party_growth_item_codes[label]);
        edx_ = (edx_ & 0xFFFF0000U) | static_cast<u32>(item_id);
        load_item_definition(
            state_.scratch, kLegacyBattleGrowthItemScratchToken, item_id
        );
        release_scratch_description();

        eax_ = 0U;
        const auto allocation = call(
            LegacyBattleGrowthActorSelectionCall::allocate_item_node,
            0U,
            0U,
            0U,
            {kLegacyBattleGrowthItemAllocationSize, 0U, 0U, 0U}
        );
        ++result_.allocation_calls;
        const u32 allocation_token = allocation.publish_allocation_token
            ? allocation.allocation_token
            : kLegacyBattleGrowthAllocatedItemBaseToken +
                (result_.allocation_calls - 1U) *
                    kLegacyBattleGrowthItemAllocationSize;
        if (allocation.allocation_failed || allocation_token == 0U) {
            eax_ = 0U;
            ecx_ = 0x2CU;
            result_.status =
                LegacyBattleGrowthActorSelectionStatus::allocation_typed_stop;
            return false;
        }

        auto* previous = &item_list.sentinel;
        if (!item_list.nodes.empty()) {
            previous = &item_list.nodes.back();
        }
        previous->legacy_next_token = allocation_token;
        item_list.nodes.emplace_back();
        auto& appended = item_list.nodes.back();
        appended.legacy_token = allocation_token;

        eax_ = 0U;
        ecx_ = 0U;
        eax_ = allocation_token;
        ecx_ = profile_offset(label);
        ecx_ = (ecx_ & 0xFFFF0000U) | static_cast<u32>(item_id);
        appended.item_id = item_id;
        load_item_definition(
            appended,
            allocation_token + kLegacyBattleGrowthItemDefinitionTokenOffset,
            item_id
        );

        bindings_.target_selection.transition_mode = 1U;
        if (!copy_caption(appended.definition_snapshot)) {
            return false;
        }

        eax_ = label;
        edx_ = static_cast<u32>(
            read_u16(state_.scratch.bytes, kLegacyBattleGrowthItemLimitOffset)
        );
        ecx_ = (ecx_ & 0xFFFF0000U) |
            static_cast<u32>(read_u16(
                state_.scratch.bytes, kLegacyBattleGrowthItemCodeOffset
            ));
        edx_ &= 0xFFFFU;
        eax_ = profile_offset(label);
        bindings_.target_selection.transition_actor_index =
            static_cast<u8>(actor_index);
        bindings_.victory.party_reward_counters[label] = 0U;
        bindings_.victory.party_growth_limits[label] = edx_;
        edx_ = static_cast<u32>(static_cast<u16>(ecx_)) | 0x80000000U;
        bindings_.victory.party_growth_item_codes[label] = edx_;
        if (static_cast<u16>(ecx_) == 0U) {
            bindings_.victory.party_growth_item_codes[label] = 0xFFFFFFFFU;
            edx_ = 0xFFFFFFFFU;
        }
        ++result_.selected_actor_count;
        return true;
    }

    [[nodiscard]] bool copy_caption(
        const std::array<u8, world_map::kLegacyItemDefinitionSnapshotBytes>&
            source
    ) {
        auto& caption = bindings_.level_advancement.growth_caption_text;
        for (std::size_t index = 0U;; ++index) {
            result_.stopped_title_index = static_cast<u32>(index);
            if (index >= source.size()) {
                result_.status = LegacyBattleGrowthActorSelectionStatus::
                    caption_source_typed_stop;
                return false;
            }
            const u8 value = source[index];
            if (index >= caption.size()) {
                result_.status = LegacyBattleGrowthActorSelectionStatus::
                    caption_destination_typed_stop;
                return false;
            }
            caption[index] = value;
            if (value == 0U) {
                return true;
            }
        }
    }

    void finish_normal() noexcept {
        result_.return_eax = eax_;
        result_.return_ecx = request_.entry_ecx;
        result_.return_edx = edx_;
        result_.maximum_matching_item_id = maximum_item_id_;
    }

    void finish_stopped() noexcept {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        result_.maximum_matching_item_id = maximum_item_id_;
    }

    LegacyBattleGrowthActorSelectionBindings bindings_;
    LegacyBattleGrowthActorSelectionPort& port_;
    const LegacyBattleGrowthActorSelectionRequest& request_;
    LegacyBattleGrowthActorSelectionState& state_;
    world_map::LegacyWorldItemListState& items_;
    LegacyBattleGrowthActorSelectionResult result_;
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
    u16 maximum_item_id_{};
};

}  // namespace

LegacyBattleGrowthActorSelectionResult
advance_legacy_battle_growth_actor_selection(
    LegacyBattleGrowthActorSelectionBindings bindings,
    LegacyBattleGrowthActorSelectionPort& port,
    const LegacyBattleGrowthActorSelectionRequest& request
) {
    return GrowthActorSelectionRunner(bindings, port, request).run();
}

}  // namespace openswd3::battle
