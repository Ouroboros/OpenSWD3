# 剧情 VM 全局位条件跳转共享 handler：0x00428533

状态：`platform_adapted`、`assembly_exact`（有效全局位/文件 owner 域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x00428533..0x0042857A`

共享 opcode：

- `21`：`OP_21_JUMP_IF_GLOBAL_BIT_SET`；
- `22`：`OP_22_JUMP_IF_GLOBAL_BIT_CLEAR`。

直接 helper：`sub_40DC50`、`sub_42E430`。

## 1. 共享8-byte编码与精确XOR谓词

```text
+0 u16 raw opcode
+2 u16 global bit index
+4 u32 same-file target
```

handler先调用`sub_40DC50(bit_index)`，再构造`(opcode == 22)`并与helper返回值做XOR；仅XOR结果等于1时跳转。因此：

- opcode21：bit set跳转，bit clear顺序执行；
- opcode22：bit clear跳转，bit set顺序执行。

两者不是两个近似分支，而是同一共享入口的严格反谓词。公共fetch的`raw & 0x3FFF`使每个opcode的四个raw alias进入同一逻辑。

本轮按用户要求将已审计剧情VM opcode从一串命名`inline constexpr`集中为固定底层类型：

```cpp
enum LegacyWorldStoryOpcode : compat::u16
```

这里保留非scoped枚举是有意设计：脚本窗口、结果、持久状态与未知opcode仍以raw `u16`表示，且审计需要自然执行`0x4000/0x8000`掩码与alias运算；若改用`enum class`，每个机器边界都会堆叠无信息量的显式转换。枚举只集中已审计/中性哨兵名称，不限制未知16-bit值。

## 2. taken branch

只有谓词成立后才读取`+4 u32 target`。机器路径把target压栈后进入公共`loc_42CCD6`/`sub_42E430`：

1. 服务一次audio；
2. 发布同文件target并把context IP置0；
3. 以`clear_before_read=false`重载当前TALK窗口；
4. 从新窗口在同一次VM调用继续取指；
5. common join发布previous effective opcode。

现代端口复用opcode15已独立审计的`load_same_file_story_window`，没有复制另一套近似loader。typed I/O失败只在原helper调用点checked-stop，并保留此前audio、target、zero-IP及previous-opcode效果；不回滚或伪造成功。

## 3. sequential path与危险点顺序

谓词不成立时，handler不读取target，直接把16-bit context IP加8，在common join发布previous opcode，并在同一次调用继续取指。

因此窗口尾行为必须分层：

- 连`+2 bit index`都不可读：`operand_out_of_range`，IP/previous不变；
- bit可读、taken branch但target截断：在target危险点`operand_out_of_range`，IP/previous不变；
- bit可读、no-branch而target区域已越界：仍先推进8并发布previous，下一fetch才`instruction_out_of_range`。

旧C++对21/22各复制一套实现、统一预检8 bytes、直接调用`load_data_window`，遗漏audio服务、loader失败前的offset顺序、window-loaded失败状态与previous publication。本轮合并为单一shared case并恢复上述顺序。

## 4. 测试与真实资产

synthetic测试覆盖：21/22各四raw alias；set/clear正反谓词；branch同文件load与same-call继续；no-branch IP+8与same-call继续；typed load failure副作用；bit operand截断；branch target截断；no-branch窗口尾不读取target。

全资产静态反查：

- opcode21：1,066条，TALK1/2/3/4分别498/267/183/118；全部raw `0x0015`、长度8、单一entry hit；534个不同bit（21..7090），972个不同target（`0x00000B9B..0x0005A7CA`）；
- opcode22：248条，TALK1/2/3/4分别130/15/16/87；全部raw `0x0016`、长度8、单一entry hit；149个不同bit（17..7088），207个不同target（`0x0000153D..0x00057C4E`）；
- 合计1,314条；全部target在对应TALK文件可装载域内，长度/target验证零差异。

真实回放：

```text
TALK1.DAT@0x000014CE
15 00 BD 01 F0 12 00 00
opcode21, bit 0x01BD, target 0x000012F0
```

测试置bit后跳转；target窗口先执行内部raw `0x0402`前缀，再到opcode76，因测试未提供其runtime而在原访问点`runtime_unavailable`停止。首跳load、audio、target、previous和三次同调用取指均被断言。

```text
TALK1.DAT@0x00002560
16 00 64 00 86 23 00 00
opcode22, bit 0x0064, target 0x00002386
```

测试保持bit clear后跳转；target窗口的内部前缀链最终以`0x3FFF`终止，并保留opcode22 previous。首跳与三次同调用取指均被断言。

定向synthetic、real-suite、initial-session-real-suite CTest为3/3；完整Linux core 186/186、Linux app 192/192均以exit 0通过。生成器Python `py_compile`通过且两次重跑幂等；两套真实CMake均已编译enum与handler改动。按执行计划v259，小handler不运行Windows；Windows LLVM app留到剧情VM P3大阶段统一编译、集中修复。未启动任何游戏EXE。

关闭后workpack为17/146。下一行严格是：

```text
0x0042857F
opcode 23
```

opcode23尚未独立审计；当前中性枚举项与导航语义不得继承完成状态。
