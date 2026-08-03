# EXE 与 IDA 导出基线

## 原 EXE

- 文件：`swd3.exe`
- SHA-256：`0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c`
- 大小：1122304 字节
- 格式：PE32、Intel 80386、Windows GUI
- PE 时间戳：2000-06-13 18:37:20
- 链接器版本：6.0，对应 Visual C++ 6.0 时代
- 入口：`0x48A740`
- 映像基址：`0x00400000`
- 子系统版本：Windows 4.0
- 节区：`.text`、`.rdata`、`.data`、`.rsrc`
- 重定位表已移除
- 没有 ASLR 等现代 PE 防护标志
- 没有调试目录，符号已剥离
- 唯一导出符号为 `start`

EXE 内静态链接了大量 Visual C++ Debug CRT 代码。字符串中存在 `Microsoft Visual C++ Debug Library`、`dbgheap.c`、`dbgrpt.c` 等内容。

这解释了为什么 IDA 识别出大量 CRT 函数，也说明 1.1 MB 的 EXE 并不全是游戏业务代码。

## 外部依赖

系统依赖：

- `KERNEL32.dll`
- `USER32.dll`
- `GDI32.dll`
- `ADVAPI32.dll`
- `ole32.dll`
- `ddraw.dll`
- `DINPUT.dll`
- `IMM32.dll`
- `WINMM.dll`

第三方依赖：

- `binkw32.dll`
- `mss32.dll`

关键 API：

- DirectDraw：`DirectDrawCreate`
- DirectInput：`DirectInputCreateA`
- Bink：打开、音频后端、解码、拷贝、逐帧推进、音量和关闭
- Miles：数字音频、样本、流、MIDI 序列和后台服务
- Win32 文件映射：`CreateFileMappingA`、`MapViewOfFile`

## IDA 导出覆盖

- 总识别函数：1486
- 有独立输出的函数：1202
  - 伪码：1197
  - 汇编回退：5
- 跳过的库函数：284
- 伪码总量：约 2.41 MB、91228 行
- 5 个回退函数总量：13004 行汇编
- 完整汇编：约 28.2 MB

5 个汇编回退函数：

- `0x402F80`：反编译器返回空
- `0x427920`：23508 字节，超过导出器限制
- `0x43A610`：反编译器返回空
- `0x446700`：反编译器返回空
- `0x469D20`：16918 字节，超过导出器限制

`sub_427920` 和 `sub_469D20` 都由主循环直接调用，是重写的核心难点，不是可以忽略的边缘函数。

## 已确认的旧系统耦合

- 固定创建 640×480 窗口。
- DirectDraw 全屏/窗口 cooperative level。
- DirectDraw 显示模式和 surface Blt。
- 16 位像素格式和按 2 bytes/pixel 计算的缓冲区。
- DirectInput 5.0 初始化。
- WINMM `timeGetTime` 轮询；当前导入表没有多媒体 timer callback/period API，旧 `mmTimer` 字符串对应的 helper 只是帧间隔全局写入。
- Miles Sound System 的样本、MP3 流和 MIDI 接口。
- Bink 旧 DLL 直接向 DirectDraw surface 拷贝。
- ANSI Win32 API 与当前代码页。
- 当前工作目录、相对路径和盘符检测。
- 注册表查询默认浏览器/文本文件打开命令。

现代重写应把这些行为全部限制在平台适配层，游戏核心不再直接调用 Win32 API。

## 证据入口

- `swd3.exe_export_for_ai/imports.txt`
- `swd3.exe_export_for_ai/function_index.txt`
- `swd3.exe_export_for_ai/decompile_skipped.txt`
- `swd3.exe_export_for_ai/disassembly_fallback.txt`
- `swd3.exe_export_for_ai/decompile/409EC0.c`
- `swd3.exe_export_for_ai/decompile/40A0D0.c`
- `swd3.exe_export_for_ai/decompile/40A570.c`
- `swd3.exe_export_for_ai/decompile/424B90.c`
