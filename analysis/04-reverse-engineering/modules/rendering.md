# B4 `rendering` 工作包

状态：实施中

当前单元：B4.9c 暂停及动作绘制 helper

## 1. 范围与非范围

B4 负责稳定的 `640×480` 16 位软件 framebuffer、实际 pitch 行寻址、RGB mask 与像素转换、RLE/原始图像 blitter、文字 mask/cache/writer、画面效果，以及在原分支位置产生最终呈现请求。

当前机器目录中 `module_candidate = rendering` 的 151 项函数是本模块的函数范围。完整逐项地址保留在 [`module-function-ownership.tsv`](../inventory/module-function-ownership.tsv)，接口级分组为：

- framebuffer、行偏移和通用绘制入口：`0x004014F0..0x00417050` 中目录明确归属 B4 的函数。
- 软件 blitter 分派、裁剪和全部已赋值像素循环：`0x00416D90..0x00423020`。
- surface 查询、像素格式状态和正反转换：`0x004233D0..0x004239D0`。
- 截图转换、场景绘制与动作画面更新边界：`0x0042E850..0x00430B60`、`0x0043B110`、`0x0043BAB0`、`0x00451A90`、`0x00451AE0`。
- 字体 renderer、GDI mask 生产边界、字形 cache 和五种软件 writer：`0x00435160..0x00436FA0`。
- DirectDraw 对象和 surface 生命周期包装：`0x00437570..0x00437F90`。现代实现把 native SDL 资源移到 `platform_sdl3`，B4 只保留其可观察的 framebuffer、恢复和提交合同。

资源容器和压缩字节属于 `resource_io`；TSW/ACT/ANI 运行时属于 `asset_runtime`；地图、剧情、菜单和战斗只生产绘制请求并拥有各自业务状态。GDI、DirectDraw 和 SDL3 都不能进入像素算法核心。

## 2. 接口与状态所有权

B4 拥有：

- 生命周期稳定的 16 位 framebuffer、逻辑宽高、byte pitch 和逐行偏移；固定画布路径与按实测 pitch 路径必须分别保留。
- `0x004A0E6C/0x004A0E70` 正反转换选择、`0x004CD79C..0x004CD7A4` 实际 RGB masks，以及 `0x00423400` 建立的有效通道 masks、shift 和混合表。
- `0x004CD318` 稀疏 blitter 表，以及颜色偏移、透明档位、固定邻行距离等像素效果状态。调用者只能通过明确参数或已恢复的状态端口修改它们。
- 三个字体 renderer 的字形 mask cache、clip、颜色和固定 advance。宿主字形栅格只通过 `GlyphProvider` 提供 MSB-first 1-bit mask。
- 兼容核心的 present 请求序列。SDL3 拥有窗口、renderer、texture 和实际上传资源，不拥有游戏像素。

`picture_action_list` 和 `picpaint_action_lists` 仍由 `story_scene` 拥有；B4 只按汇编借用、更新显示字段并在终止边界请求节点退休。`generic_action_record` 和 TSW/ACT 像素字节同样是借用视图，不转移到 B4。

首版公共边界只暴露固定宽度整数、显式 pitch/rectangle、原始资源 byte view 和平台无关 present/glyph 端口。不复制 DirectDraw COM ABI，也不提前设计未受汇编支持的场景图或 GPU 材质系统。

## 3. 生命周期与依赖

启动顺序为：建立 owned framebuffer 与平台上传对象，按实际输出 masks 建立像素状态和 blitter 表，再创建 20/16/12 三套字体 renderer。正常帧由当前互斥业务分支执行软件绘制，并在原来的 21 个 primary 提交点请求 present；没有统一帧尾强制提交。

停用时冻结正常帧并释放旧字体/platform surface 资源；恢复时重新建立 framebuffer 绑定、像素状态和三套字体输入，再恢复战斗显示边界。现代 SDL3 后端不复制 surface-lost HRESULT，但恢复失败不能让游戏逻辑偷偷推进。

核心允许依赖 `compat` 和 `resource_io` 的只读字节/容器接口。对 `asset_runtime`、`story_scene` 等旧循环依赖改为借用 view 或调用端口；`platform_sdl3` 只在最终上传、窗口尺寸和输入坐标映射边界被调用。

## 4. 自动化验证策略

- 像素转换：穷举全部 65536 个 `u16` 输入，验证四条汇编公式、零/负计数首像素缺陷和选择器重入状态；两份当前 CM 缓存继续提供 4,420,608 字节真实资产佐证。
- framebuffer：固定不同 pitch、padding、裁剪边界和硬编码 `0x500/0x96000` 路径，比较物理/逻辑缓冲哈希。
- blitter：按 43 个已赋值槽和未赋值槽建立固定 RLE/span、原始矩形、方向、裁剪、透明档位与颜色运算向量；真实 TSW 帧提供资源样本。
- 字体：compat 核心使用固定 mask 测五种 footprint、cache 插入/淘汰、字节 advance 和背景；glyph provider 以正确 CP950/細明體环境捕获的唯一 mask 基准做逐字节 oracle。
- 呈现：记录每次 present 的调用点、rectangle、wait 语义和 framebuffer hash，不用“最终图片看起来相似”代替逐像素比较。

原程序 GDI glyph 捕获链已经在正确环境动态验证。唯一基准位于
`../artifacts/glyph-oracle/win11-zh-tw-cp950-20260809/`，包含 157 个 mask：
`12x12=16`、`16x16=90`、`20x20=51`。manifest 记录 `cp950` 与经典
`mingliu.ttc`，用户同时确认原版显示与游戏原本效果一致。此前英文/错误字体
环境的运行输出已删除。受控 GDI 生成器已在 RGB555、RGB565
和 BGRA32 三种临时 surface 上对 157 个样本全部逐字节匹配，正式
atlas 及跨平台 `LegacyGlyphAtlasProvider` 已接入，B4.6 闭环。
framebuffer 和 DirectDraw RECT 捕获仍是各自的 `blocked_runtime_oracle`。

## 5. 已有证据

- [`framebuffer-and-display-presentation.md`](../evidence/framebuffer-and-display-presentation.md)
- [`legacy-framebuffer-geometry-00416d30.md`](../evidence/legacy-framebuffer-geometry-00416d30.md)
- [`legacy-blitter-copy-004170e0.md`](../evidence/legacy-blitter-copy-004170e0.md)
- [`legacy-blitter-assigned-effects-0041b280.md`](../evidence/legacy-blitter-assigned-effects-0041b280.md)
- [`legacy-blitter-color-key-and-saturated-arithmetic.md`](../evidence/legacy-blitter-color-key-and-saturated-arithmetic.md)
- [`legacy-blitter-run-edge-copy-0041ccf0.md`](../evidence/legacy-blitter-run-edge-copy-0041ccf0.md)
- [`legacy-blitter-opacity-00417950-0041d340.md`](../evidence/legacy-blitter-opacity-00417950-0041d340.md)
- [`legacy-blitter-raw-constant-fade-00417ec0.md`](../evidence/legacy-blitter-raw-constant-fade-00417ec0.md)
- [`legacy-blitter-rle-shifted-resample-0041f8d0.md`](../evidence/legacy-blitter-rle-shifted-resample-0041f8d0.md)
- [`legacy-blitter-rle-saturated-resample-004208d0.md`](../evidence/legacy-blitter-rle-saturated-resample-004208d0.md)
- [`legacy-blitter-smear-00422730.md`](../evidence/legacy-blitter-smear-00422730.md)
- [`pixel-format-selection-and-cm-cache.md`](../evidence/pixel-format-selection-and-cm-cache.md)
- [`software-blitter-dispatch-and-pixel-effects.md`](../evidence/software-blitter-dispatch-and-pixel-effects.md)
- [`font-surface-and-glyph-rendering.md`](../evidence/font-surface-and-glyph-rendering.md)
- [`legacy-rectangle-effect-0043b110.md`](../evidence/legacy-rectangle-effect-0043b110.md)
- [`legacy-tiled-frame-and-effect-panel-0042e850-0043bab0.md`](../evidence/legacy-tiled-frame-and-effect-panel-0042e850-0043bab0.md)
- [`legacy-primary-presentation-requests.md`](../evidence/legacy-primary-presentation-requests.md)
- [`legacy-bmp-writer-004303d0.md`](../evidence/legacy-bmp-writer-004303d0.md)
- [`legacy-formatted-text-004306c0.md`](../evidence/legacy-formatted-text-004306c0.md)
- [`legacy-countdown-004308c0-00430b60.md`](../evidence/legacy-countdown-004308c0-00430b60.md)
- [`legacy-image-command-stream-004014f0-00401e50.md`](../evidence/legacy-image-command-stream-004014f0-00401e50.md)
- [`legacy-drawing-helpers-0040de50-004117f0.md`](../evidence/legacy-drawing-helpers-0040de50-004117f0.md)
- [`presentation-lifecycle.md`](../evidence/presentation-lifecycle.md)
- [`p4-dynamic-oracle-capture-protocol.md`](../evidence/p4-dynamic-oracle-capture-protocol.md)
- [`rendering-closure-audit.md`](../evidence/rendering-closure-audit.md)
- [`Windows glyph-mask 动态基准`](../artifacts/glyph-oracle/win11-zh-tw-cp950-20260809/README.md)

## 6. 当前执行顺序

1. `[x]` B4.1：建立 151 项函数范围、六个接口族、核心状态所有权、生命周期、依赖和自动化验证入口。
2. `[x]` B4.2：实现 `0x004238B0..0x004239C1` 正反转换与 `0x00423400` 的转换选择子状态；四条公式穷举 262,144 个输入组合，当前 CM 缓存 4,420,608 字节验证通过。
3. `[x]` B4.3：实现 owned framebuffer、显式 pitch、1024 项旧行表与固定画布常量；Linux `core` 41/41、Linux/Windows `app` 43/43 CTest 通过。
4. `[x]` B4.4：43 个稀疏槽、普通裁剪、四条 raw/RLE copy、异常边界与真实 TSW 固定帧已闭环；Linux `core` 42/42、Linux/Windows `app` 44/44 CTest 通过。
5. `[x]` B4.5：全部正常资产可达的已赋值 blitter 已按效果族实现；最后闭环的 RLE `0x0C..0x0F` 保留纵向 10.10 行选择、逐行横移、`top_clip+1` 首行丢弃、目标 `y+1`、零目标高度的跨调用放大状态和正反 phase 不对称，真实 `all_sys.tsw` 哈希通过。RLE `0x08/0x09` 已证明当前 TSW/ACT 资产链不可达，强制异常状态保留显式安全边界；raw `0x88` 已实现。Linux `core` 42/42、Linux/Windows `app` 44/44 CTest 通过，原程序 framebuffer 差分仍为 `blocked_runtime_oracle`。
6. `[x]` B4.6：唯一动态基准、受控 GDI 生成器、32,896-key 正式 atlas、跨平台 Provider、EXE 旁资源部署和运行时校验已闭环；独立验证为 `157/157` 零差异，Linux `core` 47/47、Windows `app` 49/49 CTest 通过。
7. `[x]` B4.7：`sub_43B110` 六模式矩形效果、`sub_42E850` 九宫格绘制和 `sub_43BAB0` 效果面板组合已按完整 LST 实现并逐基本块复核；21 个 primary 提交点已形成完整请求合同，SDL smoke 的错误统一帧尾 present 已改为六条稳定分支内请求。`sub_4303D0` BMP 写入器、`sub_4306C0` 格式化原始字节文字及 `sub_4308C0/sub_430B60` 30 Hz 倒计时绘制与初始化均已闭环。Linux `core` 54/54、Windows LLVM `app` 56/56 CTest 通过。
8. `[x]` B4.8：SDL3 上传已直接使用 owned framebuffer 的稳定地址和实际 pitch，logical hash 已固定为跨平台小端 FNV-1a；恢复路径可重建纹理并重新上传现有 primary，失败会停止外壳，现代可缩放窗口恢复时保留用户尺寸。独立 primary surface 与 full/partial RECT 合成已接通，矩形外保留旧 primary 状态，快照/临时 source 缺失时显式失败。Linux `core` 54/54、Linux/Windows LLVM `app` 56/56 CTest 通过；Windows OpenSWD3 在 `1000×750` 下完成最小化、恢复、尺寸保持和零退出 smoke。
9. `[>]` B4 范围闭环审计：151 项矩阵现为 73 项实现、16 项内部物理分支、35 项平台替代、2 项当前资产不可达、5 项延后接线、19 项真实缺口和 1 项移交。`0x0040DE50/0x0040E080/0x004117F0` 已按完整 LST、精确像素顺序、缩略图全点向量和资源请求顺序闭环；下一组立即处理 `0x00411FA0/0x00414B60..0x004153D0`。

每项达到自己的汇编、UT 和资产门后立即进入下一项，不等待 B4 全部细节重新调研。
