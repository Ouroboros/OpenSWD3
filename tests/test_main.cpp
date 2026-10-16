#include <cstring>

#include "test.h"

namespace swd3test
{
namespace
{

int failures = 0;

}  // namespace

std::vector<Case>& registry()
{
    static std::vector<Case> cases;
    return cases;
}

void fail(std::string_view file, int line, const std::string& message)
{
    ++failures;
    std::fprintf(stderr, "%.*s:%d: %s\n", static_cast<int>(file.size()), file.data(), line, message.c_str());
}

}  // namespace swd3test

int main(int argc, char** argv)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    int run = 0;

    for (const auto& c : swd3test::registry())
    {
        if (argc > 1 && c.name.find(argv[1]) == std::string_view::npos)
        {
            continue;
        }

        int before = swd3test::failures;
        c.body();
        ++run;
        std::printf(
            "%s %.*s\n",
            swd3test::failures == before ? "[ OK ]" : "[FAIL]",
            static_cast<int>(c.name.size()),
            c.name.data()
        );
    }

    std::printf("%d test cases, %d failures\n", run, swd3test::failures);
    return swd3test::failures == 0 ? 0 : 1;
}
