# P4 原程序动态 oracle 捕获协议

状态：静态捕获点、产物合同和当前运行环境能力已闭环；实际原程序动态样本尚未捕获，不能标记为已验证结果。

## 原则

完整汇编仍是唯一逻辑真值。动态 oracle 的用途是：

1. 给已经从汇编恢复的整数规格建立回归样本；
2. 观察汇编调用之外的旧平台行为，例如 GDI 字体栅格、DirectDraw RECT 和 `timeGetTime` 量化；
3. 在未来 C++20 重写中做逐字节/逐像素差分。

动态结果不能覆盖汇编。例如某个现代兼容层恰好把 `{0,0,639,479}` 当成 640×480，也不能据此修改原 EXE 实际传入的四个整数；只能把该结果记录为“这个环境的 DirectDraw 后端行为”。

本协议只定义研究捕获，不创建重写工程，不修改 `swd3.exe`，不修复游戏 BUG。

## 当前环境结论

2026-08-02 对当前工作区做了只读能力盘点：

- 没有可执行的 Wine/Wine64/WineDbg；
- 没有可执行的 GDB/LLDB 或 Windows 调试器；
- 没有既有 BMP/PNG/log 动态捕获产物；
- 原 EXE、`binkw32.dll`、Miles DLL/ASI、当前 `Env.dat` 和视频资产齐全；
- `swd3.exe` 是固定 `ImageBase=0x00400000` 的 PE32，COFF 标记 relocations stripped，Base Relocation Directory 为空，`DllCharacteristics=0`。

所以本工作区现在能完成捕获规格和静态探针复核，却不能诚实地产出“原程序已经运行得到”的像素、字形或时钟样本。实际捕获需要一套可运行原 EXE 的 32 位 Windows/旧系统 VM 或另行提供的 Windows 调试执行后端。缺少执行后端不阻止后续静态逆向继续进行。

固定地址和运行文件哈希见 `../inventory/p4-oracle-runtime-baseline.tsv`。生成器同时解析 PE 头验证无重定位/无动态基址条件，因此表中地址可以直接用作该 EXE 版本的调试断点。

## 捕获层级

应保留两类环境，不能混写为一种权威样本：

### A：旧参考环境

尽可能使用原程序年代接近、能原生执行 DirectDraw/GDI/DirectInput 的 32 位 Windows VM，原样使用锁定的 EXE、DLL、资源和字体。这一层用于建立主要像素、glyph、Bink RECT 和时钟观察样本。

### B：现代兼容环境

使用目标新系统与必要的最小兼容外壳，记录相同探针。它用于定位“阻断启动/新系统兼容”的差异，不能反过来定义旧环境像素真值。任何 wrapper、系统补丁、显示缩放、字体替换都必须进入 run manifest。

两层都不得替换游戏逻辑、资源解释、随机调用顺序或帧内顺序。

## 每次运行的 manifest

每次捕获必须先记录：

- `swd3.exe`、`binkw32.dll`、`Mss32.dll`、`Mp3dec.asi`、`Env.dat`、所用存档和所用 Bink 的 SHA-256；
- Windows 版本、位数、VM/兼容层版本、CPU 架构和区域设置；
- DirectDraw 后端、显卡/虚拟显卡、色深、桌面/独占模式和任何缩放；
- GDI 实际解析出的 face、字体文件及哈希、字体版本、charset、quality 和三个 renderer size；
- 是否存在任何 wrapper、注入器、调试器脚本或 API hook，并保存其哈希；
- 当前工作目录以及 `ScrnShot`、`c:\snap` 等输出目录是否预先存在。

只写“Windows 10”或“使用細明體”不够。字体和旧 DirectDraw 都是宿主可变输入；不记录具体文件/后端就无法解释差异。

## 精确 16 位 framebuffer

### 主 oracle

普通世界、特殊模式和战斗的代表性最终提交点分别是：

```text
0x00412716  ordinary world
0x0043A854  special/menu mode
0x0045350A  battle
```

在 DirectDraw Blt 调用之前，保存：

```text
frame pointer = *(u32 *)0x004CD76C
pitch bytes   = *(u32 *)0x004A0E74
width/height  = 640/480 and the queried geometry globals
R/G/B masks   = 0x004CDE4C / 0x004CDE50 / 0x004CDE1C
```

每个选定帧生成两份数据：

1. `framebuffer-physical.bin`：`pitch×480` 字节，保留行 padding；
2. `framebuffer-logical.bin`：逐行取前 `640×2` 字节拼接，正好 `0x96000` 字节。

原程序有 19 条路径在 Lock 后发布 `0x004CD76C`、随即 Unlock、再继续使用地址。捕获工具只能读取这条既有地址，不能为了方便改变 Lock/Unlock 时序。

### 内置 P 截图不是唯一 oracle

WndProc 在 `0x0040A285` 检查大写 `P`，通过现有状态门后寻找：

```text
%sScrnShot\%05d.bmp
```

它不会在已检查的分支中创建 `ScrnShot` 目录。`sub_4303D0` 收到 `0x004CD76C,640,480,path` 后：

1. 无视实际 pitch，先连续复制 `width×height×2` 字节到私有堆缓冲；
2. 调用 `sub_4238D0`，按当前 reverse converter 把私有副本变成规范 RGB555；
3. 从最后一行向第一行写 24 位 bottom-up BMP；
4. 每个五位通道只左移三位到 8 位，不做低位复制或 gamma 处理；
5. 每 15 行调用一次 `AIL_serve`。

因此必须在 `0x00430490` 调用 reverse converter 之前先保存 `builtin-preconvert.bin`，在 `0x00430495` 再保存 `builtin-rgb555.bin`，最后保存原 BMP。BMP 已经过 reverse conversion 和 5→8 位展开，可能丢失原 surface 的收窄位，不能代替原始 16 位逻辑缓冲。

若实际 pitch 不是 1280，必须同时记录内置连续复制结果和按 pitch 提取的逻辑行；两者的差异本身就是原行为证据，不能替截图函数“修正”。

## GDI glyph-mask oracle

跨平台不确定性发生在 GDI 产生临时字形像素之前；后续“任意非零 16 位像素变成一位 mask”和五种软件 footprint 已由汇编固定。

缓存 miss 时，`sub_436AD0` 先由 `sub_4369C0` 分配/插入槽，然后调用：

```text
0x004368D0 sub_4368D0(renderer, string, destination_mask)
```

在函数入口记录：

```text
ECX       = renderer object
[ESP+4]   = raw one/two-byte CP950 string pointer
[ESP+8]   = destination mask slot
width     = [renderer+0xFD0]
height    = [renderer+0xFD4]
row bytes = [renderer+0x1C]
```

在唯一返回点 `0x00436974` 前保存 `height×row_bytes` 字节。mask 是 MSB-first；每个 GDI 临时像素只判断是否为零，不保留灰度。

首轮至少覆盖：

- 三个 renderer size 20/16/12；
- ASCII 半宽和 CP950 双字节各一组；
- 菜单、存档、地图和战斗选定画面实际出现的所有 cache miss；
- 能触发五种 footprint 的文字调用环境。

如果同一字符在旧参考环境多次生成的 mask 不同，先检查字体文件、系统 hinting、surface 像素格式和 renderer 配置，不能在重写中用模糊容差掩盖。

## Bink RECT oracle

`sub_484650` 在 `0x00484680` 调用原 `BinkCopyToBuffer`，目标是 `0x004CD76C`，pitch 来自 `0x004A0E74`，视频按宽高居中。`sub_484950` 随后构造：

```text
left=0, top=0, right=639, bottom=479
```

并在 `0x00484A11` 把同一 RECT 同时作为 source 和 destination，flags 为 `DDBLT_WAIT`。Blt 的 HRESULT 会被检查。

必须保存两层：

1. `0x00484686`：`BinkCopyToBuffer` 返回后的软件 framebuffer 和 Bink 对象帧字段；
2. `0x00484A14`：Blt 返回后的 primary 可见 640×480 像素及 HRESULT。

比较右列 `x=639` 和末行 `y=479`，才能回答旧 DirectDraw/Bink 组合是否把调用值解释成排他边界。外部桌面截图若经过缩放或色彩转换，只能作辅助；主要 post-Blt 样本应来自 primary surface 的原始 16 位像素。

首选确定性资产候选为锁定的 `Video/opening.bik`，当前 SHA-256 已写入 baseline。至少捕获首帧、一个中间帧和末帧附近；每份产物必须记录 Bink 对象中实际观察到的帧字段原值，不凭字段名猜编号。

## 时间与输入可重放 trace

三个 `timeGetTime` 调用点必须按调用顺序分别记录：

```text
0x0040A5A3  main-frame attempt
0x00411958  CD/file polling
0x00411C10  success hold loop
```

每条记录保存 callsite、全局序号、返回 `u32` 和可选的宿主高精度旁路时间。旁路时间只解释宿主量化，不能进入游戏兼容核心。

主帧 interval 门接受后，在 `0x0040A5D1` 开始一个 accepted-frame 序号。随后按汇编顺序记录：

1. `0x0040A749` 前的 `0x004B8748[256]` 键盘快照；
2. `0x00437334` 的栈上 `0x1C` 字节原始鼠标样本；
3. `0x0040A74E` 的 `0x004B7CB0[20×16]` 归一化记录和逻辑鼠标状态；
4. 该 accepted frame 的最终 framebuffer hash。

确定性重放不是“尽量睡到同一毫秒”，而是把记录的 `u32 timeGetTime` 返回序列、键盘和鼠标样本按原调用顺序注入。这样才能复现 interval 拒绝、严格 `>150`、消费者 repeat 和帧内输入顺序。

旧系统未调用 `timeBeginPeriod` 时的量化只作为 run manifest 的环境观察；它不是把游戏时钟改成浮点或宿主刷新率的理由。

## 推荐场景集

每个场景必须固定起始存档/资源哈希、初始输入状态、完整 time/input trace 和捕获帧序号：

| 场景 | 主要目标 |
|---|---|
| O1 普通世界静止帧 | 地图、角色、文字、普通 world precommit 与内置 P 截图一致性 |
| O2 菜单/存档页 | 字号 20/16/12、ASCII/CP950、背景填充和多种文字 footprint |
| O3 战斗帧 | `0x3C000` 前缀颜色处理、战斗动态提交前的完整逻辑缓冲 |
| O4 Bink opening | decoder 输出、居中、`{0,0,639,479}` source/destination RECT 的末行末列 |
| O5 输入边沿序列 | 新按、持续、两次松开、150 边界、菜单 repeat、鼠标 12 次抑制 |
| O6 时间门序列 | interval 0/35/70、同毫秒调用、u32 回绕附近的注入重放 |

O5/O6 的强制输入/时钟是研究回放，不是修改正式游戏。强制运行必须另存 manifest，不能与自然运行捕获混为一组。

## 验证与验收

每个二进制产物保存 SHA-256。比较顺序固定为：

1. 文件长度、pitch、mask 和帧序号；
2. 整体 SHA-256；
3. 首个不同字节/像素；
4. 不同像素数量与最小包围矩形；
5. 对 packed-16 差异按原 mask 分通道报告。

不先做模糊图片相似度。只有确认差异来自已记录的宿主字体或呈现层时，才可增加辅助可视报告；兼容核心仍以原 packed-16 结果和原控制流为准。

## 产物与当前缺口

- 捕获点：`../inventory/p4-oracle-capture-points.tsv`
- 产物合同：`../inventory/p4-oracle-artifacts.tsv`
- 固定运行基线：`../inventory/p4-oracle-runtime-baseline.tsv`
- 哈希/地址/PE 校验生成器：`../../tools/build_p4_oracle_inventory.py`

当前缺口只有实际执行所得样本和宿主 manifest。当前目录尚无任何伪造占位的 `.bin/.bmp/.tsv` 动态结果；在取得可运行旧 EXE 的 Windows 后端之前，P4 动态验证保持待完成。
