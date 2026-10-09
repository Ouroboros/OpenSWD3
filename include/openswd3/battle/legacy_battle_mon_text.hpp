#pragma once

#include "openswd3/compat/types.hpp"

#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

namespace openswd3::battle {

// A typed view of a MON text allocation. Copies share the same bytes, as do
// copies of the legacy text pointer. The heap port retains the allocation
// independently until the original release call, including on a typed stop.
class LegacyBattleMonText {
public:
    using Storage = std::vector<compat::u8>;
    using Release = std::function<bool()>;

    LegacyBattleMonText() = default;

    LegacyBattleMonText(std::initializer_list<compat::u8> bytes)
        : storage_{std::make_shared<Storage>(bytes)} {}

    void bind(
        std::shared_ptr<Storage> storage,
        std::shared_ptr<const Release> release = {}
    ) noexcept {
        storage_ = std::move(storage);
        release_ = std::move(release);
    }

    // Drop a view without inventing a legacy free (e.g. a parser stop).
    void clear() noexcept {
        storage_.reset();
        release_ = {};
    }

    // Called only at an original free. An absent callback denotes standalone
    // host storage, used by existing isolated copies and synthetic states.
    [[nodiscard]] bool release() {
        if (release_) {
            if (!(*release_)()) {
                return false;
            }
        } else if (storage_) {
            Storage{}.swap(*storage_);
        }

        clear();
        return true;
    }

    void assign(std::size_t size, compat::u8 value) {
        bind(std::make_shared<Storage>(size, value));
    }

    [[nodiscard]] bool has_allocation() const noexcept {
        return storage_ != nullptr || release_ != nullptr;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size() == 0U;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return storage_ ? storage_->size() : 0U;
    }

    [[nodiscard]] compat::u8* data() noexcept {
        return storage_ ? storage_->data() : nullptr;
    }

    [[nodiscard]] const compat::u8* data() const noexcept {
        return storage_ ? storage_->data() : nullptr;
    }

    [[nodiscard]] compat::u8& operator[](std::size_t offset) noexcept {
        return (*storage_)[offset];
    }

    [[nodiscard]] const compat::u8&
    operator[](std::size_t offset) const noexcept {
        return (*storage_)[offset];
    }

    [[nodiscard]] const compat::u8& back() const noexcept {
        return storage_->back();
    }

    [[nodiscard]] auto begin() const noexcept {
        return bytes().begin();
    }

    [[nodiscard]] auto end() const noexcept {
        return bytes().end();
    }

    [[nodiscard]] const Storage& bytes() const noexcept {
        // No allocation is a valid empty description, not a successful heap
        // lookup. Indexing an absent allocation remains a caller error.
        static const Storage empty;
        return storage_ ? *storage_ : empty;
    }

    friend bool operator==(
        const LegacyBattleMonText& left, const LegacyBattleMonText& right
    ) noexcept {
        return left.bytes() == right.bytes();
    }

    friend bool
    operator==(const LegacyBattleMonText& left, const Storage& right) noexcept {
        return left.bytes() == right;
    }

private:
    std::shared_ptr<Storage> storage_;
    std::shared_ptr<const Release> release_;
};

}  // namespace openswd3::battle
