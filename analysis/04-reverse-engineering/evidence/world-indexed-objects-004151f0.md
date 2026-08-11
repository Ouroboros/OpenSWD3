# 世界地图索引对象（`0x004151F0`）

状态：`assembly_exact`、`asset_verified`、`platform_adapted`；尚未
`original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。伪码只用于定位。

## 1. 装载期记录

`0x00426505..0x0042662F` 为每个 LMF indexed object 分配并清零 `0x20` 字节节点，
再按以下物理布局写入：

| 节点偏移 | 来源 | 含义 |
|---:|---:|---|
| `+0x00` | 解压目标 | 16 位 command stream |
| `+0x04` | LMF `+0x0E` byte | 绘制序号 |
| `+0x06/+0x08` | 初始为 LMF `+0x00/+0x02` 左移 4 位 | 随后由 `0x00401B70` 改写为图像宽高 |
| `+0x0A..+0x10` | LMF `+0x06..+0x0C` 各左移 4 位 | 世界矩形四边，保留 16 位回绕 |
| `+0x16/+0x18` | LMF `+0x0F/+0x10` byte | 水平、垂直位移因子 |
| `+0x1C` | 旧链头 | next |

`0x00426620` 在载入时调用 `0x00401B70` 原地转换 direct-16 literal，并取得真实宽高；
`0x00426625..0x0042662F` 将新节点插到链头。因此 OpenSWD3 保留 LMF 物理顺序存储时，
运行期必须反向遍历才能得到同一链表顺序。

## 2. `sub_4151F0` 绘制顺序

空链直接返回。非空时，外层序号严格扫描 `0..30`；每个序号从链头开始查找，只绘制
第一个与当前视口相交的节点，然后进入下一个序号。

对象矩形为 `(+0x0A, +0x0C, +0x0E + 16, +0x10 + 16)`。`IntersectRect` 成功后，
交集四边减去视口左上角形成 framebuffer 相对 clip，并调用 `0x00416F80`。绘制完成后
始终以 `0x00416FF0(0, 0, 640, 480)` 恢复全屏 clip。

每轴源偏移保留两条汇编路径：

- 因子等于 8：`max(viewport_start - object_start, 0)`；
- 其他因子：`factor * (viewport_end - object_start) / 16`，使用 32 位回绕和朝零截断；
  若视口起点在对象起点之前，再加交集的视口相对起点。

最后把 command stream 写入旧全局源槽，以节点 `+0x06/+0x08` 为宽高，调用
`0x004170E0(relative_left - x_offset, relative_top - y_offset, width, height, 0, 0)`。

## 3. 现代承接边界

OpenSWD3 在 render session 建立时移动解压 payload 所有权并执行同一 literal 转换，
避免保留裸链表和全局源指针。无效 command stream 或分配失败会在现代受检边界停止；
有效游戏数据的字段、遍历顺序、整数运算、clip 和 blitter 参数不变。

`0x00412930` 的原槽已接入该 owner，并共用实际相机视口、framebuffer、pixel
conversion、blit effects 与 row jitter。

## 4. 验证

合成 UT 固定了装载字段、16 位左移回绕、literal 转换、链头反序、`0..30` 扫描、每序号
首个相交对象、两种位移公式、clip 恢复和绘制失败边界。

真实资产使用 `huge.lmf` 地图 72 的第一个 indexed object：

| 项 | 结果 |
|---|---:|
| command stream | `1072x1024x16` |
| packed bytes | `1,790,338` |
| RGB565 转换后 FNV-1a 64 | `a70ae50b232b53de` |
| 实际结果 | 已进入 `0x004151F0` runtime blitter 路径 |

Linux Clang `core` 158/158、Windows LLVM `app` 162/162 CTest 通过。尚未与原程序逐帧
framebuffer 差分，因此不标记为 `original_diff_verified`；需要动态差分时应准备 Frida
spawn 工具并等待用户执行，Codex 不自行启动原版。
