#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace swd3test {

struct Case {
    const char* name;
    std::function<void()> body;
};

std::vector<Case>& registry();
void fail(const char* file, int line, const std::string& message);

struct Registrar {
    Registrar(const char* name, std::function<void()> body) { registry().push_back({name, std::move(body)}); }
};

}  // namespace swd3test

#define SWD3_CONCAT_INNER(a, b) a##b
#define SWD3_CONCAT(a, b) SWD3_CONCAT_INNER(a, b)
#define TEST_CASE(name)                                                                                         \
    static void SWD3_CONCAT(swd3_test_fn_, __LINE__)();                                                         \
    static ::swd3test::Registrar SWD3_CONCAT(swd3_test_reg_, __LINE__)(name, SWD3_CONCAT(swd3_test_fn_, __LINE__)); \
    static void SWD3_CONCAT(swd3_test_fn_, __LINE__)()

#define CHECK(expr)                                                         \
    do {                                                                    \
        if (!(expr)) ::swd3test::fail(__FILE__, __LINE__, "CHECK(" #expr ")"); \
    } while (0)

#define CHECK_EQ(a, b)                                                                                    \
    do {                                                                                                  \
        auto&& swd3_a = (a);                                                                              \
        auto&& swd3_b = (b);                                                                              \
        if (!(swd3_a == swd3_b))                                                                          \
            ::swd3test::fail(__FILE__, __LINE__,                                                          \
                             "CHECK_EQ(" #a ", " #b ") " + std::to_string(static_cast<long long>(swd3_a)) + \
                                 " != " + std::to_string(static_cast<long long>(swd3_b)));                \
    } while (0)
