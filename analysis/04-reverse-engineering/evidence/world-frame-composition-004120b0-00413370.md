# 普通世界帧组合与地图底图（`0x004120B0..0x00413370`）

状态：`platform_adapted`、`assembly_exact`、`unit_tested`、`asset_verified`、
`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值，固定普通世界更新、软件画面组合与
最终提交之间的边界。伪码只用于定位，不参与裁决。

## 1. `0x004120B0` 的职责边界

普通世界主帧中的唯一调用槽是 `0x0040AA90`，入口无参数，返回值不被观察。函数不是纯 renderer，
而是以下三段连续过程：

1. `0x004120B7..0x00412687`：推进公共动作记录、玩家/相机、地图角色和队伍角色；
2. `0x0041268C..0x004126E8`：准备 framebuffer，调用 `0x00412930` 组合世界画面，再处理
   固定 UI 和开发调试叠层；
3. `0x004126F0..0x00412923`：再次维护 Miles，执行普通世界唯一一次 DirectDraw `Blt`，
   随后更新格指针、角色快照和帧后状态。

中段的机器顺序固定为：

```text
0041268C  00414570
00412691  004148F0
00412696  AIL_serve
004126A2  00416F10(source surface)
004126B3  00416F60(source surface, framebuffer)
004126B8  00412930
004126C7  004308C0(400, 8, 0)
004126E8  条件调用 00413FE0(camera_left, camera_top)；调用者额外栈字 2 未被读取
004126F0  AIL_serve
004126FF  00437DF0(primary surface wrapper)
00412716  DirectDraw::Blt(..., DDBLT_WAIT, ...)
```

因此 OpenSWD3 的 `compose_legacy_world_frame` 只映射 `0x00412930`；最终提交仍必须留在
`0x004120B0` 的对应位置，不能挪成统一帧尾。外层顺序现由
[`world-frame-coordinator-004120b0.md`](world-frame-coordinator-004120b0.md) 单独固定。

## 2. `0x00412930` 的三条主体路径

入口先在 `0x00412930` 写入全屏 clip。`[0x004C9A18] & 0x04` 非零时，又以
`(focus_x-192, focus_y-192, focus_x+192, focus_y+192)` 写入外层局部 clip；
`0x00416FF0` 只按 surface 边界夹值，不做 16 像素取整。

随后主体三选一：

| 路径 | 汇编条件 | 行为 |
|---|---|---|
| ANI activity | `[0x004CAE8C] != 0` | 只调用 `0x004154A0`，然后进入公共尾部 |
| clear-only | activity 为零且 `[0x004C9A18] & 1 != 0` | 再清一次固定 `0x96000` 字节，只执行第二组 `0x004147E0`，然后进入公共尾部 |
| normal | 其余 | 前景预处理、地图底图、空间对象、角色、动作和 ANI 效果按下述顺序组合 |

activity 为零时，函数先短路查询 service `0x0F`、`0x13`；任一非零便以
`rep stosd` 清零 `0x25800` 个双字。clear-only 路径不合并该清屏，所以 service 命中时
同一帧会真实执行两次清屏。

## 3. normal 路径的精确顺序

`0x004129C8..0x00412B21` 的顺序为：

1. `0x004151F0`；
2. 若 service `0x48` 为零且 service `0x13` 非零，再写一次局部 clip；
3. 按地图像素布局和相机对齐状态选择四个底图函数之一；底图函数会独立再次短路查询
   service `0x48/0x13`，不能复用第 2 步结果；
4. 清除临时 palette 指针；service `0x0B` 为零时执行 `0x00413EA0`；
5. `0x00413870`、第一组 `0x004147E0`、`0x00414B60`；
6. service `0x48` 为零且 runtime bit 0 为零时，严格执行
   `0x004161C0 → 0x00416590 → 0x004167B0 → 0x00415B70 → 0x004163C0 →
   0x00416CC0`；
7. service `0x48` 为零时执行 `0x00416B30`；
8. 无条件执行第二组 `0x004147E0`。

三个主体分支会合后，`0x00412B28` 先恢复全屏 clip，再依次执行：

```text
00414E50
0042ED40
00414CE0
条件 004117F0(636, 460, 0, global_integer_0)
条件 004149B0
004146F0(1)
尾跳 004153D0
```

数字绘制要求 talk target 为 `0xFFFF` 或 talk phase 小于 8，并且 service `0x51` 为零。
`0x004149B0` 在 service `0x0A/9/0x51` 全部为零时无条件执行；任一非零时，只有 control
`0x2E` 活跃才执行。service 查询的短路顺序也是状态机的一部分，不能预先缓存或合并。

## 4. 四种地图底图路径

`[0x004B7938] == 16` 选择 16 位 CM，否则选择 8 位索引 CM；相机 x、y 的低四位都为
零时走 aligned 版本，否则走 clipped-edge 版本：

| 像素源 | aligned | unaligned | tile 物理布局 |
|---|---:|---:|---|
| 16 位 direct | `0x00412BE0` | `0x00412D30` | `tile_index * 0x200`，16×16 个 little-endian `u16` |
| 8 位 indexed | `0x00413220` | `0x00413370` | `0x200 + tile_index * 0x100`，16×16 个 palette index |

8 位路径使用的 256 项 palette 来自同一 CM 映射的首 `0x200` 字节：地图加载调用者
`0x0040C6F3..0x0040C726` 复制该前缀并对 256 个 RGB555 `u16` 执行 forward 像素转换，
`0x00412A42..0x00412A47` 再把转换后的缓冲区临时接到绘制器。它不是另一个资源文件；
tile 数据之所以从 `+0x200` 起，正是为了跳过这段内嵌 palette。

四条路径共同读取：

- `[0x004B7948]` 的每格 32 位 flags；bit `0x08000000` 跳过整格，bit `0x04000000`
  选择透明写入；
- `[0x004B794C]` 的每格 16 位 tile index，并叠加 `[0x004B873C]` 的 layer offset；
- 16 位 opaque 由 `0x004174D0` 写满 16×16，transparent 由 `0x00417530` 跳过
  `[0x004CD784]` 的低 16 位 color key；
- 8 位 opaque 由 `0x004175B0` 映射全部 index，transparent 由 `0x00417650` 只跳过
  **index 1**，不是跳过 palette[1] 的颜色值。

aligned 全屏恰为 40×30 tiles。unaligned 版本把边缘交给裁剪 blitter、内部 tile 直接写入；
OpenSWD3 在有效状态下以共同的逐像素坐标映射得到相同最终 framebuffer，同时保留以上
flags、透明键、索引 1 和 layer offset 规则。

底图函数内部的 service `0x13` 局部区域与外层 clip 不是同一个计算：四条路径都先执行
`(focus + 15) & ~15`，再向两侧各扩 192 像素。因此未对齐 focus 会形成“外层精确
focus clip”和“底图 16 对齐 tile 区域”两套边界；实现与 UT 分别保留二者。

## 5. 实现与验证

- `render_legacy_world_background`：映射四条底图路径的共同最终像素结果；
- `compose_legacy_world_frame`：映射 `0x00412930` 的 clip、清屏、service/control 短路、
  三条主体路径和全部绘制阶段顺序；
- stage port 为已有 B4/B6 renderer 和尚待恢复的调用保留原位置；实际 runtime adapter
  已在原槽接入 `0x004151F0`、`0x00413EA0`、`0x00413870`、两处 `0x004147E0`、`0x00414B60` 和
  `0x00414CE0`，以及 normal 路径连续的 `0x004161C0`、`0x00416590`、`0x004167B0`、
  `0x00415B70`、`0x004163C0`、`0x00416CC0`、`0x00416B30`，以及公共尾部的
  `0x00414E50`、`0x004146F0`、`0x004153D0`，以及软件鼠标/右边条
  `0x004149B0`。normal 路径只剩 `0x0042ED40` 一个 stage 继续显式转交，activity 分支另有
  `0x004154A0` 一个外部边界；不把空 stage 误报成已经绘制；
- stage 端口会报告成功与失败；现代受检失败停在原调用点、恢复全屏 clip，并返回
  `stage_failed`，正常资产下不改变汇编顺序；
- 正常、局部 clip、service `0x13/0x48/0x0B/0x51`、clear-only、ANI activity、
  control `0x2E`、talk target/phase 的无符号门和无效源隔离均有独立 UT；
- 当前游戏数据地图 24 的 `LMF → CM → frame composition` RGB565 逻辑 framebuffer
  FNV-1a64 固定为 `0x947C15A53487BF9A`。

空间、图片动作、两条 `0xB4` 动作链、七个环境效果、三个公共尾部和软件鼠标 runtime
接线后，真实 TSW 路径叠加底图的 framebuffer 哈希在对应 service 全关闭时为
`0x5889E0547682E179`。这同时证明禁用门和空队列没有引入像素副作用；service 6/7 的
合成集成另行验证实际效果路径。真实资产结果证明当前数据链可用；
Linux Clang `core` 185/185、Linux Clang `app` 190/190、Windows LLVM `app` 190/190
CTest 通过。但尚未与原程序逐帧
framebuffer 差分，所以验证等级不能写成 `original_diff_verified`。

正常原始资产不会触发现代受检错误。短 tile/grid/palette 或不可能的 framebuffer 几何
只在访问前返回并恢复全屏 clip，用于隔离旧程序会发生的越界/无效指针行为；这不改写
任何有效游戏状态下的逻辑分支。

## 6. `sub_412930` 独立闭环复核

本轮重新以 `0x00412930..0x00412BD3` 为完整物理范围，不继承此前纵向切片的完成结论。
函数没有参数，也不建立栈帧；正常主体保存并恢复 EBX/EDI，ANI activity 分支不经过这组
保存。末尾以尾调用转入 `sub_4153D0`，四个直接调用者均不读取返回 EAX：

| 调用点 | 原职责 | 当前归属 |
|---|---|---|
| `0x00407AC9` | 持久化截图/预览重组 | 转交 B11 持久化 owner |
| `0x004126B8` | 普通世界每帧组合 | SDL 世界 runtime 已接线 |
| `0x004155BC` | ANI 收尾时临时关闭 activity/bit 0 后递归重绘 | `LegacyAniActivityPorts::redraw_scene_without_ani` 边界 |
| `0x0044D929` | 特殊模式转场前世界帧捕获 | 转交 B9 特殊模式 owner |

收敛复核反复执行：

- LST→C++：逐块核对初始 full/partial clip、activity/clear-only/normal 三分支、两次可能
  清屏、四底图选择、全部 service/control 短路、十九个 normal stage 和公共尾部；
- C++→LST：从每次 query、stage、clip、clear、数字绘制和背景选择反查唯一原地址，确认
  service `0x48/0x13` 的两组查询没有缓存，service `0x0A/9/0x51` 保持短路顺序；
- 调用边界：`sub_416FF0` 的 clip 映射为 raster owner，临时 palette 指针映射为受生命周期
  保护的 background span，固定 `0x25800` dword 清屏映射为 `0x4B000` 个 u16；
- 独立分支测试：除既有三主体和 service/control 用例外，新增 talk target 非 `0xFFFF`
  时 phase 7/8 的边界，固定 phase 8 在 service `0x51` 查询前跳过数字绘制。

反向追溯最终没有产生新的有效域差异或未决基本块。被调函数仍按各自 B7/B8/B9/B11
范围独立审计；上述跨模块调用点转交不把其外围职责伪装为本函数已经实现。
