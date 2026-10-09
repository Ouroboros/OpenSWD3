#include "test.hpp"

#include "openswd3/battle/legacy_battle_actor_startup_reset.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using namespace openswd3::battle;

class ResetPort final : public LegacyBattleActorStartupResetPort,
                        public LegacyBattleActorStartupResetHeapPort {
public:
    ResetPort() {
        actor.fill(0xA5U);
        store(0x2584U, 0U);
        store(0x2AA0U, 0U);
    }

    void store(const u32 offset, const u32 value) {
        for (u32 index = 0U; index < 4U; ++index) {
            actor[offset + index] = static_cast<u8>(value >> (index * 8U));
        }
    }

    [[nodiscard]] u32 load(const u32 offset, const u32 size = 4U) const {
        u32 value{};
        for (u32 index = 0U; index < size; ++index) {
            value |= static_cast<u32>(actor[offset + index]) << (index * 8U);
        }

        return value;
    }

    std::optional<u16> read_actor_word(const u32 offset) override {
        if (offset == fail_read_offset) {
            return std::nullopt;
        }

        return static_cast<u16>(load(offset, 2U));
    }

    std::optional<u32> read_actor_dword(const u32 offset) override {
        if (offset == fail_read_offset) {
            return std::nullopt;
        }

        return load(offset);
    }

    bool write_actor_bytes(
        const u32 offset, const std::span<const u8> bytes
    ) override {
        if (writes == fail_write_ordinal) {
            return false;
        }

        std::copy(bytes.begin(), bytes.end(), actor.begin() + offset);
        ++writes;
        return true;
    }

    std::optional<u32> read_linked_action_next(const u32 token) override {
        const auto found = nodes.find(token);
        if (found == nodes.end()) {
            return std::nullopt;
        }

        return found->second;
    }

    bool release_heap_block(const u32 token) override {
        released.push_back(token);
        release_modes.push_back(load(0x26D0U, 2U));
        release_targets.push_back(load(0x2A56U));
        release_heads.push_back(load(0x2584U));
        if (token == fail_release_token) {
            return false;
        }

        nodes.erase(token);
        return true;
    }

    std::array<u8, 0x2B28U> actor{};
    std::map<u32, u32> nodes;
    std::vector<u32> released;
    std::vector<u32> release_modes;
    std::vector<u32> release_targets;
    std::vector<u32> release_heads;
    u32 writes{};
    u32 fail_write_ordinal{std::numeric_limits<u32>::max()};
    u32 fail_read_offset{std::numeric_limits<u32>::max()};
    u32 fail_release_token{std::numeric_limits<u32>::max()};
};

void test_reset_ranges(openswd3::test::Context& test) {
    ResetPort port;
    const auto result = reset_legacy_battle_actor_for_startup(port, port);
    test.expect_equal(
        result.status,
        LegacyBattleActorStartupResetStatus::completed,
        "actor reset completes"
    );

    // Byte intervals independently derived from the REP/STOS instructions.
    constexpr std::array<std::array<u32, 2>, 8> zero_ranges{{
        {0x02A0U, 0x0AF0U},
        {0x0C20U, 0x0CB8U},
        {0x0D50U, 0x0D70U},
        {0x0D90U, 0x0DB8U},
        {0x295AU, 0x29A0U},
        {0x29C4U, 0x2A0AU},
        {0x0FCCU, 0x148CU},
        {0x283CU, 0x2954U},
    }};
    for (const auto& range : zero_ranges) {
        test.expect_true(
            std::all_of(
                port.actor.begin() + range[0],
                port.actor.begin() + range[1],
                [](const u8 value) { return value == 0U; }
            ),
            "original REP range is zero"
        );
    }

    for (const u32 offset :
         {0x0174U,
          0x0AF0U,
          0x0B88U,
          0x0CB8U,
          0x0D70U,
          0x148CU,
          0x2956U,
          0x29A0U,
          0x2A76U,
          0x2B10U,
          0x2B14U,
          0x2B18U,
          0x2B1CU,
          0x2B24U}) {
        test.expect_equal(port.actor[offset], 0xA5U, "unwritten byte survives");
    }

    test.expect_equal(port.load(0x000CU), 0xA5A5A5A5U, "unowned resource kept");
    test.expect_equal(port.load(0x2A56U), 0xFFFFFFFFU, "target sentinel");
    test.expect_equal(port.load(0x29A2U, 2U), 0xFFFFU, "action sentinel");
    test.expect_equal(port.load(0x2668U), 15U, "countdown default");
    test.expect_equal(port.load(0x266CU), 1U, "base default");
    test.expect_equal(port.load(0x2A68U, 2U), 2U, "word default two");
    test.expect_equal(port.load(0x2A6AU, 2U), 24U, "word default twenty-four");
    test.expect_equal(port.load(0x2B20U), 0U, "last write");
}

void test_resource_release(openswd3::test::Context& test) {
    for (const u32 runtime : {0U, 1U, 2U, 0xFFFFFFFFU}) {
        for (const u32 resource : {0U, 0xD000U}) {
            ResetPort port;
            port.store(0x2AA0U, runtime);
            port.store(0x000CU, resource);
            const auto result =
                reset_legacy_battle_actor_for_startup(port, port);
            const bool frees = runtime == 1U && resource != 0U;
            test.expect_equal(
                result.status,
                LegacyBattleActorStartupResetStatus::completed,
                "resource branch completes"
            );
            test.expect_equal(
                port.released,
                frees ? std::vector<u32>{resource} : std::vector<u32>{},
                "only exact-one ownership frees the resource"
            );
            test.expect_equal(
                port.load(0x000CU),
                frees ? 0U : resource,
                "resource cleared only after free"
            );
            test.expect_equal(
                port.load(0x2AA0U), 0U, "ownership suffix clears"
            );
            if (frees) {
                test.expect_equal(
                    port.release_targets.front(),
                    0xA5A5A5A5U,
                    "defaults follow free"
                );
            }
        }
    }

    ResetPort failed;
    failed.store(0x2AA0U, 1U);
    failed.store(0x000CU, 0xD000U);
    failed.fail_release_token = 0xD000U;
    const auto result = reset_legacy_battle_actor_for_startup(failed, failed);
    test.expect_equal(
        result.status,
        LegacyBattleActorStartupResetStatus::heap_release_typed_stop,
        "resource free failure stops"
    );
    test.expect_equal(
        result.stopped_instruction, 0x0047D436U, "resource free callsite"
    );
    test.expect_equal(
        failed.load(0x000CU), 0xD000U, "failed resource retained"
    );
    test.expect_equal(failed.load(0x2AA0U), 1U, "ownership suffix not run");
    test.expect_equal(
        failed.load(0x2A56U), 0xA5A5A5A5U, "target suffix not run"
    );
}

void test_linked_release(openswd3::test::Context& test) {
    for (const bool fail_second : {false, true}) {
        ResetPort port;
        port.store(0x2584U, 0x1000U);
        port.nodes = {{0x1000U, 0x2000U}, {0x2000U, 0U}};
        if (fail_second) {
            port.fail_release_token = 0x2000U;
        }

        const auto result = reset_legacy_battle_actor_for_startup(port, port);
        test.expect_equal(
            port.released,
            std::vector<u32>{0x1000U, 0x2000U},
            "next read before each node is freed"
        );
        test.expect_equal(
            port.release_heads,
            std::vector<u32>{0U, 0U},
            "chain detached before free"
        );
        test.expect_equal(
            port.release_modes,
            std::vector<u32>{0xA5A5U & 0xFEBDU, 0xA5A5U & 0xFEBDU},
            "word mask precedes free"
        );
        test.expect_equal(
            result.status == LegacyBattleActorStartupResetStatus::completed,
            !fail_second,
            "second free failure stops suffix"
        );
        if (fail_second) {
            test.expect_equal(
                result.stopped_instruction, 0x0047F0E1U, "linked free callsite"
            );
            test.expect_equal(
                port.load(0x2B20U),
                0xA5A5A5A5U,
                "final field preserved after failure"
            );
        }
    }

    ResetPort missing;
    missing.store(0x2584U, 0x1000U);
    const auto result = reset_legacy_battle_actor_for_startup(missing, missing);
    test.expect_equal(
        result.status,
        LegacyBattleActorStartupResetStatus::linked_action_read_typed_stop,
        "unknown node stops at next read"
    );
    test.expect_equal(missing.load(0x2584U), 0U, "detachment survives failure");
    test.expect_true(missing.released.empty(), "invalid node not freed");
}

void test_read_and_release_boundaries(openswd3::test::Context& test) {
    constexpr std::array<std::array<u32, 2>, 4> reads{{
        {0x2584U, 0x0047F0BFU},
        {0x26D0U, 0x0047F0C5U},
        {0x2AA0U, 0x0047D422U},
        {0x000CU, 0x0047D42EU},
    }};
    for (const auto& read : reads) {
        ResetPort port;
        port.store(0x2AA0U, 1U);
        port.store(0x000CU, 0xD000U);
        port.fail_read_offset = read[0];
        const auto result = reset_legacy_battle_actor_for_startup(port, port);
        test.expect_equal(
            result.status,
            LegacyBattleActorStartupResetStatus::actor_read_typed_stop,
            "actor read failure stops at the original access"
        );
        test.expect_equal(
            result.stopped_instruction, read[1], "failed read instruction"
        );
        test.expect_equal(
            port.load(0x02A0U), 0U, "REP prefix survives read failure"
        );
        test.expect_equal(
            port.load(0x2AA0U), 1U, "ownership suffix follows successful reads"
        );
        test.expect_equal(
            port.load(0x000CU),
            0xD000U,
            "resource pointer survives read failure"
        );
        test.expect_true(
            port.released.empty(), "failed read does not release resource"
        );
    }

    ResetPort ignored_resource;
    ignored_resource.store(0x2AA0U, 2U);
    ignored_resource.fail_read_offset = 0x000CU;
    test.expect_true(
        reset_legacy_battle_actor_for_startup(
            ignored_resource, ignored_resource
        )
                .status == LegacyBattleActorStartupResetStatus::completed,
        "non-one ownership does not read resource pointer"
    );

    ResetPort failed_clear;
    failed_clear.store(0x2AA0U, 1U);
    failed_clear.store(0x000CU, 0xD000U);
    // 998 REP/STOS writes, then the three constant-one cleanup writes.
    failed_clear.fail_write_ordinal = 1001U;
    const auto clear_result =
        reset_legacy_battle_actor_for_startup(failed_clear, failed_clear);
    test.expect_equal(
        clear_result.stopped_instruction,
        0x0047D43EU,
        "resource pointer store can stop after free"
    );
    test.expect_equal(
        failed_clear.released,
        std::vector<u32>{0xD000U},
        "successful free is not rolled back"
    );
    test.expect_equal(
        failed_clear.load(0x000CU),
        0xD000U,
        "failed clear preserves pointer bytes"
    );
    test.expect_equal(
        failed_clear.load(0x2AA0U), 1U, "failed clear blocks ownership suffix"
    );

    ResetPort missing_second;
    missing_second.store(0x2584U, 0x1000U);
    missing_second.nodes = {{0x1000U, 0x2000U}};
    const auto missing_result =
        reset_legacy_battle_actor_for_startup(missing_second, missing_second);
    test.expect_equal(
        missing_result.stopped_offset_or_token,
        0x2000U,
        "later missing node stops at its own address"
    );
    test.expect_equal(
        missing_second.released,
        std::vector<u32>{0x1000U},
        "earlier node remains released"
    );
    test.expect_equal(
        missing_second.load(0x2584U),
        0U,
        "detached head is not restored after later failure"
    );
}

void test_write_stops(openswd3::test::Context& test) {
    ResetPort baseline;
    const auto complete =
        reset_legacy_battle_actor_for_startup(baseline, baseline);
    test.expect_equal(
        complete.status,
        LegacyBattleActorStartupResetStatus::completed,
        "baseline writes complete"
    );
    for (u32 ordinal = 0U; ordinal < baseline.writes; ++ordinal) {
        ResetPort port;
        port.fail_write_ordinal = ordinal;
        const auto result = reset_legacy_battle_actor_for_startup(port, port);
        test.expect_equal(
            result.status,
            LegacyBattleActorStartupResetStatus::actor_write_typed_stop,
            "each failed store stops"
        );
        test.expect_equal(port.writes, ordinal, "exact written prefix");
        test.expect_equal(
            port.load(0x2B20U),
            0xA5A5A5A5U,
            "no final write after any failed store"
        );
    }
}

}  // namespace

void test_battle_actor_startup_reset(openswd3::test::Context& test) {
    test_reset_ranges(test);
    test_resource_release(test);
    test_linked_release(test);
    test_read_and_release_boundaries(test);
    test_write_stops(test);
}
