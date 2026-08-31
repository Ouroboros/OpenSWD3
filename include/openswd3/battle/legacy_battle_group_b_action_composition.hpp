#pragma once

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/battle/legacy_battle_mon_profile.hpp"

#include <array>
#include <memory>

namespace openswd3::battle {

enum class LegacyBattleGroupBActionCompositionCall : compat::u8 {
    reserved_load_resource_definition,
    load_resource_definition = reserved_load_resource_definition,
    copy_action_text,
    reserved_load_action_profile,
};

struct LegacyBattleGroupBActionCompositionCallRequest {
    LegacyBattleGroupBActionCompositionCall call{
        LegacyBattleGroupBActionCompositionCall::
            reserved_load_resource_definition
    };
    std::array<compat::u32, 2> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleGroupBActionCompositionCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool typed_stop{};
    std::shared_ptr<const std::array<compat::u8, 0xA4>> resource_definition;
    std::shared_ptr<const std::array<std::byte, 0x28>> profile_buffer;
};

class LegacyBattleGroupBActionCompositionPort {
public:
    virtual ~LegacyBattleGroupBActionCompositionPort() = default;

    [[nodiscard]] virtual LegacyBattleGroupBActionCompositionCallReply
    invoke(const LegacyBattleGroupBActionCompositionCallRequest& request) = 0;
};

struct LegacyBattleGroupBActionCompositionRequest {
    compat::u32 definition_argument{};
    compat::u32 actor_token{};
    compat::u32 output_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupBActionCompositionStatus : compat::u8 {
    completed,
    resource_load_typed_stop,
    actor_state_typed_stop,
    output_typed_stop,
    text_copy_typed_stop,
    profile_load_typed_stop,
};

struct LegacyBattleGroupBActionCompositionResult {
    LegacyBattleGroupBActionCompositionStatus status{
        LegacyBattleGroupBActionCompositionStatus::completed
    };
    compat::u32 port_calls{};
    compat::u32 text_bytes_written{};
    compat::u32 mode_update_calls{};
    compat::u16 published_word{};
    compat::u16 profile_word{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_476160. The fixed mode-two callee path is expanded directly; the two
// data loaders and the imported string-copy register boundary stay narrow.
[[nodiscard]] LegacyBattleGroupBActionCompositionResult
compose_legacy_battle_group_b_action(
    LegacyBattleActorGroupBElementState* actor,
    compat::u32* output,
    LegacyBattleGroupBActionCompositionPort& port,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleGroupBActionCompositionRequest& request
);

}  // namespace openswd3::battle
