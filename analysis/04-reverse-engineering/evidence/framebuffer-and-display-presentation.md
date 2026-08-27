# 帧缓冲、DirectDraw surface 与显示提交

## 证据边界

本结论只以 `swd3.exe_export_for_ai/swd3.exe.lst` 的完整反汇编为逻辑真值。LST 中的机器码字节和指令是唯一真实；IDA 伪码只可用于定位和临时命名，不能覆盖指令、参数、分支或内存访问。

本轮锁定的是软件画面从内存到显示器的物理边界。当前 EXE 不是用 DirectDraw/GPU 逐精灵渲染，而是先在一块 `640×480`、16 位的游戏 surface 上完成软件绘制，再通过 DirectDraw `Blt` 把整幅或局部画面提交到 primary surface。现代重写可以用 SDL3 texture/upload/present 替换 DirectDraw，但不能改变兼容核心产生的 16 位像素、pitch 规则、提交位置和分支时序。

## 当前启动只进入独占全屏路径

`WinMain` 在 `0x00409F96–0x00409FC4` 创建固定窗口：

- `dwExStyle = 0x00040000`；
- `dwStyle = 0x86000000`；
- 初始位置 `(-1800, 0)`；
- 大小 `640×480`。

完整汇编中 `sub_437570` 只有一个调用点。`0x00424F9B–0x00424FB2` 的实参严格为：

```text
sub_437570(hwnd, 0x4E22, 640, 480, 16)
```

包装器在 `0x00437598` 比较 magic：

- `0x4E21` 会选择 cooperative flags `0x08`；
- 当前唯一实参 `0x4E22` 选择 flags `0x13`。

随后无条件调用 `SetDisplayMode(640,480,16,0,1)`。因此当前 EXE 的实际启动合同是旧式独占全屏，不是包装器普通/窗口化分支。

包装器确实保留一条 `flags == 0x08` 的备选路径：它以 `GetSystemMetrics` 的屏幕尺寸创建 raw caps `0x2840` 的 `+0x08` surface。但当前完整汇编没有能选择这条路径的初始化调用。它只能登记为物理存在但在当前程序启动链不可达的代码，不能据此声称原程序正式支持窗口模式。

## surface 拓扑

| 所有者/位置 | 物理对象 | 当前用途 |
|---|---|---|
| wrapper `+0x00` | DirectDraw 活动接口 | cooperative level、显示模式和对象生命周期 |
| wrapper `+0x04` | raw caps `0x0200` surface | primary surface；`sub_437DF0(0x2711)` 的唯一当前选择 |
| wrapper `+0x08` | raw caps `0x2840` surface | 仅普通 cooperative 分支创建；当前启动不可达 |
| wrapper `+0x10` | HWND clipper | 创建后绑定到 primary surface |
| `0x004ACBA0` | `640×480`、desc flags `7`、raw caps `0x2840` surface | 绝大多数软件绘制与主画面提交的源 |
| `0x004CD76C` | 从 Lock 返回后发布的像素地址 | 软件 writer、截图和视频拷贝使用的全局帧地址 |

`sub_437DF0` 只把 `0x2711/0x2712/0x2713` 映射到 wrapper `+0x04/+0x08/+0x0C`。完整汇编中的 27 个直接调用全部传入 `0x2711`，所以当前正常显示目标始终是 primary surface。

`0x004ACBA0` 由 `sub_437B60(640,480)` 唯一生产。创建结果没有在写入全局后立即检查，这是原始失败边界；初步重写不能虚构另一块备用画面或改变正常路径的像素语义。

## 画布、pitch 与行地址

静态初值为：

```text
0x004A0E74 = 0x500  // pitch bytes = 1280
0x004A0E78 = 0x280  // width = 640
0x004A0E7C = 0x1E0  // height = 480
```

`sub_423400` 实际 Lock surface 后，以描述结构报告值覆盖这三个全局。`sub_416D30` 再按实际 byte pitch 逐行累加，构造软件绘制使用的行偏移表。因此不能把所有行寻址统一改写成 `y*640`。

与此同时，原程序还存在明确的固定画布操作：

- `0x96000 = 640×480×2` 字节；
- `0x4B000 = 640×480` 个 16 位像素；
- `0x25800` 个 dword 正好清除 `640×480×2` 字节；
- 个别像素效果硬编码 `±0x500` 字节访问相邻行。

重写合同必须同时保留“按测得 pitch 走行”和“按固定 640×480 常量操作”的原始区别，不能用一个看似更整洁的抽象抹平。

## Lock 后立即 Unlock 的旧行为

`sub_416F10` 调用 surface 虚表 `+0x64` Lock，返回 `lpSurface`，并把 `lPitch >> 1` 写入 `0x004CDE2C`。完整汇编中没有消费者读取这个 pitch shadow。`sub_416F60` 则把同一个像素地址传给虚表 `+0x80` Unlock。

完整汇编共有 21 对这两个包装函数的调用：

- 19 对把 Lock 返回值写入 `0x004CD76C`，随后立即 Unlock，之后的软件绘制仍通过已发布地址写像素；
- `sub_42ED40` 和字体路径 `sub_436840` 是两个局部 read/copy 例外，不发布全局地址。

所以原调用顺序不是“Lock 覆盖整个软件绘制阶段”，而是：

```text
Lock → 发布像素地址 → Unlock → 软件 writer 继续使用该地址 → Blt
```

大多数调用点也不检查 Lock 失败；失败时可能发布零地址并继续把返回值交给 Unlock。这是旧平台对象和内存稳定性假设造成的危险实现细节。

跨平台重写不需要制造失效指针或依赖 unlock 后地址仍有效。允许的兼容性适配是：由平台无关兼容核心持有生命周期稳定、pitch 明确的 16 位 framebuffer，让同样的软件算法读写它，再由 SDL3 上传。必须保留的是同一帧内的像素结果、行跨度和提交时机，不是 DirectDraw 的悬空地址风险。该调整只为启动和新系统兼容性服务，不构成游戏逻辑 BUG 修复。

## 32 个 Blt 与 21 个主画面提交

完整汇编中满足 surface 虚表 `+0x14` 形式的 `Blt` 恰好有 32 个。它们包括主画面提交、surface 间快照、战斗过渡、颜色填充和普通 cooperative 分支的恢复拷贝。

其中写向 primary surface、对游戏画面可见的提交恰好有 21 个：

- 18 个 source/destination RECT 都为 NULL，即全 surface 提交；
- 2 个是战斗纵向位移使用的动态矩形；
- 1 个 Bink 提交把同一个局部 RECT 同时作为源、目标矩形，其四个值严格为 `{0,0,639,479}`。

21 个调用中：

- 9 个使用 `DDBLT_WAIT (0x01000000)`；
- 12 个 flags 为零；
- 除 Bink 外，正常画面提交均不根据 HRESULT 改变后续逻辑；
- Bink 在 `0x00484A14` 后显式分派多个 `DDERR_*`，成功才推进视频链。

`{0,0,639,479}` 必须先按原始参数值保存；不能擅自改成 `{0,0,640,480}`，也不能在尚未做像素 oracle 前用“看起来像右下边界错误”解释并修复。

过去列出的 7 个地址只是顶层稳态分支中的典型提交：高优先级、暂停、普通世界、特殊模式组、商店/特殊模式二、战斗和 Bink。它们不是完整清单。另 14 个主画面提交分布在存取档 UI、媒体检查、剧情 VM、场景清屏以及战斗转场中。逐地址、source、RECT、flags 和返回值策略见 `inventory/primary-presentation-paths.tsv`。

这证明原程序没有统一的“所有逻辑完成后只 present 一次”合同。初步重写即使对外提供统一 SDL3 present 接口，也必须由原分支在原位置请求提交；不得把 21 个边界合并成固定帧尾一次提交。

## 战斗纵向位移的两次提交

`sub_45E7D0` 不是普通整帧 present。它以 `0x0053BD60` 为状态索引读取 `0x004A75EC` 的有符号 byte 表，表的 16 个物理值为：

```text
4,3,4,3,4,3,2,3,2,3,2,1,2,1,2,0
```

每次调用的可见操作顺序是：

1. 依据状态奇偶构造源/目标矩形，把主图像向上或向下错开指定行数，并以 `DDBLT_WAIT` 做第一次 Blt。
2. 从 `0x004CD76C` 起清零 `offset×1280` 字节；这里明确硬编码 1280-byte 行宽。
3. 构造暴露带矩形，再以 `DDBLT_WAIT` 做第二次 Blt，把刚清零的顶部行送到顶部或底部空带。
4. 按 `0x0053BD64/0x0053BD68` 的节拍推进；若 `0x0053BC24 & 0x100`，状态在 0/1 间切换，否则递增；状态到 10 时把 `0x0053C008` 和状态同时清零。

两个 Blt 之间的清零位置、固定 byte 数、状态表和推进顺序共同决定原战斗抖动/位移结果。不能把它近似成一次 GPU shader 位移，也不能为减少提交而合并步骤。

该函数现已关闭为typed纵向位移：三次live表读取、两组完整destination/source矩形、等待标志、dword粒度framebuffer前缀清零、call后节拍重读和phase 10返回后清门均由独立证据与测试锁定，逐帧caller不再经过旧opaque分支。

## 暂停与 Bink 对帧地址的依赖

F8 暂停函数 `sub_411FA0` 不重新 Lock 游戏 surface。它直接在当前 `0x004CD76C` 指向的既有画面上反复画暂停文字，再以 flags 零提交。

Bink 单帧路径同样使用既有帧地址：把 `0x004CD76C`、测得的 `0x004A0E74` pitch、固定高度 480 交给视频拷贝，再按 `{0,0,639,479}` 提交。现代 owned framebuffer 必须跨这些分支保持稳定，不能把它做成只在普通世界 update 函数栈内短暂存在的内存。

## 停用、恢复和恢复循环

停用路径 `sub_40AB50(0)` 的可见顺序仍以既有生命周期证据为准：处理活动位、音频、三套字体 renderer 和战斗显示状态，写 active=0 与 suppression=1，然后 `ShowWindow(SW_MINIMIZE=6)`。它不释放 DirectDraw 对象，也不调用 DirectInput wrapper。

恢复路径 `sub_40AB50(1)`：

1. `ShowWindow(SW_RESTORE=9)`；
2. `SetWindowPos(0,0,640,480)`；
3. 恢复 primary 和 `0x004ACBA0`；
4. 重建行偏移、三个字体 renderer 和必要的战斗状态；
5. 循环检查显示 surface，直到与 `0x2711` selector 一致。

恢复过程中不会再次调用 `SetCooperativeLevel` 或 `SetDisplayMode`。

`sub_437AE0` 的当前 flags bit 0 路径检查/恢复 primary，调用 `WaitForVerticalBlank`，再调用 `Flip(NULL, wait)`。不可达的普通 cooperative 路径则反复 Blt primary `← +0x08`；所有错误都会继续重试，只有 `DDERR_SURFACELOST` 会先恢复两个 surface。这个循环是旧 DirectDraw 恢复机制，不是正常逐帧提交函数。

SDL3 后端不必复制 surface-lost HRESULT 或无限重试 API，但窗口失焦期间停止正常帧、恢复后 framebuffer/字体/战斗提交重新可用的游戏可见边界必须保持。`sub_40AB50` 不重建 DirectInput；输入设备失败和旧状态保留要按输入状态专项另行固定。平台恢复失败应由兼容外壳报告，不能悄悄推进一帧游戏逻辑。

## 释放顺序

关闭时先释放 `0x004ACBA0`，再按包装对象路径释放辅助 surface、clipper、primary；若 cooperative flags 含 `0x10`，调用 `RestoreDisplayMode`，最后释放 DirectDraw。原全局并非全部在退出前清零。

现代 RAII 可以替代 COM 手工 Release，但初步还原期仍应把关闭触发、游戏逻辑停止和平台资源销毁分成同样的可观察阶段，避免析构副作用提前改变原退出链。

## 对 C++20、CMake 与 SDL3 重写的约束

建议边界保持此前技术选型，不引入 DirectX 依赖：

```text
兼容核心（C++20）
  固定逻辑坐标 640×480
  + 明确 pitch 的 16 位 owned framebuffer
  + 原 blitter / 字体 / 视频拷贝 / 分支提交顺序
                  |
                  v
平台呈现层（SDL3）
  上传 16 位结果或做严格、可验证的格式展开
  + 窗口/全屏、缩放、显示器和事件
  + 不参与游戏像素算法
```

SDL3 在 Windows、Linux、macOS 等平台使用各自可用的系统显示后端；兼容核心不需要知道底层是 Direct3D、Metal、Vulkan、Wayland/X11 或其他实现。这里选择 SDL3 的原因正是把这些差异留在平台层，而不是让重写代码绑定 DirectX 系列。

初步实现合同应明确：

- C++20、CMake、Ninja；MSVC 或 LLVM/Clang 都能编译，不依赖 Visual Studio 工程文件。
- 逻辑 framebuffer 固定为 `640×480`、16 位 packed pixel；实际 RGB mask/转换按已恢复的像素格式规则处理。
- software writer 必须接收显式 pitch，且保留原本硬编码整帧/行距的特殊路径。
- SDL window 大小、桌面缩放和全屏方式属于平台策略；不得反向改变逻辑坐标、裁剪、输入映射或一帧内提交顺序。
- 窗口化可以作为现代平台能力提供，但必须标为外壳扩展；当前 EXE 的原始基线仍是独占 `640×480×16`。
- DirectDraw `DDBLT_WAIT` 和被忽略 HRESULT 不必逐 API 模拟，但其对应提交点的同步可见顺序必须进入帧级 oracle。
- 不修复游戏逻辑 BUG；只有为启动和新系统兼容性所必需的平台危险实现可以隔离替换，且必须证明不改变正常路径结果。

## 必须保留的 1:1 行为

- 当前启动链唯一的 `0x4E22`、`640×480×16` 独占全屏基线。
- primary、游戏软件 surface、快照/临时 surface 的源目标关系。
- 实测 pitch/宽/高与硬编码 640×480 操作并存的差异。
- 19 条全局帧地址刷新路径，以及暂停/Bink 使用既有稳定帧内容的生命周期语义。
- 32 个 Blt 的角色划分和 21 个主画面提交边界。
- 每个提交的 NULL/dynamic/fixed RECT、flags 与返回值处理差异。
- Bink 的原始 `{0,0,639,479}` 源/目标 RECT。
- 战斗纵向位移的两次 Blt、中间清零、状态表和推进顺序。
- 失焦冻结、恢复重建、显示恢复和关闭阶段的游戏可见次序。

## 可复现产物

- 生成器：`tools/build_frame_presentation_inventory.py`
- 21 对 Lock/Unlock：`inventory/frame-surface-lock-pairs.tsv`
- 全部 32 个 DirectDraw Blt：`inventory/directdraw-blt-callsites.tsv`
- 21 个主画面提交：`inventory/primary-presentation-paths.tsv`
- 15 个显示生命周期阶段：`inventory/display-lifecycle-stages.tsv`

生成器锁定原 EXE 和完整汇编 SHA-256、唯一显示初始化调用、窗口/显示参数、21 对 Lock/Unlock、27 个 surface selector、32 个 Blt、Bink 双 RECT 地址、21 个主画面提交以及 `9/12` 的 flags 分布。任一锁定事实变化都会停止生成。
