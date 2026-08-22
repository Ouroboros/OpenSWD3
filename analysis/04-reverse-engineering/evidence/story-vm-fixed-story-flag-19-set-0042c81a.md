# 剧情 VM 固定剧情位19置位 `0x0042C81A`

状态：`assembly_exact`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C81A..0x0042C838`

opcode：148 / `OP_148_SET_STORY_FLAG_19`

## 1. 固定调用

handler无operand，固定执行：

```text
push 0x13
call sub_40DC80
```

独立追入`sub_40DC80`后，固定bit index 19产生：

```text
byte_index = signed_sar(19, 3) = 2
mask = bit_mask_table[19 & 7] = 0x08
story_flags[2] |= 0x08
```

因此只把共享剧情位19置一。已经置位时幂等；同byte和其余`0x400`字节的所有位均保持。helper末尾AL值未被handler消费。

现代直接复用剧情VM进程状态中的共享`flags` owner及既有`set_legacy_world_story_flag`。固定索引19处于合法范围，无裸越界、nullable owner、外部模块或平台后端差异，因此本handler分类为`assembly_exact`。

## 2. IP、previous、audio与yield

调用返回后，handler按物理脚本指针与u16 IP分别前进2字节，再跳入`loc_42B0AE`。handler未写ESI，normal continuation carry为0，因此common join先发布normalized previous148，再进入`_AIL_serve`，执行一次audio maintenance并yield；不在同次调用fetch后继。

记录可位于`IP=0x7FFE`精确结束窗口。flag写、IP=`0x8000`、previous、audio和yield均先完成，不进行下一fetch。

## 3. 资产与验证

完整线性TALK目录中opcode148为0条物理记录/0 probes，因此使用`asset_absence_verified`，不伪造真实回放。四种raw word在四库的全文件双字节候选计数为：

```text
             0094 4094 8094 C094
TALK1.DAT      33    0    0    0
TALK2.DAT       5    0    0    0
TALK3.DAT       0    0    0    0
TALK4.DAT       5    0    0    0
```

这些基础raw字样均非线性指令入口。synthetic覆盖四个raw alias、flag19初始清零与已置位、完整`0x400`字节隔离、audio callback时序及精确窗口尾。

Story VM synthetic、real及initial-session三项通过。Linux core `186/186`与app `192/192`完整门通过。未启动原版或OpenSWD3游戏EXE。

共享helper的动态索引边界另见`story-vm-reserved-global-bit-set-0042a756.md`；本handler只使用固定安全索引19。
