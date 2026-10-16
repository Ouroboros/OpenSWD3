#include <cstring>
#include <vector>

#include "battle/fame.h"
#include "core/mem.h"
#include "crt/crt.h"
#include "test.h"

using namespace swd3;

namespace {

void freeNodes(std::uint32_t* holder) {
    auto* node = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(holder[0]));
    while (node != nullptr) {
        std::uint8_t* next = mem::ptr<std::uint8_t>(node, 0);
        crt::free(node);
        node = next;
    }
    ClearCounterList(holder);
}

int g_order[3];
int g_calls = 0;

void construct(void* object) { g_order[g_calls++] = *static_cast<int*>(object); }
void destroy(void* object) { g_order[g_calls++] = *static_cast<int*>(object); }

}  // namespace

TEST_CASE("fame.counters cap at 20 and keep percents") {
    std::uint32_t holder[5] = {};
    auto* list = reinterpret_cast<std::uint8_t*>(holder);
    CHECK_EQ(AddCounterCapped(list, 7, 30), 30);
    CHECK_EQ(FindCounterValue(list, 7), 30);
    CHECK_EQ(mem::u16(list, 4), 1);
    CHECK_EQ(AddCounterCapped(list, 7, 5), 30);
    CHECK_EQ(SetCounterCapped(list, 7, 50), 50);
    CHECK_EQ(FindCounterValue(list, 7), 20);
    CHECK_EQ(AddCounterCapped(list, 7, 5), 20);
    CHECK_EQ(StepCounterProgress(list, 9, 4, 200), 50);
    CHECK_EQ(FindCounterPercent(list, 9), 25);
    CHECK_EQ(StepCounterProgress(list, 9, 4, 200), 100);
    CHECK_EQ(FindCounterValue(list, 9), 2);
    SetCounterProgress(list, 9, 4, 9);
    CHECK_EQ(FindCounterValue(list, 9), 4);
    CHECK_EQ(FindCounterPercent(list, 9), 100);
    CHECK_EQ(mem::u16(list, 4), 2);
    CHECK_EQ(FindCounterValue(list, 3), 0);
    freeNodes(holder);
}

TEST_CASE("crt.vector iterators construct forwards and destroy backwards") {
    int objects[3] = {1, 2, 3};
    g_calls = 0;
    crt::vectorConstruct(objects, sizeof(int), 3, &construct, &destroy);
    CHECK_EQ(g_order[0], 1);
    CHECK_EQ(g_order[2], 3);
    g_calls = 0;
    crt::vectorDestruct(objects, sizeof(int), 3, &destroy);
    CHECK_EQ(g_order[0], 3);
    CHECK_EQ(g_order[2], 1);
}
