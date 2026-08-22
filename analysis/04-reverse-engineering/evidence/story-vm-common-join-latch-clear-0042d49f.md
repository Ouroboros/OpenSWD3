# 剧情 VM common join latch清除与让出 `0x0042D49F`

状态：`assembly_exact`、`unit_tested`、`asset_absence_verified`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042D49F..0x0042D4B5`

opcode：1025

## 1. LST合同

表外special分派先把1025归一化到`0x0042D49F`。handler没有operand或外部helper，固定顺序为：

```text
0x0042D49F  logical u16 instruction offset += 2
0x0042D4A4  physical instruction pointer += 2
0x0042D4A7  ESI = 0
0x0042D4A9  保存physical instruction pointer
0x0042D4AD  call-local var_28 = 0
0x0042D4B1  跳0x0042B0AE common join
```

common join先把归一化opcode1025发布到previous。此时`var_28|ESI`固定为0，故调用`0x0042D4D7 _AIL_serve`一次并从解释器返回一。后继指令不在本次调用中fetch。

这与opcode1024不同：1024把`var_28`置1并使后续common join持续same-call；1025显式清除该调用期latch并恢复共同出口audio-yield。现代只清除`step_legacy_world_story_vm`栈内bool，不增加VM持久字段。

## 2. 边界与测试

synthetic覆盖：

- `0x0401/0x4401/0x8401/0xC401`四个raw alias；
- 无latch入口固定双指针+2、previous1025、一次audio、yield；
- audio callback观察到IP和previous均已提交；
- 1024→opcode59→1025证明1025清除持久latch，只执行前一个sound副作用，在1025共同出口audio-yield且不fetch后继sound；
- `IP=0x7FFE`精确尾提交`0x8000`、previous和audio后直接yield，不因后继越界而改变状态。

旧测试曾借尚未实现的1025作为same-call停机哨兵。闭环前已迁移为明确的测试专用typed-stop：普通链使用opcode29负索引，必须保留“下一指令+2”读取的文件操作链使用无previous副作用的专用typed-stop。生产代码不含测试分支，既有previous/audio/IP断言保持通过。

完整线性TALK目录对opcode1025为0条记录/0 probes，使用`asset_absence_verified`。未以全文件raw字样替代线性记录。

Story VM synthetic/real/initial-session 3/3和SDL app编译通过。workpack/runtime-path双生成稳定hash分别为`85bd6d1f7549bb7877d1e4a2ba53db98e60dccfd314f6fce2d90604faba10df6`与`d4ef8d464ed37b2321e6ad5a9705cd5bce10ea0b06281f247b3fd705a74e1285`。Linux完整门通过：core 186/186、app 192/192。未启动原版或OpenSWD3游戏EXE。

## 3. 双向追溯

LST→C++：双指针推进映射为逻辑IP+2；`xor esi`与`var_28=0`映射为清除call-local latch；共同join继续复用previous→audio→yield窄helper。

C++→LST：没有读取operand、没有持久化latch、没有same-call后继fetch、没有把audio移到previous之前，也没有把精确尾误改为后继fetch失败。
