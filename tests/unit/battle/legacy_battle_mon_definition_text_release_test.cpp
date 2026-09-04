#include "openswd3/battle/legacy_battle_mon_definition_text_release.hpp"
#include "test.hpp"

#include <array>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleMonDatabasePort;
using openswd3::battle::LegacyBattleMonDefinitionBytes;
using openswd3::battle::LegacyBattleMonDefinitionTextReleaseCallReply;
using openswd3::battle::LegacyBattleMonDefinitionTextReleaseCallRequest;
using openswd3::battle::LegacyBattleMonDefinitionTextReleaseStatus;
using openswd3::compat::u8;
using openswd3::compat::u32;

void write_token(LegacyBattleMonDefinitionBytes& definition, const u32 token) {
    definition[0xA0U] = static_cast<u8>(token);
    definition[0xA1U] = static_cast<u8>(token >> 8U);
    definition[0xA2U] = static_cast<u8>(token >> 16U);
    definition[0xA3U] = static_cast<u8>(token >> 24U);
}

u32 read_token(const LegacyBattleMonDefinitionBytes& definition) {
    return static_cast<u32>(definition[0xA0U]) |
        (static_cast<u32>(definition[0xA1U]) << 8U) |
        (static_cast<u32>(definition[0xA2U]) << 16U) |
        (static_cast<u32>(definition[0xA3U]) << 24U);
}

class FakePort final : public LegacyBattleMonDatabasePort {
public:
    LegacyBattleMonDefinitionTextReleaseCallReply reply{};
    LegacyBattleMonDefinitionTextReleaseCallRequest request{};
    u32 calls{};

    [[nodiscard]] LegacyBattleMonDefinitionTextReleaseCallReply
    release_legacy_battle_mon_definition_text(
        const LegacyBattleMonDefinitionTextReleaseCallRequest& value
    ) override {
        request = value;
        ++calls;
        return reply;
    }
};

void test_zero_token(openswd3::test::Context& context) {
    LegacyBattleMonDefinitionBytes definition{};
    std::vector<u8> text{1U, 2U};
    FakePort port;
    const auto result =
        openswd3::battle::release_legacy_battle_mon_definition_text(
            definition,
            text,
            port,
            {
                .object_token = 0x0053CF50U,
                .entry_eax = 0xAABBCCDDU,
                .entry_ecx = 0x11223344U,
                .entry_edx = 0x55667788U,
            }
        );

    context.expect_equal(
        result.status,
        LegacyBattleMonDefinitionTextReleaseStatus::completed,
        "zero text token completes"
    );
    context.expect_equal(result.return_eax, 0U, "zero token replaces EAX");
    context.expect_equal(
        result.return_ecx, 0x11223344U, "zero token preserves ECX"
    );
    context.expect_equal(
        result.return_edx, 0x55667788U, "zero token preserves EDX"
    );
    context.expect_equal(port.calls, 0U, "zero token skips release");
    context.expect_equal(text.size(), 2U, "zero token leaves host text owner");
}

void test_release_and_clear(openswd3::test::Context& context) {
    LegacyBattleMonDefinitionBytes definition{};
    write_token(definition, 0x71002000U);
    std::vector<u8> text{3U, 4U, 5U};
    FakePort port;
    port.reply = {
        .eax = 0x10101010U,
        .ecx = 0x20202020U,
        .edx = 0x30303030U,
        .typed_stop = false,
    };
    const auto result =
        openswd3::battle::release_legacy_battle_mon_definition_text(
            definition,
            text,
            port,
            {
                .object_token = 0x0053CF50U,
                .entry_eax = 0xAAAAAAAAU,
                .entry_ecx = 0xBBBBBBBBU,
                .entry_edx = 0xCCCCCCCCU,
            }
        );

    context.expect_equal(port.calls, 1U, "nonzero token releases once");
    context.expect_equal(
        port.request.block_token, 0x71002000U, "release receives text token"
    );
    context.expect_equal(
        port.request.eax, 0x71002000U, "release call EAX is text token"
    );
    context.expect_equal(
        port.request.ecx, 0xBBBBBBBBU, "release call preserves ECX"
    );
    context.expect_equal(
        port.request.edx, 0xCCCCCCCCU, "release call preserves EDX"
    );
    context.expect_equal(
        read_token(definition), 0U, "token clears after release"
    );
    context.expect_true(text.empty(), "host text owner clears after release");
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

void test_typed_stops(openswd3::test::Context& context) {
    LegacyBattleMonDefinitionBytes definition{};
    write_token(definition, 0x72003000U);
    std::vector<u8> text{6U, 7U};
    FakePort port;

    auto result = openswd3::battle::release_legacy_battle_mon_definition_text(
        {},
        text,
        port,
        {
            .object_token = 0x0053CF50U,
            .entry_eax = 0x11U,
            .entry_ecx = 0x22U,
            .entry_edx = 0x33U,
        }
    );
    context.expect_equal(
        result.status,
        LegacyBattleMonDefinitionTextReleaseStatus::object_read_typed_stop,
        "short object stops at the token read"
    );
    context.expect_equal(result.return_eax, 0x11U, "read stop preserves EAX");

    port.reply = {
        .eax = 0x44U,
        .ecx = 0x55U,
        .edx = 0x66U,
        .typed_stop = true,
    };
    result = openswd3::battle::release_legacy_battle_mon_definition_text(
        definition, text, port, {.object_token = 0x0053CF50U}
    );
    context.expect_equal(
        result.status,
        LegacyBattleMonDefinitionTextReleaseStatus::release_call_typed_stop,
        "release trap stops before clear"
    );
    context.expect_equal(
        read_token(definition), 0x72003000U, "release trap preserves token"
    );
    context.expect_equal(text.size(), 2U, "release trap preserves host text");

    port.reply.typed_stop = false;
    result = openswd3::battle::release_legacy_battle_mon_definition_text(
        definition,
        text,
        port,
        {
            .object_token = 0x0053CF50U,
            .writable_bytes = 0xA0U,
        }
    );
    context.expect_equal(
        result.status,
        LegacyBattleMonDefinitionTextReleaseStatus::object_write_typed_stop,
        "write trap follows successful release"
    );
    context.expect_equal(
        read_token(definition), 0x72003000U, "write trap leaves stale token"
    );
    context.expect_true(
        text.empty(), "write trap retains completed free effect"
    );
}

}  // namespace

int main() {
    openswd3::test::Context context;
    test_zero_token(context);
    test_release_and_clear(context);
    test_typed_stops(context);
    return context.exit_code();
}
