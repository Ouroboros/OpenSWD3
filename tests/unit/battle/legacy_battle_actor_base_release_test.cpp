#include "openswd3/battle/legacy_battle_actor_base_release.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <stdexcept>

namespace {

using openswd3::battle::LegacyBattleActorBaseInitializationOwner;
using openswd3::battle::LegacyBattleActorBaseReleaseStatus;
using openswd3::battle::LegacyBattleMonText;
using openswd3::battle::release_legacy_battle_actor_base;
using openswd3::compat::u8;
using openswd3::compat::u32;

void write_description_token(std::span<u8> definition, const u32 token) {
    definition[0xA0U] = static_cast<u8>(token);
    definition[0xA1U] = static_cast<u8>(token >> 8U);
    definition[0xA2U] = static_cast<u8>(token >> 16U);
    definition[0xA3U] = static_cast<u8>(token >> 24U);
}

[[nodiscard]] u32 read_description_token(const std::span<const u8> definition) {
    return static_cast<u32>(definition[0xA0U]) |
        (static_cast<u32>(definition[0xA1U]) << 8U) |
        (static_cast<u32>(definition[0xA2U]) << 16U) |
        (static_cast<u32>(definition[0xA3U]) << 24U);
}

struct TextAllocation {
    void bind(LegacyBattleMonText& text) {
        text.bind(
            bytes, std::make_shared<const LegacyBattleMonText::Release>([this] {
                ++attempts;
                if (on_release) {
                    on_release();
                }

                if (throw_from_release) {
                    throw std::runtime_error{
                        "actor base description release failed"
                    };
                }

                if (reject_release || !live) {
                    return false;
                }

                LegacyBattleMonText::Storage{}.swap(*bytes);
                live = false;
                return true;
            })
        );
    }

    std::shared_ptr<LegacyBattleMonText::Storage> bytes{
        std::make_shared<LegacyBattleMonText::Storage>(
            LegacyBattleMonText::Storage{1U, 2U}
        )
    };
    std::function<void()> on_release;
    u32 attempts{};
    bool live{true};
    bool reject_release{};
    bool throw_from_release{};
};

void test_zero_token(openswd3::test::Context& context) {
    LegacyBattleActorBaseInitializationOwner owner;
    TextAllocation allocation;
    allocation.bind(owner.resource_definition_description);
    const auto result =
        release_legacy_battle_actor_base(owner, {.object_token = 0x00521598U});
    context.expect_true(
        result.status == LegacyBattleActorBaseReleaseStatus::completed &&
            allocation.attempts == 0U && allocation.live &&
            owner.resource_definition_description.size() == 2U,
        "zero description pointer preserves the allocation and skips release"
    );
}

void test_release_and_clear(openswd3::test::Context& context) {
    LegacyBattleActorBaseInitializationOwner owner;
    owner.resource_definition.fill(0xA5U);
    write_description_token(owner.resource_definition, 0x71002000U);
    TextAllocation allocation;
    allocation.bind(owner.resource_definition_description);
    auto alias = owner.resource_definition_description;
    bool pointer_live_at_release = false;
    allocation.on_release = [&] {
        pointer_live_at_release =
            read_description_token(owner.resource_definition) == 0x71002000U;
    };

    const auto result =
        release_legacy_battle_actor_base(owner, {.object_token = 0x00521598U});
    context.expect_true(
        result.status == LegacyBattleActorBaseReleaseStatus::completed &&
            result.prior_description_token == 0x71002000U &&
            allocation.attempts == 1U && !allocation.live &&
            allocation.bytes->capacity() == 0U && alias.empty() &&
            !owner.resource_definition_description.has_allocation() &&
            pointer_live_at_release &&
            read_description_token(owner.resource_definition) == 0U,
        "actual allocation and aliases are released before the actor pointer clears"
    );
    context.expect_true(
        std::ranges::all_of(
            std::span<const u8>{owner.resource_definition}.first(0xA0U),
            [](const auto value) { return value == 0xA5U; }
        ),
        "description release preserves every preceding definition byte"
    );
    const auto repeated =
        release_legacy_battle_actor_base(owner, {.object_token = 0x00521598U});
    context.expect_true(
        repeated.status == LegacyBattleActorBaseReleaseStatus::completed &&
            allocation.attempts == 1U,
        "cleared pointer prevents a repeated release"
    );
}

void test_read_stops(openswd3::test::Context& context) {
    LegacyBattleActorBaseInitializationOwner owner;
    write_description_token(owner.resource_definition, 0x72003000U);
    TextAllocation allocation;
    allocation.bind(owner.resource_definition_description);
    for (const auto request : std::array{
             openswd3::battle::LegacyBattleActorBaseReleaseRequest{
                 .object_token = 0U,
             },
             openswd3::battle::LegacyBattleActorBaseReleaseRequest{
                 .object_token = 0x00521598U,
                 .readable_bytes = 0xB3U,
             },
         }) {
        const auto result = release_legacy_battle_actor_base(owner, request);
        context.expect_true(
            result.status ==
                    LegacyBattleActorBaseReleaseStatus::
                        object_read_typed_stop &&
                result.stopped_actor_offset == 0xB0U &&
                allocation.attempts == 0U,
            "missing or short actor stops before description release"
        );
    }

    const auto short_view = release_legacy_battle_actor_base(
        std::span<u8>{owner.resource_definition}.first(0xA3U),
        owner.resource_definition_description,
        {.object_token = 0x00521598U}
    );
    context.expect_true(
        short_view.status ==
                LegacyBattleActorBaseReleaseStatus::object_read_typed_stop &&
            allocation.attempts == 0U && allocation.live &&
            owner.resource_definition_description.size() == 2U &&
            read_description_token(owner.resource_definition) == 0x72003000U,
        "short definition view preserves the allocation and actor pointer"
    );
}

void test_release_stops(openswd3::test::Context& context) {
    LegacyBattleActorBaseInitializationOwner owner;
    write_description_token(owner.resource_definition, 0x73004000U);
    TextAllocation allocation;
    allocation.bind(owner.resource_definition_description);
    auto alias = owner.resource_definition_description;
    allocation.reject_release = true;
    auto result =
        release_legacy_battle_actor_base(owner, {.object_token = 0x00521598U});
    context.expect_true(
        result.status ==
                LegacyBattleActorBaseReleaseStatus::release_call_typed_stop &&
            result.stopped_token == 0x73004000U && allocation.live &&
            owner.resource_definition_description.size() == 2U &&
            read_description_token(owner.resource_definition) == 0x73004000U,
        "failed allocation release preserves its bytes and pointer"
    );
    allocation.reject_release = false;
    result = release_legacy_battle_actor_base(
        owner, {.object_token = 0x00521598U, .writable_bytes = 0xB3U}
    );
    context.expect_true(
        result.status ==
                LegacyBattleActorBaseReleaseStatus::object_write_typed_stop &&
            !allocation.live && alias.empty() &&
            !owner.resource_definition_description.has_allocation() &&
            read_description_token(owner.resource_definition) == 0x73004000U,
        "failed pointer write retains the already completed allocation release"
    );
    const auto attempts = allocation.attempts;
    result =
        release_legacy_battle_actor_base(owner, {.object_token = 0x00521598U});
    context.expect_true(
        result.status ==
                LegacyBattleActorBaseReleaseStatus::release_call_typed_stop &&
            allocation.attempts == attempts &&
            read_description_token(owner.resource_definition) == 0x73004000U,
        "stale nonzero pointer cannot report a second successful release without an allocation"
    );
}

void test_release_exception(openswd3::test::Context& context) {
    LegacyBattleActorBaseInitializationOwner owner;
    write_description_token(owner.resource_definition, 0x74005000U);
    TextAllocation allocation;
    allocation.bind(owner.resource_definition_description);
    allocation.throw_from_release = true;
    bool caught = false;
    try {
        static_cast<void>(release_legacy_battle_actor_base(
            owner, {.object_token = 0x00521598U}
        ));
    } catch (const std::runtime_error&) {
        caught = true;
    }

    context.expect_true(
        caught && allocation.live && allocation.attempts == 1U &&
            owner.resource_definition_description.size() == 2U &&
            read_description_token(owner.resource_definition) == 0x74005000U,
        "release exception propagates without clearing the allocation or pointer"
    );
}

}  // namespace

int main() {
    openswd3::test::Context context;
    test_zero_token(context);
    test_release_and_clear(context);
    test_read_stops(context);
    test_release_stops(context);
    test_release_exception(context);
    return context.exit_code();
}
