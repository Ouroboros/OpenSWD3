# B4 `rendering` 工作包

状态：实施中

当前单元：B4.5 已赋值 blitter 效果族

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
- 字体：compat 核心使用固定 mask 测五种 footprint、cache 插入/淘汰、字节 advance 和背景；宿主 glyph provider 另以旧环境捕获的 mask 做 oracle。
- 呈现：记录每次 present 的调用点、rectangle、wait 语义和 framebuffer hash，不用“最终图片看起来相似”代替逐像素比较。

原程序动态 framebuffer、GDI glyph 和 DirectDraw RECT 捕获仍是 `blocked_runtime_oracle`。固定捕获点与产物格式已经定义，缺样本不阻止静态闭环单元继续实现，但不能标记 `original_diff_verified`。

## 5. 已有证据

- [`framebuffer-and-display-presentation.md`](../evidence/framebuffer-and-display-presentation.md)
- [`legacy-framebuffer-geometry-00416d30.md`](../evidence/legacy-framebuffer-geometry-00416d30.md)
- [`legacy-blitter-copy-004170e0.md`](../evidence/legacy-blitter-copy-004170e0.md)
- [`legacy-blitter-assigned-effects-0041b280.md`](../evidence/legacy-blitter-assigned-effects-0041b280.md)
- [`legacy-blitter-color-key-and-saturated-arithmetic.md`](../evidence/legacy-blitter-color-key-and-saturated-arithmetic.md)
- [`legacy-blitter-run-edge-copy-0041ccf0.md`](../evidence/legacy-blitter-run-edge-copy-0041ccf0.md)
- [`legacy-blitter-opacity-00417950-0041d340.md`](../evidence/legacy-blitter-opacity-00417950-0041d340.md)
- [`legacy-blitter-raw-constant-fade-00417ec0.md`](../evidence/legacy-blitter-raw-constant-fade-00417ec0.md)
- [`legacy-blitter-rle-saturated-resample-004208d0.md`](../evidence/legacy-blitter-rle-saturated-resample-004208d0.md)
- [`legacy-blitter-smear-00422730.md`](../evidence/legacy-blitter-smear-00422730.md)
- [`pixel-format-selection-and-cm-cache.md`](../evidence/pixel-format-selection-and-cm-cache.md)
- [`software-blitter-dispatch-and-pixel-effects.md`](../evidence/software-blitter-dispatch-and-pixel-effects.md)
- [`font-surface-and-glyph-rendering.md`](../evidence/font-surface-and-glyph-rendering.md)
- [`presentation-lifecycle.md`](../evidence/presentation-lifecycle.md)
- [`p4-dynamic-oracle-capture-protocol.md`](../evidence/p4-dynamic-oracle-capture-protocol.md)

## 6. 当前执行顺序

1. `[x]` B4.1：建立 151 项函数范围、六个接口族、核心状态所有权、生命周期、依赖和自动化验证入口。
2. `[x]` B4.2：实现 `0x004238B0..0x004239C1` 正反转换与 `0x00423400` 的转换选择子状态；四条公式穷举 262,144 个输入组合，当前 CM 缓存 4,420,608 字节验证通过。
3. `[x]` B4.3：实现 owned framebuffer、显式 pitch、1024 项旧行表与固定画布常量；Linux `core` 41/41、Linux/Windows `app` 43/43 CTest 通过。
4. `[x]` B4.4：43 个稀疏槽、普通裁剪、四条 raw/RLE copy、异常边界与真实 TSW 固定帧已闭环；Linux `core` 42/42、Linux/Windows `app` 44/44 CTest 通过。
5. `[>]` B4.5：按效果族逐单元实现其余已赋值 blitter；`0x10/0x24/0x28` 目标颜色偏移、常量填充和灰度，raw `0x84/0x85` 色键复制、RLE `0x04/0x2C` 饱和加减色，`0x18` literal run 边缘覆盖，raw `0x94`、RLE `0x14/0x1C` 透明度，`0x30` 的 17 拍邻像素涂抹、raw `0x88` 常量纵向淡出，以及 RLE `0x20/0x21` 纵向重采样饱和加色单元已闭环；下一单元处理 15 条真实构造路径可达的 RLE `0x0C..0x0F` 纵向重采样、逐行横移目标颜色偏移。
6. `[ ]` B4.6：实现 glyph mask/cache、五种文字 footprint 与背景。
7. `[ ]` B4.7：恢复画面动作/effect 更新和原分支 present 请求。
8. `[ ]` B4.8：接入 SDL3 上传、恢复生命周期与 framebuffer 哈希回放。

每项达到自己的汇编、UT 和资产门后立即进入下一项，不等待 B4 全部细节重新调研。
