# asset_runtime 工作包

最后更新：2026-08-09

状态：B6 单模块开始条件已满足；TSW、ACT、通用动作记录与 ANI 活动主时间线已实现

唯一行为真值：`swd3.exe.lst`。IDA 名称和伪码只用于定位。

## 1. 范围与停止线

总所有权表中 `code_origin=game` 且 `module_candidate=asset_runtime` 的 78 个地址是 B6
当前有限范围。它们按地址和已经确认的数据流形成以下工作分组：

| 地址集合 | 数量 | 当前职责 |
|---|---:|---|
| `0x00401190`、`0x0040AD10`、`0x0040DC00`、`0x0040EBF0`、`0x0040EC80`、`0x0040ECC0` | 6 | 公共动作记录初始化及跨模块查询/绘制适配 |
| `0x004154A0..0x00416CC0` 的 11 个自有入口 | 11 | ANI 活动对象、帧链、调色板、span 提交和相关动作记录 |
| `0x00424330` | 1 | TSW/ACT 缓存容量发布策略 |
| `0x00430C60..0x004311C0` 的 9 个入口 | 9 | 资产命令流使用的公共数值/缓冲变换 |
| `0x004315C0..0x00431F80` 的 11 个入口 | 11 | 六包 TSW 延迟打开、帧查询、缓存命中/装载/淘汰与关闭 |
| `0x00432010..0x00433220` 的 16 个入口 | 16 | 六包 ACT 延迟打开、两级缓存、变体切片和 `0x98` 动作更新器 |
| `0x00433270..0x00433BF0` 的 9 个入口 | 9 | TSW/ACT 物理文件对象与帧主流装载边界 |
| `0x00433C40..0x004342E0` 的 9 个入口 | 9 | 资产视图建立、释放及调用侧桥接 |
| `0x00434350..0x00434DD0` 的 5 个入口 | 5 | 动作/图像运行时的提交与复合处理 |
| `0x004350E0` | 1 | 当前范围末端的公共资产 helper |

合计严格为 78。分组只决定实施顺序，不把尚未逐基本块闭环的 helper 提前命成最终
玩法概念。实施中发现内部 helper 时仍留在本工作包，不扩大成新的总体调研阶段。

不属于 B6 的内容：地图碰撞和寻路、剧情 opcode、特殊模式 owner、战斗状态机、音频
设备与媒体解码、最终 SDL 呈现。它们只通过端口借用 B6 的视图或动作记录。

## 2. 公共接口与状态所有权

### TSW

- 逻辑查询键是两个低 16 位值；物理旧 ABI 的两个栈槽均为四字节。
- runtime 拥有六个包句柄、索引/帧缓存节点、解压命令流和 8 位包调色板。
- 返回给 rendering/world/battle 的是借用帧视图：主命令流、可选调色板、宽、高和
  主流字节数；不是提前转换的 RGBA 位图。
- 资源号 `0xFFFF` 的特殊分支保留，不在公共入口提前判无效。

### ACT 与通用动作记录

- ACT 查询键保留完整 32 位 `action_id` 和 `variant_index`，不能套用 TSW 的 16 位截断。
- runtime 拥有 10 桶索引节点和 10 桶选中流节点；动作记录的 `+0x54` 只借用缓存拥有
  的 16 位命令流。
- 通用动作记录物理边界固定为 `0x98` 字节。父对象拥有记录存储，asset_runtime 拥有
  初始化、三段键变化、选择性重置、游标、等待和 33 个命令字的更新语义。
- 不用 C++ 整结构 value-initialize 替代汇编的局部清零，也不修复两参数命令的第二参数
  再分派行为。

### ANI

- ANI runtime 拥有活动文件、`0x24` 头、帧链节点、压缩暂存、命令暂存和 256 色表。
- 剧情/场景 owner 负责开始、推进和结束请求；rendering 只接收 span 结果或目标 surface
  端口，不拥有文件或帧节点。
- 一基帧号、首次载荷排除尾 12 字节、缓存重读包含尾 12 字节、零载荷保持帧及声明
  span 数停止线都保留。

### SND 借用边界

SND 的 3000 项索引、载荷 buffer、引用计数和播放生命周期已经由 `audio_video` 持有。
B6 只保存或传递原始 sound ID，并经音频端口提交播放/停止请求；不解析 `all.snd`，不持有
sample buffer，也不建立第二套 SND cache。后期 `libffmpeg` 同样不改变这条所有权边界。

## 3. 依赖与生命周期

允许方向为：

```text
resource_io/file/decompress  → asset_runtime owned storage
runtime_platform memory port → cache-limit policy
asset_runtime borrowed view  → rendering/world/story/special/battle
asset_runtime sound request  → audio_video port
```

启动时先发布缓存上限；TSW/ACT 六包仍按首次查询延迟打开。场景或剧情建立 ANI 活动对象，
每帧由 owner 推进，结束时释放。通用动作记录随父对象建立和销毁；记录本身不得释放借用的
ACT 缓存流。总退出按 owner 顺序先停止消费者，再关闭 ANI、清 ACT/TSW 节点和文件。

## 4. 实施顺序

1. `[x]` `0x00424330/0x004315C0/0x00432010`：32 位物理内存样本除以六，TSW
   夹在 4–16 MiB，ACT 固定 512 KiB，并保留发布顺序和恒一返回；Linux `core`
   77/77、Windows LLVM `app` 81/81 CTest 通过。
2. `[x]` TSW 查询最小闭环：共享读一次打开六包、3000 物理槽、低 16 位键、
   `0xFFFF` 特殊加载端口、direct/cached/cache-only 三入口、indexed8 转 direct16、
   十桶 MRU 与原版先淘汰顺序；六个真实包首帧物理输出哈希及合成运行时向量通过，
   Linux `core` 80/80、Windows LLVM `app` 84/84 CTest 通过。
3. `[x]` ACT 变体查询最小闭环：六包独占读延迟打开、完整 32 位键、44 字节
   物理索引、零洞切片、十桶索引缓存与十桶选中流缓存、失败节点保留、命中移首和原版
   先淘汰顺序；六个真实包固定切片哈希及合成缓存向量通过，Linux `core` 83/83、
   Windows LLVM `app` 87/87 CTest 通过。
4. `[x]` `0x98` 动作记录：精确物理布局、选择性初始化、三段键重置、ACT 查询、
   跨帧等待、33 个命令字和原版第二参数再分派均已实现；六包真实 ACT 已经由
   runtime→provider→updater 执行。Linux `core` 85/85、Windows LLVM `app` 89/89
   CTest 通过。
5. `[~]` ANI：`0x24` 头、独占打开、一基帧节点链、首次排除/缓存包含 12 字节、
   RGB 调色板、零载荷保持和 span 写入已实现；19 个真实文件的 5,312 帧、
   6,057,767 个 span、178,426,082 个索引像素及每文件首帧哈希通过。
   `0x004154A0` 的冻结快照、`-13` 揭幕、逐帧推进、两种结束阈值、释放顺序和三个
   外部端口已实现。Linux `core` 88/88、Windows LLVM `app` 92/92 CTest 通过；
   当前继续收口 ANI 组剩余自有入口。
6. `[ ]` 其余公共变换和调用桥接，最后做 78 地址有限收口。

只有当前一项占执行位。每一项达到可独立验证边界就实现，不等待后面各项全部逆向。

## 5. 验证入口

- 缓存策略：`0`、24 MiB 临界前后、48 MiB、96 MiB 临界和 `UINT32_MAX`；fake port
  断言 memory→TSW→ACT 顺序与恒一返回。
- TSW：六个真实包的 18,000 槽、2,928 非空块、20,091 帧、主流输出长度与既有哈希。
- ACT：六个真实包的 1,500 索引、4,326 非空变体、33 命令和 9,209 次第二参数再分派。
- ANI：19 个真实文件、5,312 帧、首次/重读返回差异、6,057,767 span 和拼接哈希。
- 动作记录：固定字节快照、fake ACT stream、跨帧等待、重置保留字段、全部命令小组，
  以及六个真实 ACT 包的更新器集成测试。
- 原程序差分：需要时准备 Frida spawn 工具，由用户运行；OpenSWD3 不自行启动原 EXE。

当前不需要新的原程序动态捕获即可开始缓存策略、TSW/ACT 有效资产路径和动作状态机实现。
最终 framebuffer/时序差分保留为 `blocked_runtime_oracle`，不冒充 UT 或资产哈希已完成。

## 6. 证据

- [`asset-cache-limits-00424330.md`](../evidence/asset-cache-limits-00424330.md)
- [`tsw-archive-format.md`](../evidence/tsw-archive-format.md)
- [`resource-entry-abi.md`](../evidence/resource-entry-abi.md)
- [`act-action-stream-format.md`](../evidence/act-action-stream-format.md)
- [`action-field-state-machine.md`](../evidence/action-field-state-machine.md)
- [`action-subrecord-004321e0.md`](../evidence/action-subrecord-004321e0.md)
- [`action-object-families.md`](../evidence/action-object-families.md)
- [`action-external-consumers.md`](../evidence/action-external-consumers.md)
- [`ani-container-and-lzo-boundary.md`](../evidence/ani-container-and-lzo-boundary.md)
- [`legacy-snd-runtime-004862b0-00486490.md`](../evidence/legacy-snd-runtime-004862b0-00486490.md)
