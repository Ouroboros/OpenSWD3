#include "menu/menu_util.h"
#include "test.h"

using namespace swd3;

TEST_CASE("menu.cursor moves, scrolls and pages") {
    int total = 12;
    int first = 0;
    int cursor = 4;
    int rows = 5;
    MenuCursorDown(&total, &first, &cursor, &rows);
    CHECK_EQ(cursor, 4);
    CHECK_EQ(first, 1);
    MenuPageDown(&total, &first, &cursor, &rows, 5);
    CHECK_EQ(first, 6);
    CHECK_EQ(cursor, 4);
    MenuPageDown(&total, &first, &cursor, &rows, 5);
    CHECK_EQ(first, 7);
    CHECK_EQ(rows, 5);
    CHECK_EQ(cursor, 4);
    MenuPageUp(0, &first, &cursor, 0, 5);
    CHECK_EQ(cursor, 0);
    CHECK_EQ(first, 7);
    MenuPageUp(0, &first, &cursor, 0, 5);
    CHECK_EQ(first, 2);
    MenuCursorUp(0, &first, &cursor);
    CHECK_EQ(cursor, 0);
    CHECK_EQ(first, 1);
}
