# 剧情 VM 对话标志等待 `0x0042B9C2`

状态：`platform_adapted`、有效运行域`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`、`shared_handler_all_variants_tested`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B9C2..0x0042BAB7`

共享opcode：

```text
119  等待首个匹配消息 +0x08 bit0 置位
139  等待首个匹配消息 +0x08 bit15置位
```

## 1. selector与role index

两种记录均固定四字节：`u16 opcode, u16 selector`。handler先读取selector：

1. `0xFFF0`替换为context source GUID，只改局部DI，不写回脚本；
2. 替换后的值为`0xFFFD`时，直接把目标role index设为`0x0000FFFD`，不调用lookup；
3. 其他值调用`sub_40C0D0`；`0xFFFE`由helper直接复制完整32位受控角色index，普通GUID查找首个bit28清零角色；
4. lookup失败只调用无副作用诊断，固定消费4字节，不扫描消息链。

`0xFFF0`替换发生在`0xFFFD`判断之前，因此context source自身为`0xFFFD`时同样进入detached消息路径。

现代实现直接使用`resolve_legacy_world_role_selector`而非会追加span有效性检查的VM包装层，保留`0xFFFE`完整u32输出。消息的`+0x16`是u16；机器以零扩展值与完整u32 index比较，所以大于`0xFFFF`的受控index不会与低16位别名。

VM公共入口仍对真正越界的受控角色index执行既有typed-stop；生产角色表固定256项。该平台边界隔离原全局损坏域，不改变有效运行状态。synthetic以扩容有效span中的`0x00010003`锁定handler内部完整宽度比较。

## 2. 首匹配与variant极性

lookup成功或`0xFFFD`旁路后，handler从`dword_4ACF48`链头顺序扫描`record.role_index`。空链或全链miss均消费。找到首个匹配项后立即停止扫描；后续同role消息完全不参与本次判断。

variant由normalized opcode决定：

```text
opcode119: (record.flags & 0x00000001) != 0 才完成
opcode139: (record.flags & 0x00008000) != 0 才完成
```

119的bit15和139的bit0都不影响各自谓词。机器对119先识别`opcode-119==0`；对139则识别再减20后的零值，并以`test ch,80h`读取flags bit15。两个条件方向都是“目标位清零则等待，置位则完成”。

modern复用现有`LegacyDialogRuntimeState::messages`，只读首匹配消息，不修改链、flags、角色gate、counter、text或caption。owned list替代裸next链，属于平台owner适配。

## 3. 出口、previous与audio

等待路径：

- IP不推进；
- common join发布normalized previous119或139；
- service audio一次；
- 返回一并跨帧yield。

完成、空链、全链miss或lookup失败路径：

- u16 IP加4；
- 发布normalized previous；
- 不service audio；
- ESI=1，同一次解释器调用继续取后继。

只有opcode而缺selector时，在lookup和消息访问前返回operand越界，不发布previous、不service audio。完整记录精确结束在`0x8000`时：等待variant保持原IP并正常yield；完成variant先推进和发布previous，再由下一fetch返回窗口越界。

## 4. 资产锁与验证

线性TALK目录锁定850条物理记录/850 probes，全部长度4：

```text
opcode   TALK1  TALK2  TALK3  TALK4  total
119          8    138     20    669    835
139          9      2      0      4     15
```

119共有12种selector：`0001,0003,000E,000F,0068,00E6,00E7,0142,0147,0148,2710,2711`；其中`2710/2711`为431/384条。139共有5种：`0000,0001,01DA,01DB,2711`；`0001`为11条。当前线性记录没有`FFF0/FFFD/FFFE/FFFF`。

真实回放分别使用：

```text
TALK1.DAT@0x0000968B  {119,10000}
TALK1.DAT@0x000045E4  {139,0}
```

两条都先以目标位清零验证原地wait/audio，再置各自目标位验证+4/same-call。synthetic覆盖两个opcode的四raw alias、无关bit、首匹配优先、`FFF0→ordinary`、`FFF0→FFFD`、raw `FFFD`、`FFFE`、lookup失败、完整u32受控index、selector截断，以及等待/完成两种精确尾。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186、app 192/192均以exit 0通过。未启动原版或OpenSWD3游戏EXE。
