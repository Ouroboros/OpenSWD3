# 剧情 VM 宽全局整数共享 handler `0x0042B070`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B070..0x0042B1EC`，common join `0x0042B0AE..0x0042B0C8`

opcodes：181–185

## 1. 独立入口与operand宽度

本入口在二级分派先把effective opcode送入共享refinement，再进入opcode29–33已使用的后半段。不能继承窄入口的完成结论：

```text
+2  i16 index
+4  u32 value / threshold bit pattern
+8  u32 same-file target（仅184/185）
```

181–183固定长度8；184–185固定长度12。value没有符号扩展或截断，完整四字节按u32 bit pattern参与写入、回绕算术和无符号比较。

机器在index门之前读取完整value。signed index `>=64`时不读conditional target，不访问变量表，不执行variable0共享clamp，不推进IP；只发布previous、service audio一次并yield，因此下一帧重试同一记录。负index通过机器signed门，并在首次数组读写越界；现代在该unsafe point返回`script_variable_index_out_of_range`：181–183已读完整value，184–185还会先无条件读取完整target。

## 2. 五个refinement

actual owner复用`LegacyWorldStoryVmState::script_variables[64]`，即完整`dword_4ACBD0`进程期整数区，不增加第二份状态。

- opcode181：`variable = value`。
- opcode182：`variable += value`，u32回绕。
- opcode183：先u32回绕减法；结果bit31置位时把选中项写0。
- opcode184：按u32比较，`variable >= threshold`时taken。
- opcode185：按u32比较，`variable <= threshold`时taken。

184/185都在比较前读取`+8 u32 target`，not-taken也必须有完整12字节。taken调用已闭环same-file window loader：先service audio，发布目标TALK offset和IP0，读取新窗口，然后回到共享尾；正常成功同调用读取目标首指令。

所有有效index路径在操作或reload后检查`script_variables[0]`：bit31置位则完整写0。随后发布previous。181–183和not-taken 184/185按8/12字节推进并same-call；taken以IP0 same-call。若checked loader失败，仍按机器调用返回后的顺序执行variable0 clamp、previous publication，再返回`load_failed`。

## 3. 资产锁

完整线性TALK目录只有1条：

```text
TALK4.DAT@0x00031BF1
raw/effective = 0x00B8 / 184
index = 0
threshold = 30000000
same-file target = 0x00031A59
```

默认初始化variable0=100时not-taken并顺序消费12字节；variable0=30000000时按equality taken并reload目标。两个方向均由该真实记录回放锁定。opcodes181/182/183/185均为0条，使用`asset_absence_verified`。

全文件双字节候选总计：

```text
00B5=43  40B5=0  80B5=0  C0B5=1
00B6=15  40B6=0  80B6=0  C0B6=131
00B7=44  40B7=1  80B7=0  C0B7=58
00B8=13  40B8=1  80B8=0  C0B8=3
00B9=24  40B9=1  80B9=0  C0B9=91
```

除上述一条线性record外，其余候选是operand、文本或其他非入口字节，不能继承为真实记录。

## 4. 验证覆盖

synthetic覆盖五opcode的四raw alias、完整u32高位、set/add/sub回绕、subtract sign-bit clamp、unsigned taken/not-taken/equality、target无条件读取、same-file reload、variable0共享clamp、高index audio-yield重试、负index unsafe边界、value/target分阶段截断、checked load失败顺序，以及8/12字节精确尾已提交副作用后下一fetch失败。

真实opcode184记录覆盖not-taken和equality-taken。Story VM synthetic/real/initial-session 3/3通过，SDL app编译通过；Linux完整门core 186/186、app 192/192通过。未启动原版或OpenSWD3游戏EXE。
