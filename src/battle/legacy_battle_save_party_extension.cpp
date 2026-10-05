#include "openswd3/battle/legacy_battle_save_party_extension.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::u32 read_word(
    const std::array<compat::u8, 0x180U>& bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

}  // namespace

void restore_legacy_save_party_extension_b(
    const resource_io::LegacySaveContainer& save,
    LegacyBattleStartupState& battle
) noexcept {
    static_assert(sizeof(LegacyBattleGroupAConfigurationSourceRecord) == 0x60U);
    static_assert(
        sizeof(LegacyBattleGroupAConfigurationSourceRecord) * 4U == 0x180U
    );
    for (std::size_t role = 0U;
         role < battle.group_a_configuration_sources.size();
         ++role) {
        auto& destination = battle.group_a_configuration_sources[role];
        const std::size_t base = role * 0x60U;
        for (std::size_t word = 0U; word < destination.dwords.size(); ++word) {
            destination.dwords[word] =
                read_word(save.extension_b, base + word * 4U);
        }
        for (std::size_t word = 0U; word < destination.saved_tail_dwords.size();
             ++word) {
            destination.saved_tail_dwords[word] = read_word(
                save.extension_b, base + (destination.dwords.size() + word) * 4U
            );
        }
    }
}

}  // namespace openswd3::battle
