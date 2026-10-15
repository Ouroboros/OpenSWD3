# 扫描范围与缺口记录

记录日期：2026-10-16（Skill 初步分析阶段）。数据来自最终重开导出 `ida/meta_reopen.json` 及同目录 TSV。

## 扫描范围

- 可执行段：`.text` `0x00401000`–`0x00499000`（唯一可执行段）。
- 数据段指针扫描：`.text` 数据区、`.rdata`、`.data` 中每个 4 字节对齐 dword 指向 `.text` 的值（`ida/text_pointer_scan.tsv`）。
- 未覆盖：PE 资源（未加载）、运行时动态加载的 DLL（binkw32、mss32、Mp3dec.asi，外部导入按调用合同对接）。

## 术语

函数外代码头：可执行段 `idautils.Heads()` 中 `is_code` 为真且 `get_func` 为 None 的地址。函数外代码区域：首尾严格相接（下一个头地址等于上一个头的 `ea + item_size`）的函数外代码头序列。

## 函数边界补全过程

| 阶段 | 函数数 | 函数外代码头／区域 | 记录 |
| --- | --- | --- | --- |
| 自动分析（无 FLIRT） | 1448 | 1715／132 | `ida/meta_analyze.json`、`ida/unowned_regions.tsv` 初版已被覆盖，汇总见本表 |
| 区域起点建函数（6 轮） | 1591 | 183／4 | `ida/boundary_pass1.tsv`、`ida/boundary_pass1.log` |
| 合并碎片、拆分尾块、修正数据 | 1631 | 0／0 | `ida/fix_boundaries.log` |
| 回挂共享尾声 | 1630 | 0／0 | `ida/fix_boundaries2.log` |
| 删除表内假函数 | 1626 | 0／0 | `ida/fix_boundaries3.log` |

建函数规则（`scripts/boundary.py`）：只在区域起点不是前一指令顺序流入（`is_flow` 为假）、且前一项为对齐／数据／`ret`／`jmp` 时 `add_func`；建成后检查结束地址不越过下一个函数、末条指令为 `ret`／`jmp`／noreturn 调用，否则撤销。

人工核对后的修正（证据见 `ida/dump1.txt`、`ida/dump2.txt`、`ida/dump3.txt`、`ida/dump4.txt`）：

- `0x417FEA`–`0x418346`：`sub_417EC0` 用 `lea` 把块地址写入局部表并 `jmp ebx` 分派，块尾跳回 `loc_417FD7`／`loc_417FD1`。撤销误建的 7 个函数，`sub_417EC0` 结束地址延到 `0x418346`。
- `0x4896AD`（`jmp eax` 后尾声）、`0x48A87A`（start 的 `__except` 处理块）、`0x48A8F8`（ExitProcess 后尾声）、`0x4311B4`（`int 3` 后尾声）并回所属函数。
- `0x49091C`：字节为 `VC20XC0\0`，改为字符串数据。
- 尾块拆分：46 个位于 16 字节对齐边界、经尾调用 `jmp` 进入（其中多个被多个父函数共享、部分被 `0x43B7xx` 函数指针表引用）的尾块改为独立函数。
- `0x490A00`：以 `pop ebx` 开头的共享尾声，回挂为 `sub_490A10` 尾块（当前唯一多块函数）。
- `0x486AC0`–`0x487AD8`：dinput.lib `c_dfDIKeyboard` 数据，删除 IDA 误建的 `nullsub_2`–`nullsub_5`，改为 dword 表。

## 当前状态

- 函数 1626 个；函数外代码头 0 个；多块函数 1 个（`sub_490A10`）；thunk 11 个。
- `.text` 非函数间隙 1488 段：对齐填充 1404 段；其余 84 段全部为已定义数据（switch 跳转表／索引表、`c_dfDIKeyboard`），不含代码头和未定义字节（`ida/nonfunc_gaps.tsv`）。
- 函数外的代码被数据引用：0（`ida/code_drefs.tsv`，函数内部目标均由本函数内指令或本函数后的跳转表引用，未发现跨函数引用中段）。
- 指向 `.text` 的数据值（`ida/text_pointer_scan.tsv`）：函数起点 139；函数内部目标为 switch 表与 SEH scope table（filter／handler）项；`mid_instruction`／`not_code` 值经抽查来自字符串、MSVC 对齐填充 `8D 49 00` 及普通数值，暂判为巧合值，GOAL 中对相关数据结构还原时复核。
- SP 审计（`ida/sp_audit.tsv`）：17 个函数 22 处 `ret` 的 SP 差值非 0，全部位于 CRT（SEH 过滤表达式 `retn`、`__alloca_probe`、`jmp eax` 后的死尾声），游戏代码区无此类异常。

## 未闭合缺口

| 缺口 | 数量 | 说明与后续处理 |
| --- | --- | --- |
| 未解析间接 call | 170（73 个函数） | `call [reg+off]` 为主（DirectDraw／DirectInput COM 方法、C++ 虚函数），另有 `call esi`／`call ebp` 缓存的 API 指针。GOAL 中按对象类型还原 vtable 并确认目标均已在函数表 |
| 未解析间接 jmp | 33 | `sub_417950`、`sub_419570`、`sub_41A3B0`、`sub_41B9F0`、`sub_41D340`、`sub_41E5C0` 的 `jmp ebx` 块分派（同 `sub_417EC0` 模式，目标块已在本函数内）；CRT memcpy／`_JumpToContinuation` 等 |
| 函数指针表 | `0x43B4xx`–`0x43B9xx` 一组 | 菜单／状态回调表，目标已全部建为函数；表的结构与含义待 menu 模块还原时确认 |
| 函数全集 | 待确定 | 上述缺口全部核对完成前，1626 只是已识别函数数，函数总数仍记为待确定 |
