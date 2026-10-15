# 输入基线

记录日期：2026-10-16（Skill 初步分析阶段）

## 用户输入

| 项 | 值 |
| --- | --- |
| target | `/mnt/e/Game/swd3`（Windows `E:\Game\swd3`） |
| main | `swd32.exe` |
| idat | `D:\Dev\Crack\IDA\idat.exe`（IDA Professional 9.3，Hex-Rays v9.3.0.260213） |
| lang | C++20 |

## 主程序身份

| 项 | 值 |
| --- | --- |
| 原始路径 | `/mnt/e/Game/swd3/swd32.exe`（只读使用，未修改） |
| 工作副本 | `reverse/analysis/work/swd32.exe`（`cp -p` 复制，哈希与原文件一致） |
| 大小 | 1122304 字节 |
| SHA-256 | `4c4c226876fd2f3169bfe62c58ede86bba59e0036b7cef4cfaf7d49475c03f2a` |
| MD5 | `6f8a73425107e6e9d1f80f5ada9c59c5` |
| 格式 | PE32 GUI，Intel 80386，4 个节 |
| ImageBase | `0x00400000` |
| 入口 | `0x0048A740`（`start`，MSVC CRT 启动代码） |
| 补丁字节 | 0（数据库 `visit_patched_bytes` 结果） |
| 编译器 | MSVC（IDA compiler id 1），静态链接 MSVC 调试版 CRT（dbgheap.c 等断言字符串） |
| 原工程路径痕迹 | `C:\Project\swd102aDVD\manrole.cpp`、`C:\Project\swd102aDVD\org.cpp`；版本字符串 `v1.02DVD` |

## 节

| 节 | 起止 | 权限 |
| --- | --- | --- |
| .text | `0x00401000`–`0x00499000` | R-X |
| .idata | `0x00499000`–`0x00499344` | R |
| .rdata | `0x00499344`–`0x0049E000` | R |
| .data | `0x0049E000`–`0x0053F000` | RW |

PE 资源未加载（`PE_LOAD_RESOURCES = NO`）；资源内容核对属于后续 GOAL 的数据与资源核对范围。

## 导入模块

| 模块 | 函数数 | 用途 |
| --- | --- | --- |
| KERNEL32 | 83 | 文件、内存映射、堆、CRT |
| USER32 | 34 | 窗口、消息、对话框 |
| GDI32 | 10 | 字体与文字输出、像素读写 |
| ADVAPI32 | 3 | 注册表（shell open command） |
| ole32 | 2 | CoInitialize／CoUninitialize |
| DDRAW | 1 | DirectDrawCreate |
| DINPUT | 1 | DirectInputCreateA |
| IMM32 | 1 | ImmIsIME |
| WINMM | 2 | PlaySoundA、timeGetTime |
| binkw32 | 14 | Bink 视频播放 |
| mss32 | 47 | Miles Sound System（MIDI 序列、采样、流） |

完整导入表见 `ida/meta_reopen.json` 的 `imports`。

## 加载关系

同目录内 `binkw32.dll`、`Mss32.dll`、`Mp3dec.asi` 为运行时依赖；游戏数据文件（`*.DAT`、`all_*.tsw`、`all_*.act`、`all.snd`、`huge.lmf`、`battle.ffd`、`Music/`、`Video/`、`Data/`、`Save/`）由主程序按相对路径读取。

## IDAT 数据库设置

- 数据库：`reverse/analysis/swd32.i64`（唯一正式工作库）。
- 建库命令带 `-dANALYSIS=0xDF7FFFF7`：`cfg/ida.cfg` 的 `__PC__` 段默认 `ANALYSIS = 0xDFFFFFF7`，清除 `AF_FLIRT`（`0x00800000`）后得到该值。
- `scripts/analyze.py analyze` 在 `auto_wait()` 之前再次执行 `inf_set_af(inf_get_af() & ~AF_FLIRT)`，日志 `ida/analyze_analyze.log` 记录 `AF before=0xDFFFFFF7 after=0xDF7FFFF7`。
- IDA 日志中仍出现 `Using FLIRT signature: Microsoft VisualC 2-14/net runtime`；以数据库为准：重开后 `af=0xDF7FFFF7`（AF_FLIRT 关），`unknown_libname_*` 为 0，`FUNC_LIB` 仅 `start` 1 个（来源为 IDA 入口识别，非签名命名），CRT 函数均保持 `sub_*` 名称。
- 重开核对（`scripts/analyze.py reopen`，`ida/meta_reopen.json`）：输入 SHA-256、MD5、大小与工作副本一致，函数 1626 个，函数外代码头 0 个，入口反汇编可读（`ida/reopen_entry_disasm.txt`）。
