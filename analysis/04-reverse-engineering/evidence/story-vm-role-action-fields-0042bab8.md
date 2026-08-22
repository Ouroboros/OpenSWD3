# 剧情 VM 角色动作三字段更新 `0x0042BAB8`

状态：`platform_adapted`、有效运行域`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042BAB8..0x0042BC2B`

opcode：120

## 1. selector与分支

记录固定十字节：

```text
+0  u16 opcode
+2  u16 selector
+4  u16 action id
+6  u16 base variant
+8  u16 variant delta
```

handler先读取selector。`0xFFF0`替换为context source GUID，只改局部值；随后调用`sub_40C0D0`。`0xFFFE`保持helper-native受控角色选择，ordinary GUID跳过bit28角色并返回首个匹配项。

lookup发生在三个动作operand之前。lookup命中进入live路径；失败进入MAPS role-source patch路径。VM公共入口仍对真正越界的受控角色index执行既有typed-stop；有效角色域不变。

## 2. live角色路径

机器按以下顺序读取和提交，不能整条预验：

1. 读`+4`；若不是`FFFF`，按`s16`符号扩展写action `+0x00`；
2. 读`+6`；若不是`FFFF`，按`s16`符号扩展写action `+0x08`；
3. 读`+8`；若不是`FFFF`，按`u16`零扩展写action `+0x34`；
4. 把action `+0x44`低word清零；
5. 调用`sub_4321E0`刷新action；
6. 对完整role flags OR `0x00001000`。

每个`FFFF`只保留对应旧字段，不阻止后续wait清零、刷新或flags置位。`+4/+6/+8`任一后续读取失败时，前面已提交字段不回滚；wait、refresh、flags、IP和previous尚未发生。

refresh返回零只调用`nullsub_1`诊断。诊断参数从refresh后的live action/GUID重读，但没有业务副作用；handler仍置bit12并完成。

现代实现复用typed `LegacyActionRecord`和既有action update port。span边界在原首次角色字段访问点停止并保留此前写入，替代原裸固定角色表越界。

## 3. missing角色路径

lookup失败后，机器按`+8 → +6 → +4`顺序读取三个raw word。三项不做符号扩展，也不把`FFFF`解释为本地保留；它们原样传给`sub_40D460`：

```text
guid          = FFF0替换后的selector
action/base/variant = 三个raw u16
x/y/talk/path = FFFF
flags OR      = 1000
flags AND     = FFFF
logical map   = FFFF
```

MAPS source不存在时callee只诊断，caller仍消费。现代窄port的request默认值精确承接七个`FFFF`参数；不新增外部owner。

## 4. 出口与时序

live与missing两路均：

- u16 IP加10；
- common join发布normalized previous120；
- ESI=1，同一次解释器调用继续取后继；
- 不service audio，不yield。

完整记录精确结束在窗口尾时，先提交live action/flags或MAPS patch、推进IP并发布previous，再由下一fetch返回窗口越界。只有opcode/selector或中途operand截断时，不推进IP、不发布previous。

旧modern case已支持基本字段转换与初始世界missing patch，但曾在lookup前整条预验并一次读取全部operand，也遗漏previous120。本轮按机器访问点修正；既有TALK100中GUID 123/240的真实missing-role回归继续通过。

## 5. 资产锁与验证

线性TALK目录锁定800条物理记录/808 probes，全部raw `0x0078`、长度10：

```text
file       records  probes
TALK1.DAT      336     336
TALK2.DAT      124     124
TALK3.DAT      159     167
TALK4.DAT      181     181
```

selector共118种，当前记录没有`FFF0/FFFD/FFFE/FFFF`。action id有44种，555条为`FFFF`；其余范围`1..12005`。base variant有59种，仅1条为`FFFF`，其余范围`0..79`。variant delta有10种，5条为`FFFF`，其余范围`0..8`。

真实回放：

```text
TALK1.DAT@0x0000465A  {120,007B,0231,0008,0000}  live角色
TALK1.DAT@0x00004380  {120,0001,0062,FFFF,FFFF}  missing MAPS patch
```

synthetic覆盖四raw alias、`FFF0`、`FFFE`、bit28首匹配、`s16/u16`边界、三个独立`FFFF`保留、selector与四类operand截断、逐项部分提交、refresh失败顺序、missing raw patch及live/missing两类精确尾。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186、app 192/192均以exit 0通过。未启动原版或OpenSWD3游戏EXE。
