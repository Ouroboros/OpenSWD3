# 剧情 VM 角色距离条件重载 `0x0042C49E`

状态：`assembly_exact`（有效角色/窗口域）、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C49E..0x0042C566`

opcode：138 / `OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS`

## 1. 记录与机器读取顺序

记录固定14字节：

```text
+0   u16 opcode
+2   u16 tile X
+4   u16 tile Y
+6   u16 role selector
+8   u16 radius
+10  u32 same-file target
```

机器不按布局顺序读取。第一项operand固定是`+6 selector`；只有live role路径才读取`+2/+4`和`+8`，且只有距离谓词taken才读取`+10 target`。ordinary missing role不读取坐标、radius或target，直接消费14字节、发布previous138并same-call继续。

现代按原访问点检查窗口：selector不可读时零副作用停止；live role的radius不可读时在距离计算后停止；taken target不可读时不发起重载。missing和not-taken允许未读尾字段越过窗口尾，先提交IP/previous，再由same-call下一fetch失败。

## 2. selector与角色访问

selector规则严格为：

- raw `0xFFF0`先替换为受控角色数组index的低16位，再作为ordinary GUID key交给`sub_40C0D0`；它不是当前对话source GUID，也不是直接数组index。
- raw `0xFFFE`由`sub_40C0D0`直接映射为完整受控角色index并无条件返回成功。
- 其他值按GUID查找首个flags bit28清零的live role。

ordinary miss只执行无状态`nullsub_1`诊断后走sequential。原版`0xFFFE`可把越界受控index直接用于`index*0xD8`数组访问；现代在同一首次角色访问点返回`role_not_found`，不读取坐标/radius/target，也不推进IP/previous。这是明确typed平台边界。

## 3. x87距离与严格谓词

live role按完整32位回绕计算：

```text
dx = wrapping_i32(role.world_x - (u16(tile_x) << 4) + 8)
dy = wrapping_i32(role.world_y - (u16(tile_y) << 4))
squared = wrapping_i32(dx * dx + dy * dy)
distance = trunc_toward_zero(sqrt(signed_i32(squared)))
threshold = u16(radius) << 4
```

`imul`和加法只保留低32位。若`squared`回绕为负，x87 `fild/fsqrt`产生masked invalid；`sub_489654`以向零模式`fistp qword`得到integer-indefinite `0x8000000000000000`，调用者只取低dword，所以distance固定为0。现代显式返回同一低dword结果。

分支是signed严格`distance > threshold`。等于threshold不重载；taken才读取u32 target。X方向独有`+8`而Y方向没有，保持原始不对称中心偏移。

## 4. 重载与common continuation

not-taken或ordinary miss：

```text
IP += 14
previous = 138
same-call continue
no audio service
```

taken复用共享`sub_42E430`合同：先service audio，再提交当前文件号、`talk_data_offset=target`、`IP=0`并读取新窗口。成功后发布previous138并same-call执行target。现代load失败保留已提交context与audio、清`window_loaded`、发布previous138并返回`load_failed`，与既有同文件重载平台边界一致。

ordinary miss的三参数`nullsub_1`没有状态效果，现代不复制该空诊断。除此之外，有效角色与窗口域的分支、访问顺序、位宽、回绕和副作用均与机器一致。

## 5. 资产与验证

完整线性TALK目录锁定117条物理记录/117 probes，全部raw `0x008A`、长度14：

```text
TALK1  19
TALK2  35
TALK3  27
TALK4  36
```

全部真实selector均为1；radius分布为`2:11, 3:11, 4:2, 5:2, 6:73, 8:4, 10:14`，共有17个不同target。tile X范围8..191，tile Y范围6..197。四库基础raw `0x008A`字样总数为`41/55/28/36`；三个高位alias raw字样均为零。

四库代表记录：

```text
TALK1.DAT@0x00005B99  (38,68,selector1,radius5,target0x00005E6B)
TALK2.DAT@0x0001950A  (78,65,selector1,radius2,target0x0001957C)
TALK3.DAT@0x0002397F  (40,23,selector1,radius6,target0x000237DA)
TALK4.DAT@0x00004F08  (76,54,selector1,radius10,target0x00004E46)
```

四条均以远距离role状态take重载，并same-call执行target。

synthetic覆盖四raw alias、strict equal与一像素超界、FFF0受控index作GUID、FFFE直接index、bit28-aware ordinary lookup、负平方和x87低dword零、invalid controlled typed-stop、load失败、missing短尾、radius截断、taken target截断、not-taken未读target及完整精确尾。taken路径固定一次audio和一次data load；其余路径无audio。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186与app 192/192完整门以exit0通过。未启动原版或OpenSWD3游戏EXE。
