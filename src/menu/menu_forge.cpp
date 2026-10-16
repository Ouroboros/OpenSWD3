#include "menu/menu_forge.h"

#include <cstring>

#include "core/debug_log.h"
#include "core/mem.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "data/image.h"
#include "menu/menu_items.h"
#include "pic/rle.h"
#include "res/act_anim.h"
#include "res/tsw.h"

namespace swd3 {

namespace {

void* pointer(std::int32_t value) { return reinterpret_cast<void*>(static_cast<std::uintptr_t>(value)); }

void freeText(std::int32_t item) {
    auto* data = static_cast<std::uint8_t*>(pointer(item));
    if (mem::u32(data, 0xAC) != 0) {
        crt::free(mem::ptr<std::uint8_t>(data, 0xAC));
        mem::put32(data, 0xAC, 0);
    }
}

ActState* pictureAct() { return reinterpret_cast<ActState*>(&G.dword_4FC650); }

// one ACT step of 0x004FC650 decoded to a raw picture; false (logged with `error`) when the step fails
bool decodeAct(const char* error, std::int32_t* picture) {
    ActState* act = pictureAct();
    if (ActRun(act) == 0) {
        DebugLog(error, reinterpret_cast<const char*>(&G.unk_49FD0C), act->id, act->action, act->direction);
        return false;
    }
    Picture* frame = TswGetPicture(act->fr, act->ap);
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t format;
    *picture = static_cast<std::int32_t>(
        reinterpret_cast<std::uintptr_t>(RleDecode(frame->pixels, &width, &height, &format)));
    return true;
}

}  // namespace

// swd32.exe 0x0043D880: closes the forge page: picked items back to the inventory, both result items and every
// work buffer freed, close counter G.word_4FC900 = 1
void CloseForgePage() {
    G.dword_4FB970 = 0x232A;
    G.dword_4FB978 = 0x39;
    G.dword_4FC208 = 0x232A;
    G.dword_4FC210 = 3;
    ReturnPickedItems();
    if (G.dword_4FCC54 != 0) {
        crt::free(pointer(G.dword_4FCC54));
        G.dword_4FCC54 = 0;
    }
    if (G.dword_4FCD04 != 0) {
        crt::free(pointer(G.dword_4FCD04));
        G.dword_4FCD04 = 0;
    }
    std::memset(&image::at<std::uint8_t>(0x004FCBA8u), 0, 0xB0);
    std::memset(&image::at<std::uint8_t>(0x004FCC58u), 0, 0xB0);
    freeText(G.dword_4FCA88);
    freeText(G.dword_4FCBA0);
    while (G.dword_4FCAE0 != 0) {
        auto* item = static_cast<std::uint8_t*>(pointer(G.dword_4FCAE0));
        G.dword_4FCAE0 = static_cast<std::int32_t>(mem::u32(item, 0));
        crt::free(mem::ptr<std::uint8_t>(item, 0xAC));
        crt::free(item);
    }
    for (std::int32_t buffer : {G.dword_4FCA88, G.dword_4FCBA0, G.dword_4FCAC4, G.dword_4FCAB0, G.dword_4FCA90,
                                G.dword_4FCACC, G.dword_4FCAA8, G.dword_4FCB94, G.dword_4FCD08, G.dword_4FCB9C,
                                G.dword_4FCABC, G.dword_4FCAC0, G.dword_4FCAA0, G.dword_4FCD0C, G.dword_4FCA94}) {
        crt::free(pointer(buffer));
    }
    G.word_4FC900 = 1;
}

// swd32.exe 0x0043FDE0: clears the eight work buffers and decodes the pictures of the forge frame (act 0x232C step
// 0x4E), the selected source item (G.dword_4FCA8C picks the second) and both result items (act 0x44 of their
// picture ids +0x5C); stops at the first failed step. The return value is unused by callers.
void LoadForgePictures() {
    for (std::int32_t buffer : {G.dword_4FCAA8, G.dword_4FCB94, G.dword_4FCD08, G.dword_4FCB9C}) {
        std::memset(pointer(buffer), 0, 0xF0);
    }
    for (std::int32_t buffer : {G.dword_4FCABC, G.dword_4FCAC0, G.dword_4FCAA0, G.dword_4FCD0C}) {
        std::memset(pointer(buffer), 0, 0x1B8);
    }
    G.dword_4FC650 = 0x232C;
    G.dword_4FC658 = 0x4E;
    G.dword_4FC684 = 0;
    if (!decodeAct("Act Err(Botl::GetOriginal1)", &G.dword_4FCD1C)) {
        return;
    }
    const std::int32_t source = G.dword_4FCA8C != 0 ? G.dword_4FCBA0 : G.dword_4FCA88;
    G.dword_4FC650 = mem::u16(static_cast<std::uint8_t*>(pointer(source)), 0x5C);
    G.dword_4FC658 = 0x44;
    G.dword_4FC684 = 0;
    if (!decodeAct("Act Err(Botl::GetOriginal2)", &G.dword_4FCB90)) {
        return;
    }
    G.dword_4FC650 = static_cast<std::uint16_t>(G.dword_4FCC04);
    G.dword_4FC658 = 0x44;
    G.dword_4FC684 = 0;
    if (!decodeAct("Act Err(Botl::GetOriginal3)", &G.dword_4FCA9C)) {
        return;
    }
    G.dword_4FC650 = static_cast<std::uint16_t>(G.dword_4FCCB4);
    G.dword_4FC658 = 0x44;
    G.dword_4FC684 = 0;
    decodeAct("Act Err(Botl::GetOriginal4)", &G.dword_4FCA98);
}

}  // namespace swd3
