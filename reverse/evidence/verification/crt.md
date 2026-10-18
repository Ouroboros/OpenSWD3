# 静态链接 CRT 与导入跳转验证记录

## 第 1 遍

### 方法

按 AGENTS.md「CRT 与命名」，已识别的 MSVC 调试版 CRT 函数直接使用当前版本的 C/C++ 运行库，游戏调用的入口在 src/crt/ 中各包装一层（用户要求 2026-10-18）。逐行核对两点：

1. 识别：导入跳转为单条 `jmp ds:__imp_X`；CRT 函数位于 0x00487AF0–0x00497FCC 的 CRT 区（structure-overview.md），由调试版 CRT 断言里的源文件名（onexit.c、dbgheap.c、vsprintf.c 等）确定模块，表中列出各函数自身引用的断言源文件或所在模块。
2. 游戏可观察性：内部 CRT 函数只经入口影响游戏，所以以 tmp/crt_audit.py 扫描全部汇编的调用关系，找出被游戏代码区（0x00402230–0x00486A70）直接调用的入口，逐个核对重建侧行为：
   - DirectDrawCreate、DirectInputCreateA、ImmIsIME：导入跳转，调用者 Graphics::CreateDevice、Input::Init、TextInput::HandleMessage 的平台替换已登记（deviations.md 0x00437570 等、0x00436FB0、0x00438B50）。RtlUnwind 只被 CRT 的 SEH 展开调用。
   - atexit（6 个游戏调用者，均为动态初始化 thunk 登记全局对象的析构）：由 C++ 静态对象的析构承担，退出时的析构顺序不可观察。
   - malloc、free、operator new、operator delete：std 容器与 unique_ptr。调试堆的 0xCD/0xDD 填充只在原版读取未初始化或已释放内存时可见，这些位置已在各函数核对时按「未初始化」「越界」登记于 deviations.md。
   - _msize（Database::LoadObject）：调试堆返回申请的字节数，重建以说明文字长度 + 1 计算，相同。
   - memcpy（0x00489EB0）：入口 0x00489EC4 比较目标与源区间，目标落在源区间内时反向拷贝，即 memmove 语义。6 个调用者：Talk::ReplacePrefix（逐字节搬移）、Game::LeaveParty 与 TextInput::DeleteAtCursor（左移，前向拷贝安全）、FieldView::Render（右移，std::copy_backward）、SoundEffects::LoadSound（std::memmove）、Config::Update 的 Env.dat 写回（已按用户要求改为 TOML，不适用）。
   - eh vector constructor/destructor iterator（Game::Game）：std::array 成员的构造与析构。
   - strstr（Game::PlayVideo、SoundEffects::Open）：std::u8string::find / std::string::find。
   - 其余 CRT 函数没有游戏调用者，属于启动、堆、C++ 异常、printf 内部、数学错误处理与环境变量等库内部实现，不重写。

src/crt/ 中的包装（0x00489654 _ftol、0x00489B10/0x00489B20 srand/rand、0x00489B50 difftime、0x00489B70 time、0x00489D90 vsprintf、0x0048A1F0 floor 及其数学错误辅助）另见本文件末尾。

### 识别与映射表

| 地址 | 名称 | 识别依据 | 重建映射 | 游戏/全部调用者 |
|---|---|---|---|---|
| 0x00486AB0 | import:DirectDrawCreate | 导入跳转 `jmp ds:__imp_DirectDrawCreate`（1 条指令） | platform: SDL3 renderer (Display, Graphics) | 1/1 |
| 0x00487AD8 | import:DirectInputCreateA | 导入跳转 `jmp ds:__imp_DirectInputCreateA`（1 条指令） | platform: SDL3 events (WindowInputDevice, | 1/1 |
| 0x00487ADE | import:ImmIsIME | 导入跳转 `jmp ds:__imp_ImmIsIME`（1 条指令） | platform: SDL3 text input | 1/1 |
| 0x00487AF0 | CRT:_onexit | 50 条指令，引用断言源文件 onexit.c | C++ runtime: static destruction (std::atexit) | 0/1 |
| 0x00487BA0 | CRT:atexit | 12 条指令，位于 onexit.c 模块（同模块函数引用其断言源文件） | C++ runtime: std::atexit / member destructors | 6/25 |
| 0x00487BC0 | CRT:__onexitinit | 20 条指令，引用断言源文件 onexit.c | C++ runtime startup | 0/0 |
| 0x00487C10 | CRT:malloc | 13 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | std::make_unique / containers (std heap) | 118/125 |
| 0x00487C30 | CRT:_malloc_dbg | 16 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | std heap | 0/12 |
| 0x00487C60 | CRT:_nh_malloc | 13 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | std heap (operator new) | 0/1 |
| 0x00487C80 | CRT:_nh_malloc_dbg | 32 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | std heap | 0/3 |
| 0x00487CD0 | CRT:_heap_alloc_dbg | 254 条指令，引用断言源文件 dbgheap.c | std heap | 0/1 |
| 0x00487FF0 | CRT:_calloc_dbg | 37 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | std heap (value-initialised containers) | 0/1 |
| 0x00488050 | CRT:_realloc_dbg | 21 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | std heap (std::vector growth) | 0/2 |
| 0x00488080 | CRT:realloc_help | 431 条指令，引用断言源文件 dbgheap.c | std heap | 0/1 |
| 0x004885A0 | CRT:free | 9 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | std::unique_ptr / container destruction | 155/160 |
| 0x004885C0 | CRT:_free_dbg | 342 条指令，引用断言源文件 dbgheap.c | std heap | 0/10 |
| 0x004889B0 | CRT:_msize | 9 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | container size() | 1/1 |
| 0x004889D0 | CRT:_msize_dbg | 113 条指令，引用断言源文件 dbgheap.c | container size() | 0/2 |
| 0x00488B20 | CRT:CheckBytes | 55 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | not needed (debug heap check) | 0/2 |
| 0x00488BB0 | CRT:_CrtCheckMemory | 290 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | not needed (debug heap check) | 0/4 |
| 0x00488F10 | CRT:_CrtSetDbgFlag | 13 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | not needed (debug heap flags) | 0/1 |
| 0x00488F40 | CRT:_CrtIsValidPointer | 28 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | not needed (debug heap check) | 0/2 |
| 0x00488F90 | CRT:_CrtIsValidHeapPointer | 50 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | not needed (debug heap check) | 0/3 |
| 0x00489020 | CRT:_CrtMemCheckpoint | 110 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | not needed (debug heap report) | 0/1 |
| 0x00489180 | CRT:_CrtMemDumpAllObjectsSince | 247 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | not needed (debug heap report) | 0/1 |
| 0x00489430 | CRT:_printMemBlockData | 89 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | not needed (debug heap report) | 0/1 |
| 0x00489550 | CRT:_CrtDumpMemoryLeaks | 46 条指令，位于 dbgheap.c 模块（同模块函数引用其断言源文件） | not needed (debug heap report at exit) | 0/1 |
| 0x004895D0 | CRT:_fpmath | 15 条指令，位于 fpinit.c 模块（同模块函数引用其断言源文件） | C++ runtime floating point setup | 0/1 |
| 0x00489600 | CRT:_fptrap (empty) | 4 条指令，位于 fpinit.c 模块（同模块函数引用其断言源文件） | C++ runtime floating point setup | 0/0 |
| 0x00489610 | CRT:_cfltcvt_init | 10 条指令，位于 cmiscdat.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/1 |
| 0x00489680 | CRT:_JumpToContinuation | 23 条指令，位于 trnsctrl.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x004896C0 | CRT:_CallMemberFunction0 | 4 条指令，位于 trnsctrl.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/2 |
| 0x004896D0 | CRT:_CallMemberFunction1 | 4 条指令，位于 trnsctrl.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x004896E0 | CRT:_UnwindNestedFrames | 32 条指令，位于 trnsctrl.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/2 |
| 0x00489740 | CRT:__CxxFrameHandler | 31 条指令，位于 trnsctrl.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/34 |
| 0x00489780 | CRT:_CallCatchBlock2 | 36 条指令，位于 trnsctrl.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x004897F0 | CRT:CatchGuardHandler | 30 条指令，位于 trnsctrl.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/0 |
| 0x00489830 | CRT:_CallSETranslator | 57 条指令，位于 trnsctrl.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x00489900 | CRT:TranslatorGuardHandler | 53 条指令，位于 trnsctrl.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/0 |
| 0x00489990 | CRT:_GetRangeOfTrysToCheck | 57 条指令，位于 trnsctrl.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/2 |
| 0x00489A20 | CRT:__global_unwind2 | 18 条指令，位于 exsup.asm 模块（同模块函数引用其断言源文件） | compiler structured exception support | 0/1 |
| 0x00489A40 | CRT:__unwind_handler | 9 条指令，位于 exsup.asm 模块（同模块函数引用其断言源文件） | compiler structured exception support | 0/0 |
| 0x00489A62 | CRT:__local_unwind2 | 33 条指令，位于 exsup.asm 模块（同模块函数引用其断言源文件） | compiler structured exception support | 0/3 |
| 0x00489ACA | CRT:__abnormal_termination | 10 条指令，位于 exsup.asm 模块（同模块函数引用其断言源文件） | compiler structured exception support | 0/1 |
| 0x00489AED | CRT:_NLG_Notify1 | 4 条指令，位于 exsup.asm 模块（同模块函数引用其断言源文件） | compiler structured exception support | 0/1 |
| 0x00489AF6 | CRT:_NLG_Notify | 10 条指令，位于 exsup.asm 模块（同模块函数引用其断言源文件） | compiler structured exception support | 0/2 |
| 0x00489D00 | CRT:operator delete | 54 条指令，引用断言源文件 dbgdel.cpp | std::unique_ptr / container destruction | 22/40 |
| 0x00489E90 | CRT:operator new | 9 条指令，位于 new.cpp 模块（同模块函数引用其断言源文件） | std::make_unique / containers | 19/19 |
| 0x00489EB0 | CRT:memcpy | 269 条指令，位于 memcpy.asm 模块（同模块函数引用其断言源文件） | std::copy / std::memmove (overlap allowed as here) | 6/10 |
| 0x0048A330 | CRT:_cinit | 15 条指令，位于 crt0dat.c 模块（同模块函数引用其断言源文件） | C++ runtime startup | 0/1 |
| 0x0048A370 | CRT:exit | 10 条指令，位于 crt0dat.c 模块（同模块函数引用其断言源文件） | return from main | 0/1 |
| 0x0048A390 | CRT:_exit | 10 条指令，位于 crt0dat.c 模块（同模块函数引用其断言源文件） | C++ runtime exit | 0/5 |
| 0x0048A3B0 | CRT:doexit | 59 条指令，位于 crt0dat.c 模块（同模块函数引用其断言源文件） | C++ runtime exit | 0/2 |
| 0x0048A490 | CRT:_initterm | 16 条指令，位于 crt0dat.c 模块（同模块函数引用其断言源文件） | C++ runtime startup (static initialisation) | 0/2 |
| 0x0048A4C0 | CRT:eh vector constructor iterator | 52 条指令，位于 ehvecctr.cpp 模块（同模块函数引用其断言源文件） | std::array construction (Game::b | 2/2 |
| 0x0048A560 | CRT:eh vector destructor iterator | 54 条指令，位于 ehvecdtr.cpp 模块（同模块函数引用其断言源文件） | std::array destruction | 2/2 |
| 0x0048A610 | CRT:__ArrayUnwind | 42 条指令，位于 ehvecdtr.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/2 |
| 0x0048A690 | CRT:ArrayUnwindFilter | 17 条指令，位于 ehvecdtr.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x0048A6C0 | CRT:strstr | 66 条指令，位于 strstr.c 模块（同模块函数引用其断言源文件） | std::string_view::find | 2/2 |
| 0x0048A740 | CRT:WinMainCRTStartup | 98 条指令，位于 crt0.c 模块（同模块函数引用其断言源文件） | SDL3 main (src/app/main.cpp) | 0/0 |
| 0x0048A8A0 | CRT:_amsg_exit | 14 条指令，位于 crt0.c 模块（同模块函数引用其断言源文件） | C++ runtime error exit | 0/7 |
| 0x0048A8D0 | CRT:_fast_error_exit | 13 条指令，位于 crt0.c 模块（同模块函数引用其断言源文件） | C++ runtime error exit | 0/1 |
| 0x0048A900 | CRT:_callnewh | 19 条指令，位于 handler.cpp 模块（同模块函数引用其断言源文件） | C++ runtime new handler | 0/3 |
| 0x0048A930 | CRT:memset | 40 条指令，位于 memset.asm 模块（同模块函数引用其断言源文件） | std::fill / value initialisation | 0/5 |
| 0x0048A990 | CRT:_malloc_base | 10 条指令，位于 malloc.c 模块（同模块函数引用其断言源文件） | std heap | 0/1 |
| 0x0048A9B0 | CRT:_nh_malloc_base | 34 条指令，位于 malloc.c 模块（同模块函数引用其断言源文件） | std heap | 0/1 |
| 0x0048AA10 | CRT:_heap_alloc_base | 31 条指令，位于 malloc.c 模块（同模块函数引用其断言源文件） | std heap | 0/2 |
| 0x0048AA70 | CRT:_CrtDefaultAllocHook | 5 条指令，位于 dbghook.c 模块（同模块函数引用其断言源文件） | not needed (debug heap hook) | 0/3 |
| 0x0048AA80 | CRT:_CrtDbgBreak | 5 条指令，位于 dbgrpt.c 模块（同模块函数引用其断言源文件） | not needed (debug report) | 0/2 |
| 0x0048AA90 | CRT:_CrtDbgReport | 248 条指令，位于 dbgrpt.c 模块（同模块函数引用其断言源文件） | not needed (debug report); the rebuild's log (Log | 0/23 |
| 0x0048AE20 | CRT:CrtMessageWindow | 197 条指令，引用断言源文件 dbgrpt.c | not needed (debug report dialog) | 0/1 |
| 0x0048B130 | CRT:_expand_base | 50 条指令，位于 expand.c 模块（同模块函数引用其断言源文件） | std heap | 0/1 |
| 0x0048B1C0 | CRT:_realloc_base | 156 条指令，位于 realloc.c 模块（同模块函数引用其断言源文件） | std heap (std::vector growth) | 0/1 |
| 0x0048B390 | CRT:_free_base | 29 条指令，位于 free.c 模块（同模块函数引用其断言源文件） | std heap | 0/2 |
| 0x0048B3E0 | CRT:_heapchk | 26 条指令，位于 heapchk.c 模块（同模块函数引用其断言源文件） | not needed (heap check) | 0/1 |
| 0x0048B440 | CRT:_heap_init | 25 条指令，位于 heapinit.c 模块（同模块函数引用其断言源文件） | C++ runtime startup (heap) | 0/1 |
| 0x0048B490 | CRT:__sbh_heap_init | 20 条指令，位于 sbheap.c 模块（同模块函数引用其断言源文件） | std heap | 0/1 |
| 0x0048B4F0 | CRT:__sbh_find_block | 29 条指令，位于 sbheap.c 模块（同模块函数引用其断言源文件） | std heap | 0/4 |
| 0x0048B550 | CRT:__sbh_verify_block | 33 条指令，位于 sbheap.c 模块（同模块函数引用其断言源文件） | std heap | 0/1 |
| 0x0048B5B0 | CRT:__sbh_free_block | 453 条指令，位于 sbheap.c 模块（同模块函数引用其断言源文件） | std heap | 0/2 |
| 0x0048BB80 | CRT:__sbh_alloc_block | 427 条指令，位于 sbheap.c 模块（同模块函数引用其断言源文件） | std heap | 0/2 |
| 0x0048C0C0 | CRT:__sbh_alloc_new_region | 78 条指令，位于 sbheap.c 模块（同模块函数引用其断言源文件） | std heap | 0/1 |
| 0x0048C1D0 | CRT:__sbh_alloc_new_group | 149 条指令，位于 sbheap.c 模块（同模块函数引用其断言源文件） | std heap | 0/1 |
| 0x0048C3C0 | CRT:__sbh_resize_block | 489 条指令，位于 sbheap.c 模块（同模块函数引用其断言源文件） | std heap | 0/2 |
| 0x0048C9B0 | CRT:__sbh_heap_check | 343 条指令，位于 sbheap.c 模块（同模块函数引用其断言源文件） | not needed (heap check) | 0/1 |
| 0x0048CFD0 | CRT:sprintf | 92 条指令，引用断言源文件 sprintf.c | std::snprintf (only the debug report formats with it) | 0/1 |
| 0x0048D0D0 | CRT:_isctype | 58 条指令，位于 isctype.c 模块（同模块函数引用其断言源文件） | C++ runtime character classes | 0/5 |
| 0x0048D190 | CRT:_setdefaultprecision | 8 条指令，位于 fp8.c 模块（同模块函数引用其断言源文件） | C++ runtime floating point setup | 0/1 |
| 0x0048D1B0 | CRT:_ms_p5_test_fdiv | 30 条指令，位于 testfdiv.c 模块（同模块函数引用其断言源文件） | C++ runtime floating point setup | 0/1 |
| 0x0048D210 | CRT:_ms_p5_mp_test_fdiv | 22 条指令，位于 testfdiv.c 模块（同模块函数引用其断言源文件） | C++ runtime floating point setup | 0/1 |
| 0x0048D260 | CRT:_forcdecpt | 59 条指令，位于 cvt.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/0 |
| 0x0048D310 | CRT:_cropzeros | 78 条指令，位于 cvt.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/0 |
| 0x0048D3F0 | CRT:_positive | 16 条指令，位于 cvt.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/0 |
| 0x0048D420 | CRT:_fassign | 29 条指令，位于 cvt.c 模块（同模块函数引用其断言源文件） | C++ runtime scanf float support | 0/0 |
| 0x0048D470 | CRT:_cftoe | 157 条指令，位于 cvt.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/2 |
| 0x0048D630 | CRT:_cftof | 135 条指令，位于 cvt.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/2 |
| 0x0048D7C0 | CRT:_cftog | 81 条指令，位于 cvt.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/1 |
| 0x0048D8C0 | CRT:_cftoe_g | 20 条指令，位于 cvt.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/1 |
| 0x0048D900 | CRT:_cftof_g | 18 条指令，位于 cvt.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/1 |
| 0x0048D930 | CRT:_cfltcvt | 40 条指令，位于 cvt.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/0 |
| 0x0048D990 | CRT:_shift | 19 条指令，位于 cvt.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/2 |
| 0x0048D9C0 | CRT:__InternalCxxFrameHandler | 88 条指令，位于 frame.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/3 |
| 0x0048DAB0 | CRT:FindHandler | 219 条指令，位于 frame.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x0048DD50 | CRT:FindHandlerForForeignException | 106 条指令，位于 frame.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x0048DE70 | CRT:TypeMatch | 69 条指令，位于 frame.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x0048DF30 | CRT:__FrameUnwindToState | 77 条指令，位于 frame.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/2 |
| 0x0048E030 | CRT:FrameUnwindFilter | 17 条指令，位于 frame.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x0048E060 | CRT:CatchIt | 73 条指令，位于 frame.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/2 |
| 0x0048E120 | CRT:CallCatchBlock | 98 条指令，位于 frame.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x0048E270 | CRT:ExFilterRethrow | 24 条指令，位于 frame.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x0048E2C0 | CRT:BuildCatchObject | 229 条指令，位于 frame.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x0048E540 | CRT:DestructExceptionObject | 46 条指令，位于 frame.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/2 |
| 0x0048E5D0 | CRT:AdjustPointer | 27 条指令，位于 frame.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x0048E620 | CRT:_CallSettingFrame | 34 条指令，位于 lowhelpr.asm 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/2 |
| 0x0048E670 | CRT:terminate | 37 条指令，位于 hooks.cpp 模块（同模块函数引用其断言源文件） | std::terminate | 0/7 |
| 0x0048E700 | CRT:_inconsistency | 37 条指令，位于 hooks.cpp 模块（同模块函数引用其断言源文件） | std::terminate | 0/5 |
| 0x0048E790 | CRT:__loctotime_t | 75 条指令，位于 dtoxtime.c 模块（同模块函数引用其断言源文件） | crt::Time (src/crt/msvc.cpp) | 0/1 |
| 0x0048E880 | CRT:_flsbuf | 213 条指令，引用断言源文件 _flsbuf.c | C++ runtime stdio (not used by the game) | 0/5 |
| 0x0048EB00 | CRT:_output | 821 条指令，引用断言源文件 output.c | C++ runtime printf (not used by the game) | 0/4 |
| 0x0048F830 | CRT:write_char | 44 条指令，位于 output.c 模块（同模块函数引用其断言源文件） | C++ runtime printf | 0/3 |
| 0x0048F8B0 | CRT:write_multi_char | 23 条指令，位于 output.c 模块（同模块函数引用其断言源文件） | C++ runtime printf | 0/1 |
| 0x0048F8F0 | CRT:write_string | 31 条指令，位于 output.c 模块（同模块函数引用其断言源文件） | C++ runtime printf | 0/1 |
| 0x0048F940 | CRT:get_int_arg | 12 条指令，位于 output.c 模块（同模块函数引用其断言源文件） | C++ runtime printf | 0/1 |
| 0x0048F960 | CRT:get_int64_arg | 14 条指令，位于 output.c 模块（同模块函数引用其断言源文件） | C++ runtime printf | 0/1 |
| 0x0048F980 | CRT:get_short_arg | 12 条指令，位于 output.c 模块（同模块函数引用其断言源文件） | C++ runtime printf | 0/1 |
| 0x004907D0 | CRT:__initstdio | 78 条指令，引用断言源文件 _file.c | C++ runtime stdio setup | 0/0 |
| 0x00490900 | CRT:__endstdio | 9 条指令，位于 _file.c 模块（同模块函数引用其断言源文件） | C++ runtime stdio teardown | 0/0 |
| 0x00490924 | CRT:_except_handler3 | 73 条指令，位于 exsup3.asm 模块（同模块函数引用其断言源文件） | compiler structured exception support | 0/0 |
| 0x004909E1 | CRT:_seh_longjmp_unwind | 11 条指令，位于 exsup3.asm 模块（同模块函数引用其断言源文件） | compiler structured exception support | 0/0 |
| 0x00490A10 | CRT:strchr | 89 条指令，位于 strchr.asm 模块（同模块函数引用其断言源文件） | std::string_view::find | 0/1 |
| 0x00490AD0 | CRT:_XcptFilter | 110 条指令，位于 winxfltr.c 模块（同模块函数引用其断言源文件） | platform: InstallFaultLog (src/platform/system_info | 0/1 |
| 0x00490C80 | CRT:xcptlookup | 32 条指令，位于 winxfltr.c 模块（同模块函数引用其断言源文件） | platform: InstallFaultLog | 0/1 |
| 0x00490CE0 | CRT:_wincmdln | 74 条指令，位于 wincmdln.c 模块（同模块函数引用其断言源文件） | SDL3 main (the command line, src/app/main.cpp) | 0/1 |
| 0x00490DA0 | CRT:_setenvp | 102 条指令，引用断言源文件 stdenvp.c | C++ runtime startup (environment) | 0/1 |
| 0x00490EF0 | CRT:_setargv | 69 条指令，引用断言源文件 stdargv.c | SDL3 main (argv) | 0/1 |
| 0x00490FD0 | CRT:parse_cmdline | 360 条指令，位于 stdargv.c 模块（同模块函数引用其断言源文件） | SDL3 main (argv) | 0/1 |
| 0x00491400 | CRT:__crtGetEnvironmentStringsA | 173 条指令，引用断言源文件 a_env.c | C++ runtime startup (environment) | 0/1 |
| 0x00491620 | CRT:_ioinit | 230 条指令，引用断言源文件 ioinit.c | C++ runtime stdio setup | 0/1 |
| 0x00491930 | CRT:_FF_MSGBANNER | 19 条指令，位于 crt0msg.c 模块（同模块函数引用其断言源文件） | C++ runtime error message | 0/2 |
| 0x00491980 | CRT:_NMSG_WRITE | 131 条指令，位于 crt0msg.c 模块（同模块函数引用其断言源文件） | C++ runtime error message | 0/4 |
| 0x00491B40 | CRT:_itoa | 28 条指令，位于 xtoa.c 模块（同模块函数引用其断言源文件） | std::to_chars (not used by the game) | 0/1 |
| 0x00491B90 | CRT:xtoa | 71 条指令，位于 xtoa.c 模块（同模块函数引用其断言源文件） | std::to_chars | 0/1 |
| 0x00491C60 | CRT:strlen | 44 条指令，位于 strlen.asm 模块（同模块函数引用其断言源文件） | std::string_view::size | 0/11 |
| 0x00491CE0 | CRT:_snprintf | 93 条指令，引用断言源文件 sprintf.c | std::snprintf (only the debug report uses it) | 0/2 |
| 0x00491DE0 | CRT:strcpy | 3 条指令，位于 strcat.asm 模块（同模块函数引用其断言源文件） | std::string assignment | 0/9 |
| 0x00491DF0 | CRT:strcat | 84 条指令，位于 strcat.asm 模块（同模块函数引用其断言源文件） | std::string append | 0/2 |
| 0x00491ED0 | CRT:_vsnprintf | 91 条指令，引用断言源文件 vsprintf.c | std::vsnprintf (only the debug report uses it) | 0/1 |
| 0x00491FD0 | CRT:_alloca_probe | 17 条指令，位于 chkstk.asm 模块（同模块函数引用其断言源文件） | compiler stack probe | 0/5 |
| 0x00492000 | CRT:raise | 115 条指令，位于 winsig.c 模块（同模块函数引用其断言源文件） | std::raise (InstallFaultLog re-raises) | 0/2 |
| 0x004921C0 | CRT:siglookup | 32 条指令，位于 winsig.c 模块（同模块函数引用其断言源文件） | std::raise | 0/1 |
| 0x00492220 | CRT:__crtMessageBoxA | 54 条指令，位于 crtmbox.c 模块（同模块函数引用其断言源文件） | platform: SDL message boxes (DialogErrorReport | 0/2 |
| 0x004922E0 | CRT:strncpy | 109 条指令，位于 strncpy.c 模块（同模块函数引用其断言源文件） | std::string assignment | 0/3 |
| 0x004923E0 | CRT:memmove | 268 条指令，位于 memmove.asm 模块（同模块函数引用其断言源文件） | std::memmove | 0/2 |
| 0x00492720 | CRT:__crtGetStringTypeA | 148 条指令，位于 aw_str.c 模块（同模块函数引用其断言源文件） | C++ runtime locale | 0/2 |
| 0x004928E0 | CRT:_control87 | 33 条指令，位于 ieee87.c 模块（同模块函数引用其断言源文件） | C++ runtime floating point control | 0/1 |
| 0x00492930 | CRT:_controlfp | 11 条指令，位于 ieee87.c 模块（同模块函数引用其断言源文件） | C++ runtime floating point control | 0/1 |
| 0x00492950 | CRT:_abstract_cw | 115 条指令，位于 ieee87.c 模块（同模块函数引用其断言源文件） | C++ runtime floating point control | 0/1 |
| 0x00492AD0 | CRT:_hw_cw | 106 条指令，位于 ieee87.c 模块（同模块函数引用其断言源文件） | C++ runtime floating point control | 0/1 |
| 0x00492C30 | CRT:tolower | 91 条指令，位于 tolower.c 模块（同模块函数引用其断言源文件） | C++ runtime locale (not used by the game) | 0/1 |
| 0x00492D60 | CRT:_ZeroTail | 51 条指令，位于 cvt.c (mantold) 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/1 |
| 0x00492DF0 | CRT:_IncMan | 63 条指令，位于 cvt.c (mantold) 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/1 |
| 0x00492EA0 | CRT:_RoundMan | 78 条指令，位于 cvt.c (mantold) 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/1 |
| 0x00492F80 | CRT:_CopyMan | 28 条指令，位于 cvt.c (mantold) 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/1 |
| 0x00492FD0 | CRT:_FillZeroMan | 17 条指令，位于 cvt.c (mantold) 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/1 |
| 0x00493000 | CRT:_IsZeroMan | 21 条指令，位于 cvt.c (mantold) 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/1 |
| 0x00493040 | CRT:_ShrMan | 82 条指令，位于 cvt.c (mantold) 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/1 |
| 0x00493130 | CRT:_ld12cvt | 188 条指令，位于 cvt.c (mantold) 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/2 |
| 0x00493380 | CRT:_ld12tod | 11 条指令，位于 cvt.c (mantold) 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/1 |
| 0x004933A0 | CRT:_ld12tof | 11 条指令，位于 cvt.c (mantold) 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/1 |
| 0x004933C0 | CRT:_atodbl | 24 条指令，位于 cvt.c (mantold) 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/1 |
| 0x00493400 | CRT:_atoflt | 24 条指令，位于 cvt.c (mantold) 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/1 |
| 0x00493440 | CRT:_fptostr | 90 条指令，位于 _fptostr.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/3 |
| 0x00493540 | CRT:_fltout2 | 32 条指令，位于 cfout.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/3 |
| 0x004935C0 | CRT:_dtold | 104 条指令，位于 cfout.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/1 |
| 0x00493730 | CRT:_purecall | 7 条指令，位于 purevirt.c 模块（同模块函数引用其断言源文件） | compiler pure virtual call trap | 0/1 |
| 0x00493740 | CRT:unexpected | 30 条指令，位于 hooks.cpp 模块（同模块函数引用其断言源文件） | std::terminate | 0/0 |
| 0x004937A0 | CRT:__CxxSetUnhandledExceptionFilter | 7 条指令，位于 unhandld.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/0 |
| 0x004937C0 | CRT:__CxxRestoreUnhandledExceptionFilter | 7 条指令，位于 unhandld.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception sup | 0/0 |
| 0x004937E0 | CRT:_ValidateRead | 16 条指令，位于 validate.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/2 |
| 0x00493810 | CRT:_ValidateWrite | 16 条指令，位于 validate.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/1 |
| 0x00493840 | CRT:_ValidateExecute | 14 条指令，位于 validate.cpp 模块（同模块函数引用其断言源文件） | compiler C++ exception support | 0/2 |
| 0x00493870 | CRT:strcmp | 56 条指令，位于 strcmp.asm 模块（同模块函数引用其断言源文件） | std::string_view comparison | 0/2 |
| 0x00493900 | CRT:abort | 12 条指令，位于 abort.c 模块（同模块函数引用其断言源文件） | std::abort (Fault) | 0/1 |
| 0x00493920 | CRT:__tzset | 12 条指令，位于 tzset.c 模块（同模块函数引用其断言源文件） | crt::Time (local time zone) | 0/1 |
| 0x00493950 | CRT:_tzset | 251 条指令，引用断言源文件 tzset.c | crt::Time (local time zone) | 0/1 |
| 0x00493CD0 | CRT:_isindst | 246 条指令，位于 tzset.c 模块（同模块函数引用其断言源文件） | crt::Time (local time zone) | 0/1 |
| 0x00493FD0 | CRT:cvtdate | 145 条指令，位于 tzset.c 模块（同模块函数引用其断言源文件） | crt::Time (local time zone) | 0/1 |
| 0x004941D0 | CRT:_lseek | 69 条指令，位于 lseek.c 模块（同模块函数引用其断言源文件） | C++ runtime low-level files (the game uses its own cFile | 0/2 |
| 0x004942C0 | CRT:_write | 189 条指令，位于 write.c 模块（同模块函数引用其断言源文件） | C++ runtime low-level files | 0/2 |
| 0x00494570 | CRT:_getbuf | 68 条指令，引用断言源文件 _getbuf.c | C++ runtime stdio | 0/1 |
| 0x00494640 | CRT:_isatty | 16 条指令，位于 isatty.c 模块（同模块函数引用其断言源文件） | C++ runtime stdio | 0/1 |
| 0x00494670 | CRT:wctomb | 48 条指令，位于 wctomb.c 模块（同模块函数引用其断言源文件） | C++ runtime locale | 0/1 |
| 0x00494710 | CRT:_aulldiv | 43 条指令，位于 ulldiv.asm 模块（同模块函数引用其断言源文件） | compiler 64-bit division | 0/1 |
| 0x00494780 | CRT:_aullrem | 44 条指令，位于 ullrem.asm 模块（同模块函数引用其断言源文件） | compiler 64-bit remainder | 0/1 |
| 0x00494810 | CRT:fflush | 32 条指令，位于 fflush.c 模块（同模块函数引用其断言源文件） | C++ runtime stdio | 0/1 |
| 0x00494870 | CRT:_flush | 62 条指令，位于 fflush.c 模块（同模块函数引用其断言源文件） | C++ runtime stdio | 0/2 |
| 0x00494930 | CRT:_flushall | 7 条指令，位于 fflush.c 模块（同模块函数引用其断言源文件） | C++ runtime stdio | 0/1 |
| 0x00494940 | CRT:flsall | 65 条指令，位于 fflush.c 模块（同模块函数引用其断言源文件） | C++ runtime stdio | 0/2 |
| 0x00494A20 | CRT:_fcloseall | 51 条指令，位于 closeall.c 模块（同模块函数引用其断言源文件） | C++ runtime stdio teardown | 0/1 |
| 0x00494AD0 | CRT:_ismbblead | 10 条指令，位于 ismbbyte.c 模块（同模块函数引用其断言源文件） | C++ runtime multibyte (code page 950 in text/big5) | 0/1 |
| 0x00494AF0 | CRT:x_ismbbtype | 30 条指令，位于 ismbbyte.c 模块（同模块函数引用其断言源文件） | C++ runtime multibyte | 0/1 |
| 0x00494B60 | CRT:_setmbcp | 217 条指令，位于 mbctype.c 模块（同模块函数引用其断言源文件） | C++ runtime multibyte | 0/1 |
| 0x00494E60 | CRT:getSystemCP | 21 条指令，位于 mbctype.c 模块（同模块函数引用其断言源文件） | C++ runtime multibyte | 0/1 |
| 0x00494EC0 | CRT:CPtoLCID | 26 条指令，位于 mbctype.c 模块（同模块函数引用其断言源文件） | C++ runtime multibyte | 0/1 |
| 0x00494F40 | CRT:setSBCS | 29 条指令，位于 mbctype.c 模块（同模块函数引用其断言源文件） | C++ runtime multibyte | 0/1 |
| 0x00494FC0 | CRT:setSBUpLow | 175 条指令，位于 mbctype.c 模块（同模块函数引用其断言源文件） | C++ runtime multibyte | 0/1 |
| 0x004952D0 | CRT:__initmbctable | 10 条指令，位于 mbctype.c 模块（同模块函数引用其断言源文件） | C++ runtime multibyte | 0/3 |
| 0x00495300 | CRT:__crtLCMapStringA | 263 条指令，位于 a_map.c 模块（同模块函数引用其断言源文件） | C++ runtime locale | 0/2 |
| 0x00495610 | CRT:___addl | 23 条指令，位于 mantold.c 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/3 |
| 0x00495650 | CRT:___add_12 | 66 条指令，位于 mantold.c 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/2 |
| 0x00495700 | CRT:___shl_12 | 37 条指令，位于 mantold.c 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/3 |
| 0x00495760 | CRT:___shr_12 | 37 条指令，位于 mantold.c 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/2 |
| 0x004957D0 | CRT:___mtold12 | 101 条指令，位于 mantold.c 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/1 |
| 0x00495910 | CRT:__strgtold12 | 690 条指令，位于 strgtold.c 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/2 |
| 0x00496440 | CRT:$I10_OUTPUT | 386 条指令，位于 x10fout.c 模块（同模块函数引用其断言源文件） | C++ runtime printf float support | 0/1 |
| 0x00496960 | CRT:atol | 88 条指令，位于 atox.c 模块（同模块函数引用其断言源文件） | std::from_chars (not used by the game) | 0/1 |
| 0x00496A60 | CRT:getenv | 69 条指令，位于 getenv.c 模块（同模块函数引用其断言源文件） | C++ runtime environment (not used by the game) | 0/1 |
| 0x00496B30 | CRT:_dosmaperr | 37 条指令，位于 dosmap.c 模块（同模块函数引用其断言源文件） | C++ runtime errors | 0/3 |
| 0x00496BC0 | CRT:_free_osfhnd | 58 条指令，位于 osfinfo.c 模块（同模块函数引用其断言源文件） | C++ runtime low-level files | 0/1 |
| 0x00496C90 | CRT:_get_osfhandle | 26 条指令，位于 osfinfo.c 模块（同模块函数引用其断言源文件） | C++ runtime low-level files | 0/3 |
| 0x00496CF0 | CRT:_commit | 50 条指令，位于 commit.c 模块（同模块函数引用其断言源文件） | C++ runtime low-level files | 0/1 |
| 0x00496DA0 | CRT:fclose | 76 条指令，引用断言源文件 fclose.c | C++ runtime stdio teardown | 0/1 |
| 0x00496E80 | CRT:___ld12mul | 299 条指令，位于 tenpow.c 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/2 |
| 0x004972C0 | CRT:___multtenpow12 | 67 条指令，位于 tenpow.c 模块（同模块函数引用其断言源文件） | C++ runtime float parsing | 0/2 |
| 0x00497390 | CRT:_allmul | 19 条指令，位于 llmul.asm 模块（同模块函数引用其断言源文件） | compiler 64-bit multiplication | 0/0 |
| 0x004973D0 | CRT:_strnicmp | 32 条指令，位于 strnicmp.c 模块（同模块函数引用其断言源文件） | C++ runtime locale (getenv) | 0/2 |
| 0x00497420 | CRT:__wtomb_environ | 66 条指令，引用断言源文件 wtombenv.c | C++ runtime environment | 0/2 |
| 0x004974D0 | CRT:_close | 74 条指令，位于 close.c 模块（同模块函数引用其断言源文件） | C++ runtime low-level files | 0/1 |
| 0x004975C0 | CRT:_freebuf | 52 条指令，引用断言源文件 _freebuf.c | C++ runtime stdio | 0/1 |
| 0x00497660 | CRT:__crtCompareStringA | 340 条指令，引用断言源文件 a_cmp.c | C++ runtime locale | 0/1 |
| 0x00497A60 | CRT:strncnt | 32 条指令，位于 a_cmp.c 模块（同模块函数引用其断言源文件） | C++ runtime locale | 0/2 |
| 0x00497AB0 | CRT:__crtsetenv | 224 条指令，引用断言源文件 setenv.c | C++ runtime environment | 0/1 |
| 0x00497D90 | CRT:findenv | 47 条指令，位于 setenv.c 模块（同模块函数引用其断言源文件） | C++ runtime environment | 0/1 |
| 0x00497E10 | CRT:copy_environ | 80 条指令，引用断言源文件 setenv.c | C++ runtime environment | 0/1 |
| 0x00497F00 | CRT:_mbschr | 69 条指令，位于 mbschr.c 模块（同模块函数引用其断言源文件） | C++ runtime multibyte | 0/1 |
| 0x00497FC6 | import:RtlUnwind | 导入跳转 `jmp ds:__imp_RtlUnwind`（1 条指令） | compiler structured exception support | 0/2 |

### src/crt 的包装（用户要求 2026-10-18：直接使用当前版本的 CRT，最多包装一层）

| 地址 | 原版 | 重建 | 与原版的差异 |
|---|---|---|---|
| 0x00489654 _ftol | `fstcw`、`or ah, 0Ch`（向零舍入）、`fistp` 64 位；越界与 NaN 得整数不定值 | `std::llrint(std::trunc(value))` | 范围内相同；越界与 NaN 由当前 CRT 决定（x86 上同为不定值） |
| 0x00489B10 srand、0x00489B20 rand | 状态 0x004A833C（初值 1），`imul 343FDh`、`add 269EC3h`、取 16..30 位 | `std::srand`、`std::rand` 取低 15 位 | Windows 的 UCRT 用同一 LCG 且 RAND_MAX 为 0x7FFF（每线程一份状态，游戏单线程）；Linux 的 glibc 序列不同 |
| 0x00489B50 difftime | 32 位 `sub` 后 `fild` | `std::difftime`（time_t） | 仅 32 位相减溢出（相隔 68 年以上）时不同 |
| 0x00489B70 time | GetLocalTime 经 __loctotime_t 换回 UTC 秒数 | `std::time` | 正常时区下相同 |
| 0x0048A1F0 floor 及 0x0048F9A0..0x00490750、0x00494800 数学错误辅助 | `frndint` 向下舍入；无穷大与 NaN 经错误处理链原样返回（只设 errno） | `std::floor` | 相同 |
| 0x00489D90 vsprintf | 唯一游戏调用者 0x004302F0（FieldView 调试文字 "MAct[%4.1f/%4.1f] ..."）；%f 经 _cftof → _fltout2（17 位有效数字）→ _fptostr（中点进位） | `std::snprintf` 写入 256 字节缓冲区（超长截断，原版写出界） | %f 的舍入按当前 CRT（精确值舍入），精确中点（如 2.25 取一位）可能与原版不同；只影响 F1 调试显示 |

msvc_crt_test 检查 Sprintf 与当前 snprintf 的结果一致、255 字符截断与 Ftol 向零截断。差异登记于 deviations.md（0x00489D90、0x00489B20、0x00489B50）。

### EH funclet 0x00497FD0..0x00498418

每个父函数一对（或多对）：展开片段调用局部对象的析构或在 `new` 的构造抛出时 `operator delete`，处理片段 `mov eax, offset FuncInfo; jmp __CxxFrameHandler`（0x00489740）。34 个处理片段的地址都出现在父函数序言的 `push offset SEH_xxx` 中（脚本逐个核对，0 处不符）；39 个展开片段的析构目标与重建源码注释描述的对象一致：0x00402200 PathFinder 析构（RolePath::FindPath/Follow/ResumeMove/MoveRoleTo 的局部寻路器）、0x00438030 cFile 析构（存档、名声文件、地图缓存、截图与 SoundEffects 基类的文件对象，由 File 析构关闭）、0x00489D00 operator delete（MapCache、Talk::RunEvent、菜单系统、两个战斗动作帧的波纹、PlayVideo、MidiPlayer::Play 的节点，以及 Config 四个 Env.dat 函数的 `new cFile`，后者本次补上源码注释）、0x00478300 BattleUnit 析构（~BattleRole/~BattleEnemy 的基类部分）。重建以 RAII 成员与局部对象承担，异常时同样释放（exact 轮）。
