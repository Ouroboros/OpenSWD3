#include "test.hpp"

#include "openswd3/world_map/legacy_world_map_session.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <vector>

namespace {

using openswd3::compat::u32;
using openswd3::resource_io::LegacyLmfIndexedObjectDirectory;
using openswd3::resource_io::LegacyLmfIndexedObjectDirectoryStatus;
using openswd3::resource_io::LegacyLmfMapHeader;
using openswd3::resource_io::LegacyLmfMapHeaderStatus;
using openswd3::resource_io::LegacyLmfMapLookupResult;
using openswd3::resource_io::LegacyLmfMapLookupStatus;
using openswd3::resource_io::LegacyLmfOffset14Directory;
using openswd3::resource_io::LegacyLmfOffset14DirectoryStatus;
using openswd3::resource_io::LegacyLmfOffset1cDirectory;
using openswd3::resource_io::LegacyLmfOffset1cDirectoryStatus;
using openswd3::resource_io::LegacyLmfPostSurfaceRecords;
using openswd3::resource_io::LegacyLmfPostSurfaceRecordsStatus;
using openswd3::resource_io::LegacyLmfReferencedRecordDirectory;
using openswd3::resource_io::LegacyLmfReferencedRecordDirectoryStatus;
using openswd3::resource_io::LegacyLmfSurfaceGrid;
using openswd3::resource_io::LegacyLmfSurfaceGridStatus;
using openswd3::world_map::LegacyWorldMapLoadStatus;
using openswd3::world_map::LegacyWorldMapSource;
using openswd3::world_map::load_legacy_world_map;

enum class Call {
    lookup,
    header,
    surface,
    post_surface,
    referenced,
    offset14,
    indexed,
    offset1c,
};

class FakeSource final : public LegacyWorldMapSource {
public:
    LegacyLmfMapLookupResult lookup_map(const u32 map_id) override {
        calls.push_back(Call::lookup);
        seen_map_id = map_id;
        return lookup;
    }

    LegacyLmfMapHeader read_map_header(const u32 map_offset) override {
        calls.push_back(Call::header);
        offsets.push_back(map_offset);
        return header;
    }

    LegacyLmfSurfaceGrid read_surface_grid(
        const u32 map_offset,
        const LegacyLmfMapHeader&
    ) override {
        calls.push_back(Call::surface);
        offsets.push_back(map_offset);
        return surface;
    }

    LegacyLmfPostSurfaceRecords read_post_surface_records(
        const u32 map_offset,
        const LegacyLmfSurfaceGrid&
    ) override {
        calls.push_back(Call::post_surface);
        offsets.push_back(map_offset);
        return post_surface;
    }

    LegacyLmfReferencedRecordDirectory read_referenced_record_directory(
        const u32 map_offset,
        const LegacyLmfPostSurfaceRecords&
    ) override {
        calls.push_back(Call::referenced);
        offsets.push_back(map_offset);
        return referenced;
    }

    LegacyLmfOffset14Directory read_offset14_directory(
        const u32 map_offset,
        const LegacyLmfMapHeader&
    ) override {
        calls.push_back(Call::offset14);
        offsets.push_back(map_offset);
        return offset14;
    }

    LegacyLmfIndexedObjectDirectory read_indexed_object_directory(
        const u32 map_offset,
        const LegacyLmfMapHeader&
    ) override {
        calls.push_back(Call::indexed);
        offsets.push_back(map_offset);
        return indexed;
    }

    LegacyLmfOffset1cDirectory read_offset1c_directory(
        const u32 map_offset,
        const LegacyLmfMapHeader&
    ) override {
        calls.push_back(Call::offset1c);
        offsets.push_back(map_offset);
        return offset1c;
    }

    void make_ready() {
        lookup.status = LegacyLmfMapLookupStatus::ready;
        lookup.map_offset = 0x12345678U;
        header.status = LegacyLmfMapHeaderStatus::ready;
        surface.status = LegacyLmfSurfaceGridStatus::ready;
        post_surface.status = LegacyLmfPostSurfaceRecordsStatus::ready;
        referenced.status =
            LegacyLmfReferencedRecordDirectoryStatus::ready;
        offset14.status = LegacyLmfOffset14DirectoryStatus::ready;
        indexed.status = LegacyLmfIndexedObjectDirectoryStatus::ready;
        offset1c.status = LegacyLmfOffset1cDirectoryStatus::ready;
    }

    LegacyLmfMapLookupResult lookup;
    LegacyLmfMapHeader header;
    LegacyLmfSurfaceGrid surface;
    LegacyLmfPostSurfaceRecords post_surface;
    LegacyLmfReferencedRecordDirectory referenced;
    LegacyLmfOffset14Directory offset14;
    LegacyLmfIndexedObjectDirectory indexed;
    LegacyLmfOffset1cDirectory offset1c;
    std::vector<Call> calls;
    std::vector<u32> offsets;
    u32 seen_map_id{};
};

void test_ready_sequence(openswd3::test::Context& test) {
    FakeSource source;
    source.make_ready();
    const auto result = load_legacy_world_map(24U, source);

    test.expect_equal(result.status, LegacyWorldMapLoadStatus::ready,
                      "all physical stages produce a ready session");
    test.expect_equal(result.session.map_id, u32{24U},
                      "session retains the requested map id");
    test.expect_equal(source.seen_map_id, u32{24U},
                      "lookup receives the full map id");
    test.expect_equal(
        source.calls,
        std::vector<Call>{
            Call::lookup,
            Call::header,
            Call::surface,
            Call::post_surface,
            Call::referenced,
            Call::offset14,
            Call::indexed,
            Call::offset1c,
        },
        "LMF stages retain the 0x00425BE0 physical order"
    );
    test.expect_true(
        source.offsets.size() == 7U &&
            std::ranges::all_of(source.offsets, [](const u32 value) {
                return value == 0x12345678U;
            }),
        "every physical reader uses the lookup offset"
    );
}

void test_first_failure_stops(openswd3::test::Context& test) {
    struct FailureCase {
        Call call;
        LegacyWorldMapLoadStatus status;
    };
    constexpr std::array cases{
        FailureCase{Call::lookup, LegacyWorldMapLoadStatus::map_lookup_failed},
        FailureCase{Call::header, LegacyWorldMapLoadStatus::map_header_failed},
        FailureCase{Call::surface, LegacyWorldMapLoadStatus::surface_grid_failed},
        FailureCase{Call::post_surface,
                    LegacyWorldMapLoadStatus::post_surface_records_failed},
        FailureCase{
            Call::referenced,
            LegacyWorldMapLoadStatus::referenced_record_directory_failed,
        },
        FailureCase{Call::offset14,
                    LegacyWorldMapLoadStatus::offset14_directory_failed},
        FailureCase{
            Call::indexed,
            LegacyWorldMapLoadStatus::indexed_object_directory_failed,
        },
        FailureCase{Call::offset1c,
                    LegacyWorldMapLoadStatus::offset1c_directory_failed},
    };

    for (const auto& failure : cases) {
        FakeSource source;
        source.make_ready();
        switch (failure.call) {
        case Call::lookup:
            source.lookup.status = LegacyLmfMapLookupStatus::map_not_found;
            break;
        case Call::header:
            source.header.status =
                LegacyLmfMapHeaderStatus::unsupported_signature;
            break;
        case Call::surface:
            source.surface.status =
                LegacyLmfSurfaceGridStatus::decompression_failed;
            break;
        case Call::post_surface:
            source.post_surface.status =
                LegacyLmfPostSurfaceRecordsStatus::record_data_out_of_range;
            break;
        case Call::referenced:
            source.referenced.status =
                LegacyLmfReferencedRecordDirectoryStatus::referenced_record_read_failed;
            break;
        case Call::offset14:
            source.offset14.status =
                LegacyLmfOffset14DirectoryStatus::record_data_out_of_range;
            break;
        case Call::indexed:
            source.indexed.status =
                LegacyLmfIndexedObjectDirectoryStatus::decompression_failed;
            break;
        case Call::offset1c:
            source.offset1c.status =
                LegacyLmfOffset1cDirectoryStatus::record_data_out_of_range;
            break;
        }

        const auto result = load_legacy_world_map(7U, source);
        test.expect_equal(result.status, failure.status,
                          "failure is attributed to its physical stage");
        test.expect_equal(source.calls.back(), failure.call,
                          "no later stage runs after the first failure");
    }
}

void test_current_maps(
    openswd3::test::Context& test,
    const std::filesystem::path& archive_path
) {
    struct Expected {
        u32 map_id;
        u32 map_offset;
        u32 width;
        u32 height;
        u32 surface_size;
        u32 post_surface_count;
        u32 business_role_count;
    };
    constexpr std::array expected_maps{
        Expected{22U, 0x00000004U, 120U, 90U, 43'200U, 3U, 49U},
        Expected{24U, 0x026698A3U, 120U, 136U, 65'280U, 7U, 29U},
        Expected{500U, 0x1C16E962U, 40U, 457U, 73'120U, 0U, 1U},
    };

    for (const auto& expected : expected_maps) {
        const auto result = load_legacy_world_map(
            archive_path,
            expected.map_id
        );
        test.expect_equal(result.status, LegacyWorldMapLoadStatus::ready,
                          "fixed current map reaches a complete LMF session");
        test.expect_equal(result.session.lookup.map_offset, expected.map_offset,
                          "fixed map offset matches the inventory");
        test.expect_true(
            result.session.header.width == expected.width &&
                result.session.header.height == expected.height,
            "fixed map dimensions match the inventory"
        );
        test.expect_equal(
            result.session.surface_grid.actual_surface_grid_size,
            expected.surface_size,
            "fixed map surface output matches the inventory"
        );
        test.expect_equal(
            result.session.surface_grid.post_surface_record_count,
            expected.post_surface_count,
            "fixed map trailing record count matches the inventory"
        );
        test.expect_true(
            result.session.business.state.events.size() ==
                    expected.post_surface_count &&
                result.session.business.state.roles.size() ==
                    expected.business_role_count &&
                result.session.business.state.offset14_role_count + 1U ==
                    expected.business_role_count,
            "fixed map event and offset14 role conversion counts are stable"
        );
        test.expect_true(
            result.session.role_cell_binding.roles_bound + 1U ==
                    expected.business_role_count &&
                result.session.role_cell_binding.out_of_bounds_indices == 0U,
            "fixed map roles receive valid checked surface-grid indices"
        );
    }
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_ready_sequence(test);
    test_first_failure_stops(test);

    test.expect_true(argument_count == 1 || argument_count == 2,
                     "optional argument names the current huge.lmf");
    if (argument_count == 2 && arguments != nullptr && arguments[1] != nullptr) {
        test_current_maps(test, arguments[1]);
    }
    return test.exit_code();
}
