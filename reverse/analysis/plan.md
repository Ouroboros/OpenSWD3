# 还原技术方案与详细计划

## 代码组织

| 位置 | 内容 |
| --- | --- |
| `include/swd3/w32.h` | 原程序使用的 Win32／DirectX／MSS／Bink 类型、常量与 `w32::` 函数声明 |
| `src/platform/` | `w32::` 的实现：`*_win.cpp` 仅 Windows 编译，`*_posix.cpp` 仅 Linux 编译，其余两平台共用（SDL3／FFmpeg） |
| `src/crt/` | MSVC CRT 的薄包装（`crt::` 命名空间）：调用平台 CRT，只保留游戏可观察的原行为（rand LCG、0xCD／0xDD 填充、_ftol、difftime） |
| `src/core/` | 跨模块的小函数（如空调试日志 0x411F90） |
| `src/<模块>/` | 游戏代码按模块组织，模块名与 `reverse/functions.tsv` 的 `module` 一致 |
| `tests/` | Linux 单元测试（CTest） |

## 约定

1. 原函数一一对应 C++ 函数或成员函数；名称使用本项目风格（不照搬原名）；每个函数定义前一行写 `// swd32.exe 0x<地址>`，`functions.tsv` 的 `source` 记 `路径:符号`。
2. `__thiscall` 函数还原为成员函数；类布局按原偏移排列，32 位目标上用 `static_assert` 校验 `sizeof` 与关键偏移。
3. 导入 API 一律写 `w32::名称(...)`，参数、返回值与调用顺序照原汇编。文件、注册表、消息框、对话框等在 Windows 上转发真实 API；窗口与消息、DirectDraw、DirectInput、GDI 文字、MSS、Bink 在两平台上由 SDL3／FFmpeg 按原接口合同模拟（Windows 上 GDI 文字绘制可转发真实 GDI）。
4. 全局数据映像：`.idata`／`.rdata`／`.data`／`.bss`（VA 0x499000–0x53F000）整体复刻为 `image::g_bytes`（`src/data/image.*`），原始字节由 `xxd` 从 `work/swd32.exe` 生成 `src/data/image_bytes.inc`，初值与布局天然与原程序一致，越界下标访问落到与原程序相同的邻居。全局变量是 `struct Globals`（`src/data/globals.h`，生成文件）的字段，代码中写 `G.<名称>`。字段来源：`data_auto.tsv`（`gen_data_auto.js`：IDA 数据项与代码引用地址，默认 IDA 名）加 `data_map.tsv`（手工覆盖：改名、结构体、指针表），由 `gen_globals.js` 合并生成。声明为数据指针（`T*`）或代码指针（`code*`）的字段在启动时重定位（原 VA → 本程序地址）。带构造函数的全局对象（MemoryManager、File 等）也放在映像原地址，由 static_init 按原 `__xc` 表（0x49E004 起 27 项）顺序构造。
5. 编译器生成的代码（全局对象构造／析构注册 thunk、EH unwind funclet）映射到对应的 C++ 全局对象定义与 RAII 析构，`source` 记到对应定义位置。
6. 调试日志函数 `sub_411F90`（单条 `retn`）保留为空函数。
7. Linux 构建为 i386 静态程序（`/opt/sysroot-i386`，`-D_FILE_OFFSET_BITS=64`），与 Windows x86 数据布局一致，用于编译、单元测试与代码验证；Linux 版 SDL3 只有 dummy／offscreen 视频与 dummy 音频。
8. 原数据读取：`.text`、`.rdata`、`.data` 的文件偏移等于 VA − 0x400000；`.data` 原始数据到 0x4AA000 为止，其后至 0x53F000 为零初始化区。

## 还原顺序

自底向上：`file` → `memmgr` → `crt`（按游戏调用逐个映射到平台 CRT）→ `res_pack` → 平台层（ddraw、input、audio、movie、窗口消息）→ 地图、角色、对话、菜单、战斗等业务模块 → `winmain` 与启动代码 → 其余 CRT 与 unknown 模块。每完成一个模块：Linux 构建＋单元测试、Windows 构建、更新函数表与 GOAL、提交。

## 资料

- 每函数汇编（含机器码与 SP）：`reverse/analysis/asm/<地址>.asm`；反编译辅助：`reverse/analysis/decomp/<地址>.c`。由 `scripts/export_funcs.py` 从工作数据库生成，不入库。
