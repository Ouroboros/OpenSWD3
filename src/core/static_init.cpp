#include "core/static_init.h"

#include <new>

#include "crt/crt.h"
#include "data/globals.h"

namespace swd3 {

// swd32.exe 0x00401000
void InitMemoryManager() {
    ConstructMemoryManager();
    RegisterMemoryManagerDestructor();
}

// swd32.exe 0x00401010
void ConstructMemoryManager() { new (&G.memoryManager) MemoryManager(); }

// swd32.exe 0x00401020
void RegisterMemoryManagerDestructor() { crt::atexit(&DestroyMemoryManager); }

// swd32.exe 0x00401030
void DestroyMemoryManager() { G.memoryManager.~MemoryManager(); }

// swd32.exe 0x00401290
void InitFile4B74A0() {
    ConstructFile4B74A0();
    RegisterFile4B74A0Destructor();
}

// swd32.exe 0x004012A0
void ConstructFile4B74A0() { new (&G.file_4B74A0) File(); }

// swd32.exe 0x004012B0
void RegisterFile4B74A0Destructor() { crt::atexit(&DestroyFile4B74A0); }

// swd32.exe 0x004012C0
void DestroyFile4B74A0() { G.file_4B74A0.~File(); }

// swd32.exe 0x004012D0
void InitMapCacheFile() {
    ConstructMapCacheFile();
    RegisterMapCacheFileDestructor();
}

// swd32.exe 0x004012E0
void ConstructMapCacheFile() { new (&G.mapCacheFile) File(); }

// swd32.exe 0x004012F0
void RegisterMapCacheFileDestructor() { crt::atexit(&DestroyMapCacheFile); }

// swd32.exe 0x00401300
void DestroyMapCacheFile() { G.mapCacheFile.~File(); }

// swd32.exe 0x00401310
void InitFile4CAD50() {
    ConstructFile4CAD50();
    RegisterFile4CAD50Destructor();
}

// swd32.exe 0x00401320
void ConstructFile4CAD50() { new (&G.file_4CAD50) File(); }

// swd32.exe 0x00401330
void RegisterFile4CAD50Destructor() { crt::atexit(&DestroyFile4CAD50); }

// swd32.exe 0x00401340
void DestroyFile4CAD50() { G.file_4CAD50.~File(); }

// swd32.exe 0x00401350
void InitFile4B9F18() {
    ConstructFile4B9F18();
    RegisterFile4B9F18Destructor();
}

// swd32.exe 0x00401360
void ConstructFile4B9F18() { new (&G.file_4B9F18) File(); }

// swd32.exe 0x00401370
void RegisterFile4B9F18Destructor() { crt::atexit(&DestroyFile4B9F18); }

// swd32.exe 0x00401380
void DestroyFile4B9F18() { G.file_4B9F18.~File(); }

// swd32.exe 0x00401390
void InitFile4A98C8() {
    ConstructFile4A98C8();
    RegisterFile4A98C8Destructor();
}

// swd32.exe 0x004013A0
void ConstructFile4A98C8() { new (&G.file_4A98C8) File(); }

// swd32.exe 0x004013B0
void RegisterFile4A98C8Destructor() { crt::atexit(&DestroyFile4A98C8); }

// swd32.exe 0x004013C0
void DestroyFile4A98C8() { G.file_4A98C8.~File(); }

// swd32.exe 0x004013D0
void InitFile4CAA28() {
    ConstructFile4CAA28();
    RegisterFile4CAA28Destructor();
}

// swd32.exe 0x004013E0
void ConstructFile4CAA28() { new (&G.file_4CAA28) File(); }

// swd32.exe 0x004013F0
void RegisterFile4CAA28Destructor() { crt::atexit(&DestroyFile4CAA28); }

// swd32.exe 0x00401400
void DestroyFile4CAA28() { G.file_4CAA28.~File(); }

// swd32.exe 0x00401410
void InitFile4B7AD0() {
    ConstructFile4B7AD0();
    RegisterFile4B7AD0Destructor();
}

// swd32.exe 0x00401420
void ConstructFile4B7AD0() { new (&G.file_4B7AD0) File(); }

// swd32.exe 0x00401430
void RegisterFile4B7AD0Destructor() { crt::atexit(&DestroyFile4B7AD0); }

// swd32.exe 0x00401440
void DestroyFile4B7AD0() { G.file_4B7AD0.~File(); }

// swd32.exe 0x00401450
void InitFile4A9380() {
    ConstructFile4A9380();
    RegisterFile4A9380Destructor();
}

// swd32.exe 0x00401460
void ConstructFile4A9380() { new (&G.file_4A9380) File(); }

// swd32.exe 0x00401470
void RegisterFile4A9380Destructor() { crt::atexit(&DestroyFile4A9380); }

// swd32.exe 0x00401480
void DestroyFile4A9380() { G.file_4A9380.~File(); }

// swd32.exe 0x004272C0
void InitFile4CF6E0() {
    ConstructFile4CF6E0();
    RegisterFile4CF6E0Destructor();
}

// swd32.exe 0x004272D0
void ConstructFile4CF6E0() { new (&G.file_4CF6E0) File(); }

// swd32.exe 0x004272E0
void RegisterFile4CF6E0Destructor() { crt::atexit(&DestroyFile4CF6E0); }

// swd32.exe 0x004272F0
void DestroyFile4CF6E0() { G.file_4CF6E0.~File(); }

// swd32.exe 0x00451900
void InitFile521538() {
    ConstructFile521538();
    RegisterFile521538Destructor();
}

// swd32.exe 0x00451910
void ConstructFile521538() { new (&G.file_521538) File(); }

// swd32.exe 0x00451920
void RegisterFile521538Destructor() { crt::atexit(&DestroyFile521538); }

// swd32.exe 0x00451930
void DestroyFile521538() { G.file_521538.~File(); }

// swd32.exe 0x004011D0
void InitTextFont4AB998() {
    ConstructTextFont4AB998();
    RegisterTextFont4AB998Destructor();
}

// swd32.exe 0x004011E0
void ConstructTextFont4AB998() { new (&G.textFont_4AB998) TextFont(); }

// swd32.exe 0x004011F0
void RegisterTextFont4AB998Destructor() { crt::atexit(&DestroyTextFont4AB998); }

// swd32.exe 0x00401200
void DestroyTextFont4AB998() { G.textFont_4AB998.~TextFont(); }

// swd32.exe 0x00401210
void InitTextFont4C9A28() {
    ConstructTextFont4C9A28();
    RegisterTextFont4C9A28Destructor();
}

// swd32.exe 0x00401220
void ConstructTextFont4C9A28() { new (&G.textFont_4C9A28) TextFont(); }

// swd32.exe 0x00401230
void RegisterTextFont4C9A28Destructor() { crt::atexit(&DestroyTextFont4C9A28); }

// swd32.exe 0x00401240
void DestroyTextFont4C9A28() { G.textFont_4C9A28.~TextFont(); }

// swd32.exe 0x00401250
void InitTextFont4A9ED0() {
    ConstructTextFont4A9ED0();
    RegisterTextFont4A9ED0Destructor();
}

// swd32.exe 0x00401260
void ConstructTextFont4A9ED0() { new (&G.textFont_4A9ED0) TextFont(); }

// swd32.exe 0x00401270
void RegisterTextFont4A9ED0Destructor() { crt::atexit(&DestroyTextFont4A9ED0); }

// swd32.exe 0x00401280
void DestroyTextFont4A9ED0() { G.textFont_4A9ED0.~TextFont(); }

// swd32.exe 0x00401490: Graphics has no destructor, so nothing is registered
void InitGraphics() { ConstructGraphics(); }

// swd32.exe 0x004014A0
void ConstructGraphics() { new (&G.graphics) Graphics(); }

// swd32.exe 0x004014B0
void InitInput() {
    ConstructInput();
    RegisterInputDestructor();
}

// swd32.exe 0x004014C0
void ConstructInput() { new (&G.input) Input(); }

// swd32.exe 0x004014D0
void RegisterInputDestructor() { crt::atexit(&DestroyInput); }

// swd32.exe 0x004014E0
void DestroyInput() { G.input.~Input(); }

}  // namespace swd3
