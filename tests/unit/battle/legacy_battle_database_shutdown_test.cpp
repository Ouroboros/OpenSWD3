#include "openswd3/battle/legacy_battle_database_shutdown.hpp"
#include "test.hpp"

#include <vector>

namespace {

using openswd3::battle::LegacyBattleDatabaseHandleCloseReply;
using openswd3::battle::LegacyBattleDatabaseHandleCloseRequest;
using openswd3::battle::LegacyBattleDatabaseShutdownPort;
using openswd3::battle::LegacyBattleDatabaseShutdownRequest;
using openswd3::compat::u32;

class ShutdownPort final : public LegacyBattleDatabaseShutdownPort {
public:
    [[nodiscard]] LegacyBattleDatabaseHandleCloseReply
    close_legacy_battle_database_handle(
        const LegacyBattleDatabaseHandleCloseRequest& request
    ) override {
        requests.push_back(request);
        return replies[requests.size() - 1U];
    }

    std::vector<LegacyBattleDatabaseHandleCloseRequest> requests;
    std::vector<LegacyBattleDatabaseHandleCloseReply> replies;
};

void test_both_valid_handles(openswd3::test::Context& test) {
    ShutdownPort port;
    auto& mon = port.legacy_battle_mon_database_state();
    auto& level = port.legacy_battle_level_database_state();
    mon.open = false;
    mon.handle = 0x11112222U;
    level.open = true;
    level.handle = 0x33334444U;
    port.replies = {
        {.eax = 0U, .ecx = 0xAAAA0001U, .edx = 0xBBBB0002U},
        {.eax = 1U, .ecx = 0xCCCC0003U, .edx = 0xDDDD0004U},
    };

    const auto result = openswd3::battle::shutdown_legacy_battle_databases(
        port,
        {
            .entry_eax = 0xEEEE0005U,
            .entry_ecx = 0xFFFF0006U,
            .entry_edx = 0x12340007U,
        }
    );

    test.expect_true(
        port.requests.size() == 2U && port.requests[0].handle == 0x11112222U &&
            port.requests[0].eax == 0x11112222U &&
            port.requests[0].ecx == 0xFFFF0006U &&
            port.requests[0].edx == 0x12340007U &&
            port.requests[1].handle == 0x33334444U &&
            port.requests[1].eax == 0x33334444U &&
            port.requests[1].ecx == 0xAAAA0001U &&
            port.requests[1].edx == 0xBBBB0002U,
        "valid MON and LEVEL handles close in order and the second call inherits the first reply registers"
    );
    test.expect_true(
        result.initial_mon_handle == 0x11112222U &&
            result.initial_level_handle == 0x33334444U &&
            result.mon_handle_closed && result.level_handle_closed &&
            result.close_calls == 2U &&
            result.final_mon_handle == 0xFFFFFFFFU &&
            result.final_level_handle == 0xFFFFFFFFU && !mon.open &&
            !level.open && result.return_eax == 1U &&
            result.return_ecx == 0xCCCC0003U &&
            result.return_edx == 0xDDDD0004U,
        "both successful call boundaries publish invalid handles, clear both gates, and return the second reply"
    );
}

void test_invalid_handle_sentinels(openswd3::test::Context& test) {
    {
        ShutdownPort port;
        auto& mon = port.legacy_battle_mon_database_state();
        auto& level = port.legacy_battle_level_database_state();
        mon.open = true;
        mon.handle = 0xFFFFFFFFU;
        level.open = true;
        level.handle = 0U;

        const auto result = openswd3::battle::shutdown_legacy_battle_databases(
            port,
            {
                .entry_eax = 0x11111111U,
                .entry_ecx = 0x22222222U,
                .entry_edx = 0x33333333U,
            }
        );

        test.expect_true(
            port.requests.empty() && result.close_calls == 0U &&
                !result.mon_handle_closed && !result.level_handle_closed &&
                mon.handle == 0xFFFFFFFFU && level.handle == 0U && !mon.open &&
                !level.open && result.return_eax == 0U &&
                result.return_ecx == 0x22222222U &&
                result.return_edx == 0x33333333U,
            "minus-one and zero handles skip CloseHandle, remain unchanged, and still clear both session gates"
        );
    }

    {
        ShutdownPort port;
        auto& mon = port.legacy_battle_mon_database_state();
        auto& level = port.legacy_battle_level_database_state();
        mon.open = true;
        mon.handle = 0U;
        level.open = true;
        level.handle = 0xABCDEF01U;
        port.replies = {{.eax = 0U, .ecx = 0x44444444U, .edx = 0x55555555U}};

        const auto result = openswd3::battle::shutdown_legacy_battle_databases(
            port, {.entry_ecx = 0x66666666U, .entry_edx = 0x77777777U}
        );

        test.expect_true(
            port.requests.size() == 1U &&
                port.requests.front().handle == 0xABCDEF01U &&
                port.requests.front().ecx == 0x66666666U &&
                port.requests.front().edx == 0x77777777U &&
                !result.mon_handle_closed && result.level_handle_closed &&
                mon.handle == 0U && level.handle == 0xFFFFFFFFU &&
                result.return_eax == 0U && result.return_ecx == 0x44444444U &&
                result.return_edx == 0x55555555U,
            "a valid LEVEL handle closes after a skipped zero MON handle, including a failed CloseHandle reply"
        );
    }
}

void test_first_only_valid(openswd3::test::Context& test) {
    ShutdownPort port;
    auto& mon = port.legacy_battle_mon_database_state();
    auto& level = port.legacy_battle_level_database_state();
    mon.open = true;
    mon.handle = 0x01020304U;
    level.open = true;
    level.handle = 0xFFFFFFFFU;
    port.replies = {{.eax = 1U, .ecx = 0x12121212U, .edx = 0x34343434U}};

    const auto result = openswd3::battle::shutdown_legacy_battle_databases(
        port, {.entry_ecx = 0x56565656U, .entry_edx = 0x78787878U}
    );

    test.expect_true(
        port.requests.size() == 1U && result.mon_handle_closed &&
            !result.level_handle_closed && mon.handle == 0xFFFFFFFFU &&
            level.handle == 0xFFFFFFFFU && result.return_eax == 0xFFFFFFFFU &&
            result.return_ecx == 0x12121212U &&
            result.return_edx == 0x34343434U,
        "the LEVEL handle load overwrites EAX after a valid MON close while preserving the close reply ECX and EDX"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_both_valid_handles(test);
    test_invalid_handle_sentinels(test);
    test_first_only_valid(test);
    return test.exit_code();
}
