# asset_runtime 工作包

最后更新：2026-08-10

状态：B6 已完成有限收口；59 个自有地址均有实现/端口映射，进入 B7

唯一行为真值：`swd3.exe.lst`。IDA 名称和伪码只用于定位。

## 1. 范围与停止线

早期按地址簇得到的 78 个候选已经逐项核对完整汇编和调用者。19 个机械误归属已转交
真实 owner，B6 的最终有限范围为 59 个地址：

| 地址集合 | 数量 | 当前职责 |
|---|---:|---|
| `0x00401190` | 1 | 全局 framebuffer 变形链哨兵初始化 |
| `0x0040DC00`、`0x0040EBF0`、`0x0040EC80`、`0x0040ECC0` | 4 | 公共动作记录初始化及跨模块查询/绘制适配 |
| `0x004154A0..0x00416CC0` 的 11 个自有入口 | 11 | ANI 活动对象、帧链、调色板、span 提交和相关动作记录 |
| `0x00424330` | 1 | TSW/ACT 缓存容量发布策略 |
| `0x00430C60..0x0043114C` 的 7 个入口 | 7 | 双工作场 framebuffer 变形节点的建立、采样、推进和注入 |
| `0x004315C0..0x00431F80` 的 11 个入口 | 11 | 六包 TSW 延迟打开、帧查询、缓存命中/装载/淘汰与关闭 |
| `0x00432010..0x00433220` 的 16 个入口 | 16 | 六包 ACT 延迟打开、两级缓存、变体切片和 `0x98` 动作更新器 |
| `0x00433270..0x00433BF0` 的 8 个入口 | 8 | TSW 物理文件对象、普通/特殊帧装载边界与特殊 loader 端口 |

合计严格为 59。原 78 候选中另有 19 项按汇编转交：`0x0040AD10` 给 `world_map`，
`0x00431150/0x004311C0` 给 `runtime_platform`，`0x00433AA0` 和
`0x00433C40..0x00434DD0` 的 14 项给 `battle`，`0x004350E0` 给 `rendering`。
逐地址处置见 `asset-runtime-closure.tsv`，所有权依据见
`asset-runtime-closure-audit.md`。

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

### 通用动作绘制桥

- `0x0040EBF0` 先更新动作，再读取更新后的 TSW 键、偏移、完整 flags 和 opacity 字节；
  目标坐标使用 32 位回绕减法。
- `0x0040EC80` 不更新动作，两个四字节参数槽各截取低 16 位，按原坐标以零 flags/opacity
  绘制。
- `0x0040ECC0` 同样先更新动作，最终 flags 为调用者 flags 与更新后动作 flags 的
  `0x80000003` 掩码结果按位或。
- 三个入口统一映射到 `legacy_action_draw_bridge.cpp`；现代 TSW 失败隔离只保护非法宿主
  边界，有效路径顺序不变，旧调用者忽略 blit 返回值的行为也保留。

### ANI

- ANI runtime 拥有活动文件、`0x24` 头、帧链节点、压缩暂存、命令暂存和 256 色表。
- 剧情/场景 owner 负责开始、推进和结束请求；rendering 只接收 span 结果或目标 surface
  端口，不拥有文件或帧节点。
- 一基帧号、首次载荷排除尾 12 字节、缓存重读包含尾 12 字节、零载荷保持帧及声明
  span 数停止线都保留。

### framebuffer 变形节点

- 节点拥有两个连续 `i16` 工作场与一份完整 16 位 framebuffer 快照；剧情、世界和战斗
  只借用节点方法，不拥有这些存储。
- `0x004AC990 + 0x28` 就是链头 `0x004AC9B8`。重写的 `1×1` 哨兵直接拥有首节点，
  不复制成两个可能失同步的全局变量。
- 径向注入的负坐标分支借用 B3 的 CRT RNG，调用次数和先 x 后 y 的顺序固定。

### ANI framebuffer 逐行复制效果

- `0x004163C0` 的参数状态由 64 项行数、64 项字节宽度、64 项像素偏移和一个 `u16`
  帧计数器组成；实际 framebuffer 拷贝只遍历前 48 项。
- 零计数器只初始化前 48 项；周期刷新却分别写 64 项宽度、48 项偏移和 64 项行数。
  后 16 项虽不参与拷贝，仍必须保留其写入和第二套 RNG 消耗。
- 每段把下一条 `0x500` 字节物理行的指定字节范围前向复制到当前行；调用者拥有
  service 7，asset runtime 只接收启用结果并借用固定 16 位 framebuffer。

### ANI 目标跟随双帧效果

- `0x00416B30` 由 service `0x13` 启用，共用一份 `0x98` 动作记录依次执行
  动作 `0x232B` 的变体 78 和 79，再以更新后的 `+0x4A/+0x4C` 查询
  TSW 并绘制两帧。
- 第一帧裁剪矩形保留原指令宽高交叉 BUG，第二帧使用中心周围 `384×384`
  裁剪和 `0x2C` flags；两帧都按自身宽高正常居中。
- 两帧之后才按 `i32` 回绕加法移动坐标，只在加法后精确命中目标时清除
  对应速度；不做越过夹取。剧情 owner 持有 service 和六个坐标/速度字段。

### ANI 下落拖尾效果

- `0x00416590` 持有 64 个 `0x10` 字节槽，但初始化只清前 48 个；每槽保留
  1/16 像素坐标、步进、包含上界、寿命和完整 active word。
- 每帧先无条件消耗 `random(1000)`，严格大于 900 才查 service 8；新槽依次
  消耗 x、水平步进、纵向步进和拖尾长度四个有界 RNG。
- 拖尾从第 1 行开始，通过现有 `0x004239D0/0x00420490` 对应像素 helper 饱和
  增加灰度通道；越底边清 active 后仍可因寿命非零增加存活计数。

### ANI 九点星芒效果

- `0x004167B0` 拥有 96 个 `0x10` 字节槽；进程 loader 首次零填充，但场景初始化
  只清上一帧存活数和每帧创建目标两个 `i16`，不清槽池。
- 每帧无条件消耗 `random(1000)`，严格大于 900 才查 service `0x16`；新槽按
  x、水平步进、垂直步进、剩余高度四步 RNG 建立，创建帧不绘制。
- 每点以相位表偏移中心，对中心、四邻点和四对角执行九次饱和增亮；硬编码可绘制
  行为 2..478，x 不裁剪并允许在平坦 framebuffer 上跨行。
- 越过 479 行清 active 后仍会执行 16 位状态更新，并可能因剩余高度为正增加存活
  计数；该一帧不一致按汇编保留。

### ANI 四槽漂移动作效果

- `0x004161C0` 固定拥有四个 `0x10` 世界坐标/速度槽和四个一一对应的 `0x98`
  动作记录；启动和场景重置只写四个 x 哨兵，动作记录统一使用 action `0x232B`。
- service 6 关闭时零状态/RNG/资产副作用；越界槽从地图左右 64 像素外按三步 RNG
  重生并在同一帧绘制，活动槽先消耗概率 RNG，再按水平、垂直顺序扰动速度。
- 垂直速度经八字节表映射到变体 52/54/56，正水平速度再加一；ACT 更新后的
  `+0x4A/+0x4C` 查询 TSW，绘制位置只减动作偏移与 camera，最后才移动世界坐标。

### ANI 角色绑定粒子链

- `0x00415EE0` 以四个 `0x10` 发射器持有四条 `0x10` 动态节点链；selector 按原始
  有符号槽值对比调用者零扩展值，角色坐标由 world 端口逐槽重读。
- 动作 `0x232B`、变体 59 和 TSW 查询发生在发射器匹配之前；创建与更新严格保留
  secondary RNG 次序、`i16` 回绕、map 50 独立颜色衰减和创建帧立即绘制。
- 非尾过期节点复制完整后继、释放后继并在同帧重新处理；跨平台实现以一基 `u32`
  token 保持原始 32 位链字段和 `0x10` 物理布局。

### ANI 双槽方向状态效果

- `0x00415B70` 保留 motion、color、timing 三组物理分离的四槽 `0x10` 数组；地图装载
  初始化四槽，但每帧只更新前两槽。场景重置只覆盖 motion，不清另外两组。
- service 5 关闭时零副作用；范围内槽复用同一个概率值控制间隔和颜色逼近，范围外槽
  只在 1% 门命中时按四个方向重生，variant 也复用该概率值分桶而不另取 RNG。
- 共享动作固定使用 `0x232B`，variant count 为 4 时绘制 flags 为 4，否则为 `0x2C`；
  ACT→TSW→blitter 和全部 `i32` 回绕均已按汇编闭环。

### SND 借用边界

SND 的 3000 项索引、载荷 buffer、引用计数和播放生命周期已经由 `audio_video` 持有。
B6 只保存或传递原始 sound ID，并经音频端口提交播放/停止请求；不解析 `all.snd`，不持有
sample buffer，也不建立第二套 SND cache。后期 `libffmpeg` 同样不改变这条所有权边界。

## 3. 依赖与生命周期

允许方向为：

```text
resource_io/file/decompress  → asset_runtime owned storage
input_time_rng CRT rand       → deformation injection
input_time_rng secondary RNG  → ANI row-copy parameters
runtime_platform memory port → cache-limit policy
asset_runtime borrowed view  → rendering/world/story/special/battle
asset_runtime sound request  → audio_video port
```

静态初始化先建立 `1×1` 变形链哨兵。启动时发布缓存上限；TSW/ACT 六包仍按首次查询
延迟打开。场景或剧情建立 ANI 活动对象，每帧由 owner 推进，结束时释放。通用动作记录随
父对象建立和销毁；记录本身不得释放借用的 ACT 缓存流。总退出按 owner 顺序先停止
消费者，再清变形节点、关闭 ANI、清 ACT/TSW 节点和文件。

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
5. `[x]` ANI：`0x24` 头、独占打开、一基帧节点链、首次排除/缓存包含 12 字节、
   RGB 调色板、零载荷保持和 span 写入已实现；19 个真实文件的 5,312 帧、
   6,057,767 个 span、178,426,082 个索引像素及每文件首帧哈希通过。
   `0x004154A0` 的冻结快照、`-13` 揭幕、逐帧推进、两种结束阈值、释放顺序和三个
   外部端口已实现；`0x00416CC0` 的逐节点 framebuffer 快照、变形、推进、完成摘链和
   继续遍历也已闭环。`0x004163C0` 的 64 项状态、48 项有效块、16 帧刷新周期、第二套
   RNG 顺序和下一物理行前向复制也已实现。`0x00416B30` 的 service
   门、变体 78/79 双帧、裁剪 BUG 和目标跟随状态也已闭环。`0x00416590` 的
   48/64 槽差异、概率 service 门、拖尾像素和存活计数异常已闭环。`0x004167B0`
   的 96 槽、仅计数器重置、九点星芒核、跨行 x 和存活计数异常也已闭环。
   `0x004161C0` 的四槽/四动作记录、包含边界、重生与扰动 RNG、变体表、ACT→TSW
   绘制和帧尾移动也已闭环。`0x00415EE0` 的四发射器、角色绑定、节点链、创建与
   删除异常也已闭环。`0x00415B70` 的三组四槽状态、双槽更新、同值概率门、四向
   重生和共享动作绘制也已闭环；ANI 组的 11 个自有入口已全部完成实现映射。
6. `[x]` `0x00430C60..0x0043114C`：`0x2C` 变形节点、双 `i16` 工作场、固定场 0
   warp、跨行 carry 更新、16 位衰减、径向注入、CRT 随机坐标与哨兵链表调度已实现。
   Linux `core` 89/89、Windows LLVM `app` 93/93 CTest 通过。
7. `[x]` `0x004163C0`：service 7 启用门、64 项初始状态、48 项首次生成和实际拷贝、
   64/48/64 项周期刷新、第二套无偏 RNG 顺序、`u16` 计数回绕及下一物理行复制已实现。
   Linux `core` 90/90、Windows LLVM `app` 94/94 CTest 通过。
8. `[x]` `0x00416B30`：service `0x13` 门、变体 78/79 的 ACT→TSW 查询、双帧绘制、
   宽高交叉裁剪 BUG、`0x2C` 标志、绘制后目标跟随及 `i32` 回绕已实现；
   真实 ACT/TSW 路径通过，Linux `core` 92/92、Windows LLVM `app` 96/96 CTest 通过。
9. `[x]` `0x00416590`：48/64 槽重置差异、条件 service 8、四步 RNG 创建、第 0 行跳过、
   包含上界拖尾、逐点饱和增亮、`i16` 乘加回绕及越底边计数异常已实现；
   Linux `core` 93/93、Windows LLVM `app` 97/97 CTest 通过。
10. `[x]` `0x004167B0`：96 槽与只清计数器的重置差异、条件 service `0x16`、
    四步 RNG 创建、八段相位、九点饱和增亮、硬编码上下边界、横向跨扫描线、
    `i16` 状态推进及越底边计数异常已实现；Linux `core` 94/94、Windows LLVM
    `app` 98/98 CTest 通过。
11. `[x]` `0x004161C0`：四个 `0x10` 状态槽和四个 `0x98` 动作记录、只写 x 的
    重置、service 6、三步重生 RNG、活动槽概率/扰动 RNG、速度夹取与归零特殊行为、
    八字节变体表、ACT→TSW→blitter 以及帧尾移动已实现；真实四变体 framebuffer
    哈希为 `0x53695F8D8D2219DF`，Linux `core` 96/96、Windows LLVM `app` 100/100
    CTest 通过。
12. `[x]` `0x00415EE0`：四个 `0x10` 发射器、四条 `0x10` 节点链、包含边界、
    selector 符号不对称、完整 secondary RNG 顺序、`i16` 回绕、map 50 颜色、
    复制后继删除异常和 ACT→TSW→blitter 已实现；真实 variant 59 framebuffer
    哈希为 `0xFA22737232A60CF6`，Linux `core` 98/98、Windows LLVM `app` 102/102
    CTest 通过。
13. `[x]` `0x00415B70`：三组分离的四槽状态、四槽初始化/双槽更新差异、motion-only
    重置、service 5、同一概率值驱动范围内目标更新与范围外 variant 分桶、四向重生、
    ACT→TSW→blitter 和 `i32` 回绕已实现；真实 variant 0 framebuffer 哈希为
    `0xE216591950463029`，Linux `core` 100/100、Windows LLVM `app` 104/104 CTest 通过。
14. `[x]` `0x0040EBF0/0x0040EC80/0x0040ECC0` 公共动作绘制桥已实现；更新后字段读取、
    32 位回绕坐标、低 16 位 TSW 键、flags 掩码和被忽略的 blit 错误均由 UT 固定。
    原 78 地址候选完成逐项审计，59 项 B6 自有映射与 19 项跨模块转交记录在
    `asset-runtime-closure.tsv`；真实 ACT→TSW→blitter 哈希为
    `0xE216591950463029`，Linux `core` 102/102、Windows LLVM `app` 106/106
    CTest 通过。B6 状态为 `module_closed_pending_oracle`，不再继续扩张。

只有当前一项占执行位。每一项达到可独立验证边界就实现，不等待后面各项全部逆向。

## 5. 验证入口

- 缓存策略：`0`、24 MiB 临界前后、48 MiB、96 MiB 临界和 `UINT32_MAX`；fake port
  断言 memory→TSW→ACT 顺序与恒一返回。
- TSW：六个真实包的 18,000 槽、2,928 非空块、20,091 帧、主流输出长度与既有哈希。
- ACT：六个真实包的 1,500 索引、4,326 非空变体、33 命令和 9,209 次第二参数再分派。
- ANI：19 个真实文件、5,312 帧、首次/重读返回差异、6,057,767 span 和拼接哈希。
- 动作记录：固定字节快照、fake ACT stream、跨帧等待、重置保留字段、全部命令小组，
  以及六个真实 ACT 包的更新器集成测试。
- 变形节点：字段/容量、原点上界、成对 warp、30-word 跨行 carry 固定向量、径向注入、
  CRT 随机坐标、哨兵头插以及完成/保留两种摘链路径。
- ANI 逐行复制：固定 RNG 参数向量、64/48 项刷新差异、16 帧周期、计数回绕、逐字节
  前向复制和 framebuffer 哈希 `0x8ED9811DCD93C1F1`。
- ANI 目标跟随：双变体顺序、TSW 键传递、两组裁剪/绘制坐标、非方形宽高交叉、
  service 早退、blit 返回值忽略、单轴等值、越过目标和 32 位回绕；真实变体
  79 最终键 `(9225, 0)` 并完成 blit。
- ANI 下落拖尾：前 48/全 64 槽、概率门与条件 service 调用、四个创建 RNG、
  bit 0 活动判定、第 0 行跳过、包含上界、逐点灰度增亮、底边与寿命计数异常，
  以及固定 framebuffer 哈希 `0x7403975F3AB69BDD`。
- ANI 九点星芒：loader 零填充与只清计数器、96 槽、service `0x16`、四个创建 RNG、
  相位表、强度除法、九次像素调用、横向跨扫描线、上下硬边界、底边与存活计数异常，
  以及固定 framebuffer 哈希 `0xF7080E84910EFC5B`。
- ANI 四槽漂移：四槽/动作记录一一对应、只写 x 的重置、service 6 零副作用、包含
  外边界、左右重生、固定 RNG 顺序、速度夹取与归零特殊行为、变体表、动作偏移、
  camera、帧尾移动和被忽略的 blit 错误；真实四变体 framebuffer 哈希
  `0x53695F8D8D2219DF`。
- ANI 角色粒子链：四槽物理布局、裁剪零副作用、selector 符号不对称、角色坐标回读、
  bit 0 概率门、完整创建/更新 RNG 顺序、`i16` 回绕、两套颜色公式、复制后继删除、
  被忽略的 blit 错误和现代失败隔离；真实 variant 59 framebuffer 哈希
  `0xFA22737232A60CF6`。
- ANI 双槽方向状态：三组四槽物理布局、motion-only 重置、四初始化/二更新、包含边界、
  同 roll 概率与 variant 分桶、四向 RNG 顺序、无效方向、两套 flags、`i32` 回绕和
  blit 失败隔离；真实 variant 0 framebuffer 哈希 `0xE216591950463029`。
- 通用动作绘制桥：更新成功/失败边界、更新后读取全部字段、32 位回绕减法、直接入口
  低 16 位截断、`0x80000003` 掩码、TSW 失败隔离和 blit 诊断保留。
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
- [`ani-follower-effect-00416b30.md`](../evidence/ani-follower-effect-00416b30.md)
- [`ani-streak-effect-00416590.md`](../evidence/ani-streak-effect-00416590.md)
- [`ani-spark-effect-004167b0.md`](../evidence/ani-spark-effect-004167b0.md)
- [`ani-drift-effect-004161c0.md`](../evidence/ani-drift-effect-004161c0.md)
- [`ani-role-particle-effect-00415ee0.md`](../evidence/ani-role-particle-effect-00415ee0.md)
- [`ani-directional-effect-00415b70.md`](../evidence/ani-directional-effect-00415b70.md)
- [`ani-container-and-lzo-boundary.md`](../evidence/ani-container-and-lzo-boundary.md)
- [`frame-deformation-00430c60.md`](../evidence/frame-deformation-00430c60.md)
- [`ani-row-copy-effect-004163c0.md`](../evidence/ani-row-copy-effect-004163c0.md)
- [`legacy-snd-runtime-004862b0-00486490.md`](../evidence/legacy-snd-runtime-004862b0-00486490.md)
- [`asset-runtime-closure-audit.md`](../evidence/asset-runtime-closure-audit.md)
- [`asset-runtime-closure.tsv`](../inventory/asset-runtime-closure.tsv)
