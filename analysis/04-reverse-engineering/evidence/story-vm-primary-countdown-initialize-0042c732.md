# 剧情 VM 主倒计时初始化 `0x0042C732`

状态：`assembly_exact`（有效owner域）、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

handler入口：`0x0042C732..0x0042C765`

复用callee：`sub_430B60` / `0x00430B60..0x00430BDE`

opcode：142 / `OP_142_INITIALIZE_PRIMARY_COUNTDOWN`

## 1. 记录、读取顺序与调用

记录固定8字节：

```text
+0  u16 opcode
+2  u16 minutes
+4  u16 seconds
+6  u16 primary transition value
```

handler严格按`+6`、`+4`、`+2`顺序读取并零扩展为u32，然后以：

```text
sub_430B60(minutes, seconds, primary_transition_value, 0)
```

调用既有倒计时初始化器。所有operand读取都发生在callee调用前，读取之间没有状态副作用；现代连续窗口使用完整8字节访问检查，在合法窗口域与机器三个逆序word读取等价。

最后参数固定0，因此只进入primary分支。现有`initialize_legacy_countdown`已独立按完整callee LST闭环，本handler不复制算法或owner。

## 2. primary初始化与控制流

callee以x86 u32回绕计算：

```text
ticks = 30 * (seconds + 60 * minutes)
```

然后按primary合同：

1. 清零两个primary auxiliary owner；
2. 写primary ticks；
3. 写primary transition value；
4. 依次设置内部flag `0x10`、`0x12`。

secondary ticks和两个secondary auxiliary owner均不修改。现代runtime直接绑定普通世界帧已有`LegacyWorldFrameCoordinatorState::countdown`；flag adapter写入剧情VM共享的同一`0x400`字节bitset。缺countdown binding时，在三个operand均读取后的原callee调用点返回`runtime_unavailable`，不修改owner、IP或previous。

callee返回后：

1. 物理脚本指针和u16 IP各增加8；
2. common join发布previous142；
3. normal continuation carry为0，执行一次`_AIL_serve`；
4. 返回1并yield，不在同调用fetch后继。

opcode142自身不写ESI；不得把其直跳`loc_42B0AE`误判为same-call。完整8字节记录可精确结束于`IP=0x8000`：初始化、IP、previous和audio/yield全部完成，不发生下一fetch。

## 3. 资产与验证

完整线性TALK目录锁定2条物理记录/2 probes，均位于TALK1且为基础raw `0x008E`：

```text
TALK1.DAT@0x000233A7  minutes5, seconds0, transition728
TALK1.DAT@0x00058A88  minutes5, seconds0, transition1111
```

四库基础raw `0x008E`字样总数为`14/2/3/0`；三个高位alias raw字样均为零。两条线性记录均完成真实回放，得到9000 ticks、primary auxiliary清零、两flag依次生效，随后audio一次并yield。

synthetic覆盖四raw alias、零与最大u16、32位回绕、primary/secondary字段隔离、缺`+6`尾、缺typed owner、`IP=0x7FF8`精确尾、previous、audio和yield。Story VM synthetic、real及initial-session三项通过。Linux core 186/186与app 192/192完整门均通过。未启动原版或OpenSWD3游戏EXE。

关联callee证据：`legacy-countdown-004308c0-00430b60.md`。
