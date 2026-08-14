# 16 位对齐世界底图入口（`sub_412BE0`）

状态：`platform_adapted`、`assembly_exact`、`unit_tested`、`asset_verified`、
`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`

本文只以 `swd3.exe.lst` 的 `0x00412BE0..0x00412D29` 为行为真值。IDA 伪码仅用于
辅助定位，不用于裁决。`sub_412D30`、`sub_413220`、`sub_413370` 仍按各自范围独立审计，
不能从本结论继承关闭状态。

## 1. 范围、ABI 与调用边界

- 物理范围：`0x00412BE0..0x00412D29`；下一函数从 `0x00412D30` 开始。
- 函数分配 16 字节局部区并保存 EBX、EBP、ESI、EDI，所有出口完整恢复。
- 函数体不读取调用者参数。三个调用点都会额外压入零并在返回后回收四字节；这是未被
  函数消费的旧调用槽，不应在现代接口中虚构为有语义的参数。
- 退出时 EAX 是循环留下的下边界值，没有稳定业务含义；三个调用点均不观察它。

直接调用点只有三处：

| 调用点 | 条件 | 作用 |
|---|---|---|
| `0x00412A32` | 16 位底图且相机 X/Y 低四位均为零 | 普通对齐底图路径 |
| `0x00412D51` | `sub_412D30` 入口发现相机重新对齐 | 回退到对齐路径 |
| `0x00413391` | `sub_413370` 入口发现相机重新对齐 | 回退到对齐路径 |

正常 `sub_412930` 分派保证本函数处理的是 16 位、16×16 对齐的有效域；后两个回退分支
保留旧代码的自校正边界，但不改变普通调用条件。

## 2. 汇编确定的执行顺序

1. 初始 tile 区域为 `[0, 640) × [0, 480)`，即 40×30 个完整 tile。
2. 先查询 service `0x48`；仅当结果为零才查询 service `0x13`。若后者非零，分别将
   focus X/Y 加 15 后清除低四位，再形成中心前后各 `0xC0` 像素的 384×384 区域。
3. 行跨度修正默认是 `map_width - 40`。局部区域只有在 `left != 0` 时才加
   `(left - right + 640) >> 4`，通常得到 `map_width - 24`。若局部区域的 left
   恰为零，`test esi, esi` 会保留默认修正，24 个 tile 后下一行只前进
   `map_width - 16`；这是汇编可达的旧行为，不能擅自修正。该修正同时推进 flags 和
   tile 索引指针，动画层偏移仍只作用于 tile 索引的初始地址。
4. 首 cell 是
   `((camera_top + top) >> 4) * map_width + ((camera_left + left) >> 4)`；tile 索引
   指针额外加 `dword_4B873C`，flags 指针不加。
5. 每个 cell 先检查 flags 的 `0x08000000`；命中则完全跳过。否则从 tile 索引取 `u16`，
   乘 `0x200` 得到 16×16×16-bit tile 起点并写入临时源指针。
6. flags 的 `0x04000000` 未设置时调用 `sub_4174D0` 整块复制；设置时调用
   `sub_417530`，只跳过等于 `dword_4CD784` 低 16 位的像素。
7. X/Y 每次严格增加 16；每 cell 的 flags/tile 指针分别增加 4/2 字节。

## 3. 现代映射与安全边界

`legacy_world_frame_composition.cpp:compose_legacy_world_frame` 保留本入口自身的 service
`0x48/0x13` 查询及短路；`legacy_world_background.cpp:render_legacy_world_background` 合并
承载四个旧底图入口的最终像素语义。本函数对应其中 `direct_16` 且相机低四位均为零的
路径：

- `map_width`、`camera_left/top`、`tile_layer_offset` 分别对应旧宽度、相机和动画层全局；
- `cell_flags` 与 `tile_indices` 保持两条独立 span，动画层只加在 tile 索引侧；
- 16 位对齐、局部区域 left 为零时显式保留原 `map_width - 16` 行推进特例；
- `tile_index * 0x200`、hidden/transparent 位和 16 位 color key 均逐项保持；
- 旧 framebuffer base、pitch row table 和两个裸 tile blitter 映射为有界 framebuffer
  逐像素写入，正常资源的最终像素不变；
- 短 grid、短 tile 源、无效几何及越出地图的相机属于旧程序裸指针无效域，现代实现只在
  访问前隔离，不据此改变有效游戏状态的分支。

## 4. 双向收敛记录

LST→C++ 逐块核对了 service 短路、focus 对齐、40/30 与 24/24 两组循环、首 cell、
行跨度修正、动画层偏移、hidden/transparent 分派以及两个 blitter。第一次反向核对
发现 left 为零时的行修正曾被现代坐标循环无意正常化；恢复该特例后再次从每次 cell
访问、tile 字节地址、color-key 跳写和目标像素反查原地址，未发现新的有效域差异。

独立测试固定：

- 40×30 对齐 viewport、tile 行列推进与 16 位像素次序；
- 384×384 局部区域及 focus 向上对齐；
- 局部区域 left 为零时第二行从原 `map_width - 16` 位置继续；
- 动画层只偏移 tile 索引，flags 仍从同一 cell 读取；
- hidden、transparent、opaque 三种分派及精确 written-pixel 数；
- 当前游戏数据地图 24 的 RGB565 framebuffer FNV-1a64
  `0x947C15A53487BF9A`。

反向追溯已没有新的有效域差异或未决基本块。原程序逐帧 framebuffer 差分仍需用户运行
oracle；开发流程不自行启动原版。Linux Clang `core` 185/185、Linux Clang `app`
190/190、Windows LLVM `app` 190/190 CTest 全部通过，两端应用成功链接且未启动。
