# OpenSWD3 逆向调研

本目录保存 OpenSWD3 还原 `swd3.exe` 所需的技术调研结果。

当前阶段不创建重写工程，不编写实现代码，不修改原始游戏文件。

## 证据原则

汇编是原程序逻辑的唯一真实依据。

IDA 伪码只用于定位函数、辅助阅读和提出假设。任何类型、参数、分支、符号扩展、调用约定和数据流结论，都必须回到对应汇编核对。伪码与汇编冲突时，无条件以汇编为准。

字符串、资源样本和动态运行结果用于解释、命名和验证汇编逻辑，但不能覆盖或改写汇编已经确定的行为。

## 目录

- `00-overview/`：阶段结论与阅读入口
- `01-baseline/`：原 EXE、IDA 导出和游戏资产基线
- `02-feasibility/`：可行性、完成定义与主要技术风险
- `03-technology/`：语言、依赖和目标技术路线
- `04-reverse-engineering/`：函数、子系统和格式识别结果
  - `04-reverse-engineering/evidence/`：从原 EXE 字节提取并复核的汇编证据块
  - `04-reverse-engineering/inventory/`：由导出清单机械生成的机器可读目录
- `05-roadmap/`：后续调研顺序和进入实现前的门槛
- `tools/`：只服务调研产物复现的生成/检查脚本，不是重写工程

## 当前结论

结论为“有条件可行”。

可以重写出不依赖原 `swd3.exe`、DirectDraw、DirectInput、Miles 和旧 Bink DLL 的现代程序，并继续读取原游戏数据。

但现有 IDA 伪码不能直接等价转换成新源码。要达到行为兼容，必须先恢复资源格式、剧情指令、战斗状态机、像素规则、时间规则和存档结构，再用原程序做差分验证。

初步还原的验收标准是 bug-for-bug 的 1:1 兼容：原游戏逻辑 BUG 也必须保留。只允许为启动和新系统兼容做最小平台适配，不得借此改变游戏逻辑。

首选语言为 C++20。

构建系统固定为 CMake，默认采用 Ninja Multi-Config。工程不依赖 Visual Studio IDE；Windows 编译器允许使用 MSVC 或 LLVM，并通过命令行构建配置选择。

首选平台层为 SDL3。原 EXE 当前唯一启动链是旧式 `640×480×16` 独占全屏；重写画面先保留相同逻辑尺寸、16 位软件帧缓冲、pitch 和分支内提交语义，再由 SDL3 转换成现代纹理显示。窗口化只可作为平台外壳扩展，不能改变兼容核心。`.bik` 和 `.mp3` 暂定由 FFmpeg 解码；自定义 `.Ani` 必须根据原程序恢复解码器。

详细结论见：

- [当前执行 GOAL](../goal/execution-plan.md)
- [历史调研计划](plan.md)
- [首轮结论](00-overview/first-round-conclusions.md)
- [P1 汇编级顶层控制流结论](00-overview/p1-control-flow-conclusions.md)
- [可行性评估](02-feasibility/assessment.md)
- [语言与技术栈](03-technology/language-and-stack.md)
- [顶层帧流程](04-reverse-engineering/frame-flow.md)
- [汇编地址索引](04-reverse-engineering/address-index.md)
- [P2 全局状态与所有权索引](04-reverse-engineering/global-state.md)
- [P2 全局状态机器目录](04-reverse-engineering/inventory/global-ownership.tsv)
- [P2.4 候选结构体与偏移索引](04-reverse-engineering/structure-candidates.md)
- [P2.4 结构字段机器目录](04-reverse-engineering/inventory/structure-fields.tsv)
- [角色直接命名字段读写矩阵](04-reverse-engineering/inventory/role-direct-field-xrefs.tsv)
- [角色前部已证明寄存器别名矩阵](04-reverse-engineering/inventory/role-front-alias-accesses.tsv)
- [角色前部寄存器别名与 Path 字段证据](04-reverse-engineering/evidence/role-front-register-aliases.md)
- [角色内嵌动作子记录访问矩阵](04-reverse-engineering/inventory/action-subrecord-accesses.tsv)
- [`0x004321E0` 动作子记录汇编证据](04-reverse-engineering/evidence/action-subrecord-004321e0.md)
- [ACT 索引、变体、缓存与动作命令流证据](04-reverse-engineering/evidence/act-action-stream-format.md)
- [TSW 固定索引、帧描述符与主压缩流证据](04-reverse-engineering/evidence/tsw-archive-format.md)
- [SND 固定索引、运行时视图与 RIFF 现场重建证据](04-reverse-engineering/evidence/snd-archive-format.md)
- [ANI 帧记录链、公共解压与 span 边界证据](04-reverse-engineering/evidence/ani-container-and-lzo-boundary.md)
- [ANI 19 文件机器目录](04-reverse-engineering/inventory/ani-files.tsv)
- [ANI 5312 帧机器目录](04-reverse-engineering/inventory/ani-frames.tsv)
- [ANI 容器全量汇总](04-reverse-engineering/inventory/ani-container-summary.tsv)
- [存档物理容器、五条 LZO 流与嵌入 Fame 边界](04-reverse-engineering/evidence/save-container-and-lzo-boundary.md)
- [40 份存档动态偏移与 EOF 机器目录](04-reverse-engineering/inventory/save-files.tsv)
- [200 条存档压缩流逐块验证目录](04-reverse-engineering/inventory/save-compressed-blocks.tsv)
- [存档容器与解压验证汇总](04-reverse-engineering/inventory/save-container-summary.tsv)
- [`huge.lmf` 尾索引、地图块与三类 LZO 边界](04-reverse-engineering/evidence/lmf-container-and-lzo-boundary.md)
- [LMF 310 条尾索引机器目录](04-reverse-engineering/inventory/lmf-tail-index.tsv)
- [LMF 309 张地图容器目录](04-reverse-engineering/inventory/lmf-maps.tsv)
- [LMF 1094 条压缩流验证目录](04-reverse-engineering/inventory/lmf-compressed-blocks.tsv)
- [LMF 容器与解压验证汇总](04-reverse-engineering/inventory/lmf-container-summary.tsv)
- [16 位像素格式选择与 CM 缓存字节闭环](04-reverse-engineering/evidence/pixel-format-selection-and-cm-cache.md)
- [两个 CM 缓存 × 四种像素变换逐字节目录](04-reverse-engineering/inventory/pixel-conversion-cache-comparison.tsv)
- [像素转换验证汇总](04-reverse-engineering/inventory/pixel-conversion-summary.tsv)
- [16 位软件 blitter 分派、RLE 与像素效果证据](04-reverse-engineering/evidence/software-blitter-dispatch-and-pixel-effects.md)
- [软件 blitter 43 个稀疏分派槽](04-reverse-engineering/inventory/blitter-dispatch.tsv)
- [步进透明度 0..15 精确项式](04-reverse-engineering/inventory/blitter-opacity-steps.tsv)
- [coverage composite 0..15 精确项式](04-reverse-engineering/inventory/blitter-coverage-composite-steps.tsv)
- [软件 blitter 248 个直接调用参数目录](04-reverse-engineering/inventory/blitter-callsites.tsv)
- [`0x0C/0x20` 变换 flags 与调用路径目录](04-reverse-engineering/inventory/blitter-transform-callsites.tsv)
- [`0x0C/0x20` 10.10 行重采样合同](04-reverse-engineering/inventory/blitter-row-resampling.tsv)
- [TSW 主流 blitter 家族与未消费辅助载荷目录](04-reverse-engineering/inventory/tsw-blitter-family-and-aux.tsv)
- [`0x08` 当前资产链联合可达性目录](04-reverse-engineering/inventory/blitter-mode8-reachability.tsv)
- [动态 blitter flags 与空 `0x22/0x23` 槽可达性目录](04-reverse-engineering/inventory/blitter-dynamic-flag-reachability.tsv)
- [字体 surface、1-bit 字形缓存与五种软件 footprint 证据](04-reverse-engineering/evidence/font-surface-and-glyph-rendering.md)
- [字体 renderer 对象字段目录](04-reverse-engineering/inventory/font-renderer-object-fields.tsv)
- [GDI 到软件字形 mask 的十阶段流水线](04-reverse-engineering/inventory/font-glyph-pipeline.tsv)
- [五种字形 style footprint 与严格裁剪目录](04-reverse-engineering/inventory/font-glyph-style-footprints.tsv)
- [291 个直接文字绘制调用点目录](04-reverse-engineering/inventory/font-render-callsites.tsv)
- [帧缓冲、DirectDraw surface 与 21 个主画面提交证据](04-reverse-engineering/evidence/framebuffer-and-display-presentation.md)
- [21 对 framebuffer Lock/Unlock 目录](04-reverse-engineering/inventory/frame-surface-lock-pairs.tsv)
- [全部 32 个 DirectDraw Blt 调用目录](04-reverse-engineering/inventory/directdraw-blt-callsites.tsv)
- [21 个 primary presentation 路径目录](04-reverse-engineering/inventory/primary-presentation-paths.tsv)
- [15 个显示生命周期阶段目录](04-reverse-engineering/inventory/display-lifecycle-stages.tsv)
- [输入归一化、连按与重复语义](04-reverse-engineering/evidence/input-normalization-and-repeat-semantics.md)
- [`0x00424390` 默认按键与 0x80 字节运行时兼容块](04-reverse-engineering/evidence/default-key-bindings-00424390.md)
- [20 条归一化输入记录与当前绑定](04-reverse-engineering/inventory/input-normalized-records.tsv)
- [输入状态转移矩阵](04-reverse-engineering/inventory/input-state-transitions.tsv)
- [242 条归一化记录直接访问目录](04-reverse-engineering/inventory/input-state-direct-accesses.tsv)
- [77 个原始键查询调用目录](04-reverse-engineering/inventory/input-raw-key-queries.tsv)
- [5 个合成原始键写入目录](04-reverse-engineering/inventory/input-synthetic-key-writes.tsv)
- [8 个鼠标逻辑坐标重基准目录](04-reverse-engineering/inventory/input-mouse-coordinate-rebases.tsv)
- [帧时钟、等待与阻塞语义](04-reverse-engineering/evidence/frame-clock-and-wait-semantics.md)
- [11 个 `timeGetTime`/CRT `time` 来源调用](04-reverse-engineering/inventory/time-source-callsites.tsv)
- [12 个帧 interval 变更点](04-reverse-engineering/inventory/frame-interval-mutations.tsv)
- [42 条时间全局直接访问](04-reverse-engineering/inventory/time-global-accesses.tsv)
- [27 个 Sleep 调用目录](04-reverse-engineering/inventory/sleep-callsites.tsv)
- [9 类关键等待谓词](04-reverse-engineering/inventory/time-wait-rules.tsv)
- [整帧颜色偏移、双输入合成与灰度函数](04-reverse-engineering/evidence/frame-color-adjustment-and-combine.md)
- [六个 packed-16 颜色函数合同](04-reverse-engineering/inventory/frame-color-functions.tsv)
- [50 个颜色函数直接调用点](04-reverse-engineering/inventory/frame-color-callsites.tsv)
- [四种像素布局下的 24 个颜色整数向量](04-reverse-engineering/inventory/frame-color-format-vectors.tsv)
- [P4 原程序动态 oracle 捕获协议](04-reverse-engineering/evidence/p4-dynamic-oracle-capture-protocol.md)
- [19 个 P4 动态捕获点](04-reverse-engineering/inventory/p4-oracle-capture-points.tsv)
- [11 类 oracle 产物合同](04-reverse-engineering/inventory/p4-oracle-artifacts.tsv)
- [固定 EXE/DLL/资产与 PE 探针基线](04-reverse-engineering/inventory/p4-oracle-runtime-baseline.tsv)
- [剧情 VM 低 14 位 opcode 与一级分派表证据](04-reverse-engineering/evidence/story-vm-primary-dispatch.md)
- [剧情 VM 198 个显式 opcode 分派记录](04-reverse-engineering/inventory/story-vm-opcode-dispatch.tsv)
- [剧情 VM 完整分派边界](04-reverse-engineering/inventory/story-vm-dispatch-ranges.tsv)
- [剧情 VM 146 个一级入口目标组](04-reverse-engineering/inventory/story-vm-entry-target-groups.tsv)
- [剧情 VM 共享 handler 内部 opcode 细分](04-reverse-engineering/inventory/story-vm-internal-opcode-switches.tsv)
- [剧情 VM 指令长度层与 TALK 线性前缀验证](04-reverse-engineering/evidence/story-vm-length-and-talk-linear-probe.md)
- [剧情 VM 198 个 opcode 静态 CFG 初筛](04-reverse-engineering/inventory/story-vm-opcode-static-triage.tsv)
- [剧情 VM 198 行物理编码与长度规则](04-reverse-engineering/inventory/story-vm-opcode-length-rules.tsv)
- [3992 个 TALK 索引候选线性探针](04-reverse-engineering/inventory/story-vm-talk-linear-entry-probes.tsv)
- [58,782 个 TALK 唯一物理指令记录](04-reverse-engineering/inventory/story-vm-talk-linear-records.tsv)
- [剧情 VM 线性前缀 opcode 覆盖](04-reverse-engineering/inventory/story-vm-talk-opcode-coverage.tsv)
- [31 个剧情 VM 跨窗口控制转移规则](04-reverse-engineering/inventory/story-vm-control-transfer-rules.tsv)
- [138,988 个 TALK 窗口上下文 CFG 节点](04-reverse-engineering/inventory/story-vm-talk-cfg-nodes.tsv)
- [137,207 条 TALK 全分支候选边](04-reverse-engineering/inventory/story-vm-talk-cfg-edges.tsv)
- [TALK 全分支候选图的四个无效根](04-reverse-engineering/inventory/story-vm-talk-cfg-issues.tsv)
- [剧情 VM opcode 0–24 汇编语义批次](04-reverse-engineering/evidence/story-vm-opcodes-000-024.md)
- [剧情 VM opcode 25–49 汇编语义批次](04-reverse-engineering/evidence/story-vm-opcodes-025-049.md)
- [剧情 VM opcode 50–74 汇编语义批次](04-reverse-engineering/evidence/story-vm-opcodes-050-074.md)
- [剧情 VM opcode 75–99 汇编语义批次](04-reverse-engineering/evidence/story-vm-opcodes-075-099.md)
- [剧情 VM opcode 100–124 汇编语义批次](04-reverse-engineering/evidence/story-vm-opcodes-100-124.md)
- [198 行 opcode 直接状态访问底稿](04-reverse-engineering/inventory/story-vm-opcode-direct-effects.tsv)
- [1449 个 opcode 直接状态访问证据点](04-reverse-engineering/inventory/story-vm-opcode-direct-effect-sites.tsv)
- [opcode 0–24 的 25 行逐值语义规格](04-reverse-engineering/inventory/story-vm-opcode-semantics-000-024.tsv)
- [opcode 25–49 的 25 行逐值语义规格](04-reverse-engineering/inventory/story-vm-opcode-semantics-025-049.tsv)
- [opcode 50–74 的 25 行逐值语义规格](04-reverse-engineering/inventory/story-vm-opcode-semantics-050-074.tsv)
- [opcode 75–99 的 25 行逐值语义规格](04-reverse-engineering/inventory/story-vm-opcode-semantics-075-099.tsv)
- [opcode 100–124 的 25 行逐值语义规格](04-reverse-engineering/inventory/story-vm-opcode-semantics-100-124.tsv)
- [DAT 文件族、MZ 包装头与原始偏移读取证据](04-reverse-engineering/evidence/dat-file-families.md)
- [`0x004399E0` 公共解压器汇编规格](04-reverse-engineering/evidence/common-decompressor-004399e0.md)
- [LZO1X 2.10 分支兼容与命中矩阵](04-reverse-engineering/inventory/lzo1x-branch-compatibility.tsv)
- [LZO1X 全量 TSW 对照汇总](04-reverse-engineering/inventory/lzo1x-compatibility-summary.tsv)
- [LZO1X 补充分支合法向量](04-reverse-engineering/inventory/lzo1x-valid-branch-vectors.tsv)
- [LZO1X 安全接口边界控制向量](04-reverse-engineering/inventory/lzo1x-library-control-vectors.tsv)
- [公共解压器全部直接/包装调用环境矩阵](04-reverse-engineering/inventory/decompress-call-sites.tsv)
- [六个 ACT 文件机器目录](04-reverse-engineering/inventory/act-archives.tsv)
- [六个 TSW 文件机器目录](04-reverse-engineering/inventory/tsw-archives.tsv)
- [TSW 全帧解压验证汇总](04-reverse-engineering/inventory/tsw-decompression-summary.tsv)
- [SND 全包索引与返回缓冲区验证汇总](04-reverse-engineering/inventory/snd-archive-summary.tsv)
- [DAT 文件分类、头字段与哈希目录](04-reverse-engineering/inventory/dat-files.tsv)
- [DAT 偏移索引样本汇总](04-reverse-engineering/inventory/dat-offset-index-summary.tsv)
- [DAT 加载环境矩阵](04-reverse-engineering/inventory/dat-loader-contexts.tsv)
- [ACT 命令字机器目录](04-reverse-engineering/inventory/act-command-words.tsv)
- [通用动作字段生产、重置与跨帧状态机](04-reverse-engineering/evidence/action-field-state-machine.md)
- [通用动作记录首批外部消费者证据](04-reverse-engineering/evidence/action-external-consumers.md)
- [52 个已知动作字段语义矩阵](04-reverse-engineering/inventory/action-field-semantics.tsv)
- [动作记录外部访问逐指令矩阵](04-reverse-engineering/inventory/action-external-accesses.tsv)
- [ACT 命令字段效果矩阵](04-reverse-engineering/inventory/act-command-field-effects.tsv)
- [动作记录父对象族汇编证据](04-reverse-engineering/evidence/action-object-families.md)
- [动作子记录全局调用点目录](04-reverse-engineering/inventory/action-subrecord-callsites.tsv)
- [战斗角色记录与公共动作数组证据](04-reverse-engineering/evidence/battle-actor-records.md)
- [`0xB0` 物品节点与 `mon.dat` 定义快照证据](04-reverse-engineering/evidence/item-node-mon-dat.md)
- [四个队伍物品链表头与 `0x004A948C` 偏置别名纠正](04-reverse-engineering/evidence/party-item-list-heads.md)
- [P2 函数语义索引](04-reverse-engineering/function-index.md)
- [P2.5 函数边界、栈清理与返回值消费初筛](04-reverse-engineering/evidence/function-abi-triage.md)
- [P2.5 ABI 候选机器目录](04-reverse-engineering/inventory/function-abi-candidates.tsv)
- [P2.5 ABI 初筛汇总](04-reverse-engineering/inventory/function-abi-summary.tsv)
- [P2.5 关键函数 ABI 合同（当前 39 项）](04-reverse-engineering/evidence/critical-abi-contracts.md)
- [P2.5 关键 ABI 合同机器目录](04-reverse-engineering/inventory/critical-abi-contracts.tsv)
- [存档预览与大型存读写入口 ABI](04-reverse-engineering/evidence/save-entry-abi.md)
- [文件对象、TSW 与 ACT 查询入口 ABI](04-reverse-engineering/evidence/resource-entry-abi.md)
- [输入归一化与 DirectInput 边界 ABI](04-reverse-engineering/evidence/input-entry-abi.md)
- [普通世界呈现、LMF 装载与 CM 缓存 ABI](04-reverse-engineering/evidence/map-entry-abi.md)
- [Miles/SND 与 Bink 对象层 ABI](04-reverse-engineering/evidence/audio-video-entry-abi.md)
- [P2.5 特殊模式回调槽、旧值保留与二级分派 ABI](04-reverse-engineering/evidence/special-mode-callback-abi.md)
- [特殊模式 107 条主槽目标赋值矩阵](04-reverse-engineering/inventory/special-mode-callback-targets.tsv)
- [特殊模式 13 个主回调槽合同](04-reverse-engineering/inventory/special-mode-callback-slots.tsv)
- [特殊模式二级索引分派矩阵](04-reverse-engineering/inventory/special-mode-secondary-dispatch.tsv)
- [P2.5 十个必需子系统 ABI 覆盖审计](04-reverse-engineering/evidence/subsystem-abi-coverage.md)
- [P2.5 子系统 ABI 覆盖机器矩阵](04-reverse-engineering/inventory/subsystem-abi-coverage.tsv)
- [P2.2 顶层直接调用覆盖](04-reverse-engineering/evidence/top-level-direct-call-coverage.md)
- [窗口过程汇编证据](04-reverse-engineering/evidence/wndproc-0040a0d0.md)
- [单帧主循环汇编证据](04-reverse-engineering/evidence/main-frame-0040a570.md)
- [呈现、暂停与生命周期汇编证据](04-reverse-engineering/evidence/presentation-lifecycle.md)
- [高优先级画面调度汇编证据](04-reverse-engineering/evidence/high-priority-mode-00406e30.md)
- [普通世界音频状态协调汇编证据](04-reverse-engineering/evidence/audio-frame-0040cdd0.md)
- [普通世界鼠标交互汇编证据](04-reverse-engineering/evidence/world-interaction-00427300.md)
- [普通世界玩家控制与遇敌协调汇编证据](04-reverse-engineering/evidence/world-player-control-00402f80.md)
- [随机遇敌物理布局、选择与战斗切入汇编证据](04-reverse-engineering/evidence/random-encounter-0040d9e0-0040db39.md)
- [LMF、CM 与世界渲染会话组合证据](04-reverse-engineering/evidence/lmf-world-map-session-00425be0.md)
- [普通世界帧组合与地图底图汇编证据](04-reverse-engineering/evidence/world-frame-composition-004120b0-00413370.md)
- [普通世界帧外层协调器汇编证据](04-reverse-engineering/evidence/world-frame-coordinator-004120b0.md)
- [世界帧动画层与选择序列视口恢复](04-reverse-engineering/evidence/world-frame-tail-0041287f-00412923.md)
- [世界选择序列相机滚动状态机](04-reverse-engineering/evidence/world-selection-scroll-004148f0.md)
- [战斗建立入口汇编证据](04-reverse-engineering/evidence/battle-setup-00451b10.md)
- [游戏内菜单/特殊模式共同驱动器汇编证据](04-reverse-engineering/evidence/special-menu-modes-00439fd0.md)
- [商店交易模式汇编证据](04-reverse-engineering/evidence/shop-mode-0044ea60.md)
- [剧情解释器入口与让出规则](04-reverse-engineering/story-vm-entry.md)
- [战斗更新入口与返回值契约](04-reverse-engineering/battle-entry.md)
- [子系统地图](04-reverse-engineering/subsystem-map.md)
- [待验证项](04-reverse-engineering/open-questions.md)
