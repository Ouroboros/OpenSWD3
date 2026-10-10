#pragma once

#include <exception>
#include <functional>
#include <memory_resource>
#include <new>
#include <utility>
#include <vector>

namespace openswd3::battle {

class LegacyBattleExitCleanups {
public:
    explicit LegacyBattleExitCleanups(
        std::pmr::memory_resource* memory = std::pmr::get_default_resource()
    )
        : cleanups_(memory) {}

    LegacyBattleExitCleanups(const LegacyBattleExitCleanups&) = delete;
    LegacyBattleExitCleanups&
    operator=(const LegacyBattleExitCleanups&) = delete;

    ~LegacyBattleExitCleanups() {
        if (!release()) {
            std::terminate();
        }
    }

    template <typename Cleanup> [[nodiscard]] bool add(Cleanup&& cleanup) {
        try {
            cleanups_.emplace_back(std::forward<Cleanup>(cleanup));
            return true;
        } catch (const std::bad_alloc&) {
            return false;
        }
    }

    [[nodiscard]] bool empty() const noexcept {
        return cleanups_.empty();
    }

    [[nodiscard]] bool release() {
        std::pmr::vector<std::function<bool()>> pending{
            cleanups_.get_allocator()
        };
        pending.swap(cleanups_);
        while (!pending.empty()) {
            if (!pending.back()()) {
                return false;
            }

            pending.pop_back();
        }

        return true;
    }

private:
    std::pmr::vector<std::function<bool()>> cleanups_;
};

}
