# 剧情 VM 速度模式清除 `0x0042BC3D`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042BC3D..0x0042BC4B`

opcode：122

## 1. 机器合同

handler无operand、helper、条件分支或外部调用：

```text
dword_4CAEB8 = 0
jmp 0x00427E84
```

共享尾把物理脚本指针与u16 IP各加2、设置ESI=1并进入common join。common join发布normalized previous122，同一次解释器调用继续取后继。handler不service audio，不yield。

LST对`dword_4CAEB8`的交叉引用证明它是进程级速度模式owner：`sub_402F80`的按键C路径以二项序列切换它，普通世界移动据其非零状态选择速度覆盖；共享dialog runtime还据其非零状态改写文字倒计时、选择状态和文字游标推进。opcode122只执行无条件完整dword清零，不读取旧值。

现代SDL已用`LegacyWorldPlayerControlState::speed_mode`承载同一世界移动owner。VM runtime借用该u32并直接写零，因此opcode执行后下一世界移动立即恢复正常速度，不复制第二份状态。原版固定全局必然存在；现代仅在该裸写点把缺失binding收敛为`runtime_unavailable`，不推进IP、不发布previous。该typed owner边界是唯一平台适配。

完整两字节记录位于窗口`0x7FFE`时，先清owner、推进IP到`0x8000`并发布previous，再由下一fetch返回窗口越界。

## 2. 资产锁与验证

线性TALK目录锁定7条物理记录/7 probes，全部raw `0x007A`、长度2：

```text
file       records  probes
TALK1.DAT        2       2
TALK2.DAT        1       1
TALK3.DAT        3       3
TALK4.DAT        1       1
```

当前线性记录没有高位raw alias。四库另有7处`0xC07A`字节字样，均不是已证明的线性指令入口。

真实回放分别使用四个文件首条线性记录：

```text
TALK1.DAT@0x00041D98
TALK2.DAT@0x000190F9
TALK3.DAT@0x000100A6
TALK4.DAT@0x00022045
```

四条均在精确窗口尾验证owner清零、IP、previous与下一fetch。synthetic覆盖四raw alias、任意非零完整dword、目标已零、same-call后继、无audio、缺失binding和精确尾。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186、app 192/192完整门均通过。未启动原版或OpenSWD3游戏EXE。
