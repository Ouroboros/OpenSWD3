#include <cstring>

#include "test.h"

namespace swd3test {
namespace {

int g_failures = 0;

}  // namespace

std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

void fail(const char* file, int line, const std::string& message) {
    ++g_failures;
    std::fprintf(stderr, "%s:%d: %s\n", file, line, message.c_str());
}

}  // namespace swd3test

int main(int argc, char** argv) {
    int run = 0;
    for (const auto& c : swd3test::registry()) {
        if (argc > 1 && std::strstr(c.name, argv[1]) == nullptr) {
            continue;
        }
        int before = swd3test::g_failures;
        c.body();
        ++run;
        std::printf("%s %s\n", swd3test::g_failures == before ? "[ OK ]" : "[FAIL]", c.name);
    }
    std::printf("%d test cases, %d failures\n", run, swd3test::g_failures);
    return swd3test::g_failures == 0 ? 0 : 1;
}
