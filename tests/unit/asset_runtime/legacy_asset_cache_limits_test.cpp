#include "test.hpp"

#include "openswd3/asset_runtime/legacy_asset_cache_limits.hpp"

#include <limits>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyAssetCacheLimitPorts;
using openswd3::asset_runtime::LegacyAssetCacheLimits;
using openswd3::asset_runtime::calculate_legacy_asset_cache_limits;
using openswd3::asset_runtime::configure_legacy_asset_cache_limits;
using openswd3::compat::u32;

enum class PortCall {
    physical_memory,
    tsw_limit,
    act_limit,
};

struct PortEvent {
    PortCall call{};
    u32 value{};

    bool operator==(const PortEvent&) const = default;
};

class RecordingPorts final : public LegacyAssetCacheLimitPorts {
public:
    u32 total_physical_memory_bytes_32() override {
        events.push_back({PortCall::physical_memory, physical_memory});
        return physical_memory;
    }

    void set_tsw_cache_limit(const u32 bytes) override {
        events.push_back({PortCall::tsw_limit, bytes});
    }

    void set_act_cache_limit(const u32 bytes) override {
        events.push_back({PortCall::act_limit, bytes});
    }

    u32 physical_memory{};
    std::vector<PortEvent> events;
};

void test_tsw_clamp_boundaries(openswd3::test::Context& test) {
    constexpr u32 mebibyte = 1024U * 1024U;

    test.expect_equal(
        calculate_legacy_asset_cache_limits(0U),
        LegacyAssetCacheLimits{4U * mebibyte, 512U * 1024U},
        "zero 32-bit memory sample clamps TSW to four MiB"
    );
    test.expect_equal(
        calculate_legacy_asset_cache_limits(24U * mebibyte - 1U),
        LegacyAssetCacheLimits{4U * mebibyte, 512U * 1024U},
        "value below the lower division boundary still clamps"
    );
    test.expect_equal(
        calculate_legacy_asset_cache_limits(24U * mebibyte),
        LegacyAssetCacheLimits{4U * mebibyte, 512U * 1024U},
        "twenty-four MiB produces the exact minimum"
    );
    test.expect_equal(
        calculate_legacy_asset_cache_limits(48U * mebibyte),
        LegacyAssetCacheLimits{8U * mebibyte, 512U * 1024U},
        "middle value uses unsigned floor division by six"
    );
    test.expect_equal(
        calculate_legacy_asset_cache_limits(96U * mebibyte),
        LegacyAssetCacheLimits{16U * mebibyte, 512U * 1024U},
        "ninety-six MiB produces the exact maximum"
    );
    test.expect_equal(
        calculate_legacy_asset_cache_limits(std::numeric_limits<u32>::max()),
        LegacyAssetCacheLimits{16U * mebibyte, 512U * 1024U},
        "largest legacy dwTotalPhys sample clamps to sixteen MiB"
    );
}

void test_configuration_order(openswd3::test::Context& test) {
    RecordingPorts ports;
    ports.physical_memory = 60U * 1024U * 1024U;

    test.expect_true(
        configure_legacy_asset_cache_limits(ports),
        "0x00424330 always returns one"
    );
    test.expect_equal(
        ports.events,
        std::vector<PortEvent>{
            {PortCall::physical_memory, ports.physical_memory},
            {PortCall::tsw_limit, 10U * 1024U * 1024U},
            {PortCall::act_limit, 512U * 1024U},
        },
        "memory query, TSW setter and ACT setter preserve assembly order"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_tsw_clamp_boundaries(test);
    test_configuration_order(test);
    return test.exit_code();
}
