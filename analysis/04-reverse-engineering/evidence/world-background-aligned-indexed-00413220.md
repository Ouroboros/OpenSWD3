# 8 位索引对齐世界底图入口（`sub_413220`）

状态：`platform_adapted`、`assembly_exact`、`unit_tested`、`asset_verified`、
`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`

本文只以 `swd3.exe.lst` 的 `0x00413220..0x0041336C` 为行为真值。IDA 伪码仅用于
辅助定位，不参与裁决；未对齐入口 `sub_413370` 不从本结论继承关闭状态。

## 1. 范围、ABI 与调用边界

- 函数分配 16 字节局部区并保存 EBX、EBP、ESI、EDI，出口完整恢复。
- 函数体不读取调用者参数。唯一调用点 `0x00412A5F` 压入零并在返回后回收，仍是没有
  业务语义的旧调用槽。
- `sub_412930` 只在输出底图不是 16 位且相机 X/Y 低四位均为零时调用本入口；调用前将
  palette 指针写入 `dword_4CD764`，返回后立即清零。
- 退出 EAX 是循环留下的下边界，唯一调用者不观察。

## 2. 汇编确定的行为

service `0x48/0x13` 维持严格短路：先查 `0x48`，只有零结果才查 `0x13`。普通区域是
`[0, 640) × [0, 480)`；局部区域把 focus X/Y 加 15 后清低四位，再各向前后扩
`0xC0`，形成 24×24 个对齐 tile。

行跨度修正与 `sub_412BE0` 相同：默认 `map_width - 40`，只有 left 非零才补
`(left - right + 640) >> 4`。因此局部 left 恰为零时，24 个 tile 后下一行仍按
`map_width - 16` 前进；该旧特例同样存在于 8 位入口。

首 cell、动画层和 flags 的关系保持：

- cell 为 `((camera_top + top) >> 4) * map_width +
  ((camera_left + left) >> 4)`；
- flags 从该 cell 读取，tile 索引从 `cell + dword_4B873C` 读取；
- hidden 位 `0x08000000` 完全跳过；
- tile 源为 `lpBaseAddress + 0x200 + tile_index * 0x100`，即 16×16 个 palette index；
- opaque cell 调 `sub_4175B0`，每个 index 通过 `dword_4CD764` 的 256 项 u16 palette
  转色；transparent cell 调 `sub_417650`，palette index 1 不写，其余照常转色。

## 3. 现代映射与安全边界

`legacy_world_frame_composition.cpp:compose_legacy_world_frame` 保留本入口自己的 service
查询；`legacy_world_background.cpp:render_legacy_world_background` 的 `indexed_8` 对齐路径
保留 0x200 header、0x100 tile 步长、palette 转色、index-1 透明和动画层偏移。

本轮将已为 16 位入口恢复的 left-zero 行推进特例扩展到 indexed 路径。palette 的旧临时
全局所有权映射为调用期 `std::span<const u16>`；短 palette、tile/grid 或无效 framebuffer
只在裸指针访问前隔离，不改动正常游戏数据的像素语义。

## 4. 双向收敛与测试

LST→C++ 逐块核对 service 短路、focus 对齐、行跨度、首 cell、动画层、hidden 与两个
palette blitter。C++→LST 从每个源字节地址、palette index、透明跳写和目标像素反查，
发现 left-zero 特例此前仅限定于 direct-16；解除错误限定后再次核对，没有新的有效域
差异。

独立测试固定：

- 0x200 indexed header、tile index 与 256 项 u16 palette 转色；
- transparent cell 只跳过 palette index 1，而不是比较转换后的颜色；
- 相机 `(5, 7)` 的 indexed 边缘像素映射；
- 局部 left 为零时第二行按原 `map_width - 16` 位置读取 indexed tile；
- 共享真实地图/CM 管线继续覆盖原始资源装载与 framebuffer 资产基线。

原程序逐帧 framebuffer 差分仍需用户运行 oracle；开发流程不自行启动原版。Linux
Clang `core` 185/185、Linux Clang `app` 190/190、Windows LLVM `app` 190/190
CTest 全部通过，两端应用成功链接且未启动。
