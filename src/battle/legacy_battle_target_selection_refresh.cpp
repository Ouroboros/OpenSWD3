#include "openswd3/battle/legacy_battle_target_selection_refresh.hpp"

#include <bit>
#include <cstddef>
#include <optional>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;
using Call = LegacyBattleTargetSelectionRuntimeCall;
using Status = LegacyBattleTargetSelectionRefreshStatus;

enum class GroupARegisterShape : u8 {
    eax_bcd,
    eax_3ef,
    eax_3ef_edx_bcd,
};

inline constexpr u32 kGroupABaseToken = 0x005029D0U;
inline constexpr u32 kGroupAOneBasedToken = 0x004FFA9CU;
inline constexpr u32 kGroupAStride = 0x2F34U;
inline constexpr u32 kGroupACount = 10U;
inline constexpr u32 kGroupBBaseToken = 0x00525508U;
inline constexpr u32 kGroupBStride = 0x2B28U;
inline constexpr u32 kGroupBCount = 8U;
inline constexpr u32 kActorRuntimeRecordToken = 0x004FE5D4U;
inline constexpr u32 kTargetPanelToken = 0x004B8748U;
inline constexpr u32 kWarningTextAToken = 0x004A7980U;
inline constexpr u32 kWarningTextBToken = 0x004A7990U;

class ResourceSelectionAdapter final
    : public LegacyBattleActorResourceSelectionPort {
public:
    ResourceSelectionAdapter(
        LegacyBattleInputDispatchPort& port,
        LegacyBattleTargetSelectionRefreshResult& result
    ) noexcept
        : port_(port), result_(result) {}

    [[nodiscard]] LegacyBattleActorResourceSelectionProfileReply load_profile(
        std::array<u32, 10>&,
        const u16 profile_id,
        const u32 eax,
        const u32 ecx,
        const u32 edx
    ) override {
        const auto reply = port_.invoke_target_selection_runtime({
            .call =
                LegacyBattleTargetSelectionRuntimeCall::resource_profile_load,
            .arguments = {profile_id},
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        ++result_.port_calls;
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }

    void report_missing_runtime_word(const u16 resource_id) override {
        static_cast<void>(port_.invoke_target_selection_runtime({
            .call = LegacyBattleTargetSelectionRuntimeCall::
                resource_missing_word_diagnostic,
            .arguments = {resource_id},
        }));
        ++result_.port_calls;
    }

private:
    LegacyBattleInputDispatchPort& port_;
    LegacyBattleTargetSelectionRefreshResult& result_;
};

class InputTextMessageAdapter final : public LegacyBattleTextMessagePort {
public:
    explicit InputTextMessageAdapter(LegacyBattleInputDispatchPort& port)
        : port_(port) {}

    [[nodiscard]] LegacyBattleTextMessageCallReply invoke_text_message(
        const LegacyBattleTextMessageCallRequest& request
    ) override {
        const auto reply = port_.invoke_input_dispatch({
            .call = request.call == LegacyBattleTextMessageCall::allocate
                ? LegacyBattleInputDispatchCall::text_message_allocate
                : LegacyBattleInputDispatchCall::text_message_measure,
            .arguments = {request.argument},
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }

private:
    LegacyBattleInputDispatchPort& port_;
};

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
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

class TargetSelectionMachine {
public:
    TargetSelectionMachine(
        const LegacyBattleTargetSelectionRefreshBindings bindings,
        LegacyBattleInputDispatchPort& port,
        const LegacyBattleTargetSelectionRefreshRequest& request
    )
        : bindings_(bindings), port_(port),
          frame_(bindings.frame_input_resolution),
          final_actor_(bindings.final_actor), action_(bindings.action),
          metrics_(bindings.metrics), debug_(bindings.debug_hotkeys),
          input_(bindings.input_dispatch), records_(bindings.input_records),
          runtime_(bindings.runtime), eax_(request.entry_eax),
          ecx_(request.entry_ecx), edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleTargetSelectionRefreshResult run() {
        eax_ = bindings_.target_ready_gate;
        if (eax_ != 1U) {
            return finish();
        }

        eax_ = bindings_.message_state;
        input_.selection_runtime_gate = 0U;
        --eax_;
        if (eax_ > 199U) {
            return finish();
        }

        const u32 message = bindings_.message_state;
        ecx_ = message_selector(message);
        switch (message) {
        case 1U:
            case_one();
            break;
        case 2U:
            case_two();
            break;
        case 3U:
            case_three();
            break;
        case 4U:
            case_four();
            break;
        case 5U:
            case_five();
            break;
        case 7U:
            case_seven();
            break;
        case 8U:
            case_eight();
            break;
        case 27U:
            case_twenty_seven();
            break;
        case 30U:
            case_thirty();
            break;
        case 98U:
            case_ninety_eight();
            break;
        case 100U:
            case_one_hundred();
            break;
        case 101U:
            case_one_hundred_one();
            break;
        case 102U:
            case_one_hundred_two();
            break;
        case 103U:
            case_one_hundred_three();
            break;
        case 104U:
            case_one_hundred_four();
            break;
        case 110U:
            case_one_hundred_ten();
            break;
        case 111U:
            case_one_hundred_eleven();
            break;
        case 112U:
            case_one_hundred_twelve();
            break;
        case 113U:
            case_one_hundred_thirteen();
            break;
        case 200U:
            case_two_hundred();
            break;
        default:
            break;
        }
        return finish();
    }

private:
    [[nodiscard]] LegacyBattleTargetSelectionRefreshResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    void typed_stop(const Status status) noexcept {
        result_.status = status;
        stopped_ = true;
    }

    [[nodiscard]] static constexpr u32
    message_selector(const u32 message) noexcept {
        switch (message) {
        case 1U:
            return 0U;
        case 2U:
            return 1U;
        case 3U:
            return 2U;
        case 4U:
            return 3U;
        case 5U:
            return 4U;
        case 7U:
            return 5U;
        case 8U:
            return 6U;
        case 27U:
            return 7U;
        case 30U:
            return 8U;
        case 98U:
            return 9U;
        case 100U:
            return 10U;
        case 101U:
            return 11U;
        case 102U:
            return 12U;
        case 103U:
            return 13U;
        case 104U:
            return 14U;
        case 110U:
            return 15U;
        case 111U:
            return 16U;
        case 112U:
            return 17U;
        case 113U:
            return 18U;
        case 200U:
            return 19U;
        default:
            return 20U;
        }
    }

    [[nodiscard]] static constexpr u32
    action_selector(const u32 action) noexcept {
        switch (action) {
        case 1U:
            return 0U;
        case 2U:
            return 1U;
        case 3U:
            return 2U;
        case 4U:
            return 3U;
        case 5U:
            return 4U;
        case 6U:
        case 21U:
        case 23U:
        case 29U:
        case 31U:
        case 33U:
            return 5U;
        case 17U:
            return 6U;
        case 22U:
            return 7U;
        case 24U:
            return 8U;
        case 25U:
            return 9U;
        case 26U:
            return 10U;
        case 27U:
            return 11U;
        case 28U:
        case 32U:
        case 34U:
        case 35U:
        case 36U:
            return 12U;
        case 30U:
            return 13U;
        default:
            return 14U;
        }
    }

    void invoke(
        const Call call,
        const u32 actor_token = 0U,
        const std::array<u32, 5>& arguments = {}
    ) {
        const auto reply = port_.invoke_target_selection_runtime({
            .call = call,
            .actor_token = actor_token,
            .arguments = arguments,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.port_calls;
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        call_output_ = reply.output_value;
    }

    void sample(const u32 sound_id) {
        const auto reply = port_.play_input_sample(
            sound_id, input_.sample_mix_level, eax_, ecx_, edx_
        );
        ++result_.port_calls;
        ++result_.sample_calls;
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
    }

    [[nodiscard]] bool prepare_group_b(const u32 index) {
        eax_ = index * 0x565U;
        edx_ = index * 0x159U;
        ecx_ = kGroupBBaseToken + index * kGroupBStride;
        if (index >= kGroupBCount) {
            typed_stop(Status::group_b_actor_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool invoke_group_a(
        const Call call,
        const u32 actor_code,
        const std::array<u32, 5>& arguments = {},
        GroupARegisterShape shape = GroupARegisterShape::eax_bcd
    ) {
        const u32 index = actor_code - 8U;
        const u32 prior_edx = edx_;
        ecx_ = kGroupABaseToken + index * kGroupAStride;
        if (call == Call::refresh_actor_selection ||
            call == Call::apply_special_actor_action ||
            call == Call::set_actor_mode) {
            shape = GroupARegisterShape::eax_3ef_edx_bcd;
        } else if (
            call == Call::query_group_b_completion ||
            call == Call::query_actor_property_a ||
            call == Call::query_actor_cleanup ||
            call == Call::query_action_four_override
        ) {
            shape = GroupARegisterShape::eax_3ef;
        }
        if (shape == GroupARegisterShape::eax_bcd) {
            eax_ = index * 0xBCDU;
        } else {
            eax_ = index * 0x3EFU;
            if (shape == GroupARegisterShape::eax_3ef_edx_bcd) {
                edx_ = index * 0xBCDU;
            } else {
                edx_ = prior_edx;
            }
        }
        if (index >= kGroupACount) {
            typed_stop(Status::group_a_actor_typed_stop);
            return false;
        }
        const u32 token = ecx_;
        invoke(call, token, arguments);
        ++result_.group_a_calls;
        return true;
    }

    [[nodiscard]] bool invoke_group_a_one_based(
        const Call call,
        const u32 code,
        const std::array<u32, 5>& arguments = {}
    ) {
        ecx_ = kGroupAOneBasedToken + code * kGroupAStride;
        eax_ = code * (call == Call::reset_actor_selection ? 0x3EFU : 0xBCDU);
        if (code == 0U || code > kGroupACount) {
            typed_stop(Status::group_a_actor_typed_stop);
            return false;
        }
        const u32 token = ecx_;
        invoke(call, token, arguments);
        ++result_.group_a_calls;
        return true;
    }

    [[nodiscard]] bool invoke_group_b(
        const Call call,
        const u32 index,
        const std::array<u32, 5>& arguments = {}
    ) {
        if (!prepare_group_b(index)) {
            return false;
        }
        const u32 token = ecx_;
        invoke(call, token, arguments);
        ++result_.group_b_calls;
        return true;
    }

    [[nodiscard]] bool read_target_actor(const u32 index, u32& value) {
        if (index >= runtime_.target_actor_indices.size()) {
            typed_stop(Status::target_actor_index_typed_stop);
            return false;
        }
        value = runtime_.target_actor_indices[index];
        return true;
    }

    [[nodiscard]] bool
    runtime_record_index(const u32 actor_code, const u32 field, u32& index) {
        index = (actor_code - 8U) * 5U + field;
        if (index >= bindings_.startup_reset.block_520e90.size()) {
            typed_stop(Status::actor_runtime_record_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool
    read_runtime_record(const u32 actor_code, const u32 field, u32& value) {
        u32 index{};
        if (!runtime_record_index(actor_code, field, index)) {
            return false;
        }
        value = bindings_.startup_reset.block_520e90[index];
        ++result_.actor_runtime_reads;
        return true;
    }

    [[nodiscard]] bool write_runtime_record(
        const u32 actor_code, const u32 field, const u32 value
    ) {
        u32 index{};
        if (!runtime_record_index(actor_code, field, index)) {
            return false;
        }
        bindings_.startup_reset.block_520e90[index] = value;
        ++result_.actor_runtime_writes;
        return true;
    }

    [[nodiscard]] bool workspace_index(const u32 actor_code, u32& index) {
        index = actor_code + 2U;
        if (index >= action_.opponent_workspace.size()) {
            typed_stop(Status::action_workspace_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool
    write_actor_action(const u32 actor_code, const u32 value) {
        u32 index{};
        if (!workspace_index(actor_code, index)) {
            return false;
        }
        action_.opponent_workspace[index] = value;
        ++result_.workspace_writes;
        return true;
    }

    [[nodiscard]] std::optional<u16>
    read_workspace_word(const u32 byte_offset) {
        constexpr u32 kBytes =
            static_cast<u32>(sizeof(action_.opponent_workspace));
        if (byte_offset > kBytes - 2U) {
            typed_stop(Status::action_workspace_typed_stop);
            return std::nullopt;
        }
        const u32 first_index = byte_offset / 4U;
        const u32 shift = (byte_offset % 4U) * 8U;
        u32 bits = action_.opponent_workspace[first_index] >> shift;
        if (shift > 16U) {
            bits |= action_.opponent_workspace[first_index + 1U]
                << (32U - shift);
        }
        ++result_.workspace_reads;
        return static_cast<u16>(bits);
    }

    [[nodiscard]] bool
    write_workspace_word(const u32 byte_offset, const u16 value) {
        constexpr u32 kBytes =
            static_cast<u32>(sizeof(action_.opponent_workspace));
        if (byte_offset > kBytes - 2U) {
            typed_stop(Status::action_workspace_typed_stop);
            return false;
        }
        for (u32 byte = 0U; byte < 2U; ++byte) {
            const u32 offset = byte_offset + byte;
            const u32 index = offset / 4U;
            const u32 shift = (offset % 4U) * 8U;
            const u32 mask = 0xFFU << shift;
            const u32 part = static_cast<u32>(value >> (byte * 8U) & 0xFFU)
                << shift;
            action_.opponent_workspace[index] =
                (action_.opponent_workspace[index] & ~mask) | part;
        }
        ++result_.workspace_writes;
        return true;
    }

    [[nodiscard]] bool
    write_workspace_dword(const u32 byte_offset, const u32 value) {
        constexpr u32 kBytes =
            static_cast<u32>(sizeof(action_.opponent_workspace));
        if (byte_offset > kBytes - 4U) {
            typed_stop(Status::action_workspace_typed_stop);
            return false;
        }
        for (u32 byte = 0U; byte < 4U; ++byte) {
            const u32 offset = byte_offset + byte;
            const u32 index = offset / 4U;
            const u32 shift = (offset % 4U) * 8U;
            const u32 mask = 0xFFU << shift;
            const u32 part = (value >> (byte * 8U) & 0xFFU) << shift;
            action_.opponent_workspace[index] =
                (action_.opponent_workspace[index] & ~mask) | part;
        }
        ++result_.workspace_writes;
        return true;
    }

    [[nodiscard]] std::optional<u8> action_remap_byte(const u32 offset) {
        if (offset < runtime_.action_remap_prefix.size()) {
            return runtime_.action_remap_prefix[offset];
        }
        if (offset < 16U) {
            return static_cast<u8>(
                bindings_.startup_reset.value_4fe5cc >> ((offset - 12U) * 8U)
            );
        }
        if (offset < 18U) {
            return static_cast<u8>(
                bindings_.startup_reset.value_4fe5d0 >> ((offset - 16U) * 8U)
            );
        }
        if (offset < 20U) {
            return runtime_.action_remap_gap[offset - 18U];
        }
        if (offset < 60U) {
            const u32 relative = offset - 20U;
            return static_cast<u8>(
                bindings_.startup_reset.block_4fe5d4[relative / 4U] >>
                ((relative % 4U) * 8U)
            );
        }
        if (offset < 74U) {
            return runtime_.action_remap_suffix[offset - 60U];
        }
        typed_stop(Status::action_remap_typed_stop);
        return std::nullopt;
    }

    [[nodiscard]] std::optional<u16> read_action_remap(const u32 action) {
        const u32 offset = action * 2U;
        const auto low = action_remap_byte(offset);
        if (!low.has_value()) {
            return std::nullopt;
        }
        const auto high = action_remap_byte(offset + 1U);
        if (!high.has_value()) {
            return std::nullopt;
        }
        return static_cast<u16>(
            static_cast<u16>(*low) | static_cast<u16>(*high) << 8U
        );
    }

    [[nodiscard]] bool
    write_actor_result(const u32 actor_code, const u16 value) {
        if (actor_code >= runtime_.actor_result_words.size()) {
            typed_stop(Status::actor_result_word_typed_stop);
            return false;
        }
        runtime_.actor_result_words[actor_code] = value;
        return true;
    }

    [[nodiscard]] bool clear_target_marker(const u32 index) {
        if (index >= frame_.target_markers.size()) {
            typed_stop(Status::target_marker_typed_stop);
            return false;
        }
        frame_.target_markers[index] = 0U;
        return true;
    }

    [[nodiscard]] bool prime_input_records() {
        const auto primed = prime_legacy_battle_input_records(records_, edx_);
        ++result_.input_record_prime_calls;
        result_.input_record_writes += primed.record_writes;
        eax_ = primed.return_eax;
        ecx_ = primed.return_ecx;
        edx_ = primed.return_edx;
        if (primed.status != LegacyBattleInputRecordPrimingStatus::completed) {
            typed_stop(Status::input_record_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool prepare_default_target() {
        const auto cycled = cycle_legacy_battle_group_b_target(
            {
                .frame_input = frame_,
                .metrics = metrics_,
                .final_actor = final_actor_,
                .target_runtime = runtime_,
                .message_state = bindings_.message_state,
            },
            port_,
            {.entry_eax = eax_, .entry_ecx = ecx_, .entry_edx = edx_}
        );
        ++result_.group_b_target_cycle_calls;
        result_.group_b_target_cycle = cycled;
        result_.port_calls += cycled.port_calls;
        result_.group_b_calls += cycled.port_calls;
        eax_ = cycled.return_eax;
        ecx_ = cycled.return_ecx;
        edx_ = cycled.return_edx;
        if (cycled.status ==
            LegacyBattleGroupBTargetCycleStatus::target_order_typed_stop) {
            typed_stop(Status::target_actor_index_typed_stop);
            return false;
        }
        if (cycled.status ==
            LegacyBattleGroupBTargetCycleStatus::group_b_actor_typed_stop) {
            typed_stop(Status::group_b_actor_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool cycle_group_a_target() {
        const auto cycled = cycle_legacy_battle_group_a_target(
            {
                .frame_input = frame_,
                .final_actor = final_actor_,
                .metrics = metrics_,
                .target_runtime = runtime_,
                .supplemental_count_word =
                    bindings_.startup_supplemental_count_word,
            },
            {.entry_eax = eax_, .entry_ecx = ecx_, .entry_edx = edx_}
        );
        ++result_.group_a_target_cycle_calls;
        result_.group_a_target_cycle = cycled;
        eax_ = cycled.return_eax;
        ecx_ = cycled.return_ecx;
        edx_ = cycled.return_edx;
        if (cycled.status ==
            LegacyBattleGroupATargetCycleStatus::target_order_typed_stop) {
            typed_stop(Status::group_a_target_order_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool
    select_actor_resource(const u32 actor_code, const u32 category) {
        if (actor_code < 8U) {
            typed_stop(Status::group_a_actor_typed_stop);
            return false;
        }
        const u32 index = actor_code - 8U;
        if (index >= bindings_.party.size() ||
            index >= bindings_.action.group_a_action_execution.size() ||
            index >= bindings_.startup_reset.block_4fe5d4.size()) {
            typed_stop(Status::actor_mode_four_finalization_typed_stop);
            return false;
        }
        auto& party = bindings_.party[index];
        ResourceSelectionAdapter adapter(port_, result_);
        result_.resource_selection = select_legacy_battle_actor_resource(
            &party.actor_list,
            &party.configuration,
            &party.final_processing,
            &party.item_effect_application,
            &party.workspace,
            &bindings_.action.group_a_action_execution[index],
            kGroupABaseToken + index * kGroupAStride,
            adapter,
            {.category_selector = category,
             .occurrence = runtime_.target_argument,
             .entry_eax = eax_,
             .entry_edx = edx_}
        );
        ++result_.resource_selection_calls;
        eax_ = result_.resource_selection.return_eax;
        ecx_ = result_.resource_selection.return_ecx;
        edx_ = result_.resource_selection.return_edx;
        bindings_.startup_reset.block_4fe5d4[index] =
            result_.resource_selection.output_runtime_word;
        local_output_ = result_.resource_selection.output_mode;
        call_output_ = result_.resource_selection.output_mode;
        if (result_.resource_selection.status !=
            LegacyBattleActorListQueryStatus::completed) {
            typed_stop(Status::actor_mode_four_finalization_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool release_actor_resource(const u32 actor_code) {
        if (actor_code < 8U) {
            typed_stop(Status::group_a_actor_typed_stop);
            return false;
        }
        const u32 index = actor_code - 8U;
        if (index >= bindings_.party.size()) {
            typed_stop(Status::group_a_actor_typed_stop);
            return false;
        }

        auto& party = bindings_.party[index];
        result_.resource_release = release_legacy_battle_actor_resource(
            &party.actor_list,
            &party.workspace,
            kGroupABaseToken + index * kGroupAStride,
            {.entry_eax = eax_, .entry_edx = edx_}
        );
        ++result_.resource_release_calls;
        eax_ = result_.resource_release.return_eax;
        ecx_ = result_.resource_release.return_ecx;
        edx_ = result_.resource_release.return_edx;
        if (result_.resource_release.status !=
            LegacyBattleActorListQueryStatus::completed) {
            typed_stop(Status::actor_resource_release_typed_stop);
            return false;
        }

        return true;
    }

    [[nodiscard]] bool final_target_refresh() {
        bindings_.message_state = 3U;
        input_.selection_animation_frame_a = 0U;
        input_.selection_animation_frame_b = 0U;
        input_.selection_animation_phase = 4U;
        return prime_input_records();
    }

    [[nodiscard]] bool
    query_actor_properties(const u32 actor_code, const u32 output_value) {
        if (!invoke_group_a(Call::query_actor_property_a, actor_code)) {
            return false;
        }
        eax_ &= 0xFFFFU;
        input_.selection_target_cache = 0U;
        edx_ = (actor_code - 8U) * 5U;
        if (!write_runtime_record(actor_code, 3U, eax_)) {
            return false;
        }

        if (!invoke_group_a(Call::query_actor_property_b, actor_code)) {
            return false;
        }
        ecx_ = final_actor_.queued_actor_code;
        if (eax_ == 1U) {
            input_.selection_target_cache = 1U;
            edx_ = (ecx_ - 8U) * 5U;
            if (!write_runtime_record(ecx_, 2U, 1U)) {
                return false;
            }
        }

        frame_.target_selection_block = 0U;
        if (!invoke_group_a(
                Call::query_actor_property_c, final_actor_.queued_actor_code
            )) {
            return false;
        }
        static_cast<void>(output_value);
        return true;
    }

    [[nodiscard]] bool resolve_property_fallback(
        const u32 category,
        const u32 actor_code,
        const u32 output_value,
        const bool clear_before_output
    ) {
        if (eax_ == 1U) {
            if (!clear_before_output) {
                eax_ = category;
                ecx_ = metrics_.group_b_count;
            }
            frame_.target_selection_block = 1U;
            runtime_.selection_input_gate = 1U;
            final_actor_.published_actor_code = metrics_.group_b_count;
            if (clear_before_output) {
                ecx_ = category;
                eax_ = actor_code * 5U - 40U;
                if (!write_runtime_record(actor_code, 0U, 0U)) {
                    return false;
                }
            } else if (category == 2U || (output_value & 0xFFFFU) == 0U) {
                eax_ = actor_code;
                ecx_ = metrics_.group_b_count;
                edx_ = actor_code * 5U - 40U;
            }
            if (category == 2U) {
                if (!write_runtime_record(actor_code, 0U, 1U)) {
                    return false;
                }
                bindings_.startup_reset.value_53bfd0 = 1U;
            }
            if ((output_value & 0xFFFFU) == 0U) {
                if (!write_runtime_record(actor_code, 0U, 1U)) {
                    return false;
                }
                if (!clear_before_output) {
                    bindings_.startup_reset.value_53bfd0 = 1U;
                }
            }
            return true;
        }

        if (category != 0U) {
            ecx_ = actor_code;
            eax_ = actor_code * 5U - 40U;
            if (!write_runtime_record(actor_code, 0U, 0U)) {
                return false;
            }
            if ((output_value & 0xFFFFU) == 0U &&
                !write_runtime_record(actor_code, 0U, 1U)) {
                return false;
            }
            u32 record{};
            if (!read_runtime_record(actor_code, 0U, record)) {
                return false;
            }
            if (record == 1U) {
                edx_ = metrics_.group_a_count;
                eax_ = runtime_.target_effect_value >> 16U;
                edx_ -= eax_;
                eax_ =
                    static_cast<u32>(bindings_.startup_supplemental_count_word);
                edx_ -= eax_;
                ecx_ = actor_code;
                if (edx_ >= 4U) {
                    return cycle_group_a_target();
                }
                eax_ = final_actor_.pre_frame_gate_b;
                ecx_ += 0xFFFFFFF9U;
                final_actor_.published_actor_code = ecx_;
                if (eax_ == 0U) {
                    if (!invoke_group_a_one_based(
                            Call::reset_actor_selection,
                            final_actor_.published_actor_code,
                            {1U}
                        )) {
                        return false;
                    }
                    edx_ = 0U;
                    if (!invoke_group_a_one_based(
                            Call::build_selection_snapshot,
                            final_actor_.published_actor_code
                        )) {
                        return false;
                    }
                    runtime_.selection_input_gate = 1U;
                }
                return true;
            }
        }

        eax_ = final_actor_.pre_frame_gate_b;
        if (eax_ == 0U && !prepare_default_target()) {
            return false;
        }
        return true;
    }

    void draw_target_panel() {
        ecx_ = kTargetPanelToken;
        invoke(Call::draw_target_panel, kTargetPanelToken, {0xF0U, 0xC6U});
    }

    void set_cursor() {
        invoke(Call::set_cursor_position, 0U, {0x108U, 0xB0U});
    }

    [[nodiscard]] bool display_warning(const u32 text_token) {
        InputTextMessageAdapter text_message_port(port_);
        result_.text_messages.push_back(enqueue_legacy_battle_text_message(
            bindings_.text_messages,
            bindings_.startup_reset.block_5214f8[0U],
            text_message_port,
            {
                .value_04 = 0x118U,
                .value_08 = 0xAU,
                .kind = 0x14U,
                .text_token = text_token,
                .flags = 0x40000002U,
                .entry = {.eax = eax_, .ecx = ecx_, .edx = edx_},
            }
        ));
        ++result_.text_message_calls;
        const auto& message = result_.text_messages.back();
        result_.port_calls += message.allocation_calls + message.measure_calls;
        eax_ = message.return_registers.eax;
        ecx_ = message.return_registers.ecx;
        edx_ = message.return_registers.edx;
        if (message.status != LegacyBattleTextMessageStatus::completed) {
            typed_stop(Status::text_message_typed_stop);
            return false;
        }
        return true;
    }

    [[nodiscard]] bool commit_actor_action_common(const u32 action_kind) {
        const u32 actor_code = final_actor_.queued_actor_code;
        eax_ = actor_code;
        ecx_ = action_kind;
        runtime_.selected_action_kind = action_kind;
        bindings_.debug_hotkeys.committed_actor_code = actor_code;
        if (!write_actor_action(actor_code, action_kind)) {
            return false;
        }
        final_actor_.queued_actor_code = 0U;
        runtime_.actor_commit_gate = 1U;
        if (!invoke_group_a(Call::refresh_actor_selection, actor_code, {1U})) {
            return false;
        }
        eax_ = actor_code;
        bindings_.message_state = 0U;
        final_actor_.pre_frame_gate_b = 0U;
        runtime_.selection_input_gate = 0U;
        input_.selection_animation_frame_a = 0U;
        input_.selection_animation_frame_b = 0U;
        input_.selection_cache_gate_a = 0U;
        input_.selection_cache_gate_b = 0U;
        edx_ = actor_code * 5U - 40U;
        if (!write_runtime_record(actor_code, 0U, 1U)) {
            return false;
        }
        return true;
    }

    [[nodiscard]] bool
    write_effect_record(const u32 logical_index, const u16 value, const u16 y) {
        const u32 base = logical_index * 0x20U;
        if (!write_workspace_word(0x56U + base, 0x12CU) ||
            !write_workspace_word(0x58U + base, y) ||
            !write_workspace_word(0x54U + base, value)) {
            return false;
        }
        edx_ = bindings_.startup_mirror_mode;
        if (!write_workspace_dword(0x5CU + base, 1U)) {
            return false;
        }
        if (edx_ == 1U) {
            const auto x = read_workspace_word(0x56U + base);
            if (!x.has_value()) {
                return false;
            }
            edx_ = replace_low_word(edx_, 0x280U);
            edx_ = replace_low_word(edx_, edx_ - *x);
            if (!write_workspace_word(0x56U + base, static_cast<u16>(edx_))) {
                return false;
            }
        }
        return true;
    }

    void case_one() {
        if (runtime_.selection_input_gate == 0U) {
            return;
        }
        eax_ = input_.selection_animation_frame_b;
        edx_ = 6U;
        if (signed_bits(eax_) < 6) {
            return;
        }

        eax_ = input_.action_kind;
        runtime_.selection_input_gate = 0U;
        frame_.target_selection_block = 0U;
        input_.selection_target_cache = 0U;
        frame_.target_selection_gate = 1U;
        input_.fallback_action_kind = eax_;
        if (signed_bits(eax_) >= 6) {
            const auto remap = read_action_remap(eax_);
            if (!remap.has_value()) {
                return;
            }
            ecx_ = replace_low_word(ecx_, *remap);
            if (*remap != 0U) {
                ecx_ &= 0xFFFFU;
                eax_ = ecx_;
                input_.action_kind = eax_;
            }
        }

        const u32 switch_index = eax_ - 1U;
        const u32 selected_action = eax_;
        if (switch_index <= 35U) {
            ecx_ = action_selector(eax_);
            switch (selected_action) {
            case 1U: {
                edx_ = frame_.target_cursor;
                bindings_.message_state = 3U;
                u32 target{};
                if (!read_target_actor(edx_, target)) {
                    return;
                }
                eax_ = target;
                frame_.target_actor_index = eax_;
                break;
            }
            case 2U: {
                bindings_.message_state = 2U;
                runtime_.selection_input_gate = 1U;
                set_cursor();
                ecx_ = frame_.target_cursor;
                input_.selection_animation_frame_a = 0U;
                u32 target{};
                if (!read_target_actor(ecx_, target)) {
                    return;
                }
                edx_ = target;
                frame_.target_actor_index = edx_;
                input_.selection_animation_frame_b = 0U;
                input_.selection_animation_phase = 4U;
                frame_.panel_scroll_a = 0U;
                draw_target_panel();
                break;
            }
            case 3U: {
                frame_.grid_selection = 1U;
                frame_.panel_scroll_b = 0U;
                bindings_.message_state = 4U;
                runtime_.selection_input_gate = 1U;
                set_cursor();
                eax_ = frame_.target_cursor;
                u32 target{};
                if (!read_target_actor(eax_, target)) {
                    return;
                }
                ecx_ = target;
                frame_.target_actor_index = ecx_;
                draw_target_panel();
                break;
            }
            case 4U: {
                bindings_.message_state = 8U;
                frame_.narrow_list_selection = 1U;
                runtime_.selection_input_gate = 1U;
                set_cursor();
                ecx_ = frame_.target_cursor;
                u32 target{};
                if (!read_target_actor(ecx_, target)) {
                    return;
                }
                edx_ = target;
                frame_.target_actor_index = edx_;
                draw_target_panel();
                break;
            }
            case 5U: {
                const u32 actor_code = final_actor_.queued_actor_code;
                edx_ = actor_code;
                ecx_ = actor_code - 8U;
                runtime_.selected_action_kind = selected_action;
                if (!write_actor_action(actor_code, selected_action) ||
                    !invoke_group_a(Call::apply_actor_action, actor_code) ||
                    !invoke_group_a(
                        Call::refresh_actor_selection,
                        final_actor_.queued_actor_code,
                        {1U}
                    )) {
                    return;
                }
                eax_ = final_actor_.queued_actor_code;
                ecx_ = eax_ * 5U - 40U;
                edx_ = eax_ - 7U;
                final_actor_.published_actor_code = edx_;
                if (!write_runtime_record(eax_, 0U, 1U)) {
                    return;
                }
                bindings_.debug_hotkeys.committed_actor_code = eax_;
                bindings_.startup_reset.value_4ff0b0 = 0U;
                final_actor_.queued_actor_code = 0U;
                bindings_.startup_reset.value_4ff0b4 = 0U;
                runtime_.actor_commit_gate = 1U;
                bindings_.message_state = 0U;
                runtime_.selection_input_gate = 0U;
                bindings_.startup_reset.value_4ff0b8 = 0U;
                bindings_.startup_reset.value_53bf22 = 0U;
                break;
            }
            case 17U:
                eax_ = final_actor_.queued_actor_code;
                final_actor_.queued_actor_code = 0U;
                bindings_.debug_hotkeys.committed_actor_code = eax_;
                bindings_.message_state = edx_;
                break;
            case 22U: {
                const u32 actor_code = final_actor_.queued_actor_code;
                ecx_ = actor_code - 8U;
                runtime_.selected_action_kind = selected_action;
                bindings_.message_state = 0U;
                runtime_.selection_input_gate = 1U;
                if (!write_actor_action(actor_code, selected_action) ||
                    !invoke_group_a(
                        Call::refresh_actor_selection, actor_code, {1U}
                    )) {
                    return;
                }
                eax_ = final_actor_.queued_actor_code;
                edx_ = 0U;
                bindings_.startup_reset.value_4ff0b0 = 0U;
                bindings_.debug_hotkeys.committed_actor_code = eax_;
                ecx_ = eax_ - 7U;
                bindings_.startup_reset.value_4ff0b4 = 0U;
                final_actor_.published_actor_code = ecx_;
                final_actor_.queued_actor_code = 0U;
                runtime_.actor_commit_gate = 1U;
                runtime_.selection_input_gate = 0U;
                bindings_.startup_reset.value_4ff0b8 = 0U;
                bindings_.startup_reset.value_53bf22 = 0U;
                break;
            }
            case 24U:
                bindings_.message_state = 3U;
                runtime_.selection_input_gate = 1U;
                frame_.target_selection_block = 1U;
                break;
            case 25U:
                eax_ = runtime_.action_mode_flags;
                eax_ = replace_low_byte(eax_, eax_ | 0x10U);
                runtime_.action_mode_flags = eax_;
                bindings_.message_state = 3U;
                break;
            case 26U:
                bindings_.message_state = 3U;
                runtime_.selection_aux_gate = 0U;
                break;
            case 27U:
                ecx_ = kTargetPanelToken;
                bindings_.message_state = 27U;
                frame_.grid_selection = 1U;
                frame_.panel_scroll_b = 0U;
                draw_target_panel();
                break;
            case 30U:
                ecx_ = kTargetPanelToken;
                bindings_.message_state = 30U;
                frame_.grid_selection = 1U;
                draw_target_panel();
                break;
            case 28U:
            case 32U:
            case 34U:
            case 35U:
            case 36U: {
                const u32 actor_code = final_actor_.queued_actor_code;
                ecx_ = actor_code - 8U;
                runtime_.selected_action_kind = selected_action;
                if (!write_actor_action(actor_code, selected_action) ||
                    !invoke_group_a(
                        Call::refresh_actor_selection, actor_code, {1U}
                    )) {
                    return;
                }
                eax_ = final_actor_.queued_actor_code;
                edx_ = eax_ * 5U - 40U;
                ecx_ = eax_ - 7U;
                final_actor_.published_actor_code = ecx_;
                if (!write_runtime_record(eax_, 0U, 1U)) {
                    return;
                }
                bindings_.debug_hotkeys.committed_actor_code = eax_;
                bindings_.startup_reset.value_4ff0b0 = 0U;
                final_actor_.queued_actor_code = 0U;
                bindings_.startup_reset.value_4ff0b4 = 0U;
                runtime_.actor_commit_gate = 1U;
                bindings_.message_state = 0U;
                runtime_.selection_input_gate = 0U;
                bindings_.startup_reset.value_4ff0b8 = 0U;
                bindings_.startup_reset.value_53bf22 = 0U;
                break;
            }
            case 6U:
            case 21U:
            case 23U:
            case 29U:
            case 31U:
            case 33U:
                bindings_.message_state = 3U;
                break;
            default:
                break;
            }
        }
        if (stopped_) {
            return;
        }
        if (final_actor_.pre_frame_gate_b == 0U &&
            bindings_.message_state == 3U && !prepare_default_target()) {
            return;
        }
        if (!prime_input_records()) {
            return;
        }
        input_.selection_animation_phase = 4U;
        input_.selection_animation_frame_a = 0U;
        input_.selection_animation_frame_b = 0U;
    }

    void case_two() {
        eax_ = frame_.hovered_equipment;
        const u32 minus_one = 0xFFFFFFFFU;
        if (eax_ != minus_one) {
            ecx_ = std::bit_cast<u32>(input_.sample_mix_level);
            input_.action_category_index = eax_;
            frame_.hovered_equipment = minus_one;
            frame_.list_selection = 1U;
            frame_.panel_scroll_a = 0U;
            sample(0x2EU);
            return;
        }
        if (runtime_.selection_input_gate == 0U) {
            return;
        }
        eax_ = runtime_.candidate_gate_a;
        runtime_.selection_input_gate = 0U;
        if (eax_ != 0U || runtime_.candidate_gate_b != 0U) {
            return;
        }
        ecx_ = runtime_.candidate_argument;
        if (ecx_ == 0U) {
            return;
        }
        eax_ = final_actor_.queued_actor_code;
        if (eax_ == 0U) {
            return;
        }
        edx_ = input_.action_category_index;
        const u32 actor_code = eax_;
        if (!invoke_group_a(
                Call::validate_primary_action, actor_code, {edx_, ecx_}
            )) {
            return;
        }
        if (eax_ == 0U) {
            return;
        }
        const u32 category = input_.action_category_index;
        const u32 argument = runtime_.candidate_argument;
        local_output_ = 0U;
        edx_ = category;
        if (!invoke_group_a(
                Call::query_primary_target,
                final_actor_.queued_actor_code,
                {
                    category,
                    0U,
                    argument,
                    kActorRuntimeRecordToken +
                        (final_actor_.queued_actor_code - 8U) * 4U,
                }
            )) {
            return;
        }
        local_output_ = call_output_;
        const u32 current_actor = final_actor_.queued_actor_code;
        if (!query_actor_properties(current_actor, local_output_)) {
            return;
        }
        if (!resolve_property_fallback(
                input_.action_category_index,
                current_actor,
                local_output_,
                false
            )) {
            return;
        }
        if (!final_target_refresh()) {
            return;
        }
    }

    void case_three() {
        if (final_actor_.published_actor_code == 0xFFFFFFFFU) {
            return;
        }
        eax_ = final_actor_.queued_actor_code;
        if (eax_ == 0U && input_.selected_actor_cleanup_gate == 0U) {
            return;
        }
        ecx_ = input_.action_kind;
        if (ecx_ == 6U && frame_.target_action_available == 0U) {
            ecx_ = std::bit_cast<u32>(input_.sample_mix_level);
            sample(0x8CU);
            return;
        }
        if (runtime_.selection_input_gate == 0U) {
            if (final_actor_.pre_frame_gate_b == 0U) {
                runtime_.selection_input_gate = 1U;
            }
            return;
        }

        const u32 actor_code = final_actor_.queued_actor_code;
        runtime_.selected_action_kind = ecx_;
        if (!write_actor_action(actor_code, ecx_)) {
            return;
        }
        bindings_.debug_hotkeys.committed_actor_code = actor_code;
        final_actor_.queued_actor_code = 0U;
        runtime_.actor_commit_gate = 1U;
        if (!invoke_group_a(Call::refresh_actor_selection, actor_code, {1U})) {
            return;
        }
        bindings_.message_state = 0U;
        final_actor_.pre_frame_gate_b = 0U;
        runtime_.selection_input_gate = 0U;
        eax_ = bindings_.debug_hotkeys.committed_actor_code;
        if (frame_.target_selection_block == 1U) {
            ecx_ = eax_ * 5U - 40U;
            if (!write_runtime_record(eax_, 1U, 1U) ||
                !write_runtime_record(eax_, 4U, 0U)) {
                return;
            }
            ecx_ = runtime_.selected_action_kind;
            if (!write_actor_action(eax_, runtime_.selected_action_kind)) {
                return;
            }
        }

        if (input_.action_kind == 30U) {
            const u32 committed = bindings_.debug_hotkeys.committed_actor_code;
            ecx_ = committed - 8U;
            eax_ = frame_.current_equipment_selection;
            edx_ = 3U;
            runtime_.selected_action_kind = edx_;
            if (!write_actor_action(committed, edx_)) {
                return;
            }
            edx_ = runtime_.target_argument;
            if (bindings_.scripted_resource_release_test_compat) {
                if (!invoke_group_a(
                        Call::resolve_action_effect_value,
                        committed,
                        {eax_, edx_},
                        GroupARegisterShape::eax_3ef
                    )) {
                    return;
                }
            } else if (!release_actor_resource(committed)) {
                return;
            }
            edx_ = committed;
            if (committed < 8U) {
                typed_stop(Status::group_a_actor_typed_stop);
                return;
            }
            const u32 override_index = committed - 8U;
            if (override_index >=
                bindings_.action.group_a_action_execution.size()) {
                typed_stop(Status::group_a_actor_typed_stop);
                return;
            }
            ecx_ = kGroupABaseToken + override_index * kGroupAStride;
            result_.action_thirty_override =
                query_legacy_battle_actor_action_thirty_override(
                    &bindings_.action
                         .group_a_action_execution[override_index],
                    eax_,
                    ecx_,
                    edx_
                );
            ++result_.action_thirty_override_calls;
            eax_ = result_.action_thirty_override.return_eax;
            ecx_ = result_.action_thirty_override.return_ecx;
            edx_ = result_.action_thirty_override.return_edx;
            if (result_.action_thirty_override.status !=
                LegacyBattleActorActionThirtyOverrideStatus::completed) {
                typed_stop(Status::group_a_actor_typed_stop);
                return;
            }
            if (eax_ == 1U) {
                eax_ = 13U;
                ecx_ = committed;
                runtime_.selected_action_kind = eax_;
                runtime_.actor_special_gate = 1U;
                if (!write_actor_action(committed, eax_)) {
                    return;
                }
                eax_ = runtime_.special_action_count + 1U;
                runtime_.special_action_count = eax_;
            } else {
                invoke(Call::stop_effect_sample, 0U, {0x300U, 0U});
                eax_ = bindings_.debug_hotkeys.committed_actor_code;
            }
        } else if (runtime_.selected_action_kind == 4U) {
            if (bindings_.scripted_resource_selection_test_compat) {
                if (!invoke_group_a(
                        Call::apply_special_actor_action,
                        bindings_.debug_hotkeys.committed_actor_code
                    )) {
                    return;
                }
            } else if (!select_actor_resource(
                           bindings_.debug_hotkeys.committed_actor_code,
                           frame_.current_equipment_selection
                       )) {
                return;
            }
        }

        if (input_.action_kind == 27U) {
            frame_.grid_selection = 1U;
            frame_.panel_scroll_b = 0U;
        }

        eax_ = metrics_.group_b_count;
        u32 group_b_index = 0U;
        if (signed_bits(eax_) > 0) {
            do {
                ecx_ = kGroupBBaseToken + group_b_index * kGroupBStride;
                if (group_b_index >= kGroupBCount) {
                    typed_stop(Status::group_b_actor_typed_stop);
                    return;
                }
                invoke(Call::reset_actor_selection, ecx_, {0U});
                ++result_.group_b_calls;
                eax_ = metrics_.group_b_count;
                ++group_b_index;
            } while (signed_bits(group_b_index) < signed_bits(eax_));
        }

        u32 group_a_index = 0U;
        do {
            ecx_ = kGroupABaseToken + group_a_index * kGroupAStride;
            invoke(Call::reset_actor_selection, ecx_, {0U});
            ++result_.group_a_calls;
            if (!clear_target_marker(group_a_index)) {
                return;
            }
            ++group_a_index;
        } while (group_a_index < 4U);

        input_.selection_animation_frame_a = 0U;
        input_.selection_animation_frame_b = 0U;
        frame_.lower_panel_aux = 0U;
        frame_.grid_selection = 1U;
        input_.selection_cache_gate_a = 0U;
        input_.selection_cache_gate_b = 0U;
        input_.selection_cache_gate_c = 0U;
        if (!prime_input_records()) {
            return;
        }

        if (input_.selected_actor_cleanup_gate == 1U) {
            if (!invoke_group_a(
                    Call::query_actor_cleanup,
                    bindings_.debug_hotkeys.committed_actor_code
                )) {
                return;
            }
            if (eax_ == 1U) {
                const u32 committed =
                    bindings_.debug_hotkeys.committed_actor_code;
                eax_ = 5U;
                edx_ = committed;
                ecx_ = committed - 8U;
                runtime_.selected_action_kind = eax_;
                if (!write_actor_action(committed, eax_) ||
                    !invoke_group_a(Call::apply_actor_action, committed) ||
                    !invoke_group_a(
                        Call::refresh_actor_selection, committed, {1U}
                    )) {
                    return;
                }
                eax_ = committed * 5U - 40U;
                if (!write_runtime_record(committed, 0U, 1U)) {
                    return;
                }
            }
            ecx_ = final_actor_.published_actor_code;
            frame_.selection_actor_code = ecx_;
        }
        edx_ = 0U;
        bindings_.startup_reset.value_4ff0b0 = edx_;
        bindings_.startup_reset.value_4ff0b4 = edx_;
        bindings_.startup_reset.value_53bf22 = 0U;
        bindings_.startup_reset.value_4ff0b8 = edx_;
    }

    [[nodiscard]] bool commit_action_fifteen_direct() {
        const u32 actor_code = final_actor_.queued_actor_code;
        eax_ = actor_code;
        ecx_ = 15U;
        runtime_.selected_action_kind = 15U;
        bindings_.debug_hotkeys.committed_actor_code = actor_code;
        if (!write_actor_action(actor_code, 15U)) {
            return false;
        }
        final_actor_.queued_actor_code = 0U;
        runtime_.actor_commit_gate = 1U;
        if (!invoke_group_a(Call::refresh_actor_selection, actor_code, {1U})) {
            return false;
        }
        eax_ = actor_code;
        bindings_.message_state = 0U;
        final_actor_.pre_frame_gate_b = 0U;
        runtime_.selection_input_gate = 0U;
        input_.selection_animation_frame_a = 0U;
        input_.selection_animation_frame_b = 0U;
        input_.selection_cache_gate_a = 0U;
        ecx_ = actor_code * 5U - 40U;
        if (!write_runtime_record(actor_code, 0U, 1U)) {
            return false;
        }
        input_.selection_cache_gate_b = 0U;
        if (!prime_input_records()) {
            return false;
        }
        if (input_.selected_actor_cleanup_gate == 1U) {
            edx_ = final_actor_.published_actor_code;
            frame_.selection_actor_code = edx_;
        }
        ecx_ = runtime_.target_argument;
        eax_ = 0U;
        bindings_.startup_reset.value_4ff0b0 = eax_;
        bindings_.startup_reset.value_4ff0b4 = eax_;
        edx_ = frame_.current_equipment_selection;
        bindings_.startup_reset.value_4ff0b8 = eax_;
        bindings_.startup_reset.value_53bf22 = 0U;
        if (bindings_.scripted_resource_release_test_compat) {
            if (!invoke_group_a(
                    Call::resolve_action_effect_value,
                    bindings_.debug_hotkeys.committed_actor_code,
                    {edx_, ecx_},
                    GroupARegisterShape::eax_3ef
                )) {
                return false;
            }
        } else if (!release_actor_resource(
                       bindings_.debug_hotkeys.committed_actor_code
                   )) {
            return false;
        }
        runtime_.target_effect_value =
            replace_low_word(runtime_.target_effect_value, eax_);
        edx_ = eax_ & 0xFFFFU;
        invoke(Call::start_effect_sample, 0U, {1U, edx_});
        ecx_ = metrics_.group_a_count;
        edx_ = runtime_.target_effect_value & 0xFFFFU;
        eax_ = ecx_ * 0x20U;
        if (!write_effect_record(ecx_, static_cast<u16>(edx_), 0x10EU)) {
            return false;
        }
        eax_ = bindings_.debug_hotkeys.committed_actor_code;
        runtime_.target_effect_value =
            replace_low_word(runtime_.target_effect_value, ecx_);
        return write_actor_result(eax_, static_cast<u16>(ecx_));
    }

    void case_four() {
        eax_ = frame_.hovered_secondary;
        const u32 minus_one = 0xFFFFFFFFU;
        if (eax_ != minus_one) {
            frame_.current_equipment_selection = eax_;
            eax_ = std::bit_cast<u32>(input_.sample_mix_level);
            frame_.hovered_secondary = minus_one;
            frame_.grid_selection = 1U;
            frame_.panel_scroll_b = 0U;
            sample(0x2EU);
            return;
        }
        if (runtime_.selection_input_gate == 0U) {
            return;
        }
        eax_ = runtime_.candidate_gate_a;
        runtime_.selection_input_gate = 0U;
        if (eax_ != 0U || runtime_.candidate_gate_b != 0U) {
            return;
        }
        eax_ = final_actor_.queued_actor_code;
        if (eax_ == 0U) {
            return;
        }
        const u32 actor_code = eax_;
        local_output_ = minus_one;
        edx_ = runtime_.target_argument;
        eax_ = frame_.current_equipment_selection;
        if (!invoke_group_a(
                Call::resolve_action_target,
                actor_code,
                {
                    eax_,
                    edx_,
                    kActorRuntimeRecordToken + (actor_code - 8U) * 4U,
                },
                GroupARegisterShape::eax_3ef
            )) {
            return;
        }
        local_output_ = call_output_;
        if (eax_ == 0U) {
            eax_ = frame_.current_equipment_selection;
            if ((eax_ == 3U || eax_ == 0U || eax_ == 2U) &&
                (local_output_ & 0xFFFFU) == 0xFFFFU) {
                return;
            }
            if ((eax_ == 3U || eax_ == 0U || eax_ == 2U) &&
                !display_warning(kWarningTextBToken)) {
                return;
            }
            edx_ = std::bit_cast<u32>(input_.sample_mix_level);
            sample(0x8CU);
            return;
        }

        if (!invoke_group_a(Call::query_action_four_override, actor_code)) {
            return;
        }
        if (eax_ == 1U) {
            if ((runtime_.target_effect_value & 0xFFFFU) != 0U) {
                if (!display_warning(kWarningTextAToken)) {
                    return;
                }
                edx_ = std::bit_cast<u32>(input_.sample_mix_level);
                sample(0x8CU);
                return;
            }
            eax_ = runtime_.target_effect_value >> 16U;
            if (static_cast<u16>(eax_) >= 1U) {
                bindings_.message_state = 5U;
                frame_.group_b_row_selection = 2U;
                if (static_cast<u16>(eax_) == 2U) {
                    frame_.group_b_row_selection = 1U;
                }
                return;
            }
            static_cast<void>(commit_action_fifteen_direct());
            return;
        }

        if (!query_actor_properties(actor_code, local_output_)) {
            return;
        }
        if (!resolve_property_fallback(
                frame_.current_equipment_selection,
                actor_code,
                local_output_,
                true
            )) {
            return;
        }
        if (!final_target_refresh()) {
            return;
        }
    }

    void case_five() {
        eax_ = final_actor_.queued_actor_code;
        if (eax_ == 0U) {
            return;
        }
        const u32 actor_code = eax_;
        ecx_ = runtime_.target_argument;
        edx_ = frame_.current_equipment_selection;
        if (bindings_.scripted_resource_release_test_compat) {
            if (!invoke_group_a(
                    Call::resolve_action_effect_value, actor_code, {edx_, ecx_}
                )) {
                return;
            }
        } else if (!release_actor_resource(actor_code)) {
            return;
        }
        runtime_.target_effect_value =
            replace_low_word(runtime_.target_effect_value, eax_);
        if (!commit_actor_action_common(15U)) {
            return;
        }
        if (!prime_input_records()) {
            return;
        }

        eax_ = runtime_.target_effect_value >> 16U;
        ecx_ = eax_;
        eax_ = frame_.group_b_row_selection;
        edx_ = eax_ - ecx_;
        ecx_ = metrics_.group_a_count;
        const u32 logical_index = edx_ + ecx_ - 1U;
        u16 published = static_cast<u16>(runtime_.target_effect_value);
        if (eax_ == 1U) {
            const auto old = read_workspace_word(0x54U + logical_index * 0x20U);
            if (!old.has_value()) {
                return;
            }
            eax_ = replace_low_word(eax_, *old);
            if (*old != 0U) {
                invoke(Call::stop_effect_sample, 0U, {*old, 0U});
                eax_ = debug_.battle_mode_flags_53bc24;
                eax_ = replace_low_byte(eax_, eax_ | 4U);
                debug_.battle_mode_flags_53bc24 = eax_;
            }
            published = static_cast<u16>(runtime_.target_effect_value);
            if (!write_effect_record(logical_index, published, 0x10EU)) {
                return;
            }
            eax_ = published;
            ecx_ = published;
            invoke(Call::start_effect_sample, 0U, {1U, ecx_});
            eax_ = frame_.group_b_row_selection;
            ecx_ = replace_low_word(ecx_, logical_index);
            runtime_.target_effect_value =
                replace_low_word(runtime_.target_effect_value, logical_index);
        } else {
            ecx_ = replace_low_word(ecx_, runtime_.target_effect_value);
        }

        if (eax_ == 2U) {
            const auto old = read_workspace_word(0x54U + logical_index * 0x20U);
            if (!old.has_value()) {
                return;
            }
            eax_ = replace_low_word(eax_, *old);
            if (*old != 0U) {
                invoke(Call::stop_effect_sample, 0U, {*old, 0U});
                eax_ = debug_.battle_mode_flags_53bc24;
                ecx_ = replace_low_word(ecx_, runtime_.target_effect_value);
                eax_ = replace_low_byte(eax_, eax_ | 4U);
                debug_.battle_mode_flags_53bc24 = eax_;
            }
            if (!write_effect_record(
                    logical_index, static_cast<u16>(ecx_), 0x154U
                )) {
                return;
            }
            eax_ = static_cast<u16>(ecx_);
            invoke(Call::start_effect_sample, 0U, {2U, eax_});
            ecx_ = replace_low_word(ecx_, logical_index);
        }

        edx_ = bindings_.debug_hotkeys.committed_actor_code;
        eax_ = 0U;
        bindings_.startup_reset.value_4ff0b0 = eax_;
        runtime_.target_effect_value =
            replace_low_word(runtime_.target_effect_value, 0U);
        bindings_.startup_reset.value_4ff0b4 = eax_;
        if (!write_actor_result(edx_, static_cast<u16>(ecx_))) {
            return;
        }
        bindings_.startup_reset.value_4ff0b8 = eax_;
        bindings_.startup_reset.value_53bf22 = 0U;
        const u32 finalized_actor_code =
            bindings_.debug_hotkeys.committed_actor_code;
        if (finalized_actor_code < 8U) {
            typed_stop(Status::group_a_actor_typed_stop);
            return;
        }
        const u32 finalized_index = finalized_actor_code - 8U;
        if (finalized_index >= bindings_.party.size()) {
            typed_stop(Status::actor_mode_four_finalization_typed_stop);
            return;
        }
        auto& party = bindings_.party[finalized_index];
        const u32 actor_token =
            kGroupABaseToken + finalized_index * kGroupAStride;
        result_.mode_four_finalization = finalize_legacy_battle_actor_mode_four(
            &party.final_processing,
            &party.item_effect_application,
            &party.workspace,
            actor_token,
            eax_
        );
        ++result_.mode_four_finalization_calls;
        eax_ = result_.mode_four_finalization.return_eax;
        ecx_ = result_.mode_four_finalization.return_ecx;
        edx_ = result_.mode_four_finalization.return_edx;
        if (result_.mode_four_finalization.status !=
            LegacyBattleActorModeFourFinalizationStatus::completed) {
            typed_stop(Status::actor_mode_four_finalization_typed_stop);
        }
    }

    void case_seven() {
        ecx_ = frame_.alternate_selection;
        eax_ = frame_.alternate_selection_limit;
        if (ecx_ == eax_) {
            runtime_.selected_action_kind = 99U;
            if (!prime_input_records()) {
                return;
            }
            const u32 actor_code = final_actor_.queued_actor_code;
            if (!invoke_group_a(
                    Call::refresh_actor_selection, actor_code, {1U}
                )) {
                return;
            }
            eax_ = final_actor_.queued_actor_code;
            edx_ = 0U;
            bindings_.startup_reset.value_4ff0b0 = edx_;
            runtime_.actor_commit_gate = 1U;
            ecx_ = eax_ - 7U;
            bindings_.startup_reset.value_4ff0b4 = edx_;
            final_actor_.queued_actor_code = 0U;
            runtime_.selection_input_gate = 0U;
            bindings_.message_state = 0U;
            input_.selection_animation_frame_a = 0U;
            input_.selection_animation_frame_b = 0U;
            bindings_.startup_reset.value_53bf22 = 0U;
            final_actor_.published_actor_code = ecx_;
            bindings_.debug_hotkeys.committed_actor_code = eax_;
            input_.selection_animation_phase = 4U;
            bindings_.startup_reset.value_4ff0b8 = edx_;
            return;
        }

        bindings_.message_state = 3U;
        input_.action_kind = 25U;
        if (final_actor_.pre_frame_gate_b == 0U && !prepare_default_target()) {
            return;
        }
        if (!final_target_refresh()) {
            return;
        }
    }

    void case_eight() {
        if (runtime_.selection_input_gate == 0U) {
            return;
        }
        eax_ = final_actor_.queued_actor_code;
        if (eax_ == 0U) {
            return;
        }
        const u32 actor_code = eax_;
        runtime_.selection_input_gate = 0U;
        eax_ = runtime_.candidate_argument;
        local_output_ = 0U;
        edx_ = kActorRuntimeRecordToken + (actor_code - 8U) * 4U;
        if (!invoke_group_a(
                Call::query_primary_target,
                actor_code,
                {
                    0U,
                    1U,
                    eax_,
                    kActorRuntimeRecordToken + (actor_code - 8U) * 4U,
                },
                GroupARegisterShape::eax_3ef
            )) {
            return;
        }
        local_output_ = call_output_;
        if ((eax_ & 0xFFFFU) == 0xFFFFU) {
            return;
        }
        if (!query_actor_properties(actor_code, local_output_)) {
            return;
        }
        if (!resolve_property_fallback(0U, actor_code, local_output_, false)) {
            return;
        }
        if (!final_target_refresh()) {
            return;
        }
    }

    void case_twenty_seven() {
        if (runtime_.selection_input_gate == 0U) {
            return;
        }
        eax_ = runtime_.candidate_gate_a;
        runtime_.selection_input_gate = 0U;
        if (eax_ != 0U || runtime_.candidate_gate_b != 0U) {
            return;
        }
        eax_ = final_actor_.queued_actor_code;
        if (eax_ == 0U) {
            return;
        }
        local_output_ = 0U;
        edx_ = kActorRuntimeRecordToken + (eax_ - 8U) * 4U;
        if (bindings_.scripted_resource_selection_test_compat) {
            if (!invoke_group_a(
                    Call::resolve_action_target,
                    eax_,
                    {
                        4U,
                        runtime_.target_argument,
                        kActorRuntimeRecordToken + (eax_ - 8U) * 4U,
                    },
                    GroupARegisterShape::eax_3ef
                )) {
                return;
            }
        } else if (!select_actor_resource(eax_, 4U)) {
            return;
        }
        local_output_ = call_output_;
        if (eax_ == 0U) {
            return;
        }
        if (final_actor_.pre_frame_gate_b == 0U && !prepare_default_target()) {
            return;
        }
        if (!final_target_refresh()) {
            return;
        }
    }

    void case_thirty() {
        if (runtime_.selection_input_gate == 0U) {
            return;
        }
        eax_ = final_actor_.queued_actor_code;
        if (eax_ == 0U) {
            return;
        }
        ecx_ = frame_.grid_selection;
        edx_ = frame_.panel_row_limit_c;
        runtime_.selection_input_gate = 0U;
        if (signed_bits(ecx_) > signed_bits(edx_)) {
            eax_ = std::bit_cast<u32>(input_.sample_mix_level);
            sample(0x8CU);
            return;
        }
        const u32 actor_code = eax_;
        local_output_ = 0U;
        edx_ = runtime_.target_argument;
        if (bindings_.scripted_resource_selection_test_compat) {
            if (!invoke_group_a(
                    Call::resolve_action_target,
                    actor_code,
                    {
                        5U,
                        runtime_.target_argument,
                        kActorRuntimeRecordToken + (actor_code - 8U) * 4U,
                    }
                )) {
                return;
            }
        } else if (!select_actor_resource(actor_code, 5U)) {
            return;
        }
        local_output_ = call_output_;
        if (eax_ == 0U) {
            ecx_ = std::bit_cast<u32>(input_.sample_mix_level);
            sample(0x8CU);
            return;
        }
        if (!query_actor_properties(actor_code, local_output_)) {
            return;
        }
        if (!resolve_property_fallback(5U, actor_code, local_output_, false)) {
            return;
        }
        if (!final_target_refresh()) {
            return;
        }
    }

    void case_ninety_eight() {
        runtime_.transition_control_words = 0U;
        runtime_.transition_stage = 0U;
        runtime_.transition_timer = 0U;
        runtime_.transition_aux_byte = 0U;
        bindings_.message_state = 99U;
        runtime_.transition_actor_index = 0xFFU;
    }

    void case_one_hundred() {
        if (signed_bits(runtime_.transition_timer) >= 20) {
            runtime_.transition_timer = 0U;
            runtime_.transition_stage = 0U;
            bindings_.message_state = 101U;
        }
    }

    void case_one_hundred_one() {
        eax_ = runtime_.transition_state;
        runtime_.transition_packed_value &= 0x0000FFFFU;
        if (eax_ == 0U || signed_bits(runtime_.transition_timer) < 30) {
            return;
        }
        eax_ = replace_low_byte(eax_, runtime_.transition_actor_index);
        runtime_.transition_stage = 0U;
        if (runtime_.transition_actor_index == 0xFFU) {
            if (runtime_.transition_sample_word != 0U) {
                eax_ = std::bit_cast<u32>(input_.sample_mix_level);
                sample(0x160U);
            }
            runtime_.transition_timer = 0U;
            bindings_.target_ready_gate = 0U;
            runtime_.transition_state = 0U;
            bindings_.message_state = 102U;
        } else {
            edx_ = std::bit_cast<u32>(input_.sample_mix_level);
            sample(0x2CU);
            runtime_.transition_state = 0U;
            runtime_.transition_timer = 0U;
            bindings_.message_state = 110U;
        }
    }

    void case_one_hundred_two() {
        if (signed_bits(runtime_.transition_timer) >= 20) {
            runtime_.completion_gate = 1U;
            bindings_.message_state = 0U;
            runtime_.transition_timer = 0U;
            bindings_.target_ready_gate = 0U;
            runtime_.transition_stage = 0U;
        }
    }

    void case_one_hundred_three() {
        if (signed_bits(runtime_.transition_timer) >= 20) {
            runtime_.completion_gate = 1U;
            runtime_.transition_stage = 0U;
            bindings_.message_state = 0U;
            runtime_.transition_timer = 0U;
        }
    }

    void case_one_hundred_four() {
        runtime_.completion_gate = 1U;
        runtime_.transition_stage = 0U;
    }

    void case_one_hundred_ten() {
        eax_ = runtime_.transition_mode;
        runtime_.transition_stage = 0U;
        if (eax_ == 1U) {
            const i32 signed_index = static_cast<i32>(
                static_cast<compat::i8>(runtime_.transition_actor_index)
            );
            const u32 index = std::bit_cast<u32>(signed_index);
            if (!invoke_group_a(Call::query_group_b_completion, index + 8U)) {
                return;
            }
            if (eax_ == 0U) {
                if (!invoke_group_a(
                        Call::set_actor_mode, index + 8U, {8U, 1U}
                    )) {
                    return;
                }
            }
            eax_ = std::bit_cast<u32>(input_.sample_mix_level);
            sample(0x160U);
            runtime_.transition_timer = 0U;
            bindings_.message_state = 111U;
        } else {
            runtime_.transition_state = 0U;
            runtime_.transition_timer = 0U;
            runtime_.transition_actor_index = 0xFFU;
            bindings_.message_state = 101U;
        }
    }

    void case_one_hundred_eleven() {
        if (signed_bits(runtime_.transition_timer) > 20) {
            runtime_.transition_stage = 0U;
            runtime_.transition_state = 0U;
            runtime_.transition_mode = 0U;
            runtime_.transition_timer = 0U;
            runtime_.transition_actor_index = 0xFFU;
            bindings_.message_state = 101U;
        }
    }

    void case_one_hundred_twelve() {
        if (signed_bits(runtime_.transition_timer) > 20) {
            runtime_.transition_stage = 0U;
            runtime_.transition_mode = 0U;
            runtime_.transition_timer = 0U;
            runtime_.transition_actor_index = 0xFFU;
            bindings_.message_state = 112U;
        }
    }

    void case_one_hundred_thirteen() {
        if (signed_bits(runtime_.transition_timer) > 20) {
            runtime_.transition_stage = 0U;
            runtime_.transition_actor_index = 0xFFU;
            bindings_.message_state = 113U;
            runtime_.transition_mode = 0U;
            runtime_.transition_timer = 0U;
        }
    }

    void case_two_hundred() {
        eax_ = metrics_.group_b_count;
        u32 index = 0U;
        runtime_.selected_action_kind = 0U;
        input_.action_kind = 1U;
        final_actor_.queued_actor_code = 0U;
        bindings_.debug_hotkeys.committed_actor_code = 0U;
        bindings_.message_state = 0U;
        final_actor_.pre_frame_gate_b = 0U;
        runtime_.selection_input_gate = 0U;
        if (signed_bits(eax_) > 0) {
            do {
                ecx_ = kGroupBBaseToken + index * kGroupBStride;
                if (index >= kGroupBCount) {
                    typed_stop(Status::group_b_actor_typed_stop);
                    return;
                }
                invoke(Call::reset_actor_selection, ecx_, {0U});
                ++result_.group_b_calls;
                eax_ = metrics_.group_b_count;
                ++index;
            } while (signed_bits(index) < signed_bits(eax_));
        }

        eax_ = metrics_.group_a_count;
        index = 0U;
        if (signed_bits(eax_) > 0) {
            do {
                ecx_ = kGroupABaseToken + index * kGroupAStride;
                if (index >= kGroupACount) {
                    typed_stop(Status::group_a_actor_typed_stop);
                    return;
                }
                invoke(Call::reset_actor_selection, ecx_, {0U});
                ++result_.group_a_calls;
                eax_ = metrics_.group_a_count;
                ++index;
            } while (signed_bits(index) < signed_bits(eax_));
        }
        input_.selection_animation_frame_a = 0U;
        input_.selection_animation_frame_b = 0U;
        input_.selection_cache_gate_a = 0U;
        input_.selection_cache_gate_b = 0U;
        bindings_.target_ready_gate = 0U;
        if (!prime_input_records()) {
            return;
        }
        if (input_.selected_actor_cleanup_gate == 1U) {
            eax_ = final_actor_.published_actor_code;
            frame_.selection_actor_code = eax_;
        }
        ecx_ = 0U;
        bindings_.startup_reset.value_4ff0b0 = ecx_;
        bindings_.startup_reset.value_4ff0b4 = ecx_;
        bindings_.startup_reset.value_53bf22 = 0U;
        bindings_.startup_reset.value_4ff0b8 = ecx_;
    }

    LegacyBattleTargetSelectionRefreshBindings bindings_;
    LegacyBattleInputDispatchPort& port_;
    LegacyBattleFrameInputResolutionState& frame_;
    LegacyBattleFinalActorStepState& final_actor_;
    LegacyBattleActionDispatchState& action_;
    LegacyBattleActorMetricState& metrics_;
    LegacyBattleDebugHotkeyState& debug_;
    LegacyBattleInputDispatchState& input_;
    std::span<input_time_rng::LegacyInputRecord> records_;
    LegacyBattleTargetSelectionRuntimeState& runtime_;
    LegacyBattleTargetSelectionRefreshResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
    u32 local_output_{};
    u32 call_output_{};
    bool stopped_{};
};

}  // namespace

LegacyBattleActorActionThirtyOverrideResult
query_legacy_battle_actor_action_thirty_override(
    const LegacyBattleGroupAActionExecutionState* actor,
    const u32 entry_eax,
    const u32 entry_ecx,
    const u32 entry_edx
) noexcept {
    LegacyBattleActorActionThirtyOverrideResult result{
        .return_eax = entry_eax,
        .return_ecx = entry_ecx,
        .return_edx = entry_edx,
    };
    if (actor == nullptr) {
        result.status =
            LegacyBattleActorActionThirtyOverrideStatus::actor_state_typed_stop;
        return result;
    }
    result.return_eax =
        static_cast<u32>((actor->action_override_flags >> 13U) & 1U);
    return result;
}

LegacyBattleTargetSelectionRefreshResult refresh_legacy_battle_target_selection(
    const LegacyBattleTargetSelectionRefreshBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleTargetSelectionRefreshRequest& request
) {
    return TargetSelectionMachine(bindings, port, request).run();
}

}  // namespace openswd3::battle
