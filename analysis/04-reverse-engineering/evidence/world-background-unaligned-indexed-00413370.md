# 8 位索引未对齐世界底图入口（`sub_413370`）

状态：`platform_adapted`、`assembly_exact`、`unit_tested`、`asset_verified`、
`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`

本文只以 `swd3.exe.lst` 的 `0x00413370..0x0041386E` 为行为真值，并独立追入
`sub_4170E0`、`sub_4175B0`、`sub_417650`。IDA 伪码只用于导航，不参与裁决；相邻的
16 位未对齐入口与 8 位对齐入口只作为完成第一轮 LST 阅读后的交叉检查。

## 1. 范围、ABI 与调用域

- 函数分配 `0x3C` 字节局部区，保存并恢复 EBX、EBP、ESI、EDI；唯一正常出口是
  `0x0041386E`，物理对齐回退在 `0x004133A0` 提前返回。
- 函数体不读取调用者参数。唯一直接调用点 `0x00412A68` 仍先压入零并在公共尾部清栈，
  这是没有业务语义的旧调用槽；调用者不观察 EAX。
- `sub_412930` 先确认底图不是 16 位，并把 palette 指针写入 `dword_4CD764`。只有相机
  X/Y 至少一轴低四位非零时，`0x00412A66..0x00412A68` 才调用本入口；返回后 palette
  指针立即清零。
- 入口 `0x00413383..0x004133A0` 仍物理检查两轴余量；都为零时调用直接色对齐入口
  `sub_412BE0(0)`。但唯一正常调用者已在 `0x00412A4C..0x00412A5F` 把该状态分派给
  `sub_413220`，所以本回退在当前调用域不可达。

现代实现以不可变 `LegacyWorldBackgroundView`/source 快照统一四条底图路径。对齐输入直接
得到统一分派后的 indexed 对齐语义；没有增加只为暴露上述不可达物理分支的公共入口判别器。
这是平台统一分派适配，不把 `sub_412BE0` 的错误布局解释带进正常 indexed 调用域。

## 2. 相机、范围与 service 分派

`0x004133A1..0x00413449` 把相机拆为算术右移四位的 cell 坐标和低四位像素余量。目标
原点是余量负值；`0x004133B4..0x00413404` 将任意负 cell 直接钳到零、对应目标原点只加
16 一次，并把默认 40×30 数量减一，而不是逐个跳过所有负 cell。相机 X=-21 时
`cellX=floor(-21/16)=-2`、余量为 11、初始目标 X=-11，修正后是 `cellX=0`、目标 X=5。
只要余量非零就补一列/行，再由 `dword_4B7930/34` 地图宽高截断。因此右、下地图边界
可以留下不足 640×480 的未覆盖尾条。

`0x0041344D..0x004134A2` 先查询 service `0x48`，仅在它为零时查询 `0x13`：

- `0x48 != 0` 或 `0x13 == 0`：进入 `0x004134A8` 的普通路径，把内部边界设为完整
  `[0,640) × [0,480)`，先绘制顶/底两行，再绘制左/右两列。
- `0x48 == 0` 且 `0x13 != 0`：直接跳到 `0x0041376B`，完全跳过四边。focus X/Y
  各自先加 15 再清低四位，随后向前后扩 `0xC0`；本路径只绘制该 384×384 区域中严格
  位于四条边界以内的完整 tile。

两条路径在 `0x0041376B` 会合：首目标坐标和首 cell 都向右、向下各移一格，横纵循环
分别以 `right - 16`、`bottom - 16` 为严格上界。相机 `(0,7)`、focus `(320,240)` 时，
区域为 `[128,512) × [48,432)`，首写坐标为 `(144,57)`，最终是 22×23 个完整 tile；
负相机 `(-5,7)` 经 cell 钳零后首写 X 为 149，而不是 133。

## 3. cell、源地址与三个 callee 合同

flags 始终来自未偏移 cell 表，tile index 才从 `cell + dword_4B873C` 读取：

- `0x08000000` hidden 在调用任何 blitter 前跳过；
- `0x04000000` 只选择 transparent 分派；
- indexed tile 源是
  `lpBaseAddress + ((tile_index + 2) << 8)`，即
  `lpBaseAddress + 0x200 + tile_index * 0x100`。

普通路径四边的四个调用地址是 `0x00413587/0x004135D9/0x004136D0/0x0041372C`。
它们都调用 `sub_4170E0(x,y,16,16,mode,0)`；mode 为 0 或 4。独立追踪
`0x004170E0..0x004174CA` 确认该通用 blitter 先按当前 raster rectangle 裁掉目标和
对应源偏移，再由非空 `dword_4CD764` 选择 indexed-source/palette 转色写入。因此普通
未对齐路径只有最外圈受 clip 约束，内部不继承该 clip。

内部完整 tile 不经过通用 blitter：

- `sub_4175B0`（`0x004175B0..0x00417640`）固定读取 16 行、每行 16 个 u8 index，逐项
  通过 256 项 u16 palette 写目标，不比较透明键；
- `sub_417650`（`0x00417650..0x004176C4`）同样固定 16×16，但 source index 恰为 1
  时保留目标，其余 index 正常 palette 转色。

这同时证明 service-13 的内部-only 和普通路径的 edge-only clip 与像素布局无关。此前
C++ 把两项行为限定为 `direct_16`，本轮只移除该错误布局条件并把布尔量重命名为布局中性
名称，没有修改 owner、公共 API 或 composition 分派。

## 4. 双向收敛与测试

LST→C++ 依次核对对齐回退、相机拆分、负 cell 修正、地图截断、service 短路、顶底边、
左右边及内部循环；C++→LST 再从每个 cell index、目标坐标、flags、tile layer、源字节和
palette index 反查调用地址。修正 direct-only 条件及普通路径共享的负 cell 原点后，没有发现新的有效调用域差异。

新增 indexed-unaligned 专用单元向量固定：

- 普通相机 `(-21,7)` 时 framebuffer X=4 保持原值，cell 0 从 X=5 开始，首像素读取
  source `(0,7)` 的 index `0x70` 并映射为 palette 值 `0x2070`，固定上述一次性负 cell
  钳位而非跳过两格后的错误 X=21；
- service-13、相机 `(0,7)`、focus `(320,240)` 的首写 `(144,57)`、四条严格边界、
  `22 × 23 = 506` 个现代唯一 cell 和 `129536` 次像素写入；
- 相机 `(-5,7)` 的首目标 X=149；
- 普通相机 `(5,7)` 配窄 edge clip：外圈不写，39×29 个内部 tile 绕过 clip；
- hidden、transparent source index 1、opaque source index 1，以及非零 tile-layer offset
  只偏移 tile index、flags 仍来自原 cell；
- 相机 `(37,39)` 靠近 42×32 地图右下边时只访问 40×30 cell，留下 5×7 像素尾条；
- 短 palette 在访问前返回 `palette_out_of_bounds`，短 indexed tile source 在首个非法
  源字节返回 `tile_source_out_of_bounds`。

`LegacyWorldBackgroundRenderResult` 是现代唯一-cell 观测 API；普通汇编四边遍历会重复
调用角 cell，测试计数不伪装成原程序的重复 edge-call 次数。真实地图 4 的既有
LMF→CM→indexed palette 资产回归继续固定 framebuffer 哈希 `0xF00691829E9FE2D5`，
composition/runtime 路径继续使用同一 renderer。

## 5. 安全适配与动态 blocker

现代 framebuffer 固定验证 640×480 与最小 pitch；地图乘法、tile-layer span、flags
字节、palette 256 项和每个 indexed 源字节都在读取前受检。旧全局 palette/source 和
裸目标指针分别映射为调用期 span 与受检 framebuffer；这些检查只隔离原程序的越界、短源
和无效指针状态，不改变正常游戏数据上的遍历、透明或 palette 语义。

最终门禁为 Linux `core` 185/185、Linux `app` 190/190、Windows LLVM `app` 190/190
CTest 全部通过；Linux 与 Windows 应用均成功链接。验证过程没有启动 OpenSWD3 游戏 EXE
或原版。

原程序逐帧 framebuffer 差分仍需用户运行统一 Frida spawn oracle，因此动态状态保持
`blocked_runtime_oracle`。
