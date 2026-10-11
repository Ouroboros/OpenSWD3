#include "openswd3/battle/legacy_battle_deformation.hpp"

#include <new>

namespace openswd3::battle {

void LegacyBattleDeformationDeleter::operator()(
    asset_runtime::LegacyDeformationNode* const node
) const noexcept {
    std::destroy_at(node);
    memory->deallocate(
        node,
        sizeof(asset_runtime::LegacyDeformationNode),
        alignof(asset_runtime::LegacyDeformationNode)
    );
}

LegacyBattleDeformation
make_legacy_battle_deformation(std::pmr::memory_resource& memory) {
    using Node = asset_runtime::LegacyDeformationNode;
    void* storage{};
    try {
        storage = memory.allocate(sizeof(Node), alignof(Node));
    } catch (const std::bad_alloc&) {
        return {nullptr, {&memory}};
    }

    try {
        auto* node = std::construct_at(
            static_cast<Node*>(storage),
            asset_runtime::LegacyDeformationConfiguration{
                .framebuffer_width = 640U,
                .framebuffer_height = 480U,
                .origin_x = 0,
                .origin_y = 0,
                .field_width = 200U,
                .field_height = 200U,
            }

        );
        return {node, {&memory}};
    } catch (...) {
        memory.deallocate(storage, sizeof(Node), alignof(Node));
        throw;
    }
}

}
