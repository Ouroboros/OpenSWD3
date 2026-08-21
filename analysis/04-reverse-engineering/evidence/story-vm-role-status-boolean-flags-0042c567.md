# 剧情 VM 共享角色状态布尔位 `0x0042C567`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`asset_absence_verified`、`external_dependency_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C567..0x0042C6D8`，共享`+6`尾`0x00429F61..0x00429F76`。

opcodes：`102/103/117/136/140/145/146/174`。

## 1. 内部跳表与operand阶段

入口先以normalized opcode进入73项内部byte/jump table，只选择一个u32 mask：

```text
102 -> 0x0040 (bit6)
103 -> 0x0020 (bit5)
117 -> 0x0010 (bit4)
136 -> 0x1000 (bit12)
140 -> 0x0800 (bit11)
145 -> 0x2000 (bit13)
146 -> 0x0100 (bit8)
174 -> 0x4000 (bit14)
```

所有变体共享物理格式：

```text
+2 u16 role selector
+4 u16 boolean (zero=false, any nonzero=true)
```

机器先读取selector并完成替换/lookup，随后才读取boolean。modern同样先验证`opcode+selector`四字节、执行lookup，再在原`+4`访问点验证完整六字节；缺boolean时不修改live role、surface、MAPS、IP或previous。

此handler的`FFF0`规则不是current source：机器把它替换成受控角色 **index dword**，再把其低16位当GUID key交给lookup helper。它既不直接选择受控角色，也不读取context source GUID。`FFFE`不经该替换，继续由lookup helper直接选择受控角色。普通GUID仍跳过flags bit28并采用首个合法匹配。

## 2. live角色与surface刷新

lookup命中后，机器精确执行：

```text
role.flags &= ~mask
if (+4 != 0): role.flags |= mask
sub_40AE20(role)  // 清旧surface footprint
sub_40AEC0(role)  // 按新flags重建surface footprint
```

boolean不限制为0/1。clear无条件先于mark；即便所选mask不改变footprint派生位，两次helper也不能省略。mask`0x0010`影响overlay footprint，mask`0x4000`影响blocking footprint，因此测试同时锁定surface word。

modern复用已锁定的`clear_legacy_world_role_surface_occupancy`与`mark_legacy_world_role_surface_occupancy`。Win32裸surface全局映射到typed world-session owner；owner缺失或footprint越界在原helper访问点typed-stop。角色flags已在该点前提交；clear发生partial failure时此前已清除的cell同样保留，mark不执行，IP/previous不发布。

## 3. missing角色MAPS fallback

lookup失败先经过原诊断-only路径，再读取boolean并调用`sub_40D460`。传入的低16位mask精确为：

```text
false: flags_or=0,    flags_and=0xFFFF-mask
true:  flags_or=mask, flags_and=0xFFFF
```

`guid`使用替换后的selector；因此missing `FFF0`提交受控index低16位，不提交字面`FFF0`。action/base/delta/tile/Talk/path/map字段全部为`FFFF` preserve sentinel。modern通过既有typed `LegacyMapsRolePatchRequest`/SDL MAPS database owner复现；source不存在或无session owner时不伪造live角色。

两条正常路径均IP`+6`、`ESI=1`，common join发布对应normalized previous并same-call继续。完整记录起于`0x7FFA`时，flags/MAPS、surface、IP=`0x8000`及previous先完成，下一fetch再返回`instruction_out_of_range`。

## 4. 资产锁与验证

线性TALK目录共锁定683条物理记录/685 probes，全部长度6：

| opcode | mask | records/probes | TALK1/2/3/4 | boolean分布 |
| ---: | ---: | ---: | --- | --- |
| 102 | `0040` | 18/18 | 6/5/0/7 | 0:8, 1:10 |
| 103 | `0020` | 267/267 | 118/88/60/1 | 0:12, 1:255 |
| 117 | `0010` | 42/42 | 0/7/31/4 | 0:37, 1:5 |
| 136 | `1000` | 244/246 | 93/65/80/6 | 0:227, 1:17 |
| 140 | `0800` | 56/56 | 6/2/29/19 | 0:13, 1:43 |
| 145 | `2000` | 0/0 | 0/0/0/0 | asset absence verified |
| 146 | `0100` | 56/56 | 21/12/15/8 | 0:16, 1:40 |
| 174 | `4000` | 0/0 | 0/0/0/0 | asset absence verified |

只有opcode103观察到`FFF0`，共58条；八变体均无`FFFE`。真实回放代表六个有资产变体并覆盖四个TALK文件：

```text
TALK1.DAT@0x0001BA75  opcode102 selector 00E6 value0
TALK2.DAT@0x00001723  opcode103 selector FFF0 value1
TALK3.DAT@0x00002E4A  opcode117 selector 023F value0
TALK4.DAT@0x00022104  opcode136 selector 0027 value1
TALK1.DAT@0x000317E4  opcode140 selector 0009 value1
TALK2.DAT@0x00011366  opcode146 selector 00E6 value1
```

synthetic覆盖8种mask、每种四raw alias、zero与任意非零、FFF0 index-as-GUID、FFFE controlled、bit28 skip首匹配、两种missing mask、missing FFF0、成功/失败lookup后的缺boolean、owner缺失、partial surface failure及精确尾。Story VM三项通过；surface occupancy与MAPS database/real三项依赖通过。未启动原版或OpenSWD3游戏EXE。

分类：`platform_adapted`。内部mask、operand/lookup顺序、u32 live位运算、双surface helper、missing低16位mask、推进、previous与same-call均保持；仅把裸全局surface/MAPS owner的unsafe访问收敛为原访问点typed状态。
