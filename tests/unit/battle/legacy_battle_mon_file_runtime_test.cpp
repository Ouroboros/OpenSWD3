#include "test.hpp"
#include "openswd3/battle/legacy_battle_mon_file_runtime.hpp"

#include <array>
#include <chrono>
#include <fstream>
#include <stdexcept>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

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
}
