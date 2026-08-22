# 剧情 VM ANI跟随目标配置 `0x0042C8F8`

状态：`platform_adapted`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C8F8..0x0042C935`

opcode：151 / `OP_151_CONFIGURE_ANI_FOLLOWER_TARGET`

## 1. Operand与分阶段写入

handler按物理顺序读取四个signed i16，并在每次读取后立即写入共享ANI follower owner：

```text
target_x   = sign_extend_i16(+2) << 4
target_y   = sign_extend_i16(+4) << 4
velocity_x = sign_extend_i16(+6)
velocity_y = sign_extend_i16(+8)
```

只有两个目标坐标左移4；两个速度不缩放。左移按32位机器位模式回绕。handler不夹值，不读或改current X/Y。

任一后续operand截断时，保留所有已完成的前序写入，不改后续字段，不推进IP，不发布previous。测试分别锁定缺`+2`、`+4`、`+6`和`+8`四个边界。

## 2. Owner与平台边界

四项写入直接复用`asset_runtime::LegacyAniFollowerState`；普通世界`0x00416B30`每帧以同一六字段owner绘制并推进当前位置。SDL剧情runtime借用`LegacyWorldFrameEffectState::follower`，不建立第二份状态。

原程序全局恒存在。现代缺binding时在读取`+2`后的第一项全局写点返回`runtime_unavailable`，不读取`+4`，也不推进IP或发布previous。合法binding路径无行为差异；该typed-stop使handler分类为`platform_adapted`。

## 3. IP、previous、audio与yield

完整记录按物理脚本指针与u16 IP前进10字节，再跳入`loc_42B0AE`。handler未写ESI，normal continuation carry为0，因此common join先发布normalized previous151，再进入`_AIL_serve`，执行一次audio maintenance并yield；不在同次调用fetch后继。

记录可位于`IP=0x7FF6`精确结束窗口。四项写入、IP=`0x8000`、previous、audio和yield均先完成。

## 4. 资产与验证

完整线性TALK目录中opcode151为0条物理记录/0 probes，使用`asset_absence_verified`。四种raw word在四库的全文件双字节候选计数为：

```text
             0097 4097 8097 C097
TALK1.DAT      14    0    0    0
TALK2.DAT       0    0    0    0
TALK3.DAT       0    0    0    0
TALK4.DAT      12    0    0    0
```

这些基础raw字样均非线性指令入口。synthetic覆盖四raw alias、signed目标与速度极值、current坐标保持、四阶段operand截断与已提交副作用、缺owner写点、audio callback时序及精确窗口尾。

Story VM synthetic、real及initial-session三项通过。Linux core `186/186`与app `192/192`完整门通过。未启动原版或OpenSWD3游戏EXE。
