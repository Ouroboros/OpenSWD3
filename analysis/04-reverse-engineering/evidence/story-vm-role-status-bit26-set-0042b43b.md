# 剧情 VM 角色状态 bit26 置位 `0x0042B43B`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`external_dependency_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B43B..0x0042B479`，共享推进尾`0x0042D182..0x0042D193`。

opcode：`101`

## 1. selector与lookup

机器只读取一个operand：

```text
+2 u16 role selector
```

selector缺失时，modern在原读取点返回typed operand失败，不执行lookup或角色写入。完整selector为字面`FFF0`时，handler把它替换成当前`LegacyWorldTalkContext::source_guid`；`FFFE`不在handler内替换，由共享lookup helper解析成受控角色。普通GUID lookup跳过flags bit28置位的记录并采用首个合法匹配。

## 2. 状态写入与missing路径

lookup命中时，机器对角色记录`+0x10`执行：

```text
role.flags |= 0x04000000
```

这只置bit26，其他31位全部保留；重复执行幂等。`LegacyWorldRoleRecord::flags`已由布局断言锁定在`+0x10`，因此modern可直接执行同一u32 OR，无需平台转换或新端口。

lookup失败时机器静默跳过OR；不诊断、不提交MAPS role-source patch，也不伪造live角色。成功与失败随后都进入共享尾：物理脚本指针和u16 IP各`+4`，`ESI=1`，common join发布previous101并在同一VM调用继续。

## 3. 边界、资产锁与验证

synthetic覆盖四个raw alias、普通GUID、`FFF0` current source、helper-native `FFFE` controlled role、bit28 skip与首合法匹配、missing静默、selector截断、其他位保留和精确窗口尾。完整记录起于`0x7FFC`时，bit26、IP=`0x8000`及previous101先提交，下一same-call fetch再返回`instruction_out_of_range`。

线性TALK目录锁定126条物理记录/126 probes，全部raw `0x0065`、长度4，分布：

```text
TALK1/2/3/4 = 39/38/11/38
```

资产含40种selector，范围0..1061；当前线性记录没有`FFF0`或`FFFE`，两者由synthetic独立锁定。真实回放代表：

```text
TALK1.DAT@0x0001BF4B  selector 0x0001
TALK2.DAT@0x00007474  selector 0x00BD
TALK3.DAT@0x00002662  selector 0x0001
TALK4.DAT@0x000059CC  selector 0x0001
```

四条真实记录均命中live角色并在精确尾完成bit26写入。Story VM synthetic、real及initial-session三项通过；共享role lookup依赖回归通过。未启动原版或OpenSWD3游戏EXE。

分类：`assembly_exact`。operand读取、FFF0/FFFE语义、GUID lookup、u32 OR、missing静默、推进、previous与same-call均可由现有typed owner逐项直接复现。
