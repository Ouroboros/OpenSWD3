# 剧情 VM 文字控制 bit25 清除 `0x0042BCF5`

状态：`assembly_exact`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042BCF5..0x0042BD05`

opcode：124

## 1. 机器合同

handler没有operand、helper、条件分支、audio或yield。机器以完整32位宽度执行：

```text
value = dword_4A1360
value &= 0xFDFFFFFF
text_control_flags = value
IP += 2
previous = 124
same-call continue
```

该掩码只清bit25，其他31位保持。目标位已清时行为幂等。

入口跳到共享`0x00427E7E`，先把计算结果写回完整dword，再在`0x00427E84`把u16 IP和物理脚本指针各加2、把ESI置1，最后进入`0x0042B0AE`发布normalized previous并同调用继续取指。没有平台所有权替换或现代兼容差异，归类`assembly_exact`。

完整两字节记录位于窗口`0x7FFE`时，清位、IP `0x8000`和previous124均先提交；下一fetch才返回instruction out of range。

## 2. 资产锁

完整线性TALK目录58,782条记录中，opcode124为0条物理记录、0个entry probe，因此使用`asset_absence_verified`，不伪造真实回放。

全文件非入口字样统计：

```text
raw word  TALK1  TALK2  TALK3  TALK4  total
0x007C       43     51     19     14    127
0x407C        0      0      0      0      0
0x807C        0      0      0      0      0
0xC07C        5      6      1      1     13
```

这些字节序列均不在线性指令入口，不提升为资产记录。

## 3. 验证

synthetic覆盖四raw alias、目标位初始置位、目标位已清、其他位保持、same-call successor、无audio和`0x7FFE`精确尾。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186、app 192/192通过。未启动原版或OpenSWD3游戏EXE。
