#include "openswd3/battle/legacy_battle_level_advancement.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;
using world_map::LegacyWorldStoryPartyMemberResources;

[[nodiscard]] constexpr i32 as_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32
replace_low_byte(const u32 value, const u32 low) noexcept {
    return (value & 0xFFFFFF00U) | (low & 0xFFU);
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u32 low) noexcept {
    return (value & 0xFFFF0000U) | (low & 0xFFFFU);
}

[[nodiscard]] constexpr u16
wrapping_delta(const u16 advanced, const u16 baseline) noexcept {
    return static_cast<u16>(advanced - baseline);
}

void add_delta(u16& target, const u16 advanced, const u16 baseline) noexcept {
    target = static_cast<u16>(target + wrapping_delta(advanced, baseline));
}

[[nodiscard]] constexpr u32
packed_fields_1c(const LegacyWorldStoryPartyMemberResources& profile) noexcept {
    return static_cast<u32>(profile.fields_10_to_1e[6U]) |
        (static_cast<u32>(profile.fields_10_to_1e[7U]) << 16U);
}

class Runner {
public:
    Runner(
        LegacyBattleLevelAdvancementBindings bindings,
        LegacyBattleLevelAdvancementPort& port,
        const LegacyBattleLevelAdvancementRequest& request
    ) noexcept
        : bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleLevelAdvancementResult run() {
        eax_ = bindings_.metrics.group_a_count;
        actor_index_ = 0U;
        if (as_i32(eax_) <= 0) {
            complete();
            return finish();
        }

        while (true) {
            if (!visit_actor()) {
                return finish();
            }
            if (selected_) {
                complete();
                return finish();
            }

            ++actor_index_;
            eax_ = actor_index_;
            ecx_ = bindings_.metrics.group_a_count;
            if (as_i32(eax_) >= as_i32(ecx_)) {
                complete();
                return finish();
            }
        }
    }

private:
    [[nodiscard]] LegacyBattleLevelAdvancementCallReply invoke(
        const LegacyBattleLevelAdvancementCall call,
        const std::array<u32, 4>& arguments = {}
    ) {
        const auto reply = port_.invoke_level_advancement({
            .call = call,
            .arguments = arguments,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.port_calls;
        result_.call_trace.push_back(call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        if (reply.publish_group_a_count) {
            bindings_.metrics.group_a_count = reply.group_a_count;
        }
        if (reply.publish_transition_mode) {
            bindings_.target_selection.transition_mode = reply.transition_mode;
        }
        return reply;
    }

    [[nodiscard]] bool visit_actor() {
        ++result_.visited_actors;
        if (actor_index_ >= bindings_.victory.group_a_skip_primary.size() ||
            actor_index_ >= bindings_.victory.group_a_skip_secondary.size()) {
            result_.status =
                LegacyBattleLevelAdvancementStatus::group_a_actor_typed_stop;
            return false;
        }
        if (bindings_.victory.group_a_skip_primary[actor_index_] == 1U ||
            bindings_.victory.group_a_skip_secondary[actor_index_] == 1U) {
            return true;
        }

        if (actor_index_ >=
            bindings_.startup.action_mode_source.actor_label_indices.size()) {
            result_.status =
                LegacyBattleLevelAdvancementStatus::action_label_typed_stop;
            return false;
        }
        u32 label = bindings_.startup.action_mode_source
                        .actor_label_indices[actor_index_];
        eax_ = label;
        edx_ = replace_low_word(edx_, 0U);
        ecx_ = label * 7U;
        if (label >= bindings_.party_member_resources.size()) {
            result_.status = LegacyBattleLevelAdvancementStatus::
                party_member_resource_typed_stop;
            return false;
        }

        const u32 old_level = bindings_.party_member_resources[label].field_2c;
        ecx_ = replace_low_byte(ecx_, old_level);
        const u32 new_level = (ecx_ & 0xFFU) + 1U;
        edx_ = replace_low_byte(edx_, ecx_);
        if (static_cast<u16>(edx_) >=
            bindings_.victory.party_profile_threshold) {
            bindings_.target_selection.transition_actor_index = 0xFFU;
            return true;
        }

        u32 requirement = 0U;
        eax_ = label + 1U;
        ecx_ = request_.requirement_output_token;
        result_.level_load = load_legacy_battle_level_requirement(
            requirement,
            port_,
            {
                .group = label + 1U,
                .level = new_level,
                .output_token = request_.requirement_output_token,
                .stale_directory_offset =
                    request_.requirement_stale_directory_offset,
                .number_of_bytes_read_token =
                    request_.requirement_number_of_bytes_read_token,
                .entry_eax = eax_,
                .entry_ecx = ecx_,
                .entry_edx = edx_,
                .output_accessible = request_.requirement_output_accessible,
            }
        );
        ++result_.requirement_calls;
        eax_ = result_.level_load.return_eax;
        ecx_ = result_.level_load.return_ecx;
        edx_ = result_.level_load.return_edx;
        if (legacy_battle_level_requirement_load_stopped(
                result_.level_load.status
            )) {
            result_.status = LegacyBattleLevelAdvancementStatus::
                level_requirement_typed_stop;
            return false;
        }

        label = bindings_.startup.action_mode_source
                    .actor_label_indices[actor_index_];
        eax_ = label;
        ecx_ = requirement;
        edx_ = label * 0x38U;
        if (label >= bindings_.party_member_resources.size()) {
            result_.status = LegacyBattleLevelAdvancementStatus::
                party_member_resource_typed_stop;
            return false;
        }
        if (as_i32(bindings_.party_member_resources[label].field_00) <
            as_i32(ecx_)) {
            eax_ = bindings_.party_member_resources[label].field_00;
            bindings_.target_selection.transition_actor_index = 0xFFU;
            return true;
        }
        eax_ = bindings_.party_member_resources[label].field_00;

        bindings_.state.baseline_scratch = {};
        bindings_.state.advanced_scratch = {};
        bindings_.state.profile_copy_scratch =
            bindings_.party_member_resources[label];

        eax_ = label + 1U;
        ecx_ = old_level;
        result_.baseline_profile_load = load_legacy_battle_level_profile(
            bindings_.state.baseline_scratch,
            bindings_.state.growth_caption_text,
            bindings_.target_selection.transition_mode,
            port_,
            {
                .party_number_one_based = label + 1U,
                .level = old_level,
                .output_token = request_.baseline_scratch_token,
                .caption_token = request_.caption_token,
                .transition_mode_token = request_.transition_mode_token,
                .stale_directory_offset =
                    request_.profile_stale_directory_offset,
                .number_of_bytes_read_token =
                    request_.profile_number_of_bytes_read_token,
                .entry_eax = eax_,
                .entry_ecx = ecx_,
                .entry_edx = edx_,
                .output_accessible_bytes =
                    request_.baseline_output_accessible_bytes,
                .caption_accessible_bytes = request_.caption_accessible_bytes,
                .transition_mode_accessible =
                    request_.transition_mode_accessible,
                .host_item_node_allocation_succeeds =
                    request_.host_item_node_allocation_succeeds,
            }
        );
        ++result_.profile_build_calls;
        eax_ = result_.baseline_profile_load.return_eax;
        ecx_ = result_.baseline_profile_load.return_ecx;
        edx_ = result_.baseline_profile_load.return_edx;
        if (legacy_battle_level_profile_load_stopped(
                result_.baseline_profile_load.status
            )) {
            result_.status =
                LegacyBattleLevelAdvancementStatus::level_profile_typed_stop;
            return false;
        }

        const u32 live_label = bindings_.startup.action_mode_source
                                   .actor_label_indices[actor_index_];
        edx_ = live_label + 1U;
        result_.advanced_profile_load = load_legacy_battle_level_profile(
            bindings_.state.advanced_scratch,
            bindings_.state.growth_caption_text,
            bindings_.target_selection.transition_mode,
            port_,
            {
                .party_number_one_based = live_label + 1U,
                .level = new_level,
                .output_token = request_.advanced_scratch_token,
                .caption_token = request_.caption_token,
                .transition_mode_token = request_.transition_mode_token,
                .stale_directory_offset =
                    request_.profile_stale_directory_offset,
                .number_of_bytes_read_token =
                    request_.profile_number_of_bytes_read_token,
                .entry_eax = eax_,
                .entry_ecx = ecx_,
                .entry_edx = edx_,
                .output_accessible_bytes =
                    request_.advanced_output_accessible_bytes,
                .caption_accessible_bytes = request_.caption_accessible_bytes,
                .transition_mode_accessible =
                    request_.transition_mode_accessible,
                .host_item_node_allocation_succeeds =
                    request_.host_item_node_allocation_succeeds,
            }
        );
        ++result_.profile_build_calls;
        eax_ = result_.advanced_profile_load.return_eax;
        ecx_ = result_.advanced_profile_load.return_ecx;
        edx_ = result_.advanced_profile_load.return_edx;
        if (legacy_battle_level_profile_load_stopped(
                result_.advanced_profile_load.status
            )) {
            result_.status =
                LegacyBattleLevelAdvancementStatus::level_profile_typed_stop;
            return false;
        }

        label = bindings_.startup.action_mode_source
                    .actor_label_indices[actor_index_];
        ecx_ = label;
        edx_ = bindings_.state.advanced_scratch.field_20;
        eax_ = label * 0x38U;
        ecx_ =
            replace_low_byte(ecx_, bindings_.state.advanced_scratch.field_2c);
        if (label >= bindings_.party_member_resources.size()) {
            result_.status = LegacyBattleLevelAdvancementStatus::
                party_member_resource_typed_stop;
            return false;
        }

        apply_profile_delta(bindings_.party_member_resources[label]);
        bindings_.target_selection.transition_actor_index =
            static_cast<u8>(actor_index_);
        result_.selected_actor_index = actor_index_;

        const u32 final_field_1c_delta =
            packed_fields_1c(bindings_.state.advanced_scratch) -
            packed_fields_1c(bindings_.state.baseline_scratch);
        eax_ = label * 0x38U;
        ecx_ = replace_low_word(
            ecx_,
            wrapping_delta(
                bindings_.state.advanced_scratch.fields_10_to_1e[7U],
                bindings_.state.baseline_scratch.fields_10_to_1e[7U]
            )
        );
        edx_ = replace_low_byte(final_field_1c_delta, actor_index_);
        auto registers = port_.stop_level_sample(
            eax_, ecx_, edx_, kLegacyBattleLevelAdvanceStopSample
        );
        ++result_.stop_sample_calls;
        eax_ = registers.eax;
        ecx_ = registers.ecx;
        edx_ = registers.edx;

        eax_ = std::bit_cast<u32>(bindings_.input_dispatch.sample_mix_level);
        const auto sample = port_.play_level_sample(
            eax_,
            ecx_,
            edx_,
            kLegacyBattleLevelAdvancePlaySample,
            bindings_.input_dispatch.sample_mix_level
        );
        ++result_.play_sample_calls;
        eax_ = replace_low_byte(sample.eax, actor_index_);
        ecx_ = sample.ecx;
        edx_ = sample.edx;
        selected_ = static_cast<u8>(eax_) != 0xFFU;
        return true;
    }

    void apply_profile_delta(LegacyWorldStoryPartyMemberResources& profile) {
        const auto& baseline = bindings_.state.baseline_scratch;
        const auto& advanced = bindings_.state.advanced_scratch;

        profile.field_2c = advanced.field_2c;
        add_delta(
            profile.limit_first, advanced.limit_first, baseline.limit_first
        );
        profile.current_first = profile.limit_first;
        profile.field_20 = advanced.field_20;

        add_delta(
            profile.limit_second, advanced.limit_second, baseline.limit_second
        );
        profile.current_second = profile.limit_second;
        add_delta(
            profile.limit_third, advanced.limit_third, baseline.limit_third
        );
        profile.current_third = profile.limit_third;
        for (std::size_t index = 0U; index < profile.fields_10_to_1e.size();
             ++index) {
            add_delta(
                profile.fields_10_to_1e[index],
                advanced.fields_10_to_1e[index],
                baseline.fields_10_to_1e[index]
            );
        }
    }

    void complete() noexcept {
        bindings_.state.completion_gate = 1U;
    }

    [[nodiscard]] LegacyBattleLevelAdvancementResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleLevelAdvancementBindings bindings_;
    LegacyBattleLevelAdvancementPort& port_;
    const LegacyBattleLevelAdvancementRequest& request_;
    LegacyBattleLevelAdvancementResult result_{};
    u32 actor_index_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
    bool selected_{};
};

}  // namespace

LegacyBattleLevelAdvancementResult advance_legacy_battle_actor_level(
    LegacyBattleLevelAdvancementBindings bindings,
    LegacyBattleLevelAdvancementPort& port,
    const LegacyBattleLevelAdvancementRequest& request
) {
    return Runner(bindings, port, request).run();
}

}  // namespace openswd3::battle
