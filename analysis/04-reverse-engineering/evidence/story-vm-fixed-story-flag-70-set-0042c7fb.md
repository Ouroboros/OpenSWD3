# 剧情 VM 固定剧情位70置位 `0x0042C7FB`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C7FB..0x0042C819`

opcode：147 / `OP_147_SET_STORY_FLAG_70`

## 1. 固定调用

handler无operand，固定执行：

```text
push 0x46
call sub_40DC80
```

`sub_40DC80`对bit index 70执行：

```text
byte_index = signed_sar(70, 3) = 8
mask = bit_mask_table[70 & 7] = 0x40
story_flags[8] |= 0x40
```

因此只把共享剧情位70置一。已经置位时幂等；同byte和其余`0x400`字节的所有位均保持。helper末尾AL值未被handler消费。

现代直接复用剧情VM进程状态中的共享`flags` owner及既有`set_legacy_world_story_flag`。固定索引70处于合法范围，无裸越界、nullable owner、外部模块或平台后端差异，因此本handler分类为`assembly_exact`。

flag70也在世界初始化的固定初始置位集合中；这不改变opcode自身合同。测试显式覆盖初始清零与已经置位两种状态。

## 2. IP、previous、audio与yield

调用返回后，handler按物理脚本指针与u16 IP分别前进2字节，再跳入`loc_42B0AE`。handler未写ESI，normal continuation carry为0，因此common join先发布normalized previous147，再进入`_AIL_serve`，执行一次audio maintenance并yield；不在同次调用fetch后继。

记录可位于`IP=0x7FFE`精确结束窗口。flag写、IP=`0x8000`、previous、audio和yield均先完成，不进行下一fetch。

## 3. 资产与验证

完整线性TALK目录锁定32条物理记录/32 probes：

```text
TALK1.DAT  7
TALK2.DAT  2
TALK3.DAT 11
TALK4.DAT 12
```

全部为基础raw `0x0093`、长度2。四库代表记录：

```text
TALK1.DAT@0x000226C6
TALK2.DAT@0x0002E6EC
TALK3.DAT@0x0000B057
TALK4.DAT@0x00003080
```

四条均从flag70清零状态回放，保留相邻flags69/71，完成previous/audio/yield。synthetic另覆盖四个raw alias、完整`0x400`字节隔离、幂等置位、audio callback时序及精确窗口尾。

Story VM synthetic、real及initial-session三项通过。Linux core `186/186`与app `192/192`完整门通过。未启动原版或OpenSWD3游戏EXE。

共享helper的动态索引边界另见`story-vm-reserved-global-bit-set-0042a756.md`；本handler只使用固定安全索引70。
