# 剧情 VM ANI跟随位置配置 `0x0042C858`

状态：`platform_adapted`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C858..0x0042C8F7`

opcode：150 / `OP_150_CONFIGURE_ANI_FOLLOWER_POSITION`

## 1. Operand与分阶段写入

handler按物理顺序读取两个signed i16坐标，并分别执行32位左移4：

```text
current_x = sign_extend_i16(+2) << 4
current_y = sign_extend_i16(+4) << 4
```

`current_x`在读取`+4`前已经写入。若`+4`截断，保留这项尚未夹值的写入，不改其余跟随状态，不推进IP，不发布previous。

机器在两次左移后分别把结果与`-1`比较。左移结果低四位恒为零，因此两个sentinel分支对完整i16输入域均不可达；原始bug按事实保留。输入`-1`得到`-16`，随后走普通夹值，不替换为屏幕中心。

完整路径执行signed夹值：

```text
current_x = clamp(current_x, 208, 432)
current_y = clamp(current_y, 208, 272)
target_x = current_x
velocity_x = 0
velocity_y = 0
```

`target_y`完全不写。Y夹值只改current Y，不能污染EAX承载的target X。

## 2. Owner与平台边界

六个机器全局由`0x00416B30`共同消费：当前X/Y、目标X/Y、X/Y速度。现有`asset_runtime::LegacyAniFollowerState`已经完整承接同一owner和初始化值；普通世界每帧ANI follower也直接消费该对象。剧情VM通过runtime借用`LegacyWorldFrameRuntimeState::follower`，不建立镜像。

原程序全局恒存在。现代缺binding时在读取`+2`后的第一项全局写点返回`runtime_unavailable`，不读取`+4`，也不推进IP或发布previous。合法binding路径无行为差异；该typed-stop使handler分类为`platform_adapted`。

## 3. IP、previous、audio与yield

完整记录按物理脚本指针与u16 IP前进6字节，再跳入`loc_42B0AE`。handler未写ESI，normal continuation carry为0，因此common join先发布normalized previous150，再进入`_AIL_serve`，执行一次audio maintenance并yield；不在同次调用fetch后继。

记录可位于`IP=0x7FFA`精确结束窗口。全部跟随状态写入、IP=`0x8000`、previous、audio和yield均先完成。

## 4. 资产与验证

完整线性TALK目录中opcode150为0条物理记录/0 probes，使用`asset_absence_verified`。四种raw word在四库的全文件双字节候选计数为：

```text
             0096 4096 8096 C096
TALK1.DAT      29    0    0    0
TALK2.DAT       7    0    0    0
TALK3.DAT      14    0    0    0
TALK4.DAT       7    0    0    0
```

这些基础raw字样均非线性指令入口。synthetic覆盖四raw alias、域内值、四侧夹值、`-1`原始bug、target Y保持、速度清零顺序、缺`+2`、缺`+4`的已提交X、缺owner写点、audio callback时序及精确窗口尾。

Story VM synthetic、real及initial-session三项通过。Linux core `186/186`与app `192/192`完整门通过。未启动原版或OpenSWD3游戏EXE。
