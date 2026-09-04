#include "openswd3/battle/legacy_battle_actor_base_release.hpp"
#include "test.hpp"

#include <array>
#include <stdexcept>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActorBaseReleaseCallReply;
using openswd3::battle::LegacyBattleActorBaseReleaseCallRequest;
using openswd3::battle::LegacyBattleActorBaseReleasePort;
using openswd3::battle::LegacyBattleActorBaseReleaseStatus;
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

class FakePort final : public LegacyBattleActorBaseReleasePort {
public:
    [[nodiscard]] LegacyBattleActorBaseReleaseCallReply
    release_actor_base_description(
        const LegacyBattleActorBaseReleaseCallRequest& value
    ) override {
        request = value;
        ++calls;
        if (throw_from_release) {
            throw std::runtime_error{"actor base description release failed"};
        }

        return reply;
    }

    LegacyBattleActorBaseReleaseCallReply reply{};
    LegacyBattleActorBaseReleaseCallRequest request{};
    u32 calls{};
    bool throw_from_release{};
};

void test_zero_token(openswd3::test::Context& context) {
    openswd3::battle::LegacyBattleActorBaseInitializationOwner owner;
    owner.resource_definition_description = {1U, 2U};
    FakePort port;
    const auto result = openswd3::battle::release_legacy_battle_actor_base(
        owner,
        port,
        {
            .object_token = 0x00521598U,
            .entry_eax = 0xAABBCCDDU,
            .entry_ecx = 0x00521598U,
            .entry_edx = 0x11223344U,
        }
    );

    context.expect_equal(
        result.status,
        LegacyBattleActorBaseReleaseStatus::completed,
        "zero description token completes"
    );
    context.expect_equal(result.object_reads, 1U, "zero token reads once");
    context.expect_equal(result.release_calls, 0U, "zero token skips release");
    context.expect_equal(result.object_writes, 0U, "zero token skips clear");
    context.expect_equal(result.return_eax, 0U, "zero token replaces EAX");
    context.expect_equal(
        result.return_ecx, 0x00521598U, "zero token preserves ECX"
    );
    context.expect_equal(
        result.return_edx, 0x11223344U, "zero token preserves EDX"
    );
    context.expect_equal(
        owner.resource_definition_description.size(),
        2U,
        "zero token preserves host description"
    );
}

void test_release_and_clear(openswd3::test::Context& context) {
    openswd3::battle::LegacyBattleActorBaseInitializationOwner owner;
    write_description_token(owner.resource_definition, 0x71002000U);
    owner.resource_definition_description = {3U, 4U, 5U};
    FakePort port;
    port.reply = {
        .eax = 0x10101010U,
        .ecx = 0x20202020U,
        .edx = 0x30303030U,
    };
    const auto result = openswd3::battle::release_legacy_battle_actor_base(
        owner,
        port,
        {
            .object_token = 0x00521598U,
            .entry_eax = 0xAAAAAAAAU,
            .entry_ecx = 0x00521598U,
            .entry_edx = 0xCCCCCCCCU,
        }
    );

    context.expect_equal(port.calls, 1U, "nonzero token releases once");
    context.expect_equal(
        port.request.callee_token,
        openswd3::battle::kLegacyBattleActorBaseReleaseCalleeToken,
        "release uses the physical CRT wrapper"
    );
    context.expect_equal(
        port.request.actor_token, 0x00521598U, "release preserves actor token"
    );
    context.expect_equal(
        port.request.description_token,
        0x71002000U,
        "release receives description token"
    );
    context.expect_equal(
        port.request.actor_offset,
        openswd3::battle::kLegacyBattleActorBaseDescriptionTokenOffset,
        "release records the actor field offset"
    );
    context.expect_equal(
        port.request.eax, 0x71002000U, "release EAX is description token"
    );
    context.expect_equal(
        port.request.ecx, 0x00521598U, "release ECX remains actor token"
    );
    context.expect_equal(
        port.request.edx, 0xCCCCCCCCU, "release preserves EDX"
    );
    context.expect_equal(
        read_description_token(owner.resource_definition),
        0U,
        "description token clears after release"
    );
    context.expect_true(
        owner.resource_definition_description.empty(),
        "host description clears after release"
    );
    context.expect_equal(result.object_reads, 1U, "release path reads once");
    context.expect_equal(result.release_calls, 1U, "release path calls once");
    context.expect_equal(result.object_writes, 1U, "release path writes once");
    context.expect_equal(
        result.return_eax, 0x10101010U, "release EAX residue returns"
    );
    context.expect_equal(
        result.return_ecx, 0x20202020U, "release ECX residue returns"
    );
    context.expect_equal(
        result.return_edx, 0x30303030U, "release EDX residue returns"
    );
}

void test_read_stops(openswd3::test::Context& context) {
    openswd3::battle::LegacyBattleActorBaseInitializationOwner owner;
    write_description_token(owner.resource_definition, 0x72003000U);
    owner.resource_definition_description = {6U, 7U};
    FakePort port;

    auto result = openswd3::battle::release_legacy_battle_actor_base(
        owner,
        port,
        {
            .object_token = 0U,
            .entry_eax = 0x11U,
            .entry_ecx = 0x22U,
            .entry_edx = 0x33U,
        }
    );
    context.expect_equal(
        result.status,
        LegacyBattleActorBaseReleaseStatus::object_read_typed_stop,
        "null actor stops at the description read"
    );
    context.expect_equal(result.return_eax, 0x11U, "null stop preserves EAX");
    context.expect_equal(port.calls, 0U, "null actor does not release");

    result = openswd3::battle::release_legacy_battle_actor_base(
        owner,
        port,
        {
            .object_token = 0x00521598U,
            .readable_bytes = 0xB3U,
            .entry_eax = 0x44U,
            .entry_ecx = 0x55U,
            .entry_edx = 0x66U,
        }
    );
    context.expect_equal(
        result.status,
        LegacyBattleActorBaseReleaseStatus::object_read_typed_stop,
        "short actor stops at the description read"
    );
    context.expect_equal(
        result.stopped_actor_offset,
        openswd3::battle::kLegacyBattleActorBaseDescriptionTokenOffset,
        "read stop records actor +0xB0"
    );

    result = openswd3::battle::release_legacy_battle_actor_base(
        std::span<u8>{owner.resource_definition}.first(0xA3U),
        owner.resource_definition_description,
        port,
        {.object_token = 0x00521598U}
    );
    context.expect_equal(
        result.status,
        LegacyBattleActorBaseReleaseStatus::object_read_typed_stop,
        "short definition view stops at the same physical read"
    );
    context.expect_equal(
        read_description_token(owner.resource_definition),
        0x72003000U,
        "read stops preserve description token"
    );
}

void test_release_stops(openswd3::test::Context& context) {
    openswd3::battle::LegacyBattleActorBaseInitializationOwner owner;
    write_description_token(owner.resource_definition, 0x73004000U);
    owner.resource_definition_description = {8U, 9U};
    FakePort port;
    port.reply = {
        .eax = 0x44U,
        .ecx = 0x55U,
        .edx = 0x66U,
        .typed_stop = true,
    };
    auto result = openswd3::battle::release_legacy_battle_actor_base(
        owner, port, {.object_token = 0x00521598U, .entry_ecx = 0x00521598U}
    );
    context.expect_equal(
        result.status,
        LegacyBattleActorBaseReleaseStatus::release_call_typed_stop,
        "release trap stops before clear"
    );
    context.expect_equal(
        read_description_token(owner.resource_definition),
        0x73004000U,
        "release trap preserves token"
    );
    context.expect_equal(
        owner.resource_definition_description.size(),
        2U,
        "release trap preserves host description"
    );

    port.reply.typed_stop = false;
    result = openswd3::battle::release_legacy_battle_actor_base(
        owner,
        port,
        {
            .object_token = 0x00521598U,
            .writable_bytes = 0xB3U,
            .entry_ecx = 0x00521598U,
        }
    );
    context.expect_equal(
        result.status,
        LegacyBattleActorBaseReleaseStatus::object_write_typed_stop,
        "write trap follows successful release"
    );
    context.expect_equal(
        read_description_token(owner.resource_definition),
        0x73004000U,
        "write trap leaves the stale token"
    );
    context.expect_true(
        owner.resource_definition_description.empty(),
        "write trap preserves the completed release effect"
    );
}

void test_release_exception(openswd3::test::Context& context) {
    openswd3::battle::LegacyBattleActorBaseInitializationOwner owner;
    write_description_token(owner.resource_definition, 0x74005000U);
    owner.resource_definition_description = {10U, 11U};
    FakePort port;
    port.throw_from_release = true;
    bool caught = false;
    try {
        static_cast<void>(openswd3::battle::release_legacy_battle_actor_base(
            owner, port, {.object_token = 0x00521598U, .entry_ecx = 0x00521598U}
        ));
    } catch (const std::runtime_error&) {
        caught = true;
    }

    context.expect_true(caught, "release exception propagates");
    context.expect_equal(
        read_description_token(owner.resource_definition),
        0x74005000U,
        "release exception preserves token"
    );
    context.expect_equal(
        owner.resource_definition_description.size(),
        2U,
        "release exception preserves host description"
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
