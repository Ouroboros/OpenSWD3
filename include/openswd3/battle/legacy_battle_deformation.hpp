#pragma once

#include "openswd3/asset_runtime/legacy_frame_deformation.hpp"

#include <memory>
#include <memory_resource>

namespace openswd3::battle {

struct LegacyBattleDeformationDeleter {
    std::pmr::memory_resource* memory{std::pmr::get_default_resource()};

    void operator()(asset_runtime::LegacyDeformationNode* node) const noexcept;
};

using LegacyBattleDeformation = std::unique_ptr<
    asset_runtime::LegacyDeformationNode,
    LegacyBattleDeformationDeleter>;

[[nodiscard]] LegacyBattleDeformation
make_legacy_battle_deformation(std::pmr::memory_resource& memory);

}
