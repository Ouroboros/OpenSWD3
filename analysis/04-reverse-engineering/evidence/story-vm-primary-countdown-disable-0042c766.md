# 剧情 VM 主倒计时停用 `0x0042C766`

状态：`assembly_exact`（有效owner域）、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C766..0x0042C79C`

opcode：143 / `OP_143_DISABLE_PRIMARY_COUNTDOWN`

## 1. 状态顺序

记录固定2字节，无operand。机器严格按以下顺序执行：

1. `dword_4CAD20 = 0xFFFFFFFF`，停用primary ticks。
2. `sub_40DCB0(0x10)`，清primary active flag。
3. `sub_40DCB0(0x4C)`，清countdown suppression flag。
4. `sub_40DCB0(0x12)`，清primary companion flag。
5. 物理脚本指针和u16 IP各增加2。
6. common join发布previous143。
7. normal continuation carry为0，执行一次`_AIL_serve`并yield。

`sub_40DCB0`以`bit_index>>3`选byte，并以`AND (0xFF-mask)`清单bit；三个固定index均在共享`0x400`字节story flag owner内。现代复用现有`clear_legacy_world_story_flag`，对这些power-of-two mask逐bit等价。

primary transition value、两个primary auxiliary、secondary ticks及两个secondary auxiliary均不修改。handler不执行倒计时初始化或绘制。

## 2. 现代owner边界

opcode142已把普通世界实际`LegacyWorldFrameCoordinatorState::countdown`接入VM runtime；opcode143借用同一owner，不建立VM镜像。缺countdown binding时，在原第一项`dword_4CAD20`写入点返回`runtime_unavailable`；三个flag、IP、previous和audio均不修改。

完整记录可位于`IP=0x7FFE`并精确结束在窗口尾：ticks与三flag先提交，IP=`0x8000`、previous143及audio/yield全部完成，不读取下一条指令。

## 3. 资产与验证

完整线性TALK目录锁定4条物理记录/4 probes：

```text
TALK1.DAT@0x0002D30D
TALK1.DAT@0x00038E4F
TALK3.DAT@0x0002D19E
TALK3.DAT@0x0002D1B2
```

四库基础raw `0x008F`字样总数为`37/2/2/0`。TALK1另有1处`0x408F`字样，但不在线性指令目录；其余高位alias字样均为零，不能冒充资产记录。四条线性记录均完成真实回放，primary ticks置为`0xFFFFFFFF`、secondary保留、三flag清除，随后audio一次并yield。

synthetic覆盖四raw alias、全部倒计时字段隔离、无关相邻flag保留、audio调用时的已提交状态、缺typed owner及`IP=0x7FFE`精确尾。Story VM synthetic、real及initial-session三项通过。Linux core 186/186与app 192/192完整门均通过。未启动原版或OpenSWD3游戏EXE。

关联owner/callee证据：`legacy-countdown-004308c0-00430b60.md`、`story-vm-primary-countdown-initialize-0042c732.md`。
