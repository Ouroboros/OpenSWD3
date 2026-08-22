# 剧情 VM Scene_Music 表项更新 `0x0042BC4C`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042BC4C..0x0042BCF4`

opcode：123

## 1. MAPS owner与查找

handler先读取`u16(+2)` raw key；只有字面`FFF0`在比较时替换为当前Talk source GUID低16位，不修改脚本字节。

原`dword_4C9A10`对应去掉文件前`0x200`字节后的可写MAPS payload。Scene_Music表按以下三层u32相对链定位：

```text
first  = u32[payload + 0x08]
second = u32[payload + first + 0x04]
table  = u32[payload + second]
entry  = payload + table
```

表项固定8字节，以entry key `u16(+0) == 0`终止。扫描从首项开始，每次不匹配时先读取下一项key，再推进8字节；只处理第一个匹配项，不追加记录，不修改终止项。

现代实现直接借用既有`LegacyResourceDatabases` mutable MAPS payload。固定裸指针、三层相对链、表扫描和两次目标写入分别在原访问点增加typed bounds stop；合法payload中的地址回绕、顺序和写入不变，故分类为`platform_adapted`。

## 2. 非对称分阶段写入

匹配成功时机器顺序为：

```text
raw_pair = u32[instruction + 2]
entry[0..3] = raw_pair
third = u16[instruction + 6]
entry[4..5] = third
entry[6..7] = 保持
```

因此：

- `+2` key和`+4` value作为原始dword一起复制。
- `FFF0`只参与匹配替换；成功后entry key仍被字面`FFF0`覆盖。
- 目标dword写入后才读取`+6`。`+6`截断会保留已提交的前四字节。
- 成功路径完全不读取名义`+8` word。
- 若第一或第二次目标写越界，分别在原dword/word裸写点停止；第二次失败保留前四字节。

未找到key时，机器才按`+8 → +6 → +4 → +2`读取四个u16供空诊断函数使用。诊断不改变业务状态，但`+8`截断会阻止IP和previous提交。

成功和完整miss最终都固定推进10字节、发布previous123、无audio、无yield，并在同一次解释器调用继续。成功记录从窗口`0x7FF8`开始时，只需可读到`+7`，即可完成写入、把u16 IP回绕式推进到`0x8002`并发布previous；下一fetch再失败。

## 3. 资产锁与验证

当前MAPS payload链为：

```text
first=0x00000070  second=0x00000078  table=0x00000E86
```

Scene_Music表含314个非零项，terminator位于payload offset `0x1856`。

线性TALK目录锁定71条物理记录/71 probes，全部raw `0x007B`、长度10，且71/71个key均在当前MAPS表中命中：

```text
file       records  probes
TALK1.DAT       37      37
TALK2.DAT        1       1
TALK3.DAT        3       3
TALK4.DAT       30      30
```

`+2`有69个唯一key，范围7..310；`+4/+6`各有9种值。全部71条`+8`均为`0x4000`，但成功路径不读取。资产没有高位raw alias；TALK1中另有`0x407B/0xC07B`各一处字节字样，均不是线性指令入口。

真实MAPS回放分别使用：

```text
TALK1.DAT@0x00022C77
TALK2.DAT@0x0001836F
TALK3.DAT@0x0000CD7A
TALK4.DAT@0x000289D1
```

四条均从窗口`0x7FF8`只复制前8字节，验证对应MAPS表项前六字节更新、尾word保持、IP `0x8002`、previous及下一fetch。

synthetic覆盖四raw alias、FFF0比较替换与字面回写、重复key首匹配、完整miss、selector/dword/third三阶段截断、成功未读`+8`与miss必读`+8`、三层链/表首/next边界、两次目标写边界和same-call后继。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186、app 192/192通过。未启动原版或OpenSWD3游戏EXE。
