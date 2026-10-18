# 战斗模块核对记录（第 1 遍，2026-10-18）

## BattleHud 0x0044FFE0..0x00451730

- 0x0044FFE0 DrawTileBox（第 4 帧取色填充 16 × (n + 2)，帧 0..2 顶、3/5 两侧、6..8 底；有符号字节填充色）、0x00450270 DrawPictureFrame0（参数 0）、0x00450490 DrawBar（宽 > 0 才画）、0x004504E0、0x00450530 DrawGauge、0x004505B0 DrawGaugeAlt（恒返回 1）、0x00450630 DrawGaugeWide（宽 + 2）：模式 0 的复制画法不读第 6 参数，省略调色板参数等价；通过。
- 0x004506B0 DrawSmallNumber（四位有符号除法，至少一位，从个位向左以当前图宽递减）、0x004507A0 DrawNumber（十位，字体在 0x004FDD08 高字，个位必画）、0x00450900 DrawNumberDigit（无符号除法，0x004FD784 为 0x8000 时模式 0x20，返回当前图宽）、0x004509D0 DrawSlotAnimation、0x00450A50 FillColor（参数本身为源，模式 8）、0x00450B40、0x00450B60、0x00450BD0 DrawDigit（字体 0/1 取 0x2359/0x2358，其他重画上一张；0x4000 时模式 0x20）：通过。
- 0x00450C50 DrawScrollBar（顶 0x1A/0x1E、轨道 0x18、滑块 0x19 长 Ftol(7 / 总数 × 轨道)、步长 (轨道 − 长) / (总数 − 7)、最短 5、到底时伸到轨道末、裁剪 x..x + 0x20、底 0x1B/0x1F；第一个动作沿用上次状态，之后清零）、0x00450F90 DrawCoinToss（帧计数为 0 时：高字非 0 全清，否则 Random(2) 定低字；画 0x2329 的帧 0（图片号 0）；每 25 帧延迟加倍（16 位），< 0x40 时倒数到 0 翻面并以音量 1 播 0x2E，否则延迟复位 0x10001、落在 1 面时高字加一、清状态返回 1）、0x00451100 DrawTimingBar（三条标记 0x004A74D0、指针 (计数 >> 1) + 1 在标记 0..+2 内为命中，等于 0x004FDD08 低字时取消，否则清该低字；到 0x3D 时置 0x8000 返回 1）、0x004512B0 DrawTallGauge（第 2 帧高 / 6 × level，level < 6 时裁剪，光泽模式 0x14 透明度 8）：通过。
- 0x00451420 LoadAnimation（+0x88 字节非 0 时记波形；按图片号载入一次并右移 640 / divisor；+0x42 为 0 结束；越过 6 张的图片号原版读写栈外，重建跳过）、0x00451540 DrawAnimation（波形 0x004CD724 只在这一笔）、0x004515E0 PlayAnimationShifted（每张新图片按正负左右移后画一次，+0x44/+0x46 清零）、0x00451730 FreeAnimation：通过。
- 0x004502B0 DrawPortrait（0x2390 动作 index；高亮时 0x07E007E0 描边；dim 为 1 时绿蓝 −10 红 0、模式 +0x18 | 0x10 再画一次）、0x00450400 DrawArrow（四个槽 0x004FDD10 + 0x98n，0x2392 动作 0，模式 +0x18）。差异（diff 轮）：原版这两处把图片 +4 的调色板指针作为第 6 参数（模式来自动作脚本或含 0x10，可能读取该参数），重建传默认参数；改为 PaletteArgument。修正后通过（exact 轮）。
- 0x00450A80 DrawEffectAt：动作运行一步，mirror 为 1 时翻转位 0。差异（diff 轮）：镜像时原版 `mov dx, [eax+0Ch]; sub dx, si`（0x00450AE2）只把偏移的低字替换为 宽 − x，高字保留 x 的高字，不镜像时为完整 32 位 x；重建两种情况都截成 i16。改为按原式组合。修正后通过（exact 轮）。
