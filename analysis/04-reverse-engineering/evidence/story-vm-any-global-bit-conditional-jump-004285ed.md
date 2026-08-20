# 剧情 VM 全局位列表任真条件跳转：0x004285ED

状态：`platform_adapted`、`assembly_exact`（有效全局位/文件 owner 域）、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x004285ED..0x00428656`

opcode：`24`，枚举项`OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET`。

直接helper：`sub_42E430`；bit查询由handler直接读取`byte_4AB384`，不调用`sub_40DC50`。

## 1. FF00列表与任真谓词

编码与opcode23相同：

```text
+0 u16 raw opcode
+2 repeated u16 global bit ids
   u16 0xFF00 sentinel
   u32 same-file target
```

非sentinel条目数为`n`时逻辑长度是`8 + 2*n`。handler逐项查询并分别累计总数与set数；找到set bit后也不会短路，仍扫描到`0xFF00`。set数大于0才taken branch。

因此opcode24不是opcode23的简单布尔反向：

- opcode23：all set跳转，empty list为true；
- opcode24：any set跳转，empty list为false；
- 混合列表（部分set、部分clear）对两者都可能跳转。

现代实现让23/24共用一个列表扫描器，同时维护`all_bits_set`和`any_bit_set`，再按effective opcode选择谓词；没有复制第二套近似循环。公共fetch的`raw & 0x3FFF`保留四个raw alias。

## 2. taken/sequential与危险点

任一bit set时，只有扫描完`0xFF00`后才读取u32 target，随后复用已审计`load_same_file_story_window`：audio一次、发布target/zero IP、不清窗口重载、同调用继续，并在common join发布previous24。typed I/O失败保留这些先行副作用。

全clear或empty list时不读取target，IP推进`8+2*n`、发布previous24并同调用继续。

危险点顺序与LST一致：

- 缺少列表项或`0xFF00`：在下一u16读取处`operand_out_of_range`；
- any-set路径缺target：扫描完成后在u32 target读取处`operand_out_of_range`；
- all-clear路径target区域不可读：仍先推进，下一fetch才`instruction_out_of_range`。

原机器列表扫描无边界检查；现代checked-stop只替代实际危险读，不做完整指令预检。

## 3. 测试与资产缺席

synthetic测试独立覆盖：四raw alias；首项clear/后项set的any-set跳转；全clear顺序路径；empty-list false；taken load failure；any-set target截断；all-clear target不可读仍推进。opcode23既有矩阵继续覆盖共享扫描器的缺失FF00与all/empty另一组谓词。

完整`story-vm-talk-linear-records.tsv`共58,782条物理记录，opcode24命中0条；workpack为`asset_observed_rows=0/1`。因此本组不声称`real_asset_tested`，而是记录`asset_absence_verified`。real-suite与initial-session-real-suite仍与synthetic一起3/3通过，证明当前真实资产路径无回归，但不能替代不存在的opcode24实物样本。

定向三项CTest为3/3；完整Linux core 186/186、Linux app 192/192均以exit 0通过。生成器Python `py_compile`通过且两次重跑幂等；两套真实CMake均已编译opcode24与共享扫描器改动。按执行计划v261，小handler不运行Windows；Windows LLVM app留到剧情VM P3大阶段统一编译、集中修复。未启动任何游戏EXE。

关闭后workpack为19/146。下一行严格是：

```text
0x0042865B
opcode 25
```

opcode25尚未独立审计；现有C++与导航语义不得继承完成状态。
