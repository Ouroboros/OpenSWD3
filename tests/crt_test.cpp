#include <cstdint>

#include "crt/crt.h"
#include "test.h"

using namespace swd3;

TEST_CASE("crt.rand matches MSVC LCG") {
    crt::srand(1);
    CHECK_EQ(crt::rand(), 41);
    CHECK_EQ(crt::rand(), 18467);
    CHECK_EQ(crt::rand(), 6334);
    CHECK_EQ(crt::rand(), 26500);
    crt::srand(0x12345678);
    int first = crt::rand();
    CHECK_EQ(crt::randSeed(), 0x12345678u * 0x343FDu + 0x269EC3u);
    CHECK_EQ(first, static_cast<int>(((0x12345678u * 0x343FDu + 0x269EC3u) >> 16) & 0x7FFF));
}

TEST_CASE("crt.malloc fills fresh blocks with 0xCD") {
    auto* p = static_cast<std::uint8_t*>(crt::malloc(37));
    CHECK(p != nullptr);
    for (int i = 0; i < 37; ++i) {
        CHECK_EQ(p[i], 0xCD);
    }
    crt::free(p);
    auto* q = static_cast<std::uint8_t*>(crt::operator_new(5));
    CHECK_EQ(q[4], 0xCD);
    crt::operator_delete(q);
    crt::free(nullptr);
    crt::operator_delete(nullptr);
}

TEST_CASE("crt.ftol truncates and saturates to integer indefinite") {
    CHECK_EQ(crt::ftol(2.9), 2);
    CHECK_EQ(crt::ftol(-2.9), -2);
    CHECK_EQ(crt::ftol(4294967296.5), 4294967296LL);
    CHECK(crt::ftol(1e30) == INT64_MIN);
    CHECK(crt::ftol(-1e30) == INT64_MIN);
}

TEST_CASE("crt.difftime wraps through 32-bit subtraction") {
    CHECK(crt::difftime(10, 3) == 7.0);
    CHECK(crt::difftime(INT32_MIN, 1) == 2147483647.0);
}
