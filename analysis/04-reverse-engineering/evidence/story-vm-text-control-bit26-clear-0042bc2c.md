# 剧情 VM 文字控制 bit26 清除 `0x0042BC2C`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042BC2C..0x0042BC3C`

opcode：121

## 1. 机器合同

handler无operand、helper、条件分支或外部调用：

```text
EDX = dword_4A1360
EDX &= 0xFBFFFFFF
jmp 0x00427E7E
```

共享尾先把完整EDX写回`dword_4A1360`，再把物理脚本指针与u16 IP各加2、设置ESI=1并进入common join。common join发布normalized previous121，同一次解释器调用继续取后继。

掩码只清bit26；其他31位完整保留。目标位已清时结果幂等。handler不读取opcode后的任何字节，不service audio，不yield。

现代实现直接映射`LegacyWorldStoryVmState::text_control_flags`完整u32 owner。该owner同时由共享dialog生产读取：bit26清零时新消息flags置bit1。无裸指针、宿主API或平台替代，分类为`assembly_exact`。

完整两字节记录位于窗口`0x7FFE`时，先清bit、推进IP到`0x8000`并发布previous，再由下一fetch返回窗口越界。

## 2. 资产锁与验证

线性TALK目录锁定815条物理记录/815 probes，全部raw `0x0079`、长度2：

```text
file       records  probes
TALK1.DAT        6       6
TALK2.DAT      139     139
TALK3.DAT        2       2
TALK4.DAT      668     668
```

当前线性记录没有高位raw alias。全文件仅有一处`0xC079`字节字样，位于TALK1非指令入口，不能计作资产记录。

真实回放分别使用四个文件首条线性记录：

```text
TALK1.DAT@0x0000965C
TALK2.DAT@0x0000F18D
TALK3.DAT@0x00023123
TALK4.DAT@0x0000135C
```

四条均在精确窗口尾验证bit清除、IP、previous与下一fetch。synthetic覆盖四raw alias、目标位已清、其他位保留、same-call后继、无audio及精确尾。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186、app 192/192完整门均通过。未启动原版或OpenSWD3游戏EXE。
