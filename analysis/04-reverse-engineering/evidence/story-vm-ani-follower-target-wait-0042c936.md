# 剧情 VM ANI跟随目标等待 `0x0042C936`

状态：`platform_adapted`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C936..0x0042C95A`

opcode：152 / `OP_152_WAIT_ANI_FOLLOWER_TARGET`

## 1. 短路条件与状态

handler无operand，按固定顺序读取共享ANI follower owner：

```text
if current_x != target_x:
    wait
if current_y != target_y:
    wait
complete
```

X不等时不读取Y。等待与完成均不修改六个follower字段。只有两轴均相等时把物理脚本指针和u16 IP推进2字节；任一轴不等都保持原IP。

## 2. Common join、previous与yield

X不等直接跳`loc_42B0AE`。X相等后，Y比较结果原样进入`loc_42D1BE`：不等跳common join，相等执行`+2`后再跳common join。所有路径均未写ESI，因此normal continuation carry保持0。

所以三条路径都先发布normalized previous152，再进入`_AIL_serve`，执行一次audio maintenance并yield：

- X不等：原地发布previous、audio、yield；
- X相等/Y不等：原地发布previous、audio、yield；
- 两轴相等：推进2、发布previous、audio、yield，下次调用才取后继。

完成路径不是same-call continuation。记录位于`IP=0x7FFE`时，等待保持`0x7FFE`；完成推进到`0x8000`。两路都不fetch后继，均正常yield。

## 3. Owner与平台边界

四项读取直接复用`asset_runtime::LegacyAniFollowerState`，与opcode150/151及普通世界`0x00416B30`共享唯一owner。SDL剧情runtime借用`LegacyWorldFrameEffectState::follower`。

原程序全局恒存在。现代缺binding时在第一项状态读取前返回`runtime_unavailable`，不发布previous、不service audio。合法binding路径无行为差异；该typed-stop使handler分类为`platform_adapted`。

## 4. 资产与验证

完整线性TALK目录中opcode152为0条物理记录/0 probes，使用`asset_absence_verified`。四种raw word在四库的全文件双字节候选计数为：

```text
             0098 4098 8098 C098
TALK1.DAT      10    0    0    0
TALK2.DAT       1    0    0    0
TALK3.DAT       0    0    0    0
TALK4.DAT       1    0    0    0
```

这些基础raw字样均非线性指令入口。synthetic覆盖四raw alias、X首项短路、Y等待、两轴完成、完整状态保持、缺owner读取点、audio callback时序，以及`IP=0x7FFE`的等待尾和完成尾。

Story VM synthetic、real及initial-session三项通过。Linux core `186/186`与app `192/192`完整门通过。未启动原版或OpenSWD3游戏EXE。
