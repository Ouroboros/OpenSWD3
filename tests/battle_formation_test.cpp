#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <vector>

#include "battle/battle_formation.h"
#include "test.h"

using namespace swd3;

namespace {

#if defined(_WIN32)
const char* kFormations = "E:\\Game\\swd3\\battle.ffd";
#else
const char* kFormations = "/mnt/e/Game/swd3/battle.ffd";
#endif

}  // namespace

TEST_CASE("battle.formations load from battle.ffd") {
    std::unique_ptr<std::uint8_t[]> storage{new std::uint8_t[sizeof(BattleFormation)]()};
    auto* formations = new (storage.get()) BattleFormation(nullptr);
    CHECK_EQ(formations->at<std::int32_t>(0x3104 + 8 * 29), 29);
    CHECK_EQ(formations->at<std::int32_t>(0x3108 + 8 * 29), 145 / 4);
    std::uint8_t* counts = nullptr;
    if (formations->LoadIndex(kFormations, &counts) == 0) {
        std::printf("  (%s not found, skipped)\n", kFormations);
        return;
    }
    CHECK(counts == formations->m_data + 0x1F48);
    CHECK_EQ(counts[1], 1);
    std::uint8_t formation[0x10C] = {};
    CHECK_EQ(formations->LoadFormation(kFormations, formation, 1, 1), 1);
    std::FILE* file = std::fopen(kFormations, "rb");
    CHECK(file != nullptr);
    // group 1, formation 1: no formations before it, so the number is entry 1 of the table at +8
    const std::int32_t number = formations->at<std::int32_t>(8 + 4 * 1);
    std::vector<std::uint8_t> expected(0x10C);
    std::fseek(file, 0x10C * number + 0x2714, SEEK_SET);
    CHECK_EQ(std::fread(expected.data(), 1, expected.size(), file), expected.size());
    std::fclose(file);
    CHECK(std::memcmp(formation, expected.data(), expected.size()) == 0);
    CHECK_EQ(formations->LoadFormation(kFormations, formation, 1, 2), 0);
}
