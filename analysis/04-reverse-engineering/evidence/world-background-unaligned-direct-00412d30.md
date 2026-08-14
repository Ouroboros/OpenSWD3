# 16 位未对齐世界底图入口（`sub_412D30`）

状态：`platform_adapted`、`assembly_exact`、`unit_tested`、`asset_verified`、
`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`

本文只以 `swd3.exe.lst` 的 `0x00412D30..0x00413219` 为行为真值。IDA 伪码仅用于
辅助定位，不参与裁决。相邻的 8 位入口 `sub_413220`、`sub_413370` 仍须独立闭环。

## 1. 范围、ABI 与调用边界

- 函数分配 `0x3C` 字节局部区，保存并恢复 EBX、EBP、ESI、EDI；唯一正常出口是
  `0x00413219`，相机重新对齐时从 `0x00412D60` 提前返回。
- 函数体不读取调用者参数。唯一调用点 `0x00412A3B` 仍压入零并由调用者回收，这是没有
  业务语义的旧调用槽。
- 调用者只在 16 位底图且相机 X/Y 至少一轴低四位非零时进入。入口再次检查；若两轴均
  对齐，立即转交已闭环的 `sub_412BE0`。
- 返回 EAX 是最后一次循环留下的边界或宽度值，调用者不观察。

## 2. 汇编确定的几何与分派

入口先将相机拆为算术右移四位的 cell 坐标和低四位像素余量，目标起点是余量的负值。
若 cell X/Y 为负，分别钳到零、目标起点加 16，并把默认 40×30 cell 数减一；只要余量
非零又分别补一列/一行，随后按地图宽高截断。

service `0x48/0x13` 保持短路顺序：

- `0x48 != 0` 或 `0x13 == 0`：使用完整 640×480 区域。先沿顶/底两行遍历，再沿
  左/右两列遍历；这些边缘 cell 通过 `sub_4170E0`，因此受当前 raster clip 约束。
  hidden 位 `0x08000000` 完全跳过，transparent 位 `0x04000000` 转为 blitter mode 4。
- `0x48 == 0` 且 `0x13 != 0`：保留 focus 向上对齐后前后各 `0xC0` 的区域，直接跳过
  上述边缘遍历。

两条路径在 `0x0041311E` 汇合后都把目标 X/Y 起点加 16，并把首 cell 向右、向下各移
一格，再以 `right - 16`、`bottom - 16` 为严格上界绘制内部完整 tile。内部 opaque 与
transparent cell 分别走 `sub_4174D0`、`sub_417530`，不读取 raster clip。

因此局部路径不是“把所有相交 tile 裁到局部矩形”：它只绘制内部完整 tile。即使相机
某一轴已经对齐，该轴也会留下一个完整 tile 的边界；负相机 cell 被钳零后，内部首 tile
还必须使用加 16 后的旧目标原点。

## 3. 现代映射

`legacy_world_frame_composition.cpp:compose_legacy_world_frame` 保留独立 service 查询和调用
时 raster clip；`legacy_world_background.cpp:render_legacy_world_background` 承载最终像素
结果：

- 普通未对齐路径识别最外圈 cell，只对该圈应用 `edge_clip_*`；内部 cell 忽略 clip；
- service-13 局部路径按旧的首 cell、目标原点和严格内边界，只绘制完整内部 tile；
- 两轴重新对齐时落到 16 位对齐语义，包括 `sub_412BE0` 已恢复的 left-zero 行推进特例；
- tile 动画层只偏移 tile 索引，flags、hidden/transparent 和 16 位 color key 保持原位；
- 裸 tile/flags/framebuffer 指针改为有界 span 与 framebuffer。短源、无效几何和真正越界
  相机只在访问前隔离，不改变有效域的分派和像素。

旧 scratch 全局 `dword_4CC2E4`、`dword_4CC2E8` 只在本函数及尚待审计的
`sub_413370` 内部读写，没有跨帧消费者；现代实现将其收束为局部几何量。

## 4. 双向收敛与测试

LST→C++ 按对齐回退、相机拆分、负 cell 调整、宽高截断、service 短路、四边遍历及内部
遍历逐块核对。第一次 C++→LST 反查发现通用 renderer 曾重画 service-13 边缘碎片；
继续追入 `sub_4170E0` 后又发现普通路径只有外圈受 raster clip。两处差异恢复后，再从
每个目标 tile、cell 索引、flags 分派和 clip 交点反查汇编，未发现新的有效域差异。

独立测试固定：

- 相机 `(5, 7)` 时的边缘像素、cell 推进、hidden 与 transparent color key；
- service-13 局部路径不重画边缘碎片，并在单轴对齐时仍保留整 tile 边界；
- 负相机 cell 钳零后使用旧的内部 tile 原点；
- runtime partial clip 只裁普通路径最外圈，内部完整 tile 仍按原逻辑写入；
- 共享真实地图 24/CM/RGB565 路径继续保持已固定的 framebuffer 资产基线。

原程序逐帧 framebuffer 差分仍需用户运行 oracle；开发流程不自行启动原版。Linux
Clang `core` 185/185、Linux Clang `app` 190/190、Windows LLVM `app` 190/190
CTest 全部通过，两端应用成功链接且未启动。
