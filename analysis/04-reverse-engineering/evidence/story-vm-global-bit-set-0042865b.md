# 剧情 VM 全局位置位：0x0042865B

状态：`platform_adapted`、`assembly_exact`（有效全局位 owner 域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042865B..0x004286C0`

opcode：`25`，枚举项`OP_25_SET_GLOBAL_BIT`。

直接helper：`sub_40DC80`、`wsprintfA`、`nullsub_1`。

## 1. 4-byte置位协议

```text
+0 u16 raw opcode
+2 u16 global bit id
```

`sub_40DC80`以`bit >> 3`选择`byte_4AB384`字节，以低3位选择`byte_4994EC`mask，再把mask OR回原字节。它是幂等RMW：目标bit已置时其余bit与结果均不变。公共fetch的`raw & 0x3FFF`使四个raw alias进入同一handler。

helper返回后原程序再次读取同一operand，用`wsprintfA("fON [%d]")`写入共享scratch，再把字符串交给`nullsub_1`。该调用链没有业务消费者；现代端口按既定“纯诊断省略”规则不伪造logger或callback，但保留诊断前已经发生的bit置位。省略scratch文本不替代任何业务效果。

随后IP推进4，common join发布previous25，并在同一次VM调用继续取指。旧C++已有置位与IP推进，但使用裸数字25/26且遗漏previous publication；本轮把25命名为语义枚举、26保留中性`OP_26`，恢复共享尾previous。opcode26仍须下一组独立审计，不能继承关闭状态。

## 2. 边界与typed owner

机器helper不检查bit id，直接索引全局字节数组。现代`set_legacy_world_story_flag`只在typed `flags` owner内RMW；有效域语义与机器一致，越界写不被移植。handler只预检实际要读的4 bytes：operand缺失时在bit读取危险点`operand_out_of_range`，不修改flags、IP或previous。

synthetic测试覆盖：四raw alias；普通bit置位并同调用继续；已置bit幂等；typed owner最后一个有效bit；窗口尾operand截断。既有opcode24/23查询测试也消费同一flag owner。

## 3. 全资产与真实回放

全资产静态反查：

- 635条物理指令，TALK1/2/3/4分别255/106/139/135；
- 全部raw `0x0019`、长度4；631条单entry hit、4条双entry hit；
- 488个不同bit，范围4..7084；bit10出现81次，其余高频包括32/34/7/38；
- 所有bit均落在typed全局flag owner有效范围，长度验证零差异。

真实回放：

```text
TALK1.DAT@0x000074C1
19 00 A8 1B 3B 00 AB 00
opcode25 bit=7080; next opcode59 sound=0x00AB
```

回放断言bit7080被置、IP推进到4后同调用执行opcode59、sound `0x00AB`发出并yield，最终IP为8且previous保持25。

定向synthetic、real-suite、initial-session-real-suite CTest为3/3；完整Linux core 186/186、Linux app 192/192均以exit 0通过。生成器Python `py_compile`通过且两次重跑幂等；两套真实CMake均已编译opcode25、previous修复与测试改动。按执行计划v262，小handler不运行Windows；Windows LLVM app留到剧情VM P3大阶段统一编译、集中修复。未启动任何游戏EXE。

关闭后workpack为20/146。下一行严格是：

```text
0x00428679
opcode 26
```

opcode26尚未独立审计；当前中性枚举与既有C++不得继承完成状态。
