# 剧情 VM 全局位列表全真条件跳转：0x0042857F

状态：`platform_adapted`、`assembly_exact`（有效全局位/文件 owner 域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042857F..0x004285E8`

opcode：`23`，枚举项`OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET`。

直接helper：`sub_42E430`；bit查询由handler直接读取`byte_4AB384`，不调用`sub_40DC50`。

## 1. FF00终止的变长编码

```text
+0 u16 raw opcode
+2 repeated u16 global bit ids
   u16 0xFF00 sentinel
   u32 same-file target
```

设非sentinel条目数为`n`，逻辑长度是`8 + 2*n`。公共fetch的`raw & 0x3FFF`使四个raw alias进入同一handler。

机器循环对每个bit执行索引/掩码查询，分别累计总条目数与set条目数。发现clear bit后不会短路，仍逐项读取并查询直到`0xFF00`；现代实现同样扫描完整列表。空列表在首个`+2`即遇`0xFF00`，总数与set数都为0，因此无条件进入taken branch。

## 2. taken branch

仅当`set_count == item_count`时，handler才读取sentinel后的`u32 target`，随后进入公共`loc_42CCD6`/`sub_42E430`：

1. 服务一次audio；
2. 发布同文件target并把context IP置0；
3. 以`clear_before_read=false`重载当前TALK窗口；
4. 从新窗口在同一次VM调用继续取指；
5. common join发布previous effective opcode。

现代端口复用opcode15/21/22已审计的`load_same_file_story_window`。typed I/O失败只在原helper调用点checked-stop，并保留audio、target、zero-IP、window-loaded失败状态与previous publication。

## 3. sequential path与危险点顺序

任一bit clear时，handler不读取target；在找到`0xFF00`后把context IP推进`8 + 2*n`，发布previous opcode，并在同一次调用继续取指。

危险点顺序：

- 列表或sentinel超出窗口：在下一次u16列表读取处`operand_out_of_range`，IP/previous不变；
- 空列表或all-set列表的target截断：在sentinel后u32危险点`operand_out_of_range`，IP/previous不变；
- 有clear bit且target区域已越界：target不被读取，仍先按逻辑长度推进并发布previous，下一fetch才`instruction_out_of_range`。

原机器列表扫描不做边界检查；现代checked-stop只替代实际越界读，不提前完整预检，不改变此前逐项查询顺序。资产bit范围均落在typed全局flag owner内。

## 4. opcode枚举与测试哨兵

关闭opcode23后，它不再能充当测试中的“下一条未实现opcode”。所有旧测试通过LSP统一迁移到中性枚举项`OP_1025 = 1025`；1025仍是明确未实现特殊值，不加入source case，也不计入64个现代显式opcode。该迁移只改变测试哨兵值，不改变生产分派行为。

synthetic测试覆盖：四raw alias；两bit全set跳转；任一clear顺序路径；empty-list无条件跳转；taken load failure；缺失FF00；empty-list target截断；clear-bit路径target不可读仍推进。

## 5. 真实资产与回放

全资产仅10条：

- TALK1/2/3分别2/4/4条，TALK4为0；
- 全部raw `0x0017`、单一entry hit；
- 列表长度分布：2项4条、4项5条、11项1条；合计39个bit、27个不同bit，范围215..4502；
- 10条逻辑长度均严格满足`8+2*n`，实际长度分布12 bytes四条、16 bytes五条、30 bytes一条；
- 7个不同target，范围`0x000088E1..0x000485A9`，全部落在对应TALK文件可装载域；
- 资产没有empty list，empty语义由LST与synthetic覆盖。

真实回放：

```text
TALK1.DAT@0x00008AD3
17 00 27 01 08 01 00 FF E1 88 00 00
bits = {295,264}, target = 0x000088E1
```

测试置两bit后跳转；target首条opcode59播放sound `0x0039`并yield。断言taken load、audio、target/zero-IP后推进到4、previous23、same-call opcode59以及真实sound副作用。

定向synthetic、real-suite、initial-session-real-suite CTest为3/3；完整Linux core 186/186、Linux app 192/192均以exit 0通过。生成器Python `py_compile`通过且两次重跑幂等；两套真实CMake均已编译opcode23、枚举哨兵迁移与测试改动。按执行计划v260，小handler不运行Windows；Windows LLVM app留到剧情VM P3大阶段统一编译、集中修复。未启动任何游戏EXE。

关闭后workpack为18/146。下一行严格是：

```text
0x004285ED
opcode 24
```

opcode24尚未独立审计；现有导航语义与未实现状态不得继承完成状态。
