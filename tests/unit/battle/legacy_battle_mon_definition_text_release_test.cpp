#include "openswd3/battle/legacy_battle_mon_definition_text_release.hpp"
#include "openswd3/battle/legacy_battle_mon_text_runtime.hpp"
#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleMonDatabasePort;
using openswd3::battle::LegacyBattleMonDefinitionBytes;
using openswd3::battle::LegacyBattleMonDefinitionTextReleaseStatus;
using openswd3::battle::LegacyBattleMonText;
using openswd3::battle::LegacyBattleMonTextRuntime;
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

class TextPort final : public LegacyBattleMonDatabasePort {
public:
    void release_mon_text(const u32 block_token) override {
        ++release_count;
        released_token = block_token;
        heap.free(block_token);
    }

    LegacyBattleMonTextRuntime heap;
    u32 release_count{};
    u32 released_token{};
};

void test_zero_token(openswd3::test::Context& context) {
    LegacyBattleMonDefinitionBytes definition{};
    LegacyBattleMonText text{1U, 2U};
    TextPort port;
    const auto result =
        openswd3::battle::release_legacy_battle_mon_definition_text(
            definition, text, port, 0x0053CF50U
        );

    context.expect_equal(
        result.status,
        LegacyBattleMonDefinitionTextReleaseStatus::completed,
        "zero text token completes"
    );
    context.expect_equal(port.release_count, 0U, "zero token skips release");
    context.expect_equal(text.size(), 2U, "zero token leaves host text owner");
}

void test_release_and_clear(openswd3::test::Context& context) {
    TextPort port;
    const auto allocation = port.heap.allocate(3U);
    LegacyBattleMonDefinitionBytes definition{};
    write_token(definition, allocation.block_token);
    LegacyBattleMonText text;
    text.bind(allocation.storage, allocation.release);
    text[0U] = 3U;
    auto alias = text;
    const auto result =
        openswd3::battle::release_legacy_battle_mon_definition_text(
            definition, text, port, 0x0053CF50U
        );

    context.expect_equal(
        result.status,
        LegacyBattleMonDefinitionTextReleaseStatus::completed,
        "owned text release completes"
    );
    context.expect_equal(port.release_count, 1U, "nonzero token releases once");
    context.expect_equal(
        port.released_token,
        allocation.block_token,
        "release receives the allocated text token"
    );
    context.expect_equal(read_token(definition), 0U, "token clears after free");
    context.expect_true(
        text.empty() && alias.empty() && !alias.release(),
        "actual free invalidates the owner and every borrowed text view"
    );
}

void test_failure_prefixes(openswd3::test::Context& context) {
    LegacyBattleMonDefinitionBytes definition{};
    write_token(definition, 0x72003000U);
    LegacyBattleMonText text{6U, 7U};
    TextPort port;
    auto result = openswd3::battle::release_legacy_battle_mon_definition_text(
        {}, text, port, 0x0053CF50U
    );
    context.expect_true(
        result.status ==
                LegacyBattleMonDefinitionTextReleaseStatus::
                    object_read_typed_stop &&
            port.release_count == 0U && text.size() == 2U,
        "short object stops at its token read before touching text"
    );

    result = openswd3::battle::release_legacy_battle_mon_definition_text(
        definition, text, port, 0x0053CF50U
    );
    context.expect_true(
        result.status ==
                LegacyBattleMonDefinitionTextReleaseStatus::
                    release_call_typed_stop &&
            read_token(definition) == 0x72003000U && text.size() == 2U,
        "unknown allocation stops before clearing the token or text view"
    );

    const auto allocation = port.heap.allocate(2U);
    write_token(definition, allocation.block_token);
    text.bind(allocation.storage, allocation.release);
    auto alias = text;
    result = openswd3::battle::release_legacy_battle_mon_definition_text(
        definition, text, port, 0x0053CF50U, 0xA0U
    );
    context.expect_true(
        result.status ==
                LegacyBattleMonDefinitionTextReleaseStatus::
                    object_write_typed_stop &&
            read_token(definition) == allocation.block_token && text.empty() &&
            alias.empty() && !alias.release(),
        "failed token clear retains the stale token after actual memory release"
    );
}

}  // namespace

int main() {
    openswd3::test::Context context;
    test_zero_token(context);
    test_release_and_clear(context);
    test_failure_prefixes(context);
    return context.exit_code();
}
