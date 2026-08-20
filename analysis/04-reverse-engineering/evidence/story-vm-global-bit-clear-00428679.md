# 剧情 VM 全局位清除：0x00428679

状态：`platform_adapted`、`assembly_exact`（有效全局位 owner 域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x00428679..0x004286C0`

opcode：`26`，枚举项`OP_26_CLEAR_GLOBAL_BIT`。

直接helper：`sub_40DCB0`、`wsprintfA`、`nullsub_1`。

## 1. 4-byte清位协议

```text
+0 u16 raw opcode
+2 u16 global bit id
```

`sub_40DCB0`以`bit >> 3`选择`byte_4AB384`字节，以低3位选择`byte_4994EC`mask；它从`0xFF`减去该单bit mask得到补码，再AND回原字节。因此只清目标bit并保留同字节其他7位。目标bit已clear时操作幂等。公共fetch的`raw & 0x3FFF`使四个raw alias进入同一handler。

helper后原程序再次读取operand，用`wsprintfA("fOFF[%d]")`写共享scratch并调用`nullsub_1`。与opcode25相同，该链没有业务消费者；现代端口省略纯诊断格式化，不伪造callback，同时保留此前清位。

随后IP推进4，common join发布previous26并同调用继续。opcode25闭环已恢复共享尾previous；本组独立验证opcode26的入口、helper与清位语义后，把中性`OP_26`升级为语义枚举项。

## 2. 边界与测试

机器helper不检查bit id；现代`clear_legacy_world_story_flag`只在typed flags owner内RMW，有效域与机器一致，越界写不移植。handler在读取4 bytes前不改状态；operand截断时`operand_out_of_range`，flags/IP/previous保持。

synthetic测试覆盖：四raw alias；同字节三个set bit中只清目标；已clear幂等；typed owner最后有效bit；窗口尾operand截断。

## 3. 全资产与真实回放

全资产静态反查：

- 312条物理指令，TALK1/2/3/4分别138/22/64/88；
- 全部raw `0x001A`、长度4；310条单entry hit、2条双entry hit；
- 145个不同bit，范围4..7073；bit10出现83次、bit213出现19次；
- 所有bit均落在typed flag owner有效域，长度验证零差异。

真实回放：

```text
TALK1.DAT@0x000265FE
1A 00 67 02 3B 00 38 00
opcode26 bit=615; next opcode59 sound=0x0038
```

测试预置bit614/615/616，回放后只清615；IP+4后同调用opcode59发出sound `0x0038`并yield，最终IP为8且previous保持26。

定向synthetic、real-suite、initial-session-real-suite CTest为3/3；完整Linux core 186/186、Linux app 192/192均以exit 0通过。生成器Python `py_compile`通过且两次重跑幂等；两套真实CMake均已编译opcode26、枚举与测试改动。按执行计划v263，小handler不运行Windows；Windows LLVM app留到剧情VM P3大阶段统一编译、集中修复。未启动任何游戏EXE。

关闭后workpack为21/146。下一行严格是：

```text
0x004286C5
opcode 27
```

opcode27尚未独立审计；现有C++与导航语义不得继承完成状态。
