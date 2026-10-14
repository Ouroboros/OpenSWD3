#pragma once

#include "openswd3/battle/legacy_battle_actor_list_query.hpp"

#include <functional>
#include <vector>

namespace openswd3::test {

template <class Base>
class LegacyBattleResourceSelectionDiagnosticFixture
    : public Base,
      public virtual battle::LegacyBattleActorResourceSelectionPort {
public:
    void report_missing_runtime_word(const compat::u16 resource_id) override {
        missing_resource_ids.push_back(resource_id);
        if (on_missing_resource) {
            on_missing_resource(resource_id);
        }
    }

    std::vector<compat::u16> missing_resource_ids;
    std::function<void(compat::u16)> on_missing_resource;
};

}
