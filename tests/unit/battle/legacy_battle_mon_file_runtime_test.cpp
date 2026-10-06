#include "test.hpp"
#include "openswd3/battle/legacy_battle_mon_file_runtime.hpp"
#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/battle/legacy_battle_mon_stream_runtime.hpp"
#include "openswd3/battle/legacy_battle_mon_text_runtime.hpp"
#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <stdexcept>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

class RuntimeMonPort final
    : public openswd3::battle::LegacyBattleMonDatabasePort {
public:
    openswd3::battle::LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const openswd3::battle::LegacyBattleMonDatabaseCallRequest& request,
        const std::span<openswd3::compat::u8> destination
    ) override {
        using Call = openswd3::battle::LegacyBattleMonDatabaseCall;
        switch (request.call) {
        case Call::open_file:
        case Call::seek_file:
        case Call::read_file:
            if (request.call == Call::read_file &&
                request.requested_bytes == 0x400U) {
                borrowed_storage = borrowed_storage &&
                    destination.data() == allocation.stream_bytes.data();
            }

            return files.invoke(request, destination);

        case Call::allocate_stream:
            allocation = streams.invoke(request);
            return allocation;

        case Call::release_stream:
            ++release_calls;
            return streams.invoke(request);

        case Call::allocate_definition_text:
        case Call::query_definition_text_size:
        case Call::release_definition_text:
            return text_heap.invoke(request);

        default:
            throw std::invalid_argument("unexpected MON runtime test request");
        }
    }

    openswd3::battle::LegacyBattleMonFileRuntime files;
    openswd3::battle::LegacyBattleMonStreamRuntime streams;
    openswd3::battle::LegacyBattleMonTextRuntime text_heap;
    openswd3::battle::LegacyBattleMonDatabaseCallReply allocation;
    unsigned release_calls{};
    bool borrowed_storage{true};
};

class MonTestFiles {
public:
    MonTestFiles()
        : root{
              std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
              ("battle-mon-file-" +
               std::to_string(
                   std::chrono::steady_clock::now().time_since_epoch().count()
               ))
          } {
        std::filesystem::create_directories(root);
        std::ofstream file{root / "MON.DAT", std::ios::binary};
        file.put('\x10');
        file.put('\x20');
        file.put('\x30');

        std::array<openswd3::compat::u8, 0x280U> definition{};
        definition[0x208U] = 0x40U;
        constexpr std::array<openswd3::compat::u8, 13U> stream{
            0xE8U, 0x03U, '$', '$', 30U, 0U, 'D', 'E', 'F', '$', '$', 5U, 0U,
        };
        std::copy(stream.begin(), stream.end(), definition.begin() + 0x240U);
        std::ofstream mon{root / "definition.dat", std::ios::binary};
        mon.write(reinterpret_cast<const char*>(definition.data()),
                  static_cast<std::streamsize>(definition.size()));
    }

    ~MonTestFiles() {
        std::error_code ignored;
        std::filesystem::remove_all(root, ignored);
    }

    std::filesystem::path root;
};

}  // namespace

void test_battle_mon_file_runtime(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    using Call = LegacyBattleMonDatabaseCall;
    const MonTestFiles files;
    LegacyBattleMonFileRuntime runtime;
    const std::filesystem::path empty_path;
    LegacyBattleMonDatabaseCallRequest open{
        .call = Call::open_file,
        .path = &empty_path,
        .desired_access = 0x80000000U,
        .share_mode = 1U,
        .creation_disposition = 4U,
        .flags_and_attributes = 0x80U,
        .ecx = 0x12345678U,
        .edx = 0x87654321U,
    };
    const auto first = runtime.invoke(open, {}, files.root);
    test.expect_true(
        first.eax != 0U && first.eax != 0xFFFFFFFFU,
        "MON default path resolves the existing uppercase asset"
    );
    test.expect_equal(first.ecx, open.ecx, "file boundary retains ECX residue");
    test.expect_equal(first.edx, open.edx, "file boundary retains EDX residue");
    std::array<openswd3::compat::u8, 4> bytes{0xAA, 0xBB, 0xCC, 0xDD};
    auto reply = runtime.invoke(
        {.call = Call::read_file, .handle = first.eax, .requested_bytes = 2U},
        bytes
    );
    test.expect_equal(reply.eax, 1U, "real MON read succeeds");
    test.expect_equal(reply.bytes_read, 2U, "requested read size is respected");
    test.expect_true(
        bytes == std::array<openswd3::compat::u8, 4>{0x10, 0x20, 0xCC, 0xDD},
        "read preserves bytes beyond the request"
    );
    reply = runtime.invoke(
        {.call = Call::seek_file,
         .handle = first.eax,
         .distance = 0xFFFFFFFFU,
         .move_method = 1U},
        {}
    );
    test.expect_equal(
        reply.eax, 1U, "negative relative seek returns absolute position"
    );
    reply = runtime.invoke(
        {.call = Call::read_file, .handle = first.eax, .requested_bytes = 4U},
        bytes
    );
    test.expect_equal(
        reply.bytes_read, 2U, "short read returns its actual count"
    );
    test.expect_true(
        bytes == std::array<openswd3::compat::u8, 4>{0x20, 0x30, 0xCC, 0xDD},
        "short MON read retains stale suffix"
    );
    reply = runtime.invoke(
        {.call = Call::read_file, .handle = first.eax, .requested_bytes = 4U},
        bytes
    );
    test.expect_true(
        reply.eax == 1U && reply.bytes_read == 0U,
        "EOF is a successful zero-byte ReadFile"
    );
    reply = runtime.invoke(
        {.call = Call::seek_file, .handle = first.eax, .distance = 0xFFFFFFFFU},
        {}
    );
    test.expect_equal(reply.eax, 0xFFFFFFFFU, "seek before beginning fails");
    reply = runtime.invoke(
        {.call = Call::seek_file,
         .handle = first.eax,
         .distance = 0xFFFFFFFFU,
         .move_method = 2U},
        {}
    );
    test.expect_equal(reply.eax, 2U, "end-relative seek recovers after EOF");

    const auto missing_parent = files.root / "absent" / "MON.DAT";
    open.path = &missing_parent;
    reply = runtime.invoke(open, {});
    test.expect_equal(
        reply.eax, 0xFFFFFFFFU, "failed open is not replaced by another file"
    );
    reply = runtime.invoke(
        {.call = Call::read_file, .handle = first.eax, .requested_bytes = 1U},
        bytes
    );
    test.expect_true(
        reply.eax == 1U && reply.bytes_read == 1U && bytes[0] == 0x30U,
        "failed open retains the prior file and its cursor"
    );
    reply = runtime.invoke(
        {.call = Call::read_file, .handle = 0xFFFFFFFFU, .requested_bytes = 4U},
        bytes
    );
    test.expect_true(
        reply.eax == 0U && reply.bytes_read == 0U,
        "invalid handle reports read failure"
    );
    reply =
        runtime.invoke({.call = Call::seek_file, .handle = 0xFFFFFFFFU}, {});
    test.expect_equal(
        reply.eax, 0xFFFFFFFFU, "invalid handle reports seek failure"
    );

    const auto created_path = files.root / "created.dat";
    open.path = &created_path;
    reply = runtime.invoke(open, {});
    test.expect_true(
        reply.eax != 0xFFFFFFFFU && reply.eax != first.eax &&
            std::filesystem::exists(created_path),
        "OPEN_ALWAYS creates a missing file with a distinct owned handle"
    );
    bool rejected = false;
    try {
        static_cast<void>(runtime.invoke(
            {.call = Call::read_file,
             .handle = first.eax,
             .requested_bytes = 5U},
            bytes
        ));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    test.expect_true(
        rejected, "undersized typed destination is not silently truncated"
    );

    RuntimeMonPort port;
    LegacyBattleMonProfile profile{};
    const auto stopped =
        load_legacy_battle_mon_profile(profile, port, {.path = created_path});
    test.expect_equal(
        stopped.status,
        LegacyBattleMonProfileLoadStatus::stream_access_typed_stop,
        "unterminated real empty-file profile stops parsing"
    );
    test.expect_equal(
        port.release_calls, 0U, "stopped parser retains its allocation"
    );
    const auto retained = port.allocation;
    test.expect_equal(
        retained.stream_bytes.size(),
        0x400U,
        "allocation reply borrows actual complete storage"
    );
    retained.stream_bytes[12U] = 0xA5U;
    std::array<openswd3::compat::u8, 0xA4U> definition{};
    LegacyBattleMonText description;
    const auto failed_tag = load_legacy_battle_mon_definition(
        definition, description, port, {.path = created_path}
    );
    test.expect_equal(
        failed_tag.return_eax, 0U, "bad definition tag returns zero"
    );
    test.expect_equal(
        port.release_calls, 1U, "bad tag releases its own allocation"
    );
    test.expect_true(
        port.allocation.eax != retained.eax,
        "a second live stream receives a distinct guest range"
    );
    test.expect_equal(
        retained.stream_bytes[12U],
        0xA5U,
        "stopped stream remains alive after another loader returns"
    );
    test.expect_true(
        port.borrowed_storage, "ReadFile writes the actual allocated block"
    );
    static_cast<void>(port.streams.invoke(
        {.call = Call::release_stream, .block_token = retained.eax}
    ));
    rejected = false;
    try {
        static_cast<void>(port.streams.invoke(
            {.call = Call::release_stream, .block_token = retained.eax}
        ));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    test.expect_true(
        rejected, "duplicate stream release cannot return success"
    );

    LegacyBattleMonTextRuntime text_heap;
    auto text_allocation = text_heap.invoke({
        .call = Call::allocate_definition_text,
        .allocation_size = 7U,
    });
    test.expect_true(
        text_allocation.eax != 0U && text_allocation.definition_text_storage &&
            text_allocation.definition_text_storage->size() == 7U,
        "text allocation returns a guest identity and its actual storage"
    );
    LegacyBattleMonText text;
    text.bind(text_allocation.definition_text_storage);
    auto alias = text;
    alias[3U] = 0x12U;
    test.expect_equal(
        (*text_allocation.definition_text_storage)[3U],
        0x12U,
        "record aliases write the heap allocation without a copied buffer"
    );
    const auto text_token = text_allocation.eax;
    std::weak_ptr<LegacyBattleMonText::Storage> retained_text =
        text_allocation.definition_text_storage;
    text_allocation.definition_text_storage.reset();
    text.clear();
    alias.clear();
    test.expect_true(
        !retained_text.expired(),
        "heap retains text after local parser views are discarded"
    );
    const auto text_size = text_heap.invoke({
        .call = Call::query_definition_text_size,
        .block_token = text_token,
    });
    test.expect_equal(
        text_size.eax, 7U, "size query returns requested debug-header bytes"
    );
    const auto second_text = text_heap.invoke({
        .call = Call::allocate_definition_text,
        .allocation_size = 2U,
    });
    test.expect_true(
        second_text.eax != text_token && (*retained_text.lock())[3U] == 0x12U,
        "later allocation preserves the previously retained text"
    );
    const auto text_release = text_heap.release({.block_token = text_token});
    test.expect_true(
        !text_release.typed_stop && retained_text.expired(),
        "explicit release removes the heap's remaining allocation owner"
    );
    test.expect_true(
        text_heap.release({.block_token = text_token}).typed_stop,
        "duplicate typed text release stops"
    );
    rejected = false;
    try {
        static_cast<void>(text_heap.invoke({
            .call = Call::query_definition_text_size,
            .block_token = text_token,
        }));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    test.expect_true(rejected, "released text cannot report a fake zero size");
    static_cast<void>(text_heap.invoke({
        .call = Call::release_definition_text,
        .block_token = second_text.eax,
    }));

    RuntimeMonPort definition_port;
    LegacyBattleMonDefinitionBytes loaded_definition{};
    LegacyBattleMonText loaded_description;
    const auto loaded = load_legacy_battle_mon_definition(
        loaded_definition, loaded_description, definition_port,
        {.path = files.root / "definition.dat", .definition_id = 1U}
    );
    test.expect_true(
        loaded.status == LegacyBattleMonDefinitionLoadStatus::completed &&
            loaded.definition_found &&
            loaded_description == LegacyBattleMonText::Storage{'D', 'E', 'F', 0U},
        "real MON file parser writes the allocation returned by the text heap"
    );
    test.expect_true(
        loaded_description.release() &&
            definition_port.text_heap.release({
                .block_token = loaded.definition_text_token,
            }).typed_stop,
        "parser forwards the heap release binding into its returned text view"
    );

    auto world_text = text_heap.invoke({
        .call = Call::allocate_definition_text,
        .allocation_size = 4U,
    });
    openswd3::world_map::LegacyWorldItemListState items;
    auto& item = items.player_inventory.emplace_back();
    item.description.bind(
        world_text.definition_text_storage, world_text.definition_text_release
    );
    auto stale_view = item.description;
    const auto cleaned =
        openswd3::world_map::release_legacy_world_item_lists(items);
    test.expect_equal(
        cleaned.status,
        openswd3::world_map::LegacyWorldItemListReleaseStatus::ready,
        "world cleanup releases registered MON text before its node"
    );
    test.expect_true(
        stale_view.empty() && !stale_view.release() &&
            text_heap.release({.block_token = world_text.eax}).typed_stop,
        "world free invalidates aliases and removes the heap registration"
    );

    world_text = text_heap.invoke({
        .call = Call::allocate_definition_text,
        .allocation_size = 4U,
    });
    openswd3::world_map::LegacyWorldItemListState duplicate_items;
    auto& first_item = duplicate_items.player_inventory.emplace_back();
    first_item.legacy_next_token = 42U;
    first_item.description.bind(
        world_text.definition_text_storage, world_text.definition_text_release
    );
    auto& duplicate_item = duplicate_items.player_inventory.emplace_back();
    duplicate_item.legacy_next_token = 43U;
    duplicate_item.description = first_item.description;
    duplicate_items.player_inventory.emplace_back().legacy_token = 43U;
    const auto cleanup_stop =
        openswd3::world_map::release_legacy_world_item_lists(duplicate_items);
    test.expect_equal(
        cleanup_stop.status,
        openswd3::world_map::LegacyWorldItemListReleaseStatus::
            description_release_typed_stop,
        "duplicate registered text stops at its original free"
    );
    test.expect_true(
        cleanup_stop.player_nodes_released == 1U &&
            cleanup_stop.description_release_calls == 2U &&
            duplicate_items.player_inventory_head_token == 43U &&
            duplicate_items.player_inventory.size() == 2U &&
            duplicate_items.party_item_lists[0U].has_value(),
        "failed cleanup preserves the unlink prefix and skips later frees"
    );
}
