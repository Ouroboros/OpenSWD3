# 剧情 VM 文本控制 bit27 清除 `0x0042B4B9`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B4B9..0x0042B4C5`，共享尾`0x00427E7E..0x00427E95`。

opcode：`105`

## 行为合同

机器读取完整text control dword并执行：

```text
text_control_flags &= 0xF7FFFFFF
IP += 2
ESI = 1
common join publishes previous105
```

没有operand、helper、条件分支或额外状态访问。只清bit27，其余31位原样保留；重复执行幂等。modern直接使用已集成`LegacyWorldStoryVmState::text_control_flags` u32 owner，无需平台适配。

完整两字节记录起于窗口`0x7FFE`合法：bit清除、IP=`0x8000`和previous105先提交，下一same-call fetch再返回`instruction_out_of_range`。

## 资产锁与验证

线性TALK目录锁定806条物理记录/806 probes，全部raw `0x0069`、长度2，分布：

```text
TALK1/2/3/4 = 308/173/145/180
```

真实回放代表：

```text
TALK1.DAT@0x0000256C
TALK2.DAT@0x00001703
TALK3.DAT@0x00009DDD
TALK4.DAT@0x0000169B
```

synthetic覆盖四个raw alias、其他位保留、same-call successor和精确尾。Story VM synthetic、real及initial-session三项通过。未启动原版或OpenSWD3游戏EXE。

分类：`assembly_exact`。u32 AND、位宽、推进、previous和same-call均由现有typed owner逐项直接复现。
