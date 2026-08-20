#include "test.hpp"

#include "openswd3/world_map/legacy_world_map_session.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u32;
using openswd3::resource_io::LegacyLmfIndexedObjectDirectory;
using openswd3::resource_io::LegacyLmfReadObserver;
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
using openswd3::world_map::LegacyLmfWorldMapSource;
using openswd3::world_map::LegacyWorldMapLoadStatus;
using openswd3::world_map::LegacyWorldMapSource;
using openswd3::world_map::load_legacy_world_map;

enum class Call {
    lookup,
    header,
    pre_surface_stage,
    surface,
    post_surface,
    referenced,
    offset14,
    indexed,
    offset1c,
};

class FakeSource final : public LegacyWorldMapSource {
public:
    [[nodiscard]] bool
    set_read_observer(const LegacyLmfReadObserver* observer) noexcept override {
        observer_ = observer;
        return true;
    }

    LegacyLmfMapLookupResult lookup_map(const u32 map_id) override {
        calls.push_back(Call::lookup);
        seen_map_id = map_id;
        maintain_audio(2U);
        return lookup;
    }

    LegacyLmfMapHeader read_map_header(const u32 map_offset) override {
        calls.push_back(Call::header);
        offsets.push_back(map_offset);
        if (header.status == LegacyLmfMapHeaderStatus::ready &&
            observer_ != nullptr) {
            if (observer_->map_header_signature_ready) {
                observer_->map_header_signature_ready();
            }
            maintain_audio(1U);
        }
        return header;
    }

    LegacyLmfSurfaceGrid read_surface_grid(
        const u32 map_offset, const LegacyLmfMapHeader&
    ) override {
        calls.push_back(Call::surface);
        offsets.push_back(map_offset);
        if (surface.status == LegacyLmfSurfaceGridStatus::ready) {
            maintain_audio(4U);
        } else if (
            surface.status == LegacyLmfSurfaceGridStatus::decompression_failed
        ) {
            maintain_audio(3U);
        }
        return surface;
    }

    LegacyLmfPostSurfaceRecords read_post_surface_records(
        const u32 map_offset, const LegacyLmfSurfaceGrid&
    ) override {
        calls.push_back(Call::post_surface);
        offsets.push_back(map_offset);
        if (post_surface.status == LegacyLmfPostSurfaceRecordsStatus::ready) {
            maintain_audio(1U);
        }
        return post_surface;
    }

    LegacyLmfReferencedRecordDirectory read_referenced_record_directory(
        const u32 map_offset, const LegacyLmfPostSurfaceRecords&
    ) override {
        calls.push_back(Call::referenced);
        offsets.push_back(map_offset);
        if (referenced.status ==
            LegacyLmfReferencedRecordDirectoryStatus::ready) {
            maintain_audio(referenced.records.size());
        }
        return referenced;
    }

    LegacyLmfOffset14Directory read_offset14_directory(
        const u32 map_offset, const LegacyLmfMapHeader&
    ) override {
        calls.push_back(Call::offset14);
        offsets.push_back(map_offset);
        if (offset14.status == LegacyLmfOffset14DirectoryStatus::ready) {
            maintain_audio(1U);
        }
        return offset14;
    }

    LegacyLmfIndexedObjectDirectory read_indexed_object_directory(
        const u32 map_offset, const LegacyLmfMapHeader&
    ) override {
        calls.push_back(Call::indexed);
        offsets.push_back(map_offset);
        if (indexed.status == LegacyLmfIndexedObjectDirectoryStatus::ready) {
            maintain_audio(2U);
            for (std::size_t object = 0U; object < indexed.objects.size();
                 ++object) {
                maintain_audio(4U);
                const bool consumer_ready = observer_ == nullptr ||
                    !observer_->indexed_object_ready ||
                    observer_->indexed_object_ready(indexed, object);
                maintain_audio(1U);
                if (!consumer_ready) {
                    indexed.status = LegacyLmfIndexedObjectDirectoryStatus::
                        indexed_object_consumer_failed;
                    break;
                }
            }
        }
        return indexed;
    }

    LegacyLmfOffset1cDirectory read_offset1c_directory(
        const u32 map_offset, const LegacyLmfMapHeader&
    ) override {
        calls.push_back(Call::offset1c);
        offsets.push_back(map_offset);
        if (offset1c.status == LegacyLmfOffset1cDirectoryStatus::ready) {
            maintain_audio(2U);
        }
        return offset1c;
    }

    void make_ready() {
        lookup.status = LegacyLmfMapLookupStatus::ready;
        lookup.map_offset = 0x12345678U;
        header.status = LegacyLmfMapHeaderStatus::ready;
        header.width = 8U;
        header.height = 10U;
        surface.status = LegacyLmfSurfaceGridStatus::ready;
        post_surface.status = LegacyLmfPostSurfaceRecordsStatus::ready;
        referenced.status = LegacyLmfReferencedRecordDirectoryStatus::ready;
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
    const LegacyLmfReadObserver* observer_{};

private:
    void maintain_audio(const std::size_t count) {
        if (observer_ == nullptr || !observer_->maintain_audio) {
            return;
        }
        for (std::size_t index = 0U; index < count; ++index) {
            observer_->maintain_audio();
        }
    }
};

void test_ready_sequence(openswd3::test::Context& test) {
    FakeSource source;
    source.make_ready();
    const auto result = load_legacy_world_map(24U, source);

    test.expect_equal(
        result.status,
        LegacyWorldMapLoadStatus::ready,
        "all physical stages produce a ready session"
    );
    test.expect_equal(
        result.session.map_id, u32{24U}, "session retains the requested map id"
    );
    test.expect_equal(
        source.seen_map_id, u32{24U}, "lookup receives the full map id"
    );
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
            std::ranges::all_of(
                source.offsets,
                [](const u32 value) { return value == 0x12345678U; }
            ),
        "every physical reader uses the lookup offset"
    );
}

void test_progress_sequence_and_business_milestones(
    openswd3::test::Context& test
) {
    FakeSource source;
    source.make_ready();
    source.post_surface.records.push_back({
        .relative_offset = 0U,
        .field_00 = 7U,
        .field_02 = 8U,
        .field_06 = 9U,
        .field_0a = 10U,
        .name_bytes_with_terminator = {'e', 0U},
    });
    source.offset14.records.push_back({
        .relative_offset = 0U,
        .field_00 = 1U,
        .field_02 = 0,
        .field_04 = 0U,
        .field_06 = 0U,
        .field_08 = 0,
        .field_0a = 0U,
        .name_bytes_with_terminator = {0U},
    });
    source.offset1c.records.push_back({
        .relative_offset = 0U,
        .field_00 = 2U,
        .field_02 = 3U,
        .field_04 = 0U,
        .field_06 = 0U,
        .packed_field_08 = 1U,
        .name_bytes_with_terminator = {0U},
    });

    struct Milestone {
        i32 progress{};
        std::size_t event_count{};
        std::size_t role_count{};
        u32 offset14_role_count{};
        u32 offset1c_role_count{};

        [[nodiscard]] bool operator==(const Milestone&) const = default;
    };
    std::vector<Milestone> milestones;
    const auto result = load_legacy_world_map(
        24U, source, {}, {}, [&](const i32 progress, const auto& session) {
            milestones.push_back({
                progress,
                session.business.state.events.size(),
                session.business.state.roles.size(),
                session.business.state.offset14_role_count,
                session.business.state.offset1c_role_count,
            });
        }
    );

    test.expect_equal(
        result.status,
        LegacyWorldMapLoadStatus::ready,
        "staged progress load reaches a ready session"
    );
    test.expect_equal(
        milestones,
        std::vector<Milestone>{
            {15, 0U, 0U, 0U, 0U},
            {60, 0U, 1U, 0U, 0U},
            {65, 1U, 1U, 0U, 0U},
            {70, 1U, 1U, 0U, 0U},
            {75, 1U, 2U, 1U, 0U},
            {80, 1U, 2U, 1U, 0U},
            {85, 1U, 3U, 1U, 1U},
        },
        "sub_425BE0 progress points observe the exact staged business state"
    );
}

void test_progress_and_audio_maintenance_sequence(
    openswd3::test::Context& test
) {
    FakeSource source;
    source.make_ready();
    source.referenced.records.push_back({1U, 2U});
    source.indexed.objects.emplace_back();

    constexpr i32 kAudioMaintenance = -1;
    std::vector<i32> sequence;
    const auto result = load_legacy_world_map(
        24U,
        source,
        {},
        {},
        [&](const i32 progress, const auto&) { sequence.push_back(progress); },
        {},
        [&]() { sequence.push_back(kAudioMaintenance); }
    );

    test.expect_equal(
        result.status,
        LegacyWorldMapLoadStatus::ready,
        "observed map load reaches the final maintenance point"
    );
    test.expect_equal(
        sequence,
        std::vector<i32>{
            -1, -1, -1, -1, -1, 15, -1, -1, 60, -1, -1, -1,
            -1, -1, -1, 65, -1, -1, 70, -1, -1, 75, -1, -1,
            -1, -1, -1, -1, -1, -1, 80, -1, -1, 85, -1,
        },
        "28 AIL_serve calls interleave exactly with seven progress stages"
    );
    test.expect_true(
        source.observer_ == nullptr,
        "the synchronous LMF observer is cleared before returning"
    );
}

void test_indexed_object_consumer_failure_stops_per_item(
    openswd3::test::Context& test
) {
    FakeSource source;
    source.make_ready();
    source.indexed.objects.resize(2U);

    std::vector<i32> progress;
    std::size_t audio_maintenance_count{};
    std::size_t consumer_count{};
    const auto result = load_legacy_world_map(
        24U,
        source,
        {},
        {},
        [&](const i32 value, const auto&) { progress.push_back(value); },
        [&](auto&, const std::size_t) {
            ++consumer_count;
            return false;
        },
        [&]() { ++audio_maintenance_count; }
    );

    test.expect_true(
        result.status ==
                LegacyWorldMapLoadStatus::indexed_object_stage_failed &&
            source.calls.back() == Call::indexed && consumer_count == 1U,
        "first indexed-object consumer failure stops before the second object"
    );
    test.expect_equal(
        progress,
        std::vector<i32>{15, 60, 65, 70, 75},
        "indexed-object consumer failure stops before progress 80"
    );
    test.expect_equal(
        audio_maintenance_count,
        std::size_t{24U},
        "failed first object still reaches its fifth maintenance point"
    );
}

void test_pre_surface_stage_slot(openswd3::test::Context& test) {
    FakeSource source;
    source.make_ready();
    const auto result =
        load_legacy_world_map(24U, source, [&](const auto& session) {
            source.calls.push_back(Call::pre_surface_stage);
            return session.lookup.map_offset == 0x12345678U &&
                session.header.status == LegacyLmfMapHeaderStatus::ready;
        });

    test.expect_equal(
        result.status,
        LegacyWorldMapLoadStatus::ready,
        "successful pre-surface stage resumes physical loading"
    );
    test.expect_equal(
        source.calls,
        std::vector<Call>{
            Call::lookup,
            Call::header,
            Call::pre_surface_stage,
            Call::surface,
            Call::post_surface,
            Call::referenced,
            Call::offset14,
            Call::indexed,
            Call::offset1c,
        },
        "pre-surface stage occupies the original sub_426840 slot"
    );

    FakeSource stopped;
    stopped.make_ready();
    std::vector<i32> progress;
    const auto failed = load_legacy_world_map(
        24U,
        stopped,
        [&](const auto&) {
            stopped.calls.push_back(Call::pre_surface_stage);
            return false;
        },
        {},
        [&](const i32 value, const auto&) { progress.push_back(value); }
    );
    test.expect_equal(
        failed.status,
        LegacyWorldMapLoadStatus::pre_surface_stage_failed,
        "failed pre-surface stage has a distinct load status"
    );
    test.expect_equal(
        stopped.calls,
        std::vector<Call>{Call::lookup, Call::header, Call::pre_surface_stage},
        "failed pre-surface stage stops before the surface stream"
    );
    test.expect_equal(
        progress,
        std::vector<i32>{15},
        "failed CM slot keeps progress 15 and stops before progress 60"
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
        FailureCase{
            Call::surface, LegacyWorldMapLoadStatus::surface_grid_failed
        },
        FailureCase{
            Call::post_surface,
            LegacyWorldMapLoadStatus::post_surface_records_failed
        },
        FailureCase{
            Call::referenced,
            LegacyWorldMapLoadStatus::referenced_record_directory_failed,
        },
        FailureCase{
            Call::offset14, LegacyWorldMapLoadStatus::offset14_directory_failed
        },
        FailureCase{
            Call::indexed,
            LegacyWorldMapLoadStatus::indexed_object_directory_failed,
        },
        FailureCase{
            Call::offset1c, LegacyWorldMapLoadStatus::offset1c_directory_failed
        },
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
        case Call::pre_surface_stage:
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
                LegacyLmfReferencedRecordDirectoryStatus::
                    referenced_record_read_failed;
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

        std::vector<i32> progress;
        const auto result = load_legacy_world_map(
            7U, source, {}, {}, [&](const i32 value, const auto&) {
                progress.push_back(value);
            }
        );
        test.expect_equal(
            result.status,
            failure.status,
            "failure is attributed to its physical stage"
        );
        test.expect_equal(
            source.calls.back(),
            failure.call,
            "no later stage runs after the first failure"
        );

        std::vector<i32> expected_progress;
        switch (failure.call) {
        case Call::lookup:
        case Call::header:
            break;
        case Call::pre_surface_stage:
            expected_progress = {15};
            break;
        case Call::surface:
        case Call::post_surface:
            expected_progress = {15, 60};
            break;
        case Call::referenced:
            expected_progress = {15, 60, 65};
            break;
        case Call::offset14:
            expected_progress = {15, 60, 65, 70};
            break;
        case Call::indexed:
            expected_progress = {15, 60, 65, 70, 75};
            break;
        case Call::offset1c:
            expected_progress = {15, 60, 65, 70, 75, 80};
            break;
        }
        test.expect_equal(
            progress,
            expected_progress,
            "first failure preserves only earlier progress side effects"
        );
    }
}

void test_current_maps(
    openswd3::test::Context& test, const std::filesystem::path& archive_path
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
        LegacyLmfWorldMapSource source{archive_path};
        std::vector<i32> progress;
        std::size_t audio_maintenance_count{};
        bool header_ready_at_progress_15{};
        const auto result = load_legacy_world_map(
            expected.map_id,
            source,
            {},
            {},
            [&](const i32 value, const auto& session) {
                progress.push_back(value);
                if (value == 15) {
                    header_ready_at_progress_15 = session.header.status ==
                        LegacyLmfMapHeaderStatus::ready;
                }
            },
            {},
            [&]() { ++audio_maintenance_count; }
        );
        test.expect_equal(
            result.status,
            LegacyWorldMapLoadStatus::ready,
            "fixed current map reaches a complete LMF session"
        );
        test.expect_equal(
            result.session.lookup.map_offset,
            expected.map_offset,
            "fixed map offset matches the inventory"
        );
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
        test.expect_true(
            progress == std::vector<i32>{15, 60, 65, 70, 75, 80, 85} &&
                !header_ready_at_progress_15,
            "real source reports progress 15 at the mid-header signature gate"
        );
        test.expect_equal(
            audio_maintenance_count,
            std::size_t{22U} +
                result.session.referenced_records.records.size() +
                result.session.indexed_objects.objects.size() * 5U,
            "real source preserves fixed and per-record AIL_serve counts"
        );
    }
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_ready_sequence(test);
    test_progress_sequence_and_business_milestones(test);
    test_progress_and_audio_maintenance_sequence(test);
    test_indexed_object_consumer_failure_stops_per_item(test);
    test_pre_surface_stage_slot(test);
    test_first_failure_stops(test);

    test.expect_true(
        argument_count == 1 || argument_count == 2,
        "optional argument names the current huge.lmf"
    );
    if (argument_count == 2 && arguments != nullptr &&
        arguments[1] != nullptr) {
        test_current_maps(test, arguments[1]);
    }
    return test.exit_code();
}
