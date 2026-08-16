# OpenSWD3 当前工作交接

更新时间：2026-08-16

仓库：`/mnt/e/Game/swd3/OpenSWD3`

本文用于把当前仓库、执行计划、验证基线和精确开发断点交给下一位执行者。它不替代
[`execution-plan-pi.md`](execution-plan-pi.md) 或
[`story-vm-closure-plan-pi.md`](story-vm-closure-plan-pi.md)，也不创造新的完成条件。
在 `pi-execution` 分支，这两份带 `-pi` 后缀的副本是唯一执行权威；不带后缀的副本是
冻结的上游参考，不驱动本分支的执行。

## 1. 接手后的最短路径

按以下顺序读取，不要先凭 README 或旧模块摘要猜测进度：

1. [`AGENTS.md`](../AGENTS.md)：仓库操作约束。
2. [`execution-plan-pi.md`](execution-plan-pi.md)：本分支唯一主执行计划，当前版本 `v213`。
3. [`story-vm-closure-plan-pi.md`](story-vm-closure-plan-pi.md)：主计划挂载的高优先级追加计划。
4. [`world-map-closure.tsv`](../analysis/04-reverse-engineering/inventory/world-map-closure.tsv)：
   当前 B7 的 114 项逐函数闭环真值表。
5. [`world-map.md`](../analysis/04-reverse-engineering/modules/world-map.md)：B7 范围、接口和
   既有证据导航。该文件开头的完成数量已经落后，计数必须以 TSV 和主计划为准。
6. `/mnt/e/Game/swd3/swd3.exe_export_for_ai/swd3.exe.lst`：完整 IDA 反汇编列表，所有行为
   判断的唯一汇编真值。

当前没有 `MEMORY.md` 或其他 MEMORY 文件。不要把聊天摘要、IDA 伪码、函数名或旧说明
当作汇编证据。

## 2. Git 与工作区断点

- 分支：`pi-execution`。
- 远端：`origin`（`git@github.com:Ouroboros/OpenSWD3.git`）。
- 分支创建/Pi 执行框架提交：`5232dad6330ba90cd00400d15063b5e028504d1e`。
- 父提交/分支基线（当时的 `origin/main`）：`da098fa6f0db79e147dfee77d2422e78096537d6`。
- 当前精确 `HEAD` 和工作区洁净状态不冻结在本文中；每个工作包开始时都必须直接从 Git
  重新读取并确认，不能继承上次交接或聊天中的值。
- 预期推送/上游目标为 `origin/pi-execution`；首次阶段推送前由主 Agent 建立该上游关系。

`da098fa` 已关闭 `sub_413220` 的独立审计：8 位 indexed 对齐底图保留原版
left-zero 局部刷新行推进异常，证据、清单、实现和 UT 已一起提交。

本次交接没有启动构建、测试、Windows EXE 或原版；不要据此推断用户机器上不存在其自行
启动的进程。

## 3. 当前目标与计划优先级

总目标是：以完整 LST 汇编为唯一行为真值，使用 C++20、CMake 和 SDL3 重写原 EXE，
继续读取原始游戏资产和存档，并保持 bug-for-bug 的 1:1 可观察行为。

当前主阶段为：

```text
B7.1 · 地图、世界、角色与碰撞最小纵向闭环
```

但当前执行队列受 [`story-vm-closure-plan-pi.md`](story-vm-closure-plan-pi.md) 覆盖：

1. `P0`：有限收口 B7 的固定 114 项全集；当前正在执行。
2. `P1`：锁定完整剧情 VM 工作包，即 198 个显式 opcode、146 个唯一 handler 入口、
   25 个共享入口组、默认非法分支、表外特殊值、窗口切换和公共解释循环。
3. `P2`：按 handler 组边逆向、边实现、边验证；不再按剧情实机命中顺序补单个 opcode。
4. `P3`：全 VM 验收。

P0 完成前不进入 opcode `125+` 的逐值恢复，不回到延期的 `libffmpeg`，不扩大首场战斗。
P0 达到 B7 模块移交条件后必须立即停止扩展 world-map，转入 P1。

## 4. 不可改变的技术与还原决定

### 4.1 行为真值和验证

- LST 中的机器码字节和指令是唯一行为真值。
- IDA 反编译 C 伪码仅用于导航；与 LST 冲突时无条件服从 LST。
- 初步还原不修复游戏逻辑 BUG。只有阻断启动或现代系统兼容的问题允许在平台边界做
  最小、隔离、可验证的适配。
- 每个函数或 handler 都必须反复执行 LST→C++、C++→LST 双向逐基本块追溯，直到完整
  正向和反向核对都不再产生新差异或未决项；固定核对一次、两次或 UT 通过都不够。
- 测试向量必须从汇编比较、跳转、位宽、符号扩展、回绕和调用顺序独立推导，不能由
  当前 C++ 反向制造。
- 任何差异都要同步修正实现、测试、规格、证据和清单，并从函数入口重新核对全部出口。

### 4.2 工程技术

- 语言：C++20。
- 构建：CMake 3.25+；默认 Ninja Multi-Config；不依赖 Visual Studio IDE。
- 编译器：LLVM Clang 或 MSVC。
- 平台边界：SDL3。业务核心不直接依赖 DirectDraw、DirectInput 或其他旧 DirectX API。
- 项目源码、头文件和测试统一四空格缩进，不使用 Tab。
- 游戏内分辨率固定为 `640×480`；SDL3 宿主窗口只做等比缩放和 letterbox。
- 普通窗口大小与 `maximized` 状态保存在 EXE 同目录 `openswd3.toml`。
- SDL3 焦点切换不进入原版“失焦最小化”路径，窗口可以在后台保持运行；失焦时只清输入
  latch。这是已登记的现代窗口兼容隔离。
- 游戏数据目录优先级：`--data-dir` > EXE 同目录 TOML > 启动工作目录。
- 日志位于 EXE 同目录 `logs/openswd3.log`，记录 UTC 毫秒时间、级别、线程、
  `file:line` 和消息，并有 stderr/Windows debugger 回退。

### 4.3 文本、字体和音视频

- 原始剧情与地图文本仍以原字节和字节偏移参与解释；当前游戏资产基线是 Big5/CP950，
  简体资产预计为 GBK/CP936。
- 固定目标是从 TOML 的 `[scripts].encoding` 选择 `big5` 或 `gbk`，在边界解码，公共文本
  内核使用 `char16_t`/UTF-16，不使用平台宽度不同的 `wchar_t`。
- 该 `[scripts].encoding` 配置目前尚未出现在模板或源码中，属于仍需实现的固定需求，
  不能误报完成。
- 原版字体为经典 `細明體`。正式
  `assets/fonts/legacy-glyph-atlas.bin` 长 `3,816,016` 字节，SHA-256 为
  `0a530284a3ff5fa5c426376571bd31acc8c4443f2237526273c5aefe10708df4`。
- atlas 每个 12/16/20 字号覆盖 32,896 个原始字节 key；唯一动态基准来自 Windows 11
  台湾繁体中文、CP950 和正确 `mingliu.ttc` 环境，157 个动态 mask 达到逐字节零差异。
- 压缩音频和 Bink 视频最终统一放入项目自有 `libffmpeg` 动态库；当前只保留接口和 TODO，
  不阻塞 P0/P1/P2。

## 5. 操作规则

- 搜索、阅读、分析、编辑和普通文件操作使用 Bash；优先 `rg`/`rg --files`。
- Windows CMD 只用于 Bash 无法完成的 Windows 专属构建或启动行为。
- 未经用户明确许可，不启动任何 EXE，包括原版和 OpenSWD3；构建与 CTest 可以执行。
- 需要原版动态值时，准备 Frida spawn 一键工具，给出明确操作路径，然后等待用户执行。
  不自行启动原版。
- 统一 oracle 位于 [`analysis/tools/swd3-oracle/`](../analysis/tools/swd3-oracle/)，便携入口
  为 `build/vm/swd3-oracle/swd3-oracle.exe`，当前工具校验并 spawn `swd32.exe`。
- 所有生成文件必须放在 OpenSWD3 内分类保存；构建临时结果放 `build/`，可提交证据放
  `analysis/04-reverse-engineering/`。
- 不提交 `__pycache__`；不要把构建输出、VM 原始输出或用户游戏数据误加入 Git。
- 达到可独立回退的阶段边界后，自动执行“精确暂存本阶段文件 → 使用 `$commit` Skill
  提交 → `git push`”。不能绕过 commit Skill，也不能混入无关改动。
- TG 阶段汇报使用：

  ```bash
  python3 /mnt/d/Dev/Source/Project/stockkit/scripts/tg_notify.py "中文标题

  中文正文分段"
  ```

  使用普通双引号，不使用 `$"..."`，文本必须为中文并保留段落。

  [`execution-plan-pi.md`](execution-plan-pi.md) 中规定的规范命令
  `D:\Dev\Source\Project\stockkit\scripts\tg_notify.py "CONTENT"` 与上述命令是同一入口；
  `python3 /mnt/d/Dev/Source/Project/stockkit/scripts/tg_notify.py ...` 只是它在 WSL/Bash 下的
  路径映射和启动形式。两者遵守相同的 `CONTENT` 内容合同、中文格式和分段要求。

## 6. 当前模块状态

| 阶段 | 状态 | 当前结论 |
|---|---|---|
| A1–A5 | 完成 | 架构、函数归属、状态所有权、依赖和首轮模块顺序已冻结 |
| B1 | 闭环，待原版 oracle | compat、SDL3 平台生命周期、app 顶层 |
| B2 | 闭环，待原版 oracle | 文件、映射、资源容器、公共解压 |
| 日志 | 完成 | 线程安全落盘与失败回退已接入 |
| B3 | 闭环，待原版 oracle | 输入、时间、等待、RNG、DBCS/IME |
| B4 | 闭环，待原版 oracle | 软件 framebuffer、blitter、文字与呈现 |
| B5 | 延期后端 | VM 需要的音视频状态合同已有大量实现；`libffmpeg` 后端最后补 |
| B6 | 闭环，待原版 oracle | TSW/ACT/ANI/SND 资产运行时与动作记录 |
| B7 | 执行中 | 114 项中关闭 91 项，剩余 23 项 |
| B8 | 未正式进入 | 受追加 PLAN 的 P1/P2/P3 取代为完整剧情 VM 闭环 |
| B9 | 未完成 | 菜单、商店与特殊模式 |
| B10 | 未完成 | 战斗状态机、AI 与数值 |
| B11 | 未完成 | 存档、配置与持久化字段语义 |

当前程序已经具备 SDL3 窗口、启动、新游戏初始世界、角色移动/跑步、真实地图与部分剧情
对话链等纵向能力，但不能完整游玩。完整 VM、菜单/商店、战斗、音视频后端和存档业务语义
均未完成；不要用“能进入地图”代替模块完成证明。

## 7. B7 的精确状态

[`world-map-closure.tsv`](../analysis/04-reverse-engineering/inventory/world-map-closure.tsv)
当前共 114 项：

- `assembly_exact`：43
- `platform_adapted`：48
- `pending_audit`：23

剩余 23 项必须逐项审计，不能继承旧实现或旧叙述的完成状态：

```text
0x00402F80  sub_402F80
0x00413370  sub_413370   <- 当前断点
0x00413870  sub_413870
0x00413910  sub_413910
0x00413CA0  sub_413CA0
0x00413EA0  sub_413EA0
0x00413F00  sub_413F00
0x00413FE0  sub_413FE0
0x00414570  sub_414570
0x004145F0  sub_4145F0
0x004146F0  sub_4146F0
0x004147E0  sub_4147E0
0x004148F0  sub_4148F0
0x004149B0  sub_4149B0
0x004151F0  sub_4151F0
0x00425B50  sub_425B50
0x00425BE0  sub_425BE0
0x00426840  sub_426840
0x00426DF0  sub_426DF0
0x004270F0  sub_4270F0
0x00427140  sub_427140
0x004272C0  sub_4272C0
0x00427300  sub_427300
```

其中若干函数已有实现、集成测试或旧证据，但 `pending_audit` 明确表示它们仍需独立完成
全函数 LST→C++→LST 收敛；不得因现有测试通过直接改为关闭。

## 8. 当前精确开发断点：`sub_413370`

### 8.1 已锁定、可直接继承的事实

- 物理范围：`0x00413370..0x0041386E`；`0x0041386F` 是对齐 NOP，下一函数
  `sub_413870` 从 `0x00413870` 开始。
- 栈局部：`0x3C` 字节；保存并恢复 `EBX/EBP/ESI/EDI`。
- 唯一直接调用点：`0x00412A68`。调用者在 `0x00412A66` 压入一个零，函数本身不读取
  该参数，调用者在公共尾部清栈，也不消费返回值。
- 调用前 `sub_412930` 把 indexed palette 指针写入 `dword_4CD764`；调用后清零。
- 调用者只在 indexed 8 位底图且相机 X/Y 至少一轴未按 16 像素对齐时选择本入口。
- 函数内部仍有“X/Y 都对齐则调用 `sub_412BE0`”的物理分支
  `0x00413383..0x004133A0`。从当前唯一调用者的正常单线程路径看该分支冗余，但必须按
  汇编保留；不能擅自改为 `sub_413220`。
- indexed tile 源地址公式为：

  ```text
  lpBaseAddress + ((tile_index + 2) << 8)
  = lpBaseAddress + 0x200 + tile_index * 0x100
  ```

- cell flag `0x08000000` 跳过绘制；`0x04000000` 选择透明路径。
- 完整 tile 的 opaque/transparent 分别调用 `sub_4175B0`/`sub_417650`；透明路径只跳过
  palette index `1`，不是跳过某个转换后的颜色值。
- 四个边缘 tile 调用点走 `sub_4170E0`；本函数的四个调用地址为
  `0x00413587/0x004135D9/0x004136D0/0x0041372C`。
- service `0x48` 与 `0x13`、负 cell 修正、地图宽高截断和四边/内部循环的形状与相邻
  `sub_412D30` 高度相似，但本函数不从 `sub_412D30` 继承关闭状态。

### 8.2 尚未收敛，不能写成结论的点

当前只完成了范围、ABI、调用者和主分派的第一轮阅读，没有修改 C++、测试、证据或 TSV。
下一位执行者必须从函数入口重新独立记录所有基本块，重点证明：

1. service `0x13` 局部刷新是否与 direct-16 一样只绘制内部完整 tile，以及单轴对齐时
   是否仍保留一格边界。
2. 普通未对齐路径是否同样只让最外圈 tile 经过当前 raster clip，内部 tile 是否绕过
   该 clip。
3. 负相机、地图右/下边界、宽高为零或小于视口时的精确截断和地址推进。
4. 动画层偏移只作用于 tile index 还是也作用于 flag 指针。
5. `sub_4170E0` 在 `dword_4CD764 != 0` 时的 indexed clip、palette 和透明 flag 合同。
6. 内部对齐回退到 `sub_412BE0` 的可观察语义及当前调用域不可达证明。

现有 [`legacy_world_background.cpp`](../src/world_map/legacy_world_background.cpp) 中
`legacy_unaligned_direct_partial_interior` 和 `legacy_unaligned_direct_edge_clip` 都被限定为
`direct_16`。第一轮阅读表明 indexed 未对齐路径可能也需要这些原始异常，但这只是待证明
差异，不能先删条件再用现有测试倒推正确性。

### 8.3 推荐的下一步顺序

```text
1. 重新读取 LST 0x00413370..0x0041386E，不看现有 C++，建立基本块表。
2. 追入 0x004170E0、0x004175B0、0x00417650，仅记录本调用域实际使用的合同。
3. 从汇编独立导出 aligned fallback、service-13、四边、内部、负坐标、地图边界、
   hidden、transparent、opaque 和 tile-source 越界测试向量。
4. 再读取 legacy_world_background.cpp 与现有测试，逐项反查差异。
5. 用最小改动修正实现，补 indexed 未对齐专用 UT。
6. 完成 LST→C++、C++→LST 双向核对；若出现差异，从入口重做。
7. 新建 world-background-unaligned-indexed-00413370.md，更新两个 TSV 与主计划版本/计数。
8. 运行定向测试、Linux core/app 和 Windows app 全门禁。
9. 达到独立闭环边界后，按 commit Skill 提交并 push，再进入 sub_413870。
```

## 9. 构建与验证基线

### 9.1 最近一次已记录的完整门禁

在当前 `HEAD` 对应阶段，主计划记录：

- Linux core：`185/185` CTest 通过。
- Linux app：`190/190` CTest 通过。
- Windows LLVM app：`190/190` CTest 通过。
- Linux 与 Windows 应用均成功链接。
- 没有启动任何 EXE。

当前构建树用 `ctest -N` 仍枚举出 Linux core 185 项和 Linux app 190 项。这里的枚举不
替代最近一次实际全量通过记录；下一次实现修改后必须重新执行测试。

### 9.2 Linux

```bash
cd /mnt/e/Game/swd3/OpenSWD3
./build.sh core
./build.sh app
```

输出：

```text
build/linux-app/src/platform/sdl3/openswd3
```

测试会在检测到仓库父目录中的 `huge.lmf`、TALK 和其他原始数据时自动加入真实资产回归。

### 9.3 Windows LLVM

只在需要 Windows 构建门禁时执行：

```bat
cd /d E:\Game\swd3\OpenSWD3
build.bat core
build.bat app
```

脚本使用：

```text
D:\Dev\lldb\tools\cmake\bin\cmake.exe
D:\Dev\lldb\tools\ninja\ninja.exe
D:\Dev\Compiler\LLVM\x64\bin\clang.exe
D:\Dev\Compiler\LLVM\x64\bin\clang++.exe
```

Windows EXE 输出：

```text
E:\Game\swd3\OpenSWD3\build\app\src\platform\sdl3\Debug\openswd3.exe
```

`build.bat` 不含 `pause`。构建完成后不要自动启动 EXE。

## 10. 原始数据、动态 oracle 与可提交证据

- 原始游戏数据根：`/mnt/e/Game/swd3`，不复制进仓库。
- 完整 LST：`/mnt/e/Game/swd3/swd3.exe_export_for_ai/swd3.exe.lst`。
- IDA 伪码：`/mnt/e/Game/swd3/swd3.exe_export_for_ai/decompile/`，只作导航。
- 正式 glyph 基准：
  [`win11-zh-tw-cp950-20260809`](../analysis/04-reverse-engineering/artifacts/glyph-oracle/win11-zh-tw-cp950-20260809/)。
- 统一 Frida 工具：[`analysis/tools/swd3-oracle/`](../analysis/tools/swd3-oracle/)。
- 用户返回的原始 VM/Frida 输出先放 `build/vm/`；只有整理成最小、来源明确、可复核的证据
  后才进入 `analysis/04-reverse-engineering/artifacts/` 或 evidence 文档。
- 不提交整份用户 VM 目录、游戏 EXE、原始数据、临时输出或无必要的大型动态 dump。

## 11. 已知延期与非当前范围

- 剧情 VM 当前固定事实：198 个显式 opcode、146 个唯一 handler、25 个共享入口；约 50
  个 opcode 已接入 C++，但这些实现不继承完成状态，必须随 handler 组重新审计。
- 当前资产静态控制流观察到 143 个 opcode，其中仍有 93 个未实现；另 55 个未观察到，
  但不能删除或判定不可达。
- opcode `0..124` 已有人工汇编语义，`125..193` 只能随 handler 组边实施边补语义，
  不能先无限研究，也不能按编号一次堆完。
- 菜单、商店、战斗、音视频等外部模块未完成时，VM 仍必须实现参数解析、状态修改、请求和
  等待合同；不得伪造外部成功。
- `libffmpeg`、完整菜单/商店、完整战斗、存档业务字段和原版全量动态差分均非当前 P0
  的执行项。
- [`TODO.md`](../TODO.md) 中的多周目、装备继承和敌人增强属于还原完成后的扩展，不得
  混入当前 1:1 初步还原。

## 12. 关键文件索引

- 主计划：[`execution-plan-pi.md`](execution-plan-pi.md)
- VM 追加计划：[`story-vm-closure-plan-pi.md`](story-vm-closure-plan-pi.md)
- 仓库约束：[`AGENTS.md`](../AGENTS.md)
- B7 工作包：[`world-map.md`](../analysis/04-reverse-engineering/modules/world-map.md)
- B7 逐项真值表：
  [`world-map-closure.tsv`](../analysis/04-reverse-engineering/inventory/world-map-closure.tsv)
- 总函数所有权：
  [`module-function-ownership.tsv`](../analysis/04-reverse-engineering/inventory/module-function-ownership.tsv)
- 当前底图实现：
  [`legacy_world_background.cpp`](../src/world_map/legacy_world_background.cpp)
- 当前底图测试：
  [`legacy_world_background_test.cpp`](../tests/unit/world_map/legacy_world_background_test.cpp)
- 最近相邻证据：
  [`indexed 对齐底图`](../analysis/04-reverse-engineering/evidence/world-background-aligned-indexed-00413220.md)
- direct-16 未对齐证据：
  [`direct-16 未对齐底图`](../analysis/04-reverse-engineering/evidence/world-background-unaligned-direct-00412d30.md)
- 配置模板：[`openswd3.example.toml`](../config/openswd3.example.toml)
- 构建脚本：[`build.sh`](../build.sh)、[`build.bat`](../build.bat)

## 13. 接手检查表

接手者开始实现前应确认：

- 当前分支是 `pi-execution`，并已直接从 Git 重新读取精确 `HEAD` 和工作区洁净状态；若有
  未知改动或基线不符，立即停止，不得覆盖用户文件。
- 已确认分支创建/Pi 框架提交为 `5232dad`、其父提交/分支基线为 `da098fa`，并已理解之后
  的全部提交。
- 当前只以 [`execution-plan-pi.md`](execution-plan-pi.md) 和
  [`story-vm-closure-plan-pi.md`](story-vm-closure-plan-pi.md) 为执行权威，没有让冻结的
  无后缀上游参考驱动本分支。
- 预期推送/上游目标是 `origin/pi-execution`；首次阶段推送前由主 Agent 建立上游关系。
- 当前执行位仍是 P0/B7 的 `sub_413370`，没有跳到 VM、战斗或音视频。
- 已从 LST 独立建立 `sub_413370` 基本块和测试向量，再打开 C++。
- 所有新增结论都能定位到具体汇编地址。
- 实现修改后重新执行定向、Linux core/app 和 Windows app 门禁。
- 未启动任何 EXE；动态验证需要用户操作时已经停下等待。
- 阶段闭环后已更新证据、两个 TSV、主计划版本与 114 项计数。
- 阶段闭环后已按 commit Skill 自动提交并 push，并发送中文分段 TG 汇报。
