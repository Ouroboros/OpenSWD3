#include <string>

#include "crt/msvc.h"
#include "test.h"

using swd3::crt::Sprintf;

TEST_CASE("msvc crt: %f rounds an exact tie half up as _fptostr does")
{
    CHECK(Sprintf("%4.1f", 2.25) == " 2.3");
    CHECK(Sprintf("%4.1f", 2.75) == " 2.8");
    CHECK(Sprintf("%4.1f", 0.0) == " 0.0");
    CHECK(Sprintf("%4.1f", 9.96) == "10.0");
    CHECK(Sprintf("%.1f", 0.04) == "0.0");
    CHECK(Sprintf("%.1f", 0.06) == "0.1");
}

TEST_CASE("msvc crt: Sprintf takes each conversion in turn")
{
    CHECK(Sprintf("MAct[%4.1f/%4.1f] LTCor[%4d/%4d]", 10.25, 3.0625, 12, 3) == "MAct[10.3/ 3.1] LTCor[  12/   3]");
    CHECK(Sprintf("ULck[%x] %d%%", 0x2A, 5) == "ULck[2a] 5%");
}
