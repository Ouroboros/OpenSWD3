# B5 `audio_video` 工作包

状态：实施中

当前单元：B5.7 stream manager 与剩余游戏侧 stream wrapper

## 1. 范围与移交

完整 LST 是唯一行为真值；IDA 伪码和函数名只用于定位。机械所有权目录给出 76 个
`audio_video` 候选，逐函数接口复核后，B5 自有 73 个，另 3 个移交 B10：

- `0x004841B0` 只修改 battle 对象字段、取得/释放战斗图像记录，唯一调用者为
  `0x00455D60`；没有 Miles/Bink 行为。
- `0x00484230` 由 `0x00474E60` 调用，主要建立战斗对象绘制状态；调用音效 wrapper
  只是该战斗动作的副作用，不转移函数所有权。
- `0x00484500` 的 8 个调用者全部属于 battle，只从 battle 对象两种布局取得坐标。

B5 自有地址按可验证职责固定为：

- 世界/媒体协调（5）：`0x0040CDD0`、`0x0040CF10`、`0x0040CF40`、
  `0x0040EB60`、`0x004118B0`。
- Bink 视频（11）：`0x00484550`、`0x004845A0`、`0x00484620`、
  `0x00484650`、`0x004846F0`、`0x00484710`、`0x00484720`、
  `0x00484730`、`0x00484920`、`0x00484950`、`0x00484DA0`。
- Miles 启动、sequence 与排队切换（10）：`0x00484DD0`、`0x00484F60`、
  `0x00485180`、`0x00485290`、`0x004852C0`、`0x004852F0`、
  `0x00485330`、`0x00485360`、`0x00485460`、`0x004854B0`。
- 游戏侧音频 wrapper 与空间音效（13）：`0x00485610`、`0x00485650`、
  `0x00485670`、`0x004856C0`、`0x00485710`、`0x00485720`、
  `0x00485740`、`0x00485750`、`0x00485830`、`0x00485850`、
  `0x00485880`、`0x004858D0`、`0x00485910`。
- sample/SND 管理（22）：`0x00485960`、`0x004859B0`、`0x00485C20`、
  `0x00485CC0`、`0x00485CD0`、`0x00485CE0`、`0x00485E90`、
  `0x00485F30`、`0x00485FE0`、`0x00486030`、`0x00486080`、
  `0x00486160`、`0x00486190`、`0x004861B0`、`0x004861D0`、
  `0x00486210`、`0x00486260`、`0x00486280`、`0x004862A0`、
  `0x004862B0`、`0x00486430`、`0x00486490`。
- stream 管理（12）：`0x004865B0`、`0x004866A0`、`0x004866B0`、
  `0x004866C0`、`0x00486730`、`0x00486860`、`0x004868E0`、
  `0x00486900`、`0x00486A10`、`0x00486A30`、`0x00486A50`、
  `0x00486A70`。

SND 的磁盘字节和文件访问由 `resource_io` 提供；B5 拥有 3000 项运行时索引、返回
sample buffer、引用计数及回收。`rendering` 拥有 framebuffer 和 present；B5 只在
原视频分支提交解码结果。脚本、世界、菜单和战斗只提交播放/停止/音量请求，不持有
后端对象。

## 2. 公共接口与状态所有权

兼容核心只暴露固定宽度值和平台无关端口：

- sample 请求包含一基 sound ID、原始 32 位 volume/pan/loop/aux 值；有效路径不得
  因现代 API 改成成功布尔值或重新排序。
- sequence/stream 请求保留 user-data ID、循环次数、音量和逐帧 status 迁移；
  `service` 仍是原忙等/文件循环中的显式维护点。
- 视频端口提供 open、wait、decode、copy、advance、service、close；`wait != 0`
  必须继续表示当帧跳过解码。解码器不拥有窗口或最终呈现。
- 媒体源端口提供数据根和文件存在性。旧 CD 轮询只作为显式兼容回退，正常启动使用
  已配置的数据目录，不复制 Win32 drive-letter API。

B5 拥有四组逻辑运行态：

1. `0x004C8450` sample/SND manager 及其 `+0x67C` 3000×16 索引；空闲/活动 sample
   槽、单槽 ref-count 和最后 buffer 指针必须按汇编恢复。
2. `0x004B7EB8` sequence、`0x004C9288` stream、`0x004BA6E8` 排队切换对象；现代
   C++ 可以拆成明确结构，但状态迁移、链表顺序和回收时点不能合并。
3. `0x0053D070` 活动视频对象与 `0x0053D06C` copy surface type 的兼容语义；Bink
   句柄和 DLL ABI 只存在于替换端口之外的历史证据中。
4. `0x004ACDBC`、`0x004B7C74` 起的音乐请求槽，以及 `0x004B7380`、
   `0x004B7378`、`0x004B74F0` 的 stream 过渡状态。其业务位名只在写入点证明后命名。

进程生命周期位 `0x004B7A9C` 仍由 `runtime_platform` 拥有；B5 只提交/清除已证明的
视频活动位 `0x20` 和媒体失败关闭请求 `0x04`。

## 3. 生命周期与依赖

启动顺序严格保留：Miles service 启动 → sample/SND manager → 第一次 driver 查询与
sequence manager → 第二次 driver 查询与 stream manager → 显示建立 → 第三次 driver
查询与视频声音/copy 配置。三次 driver 查询不能合并，旧调用者忽略的返回也不能改成
新的业务分支。

无消息循环中，活动视频优先执行视频 wait/decode/copy/present，再执行公共音频维护；
普通有效帧先在原世界/模式位置提交音频请求，未提前返回时再维护 queue、stream、
sequence、sample。文件/提示忙等中的 `AIL_serve` 映射到同一个显式 service 端口。

显示停用按原顺序停止/过渡 stream、停止 sample 并维护一次；恢复没有伪造一次全新
音频初始化。总退出按活动对象门控回收视频、sample buffers、SND 表、sequence/stream
节点和设备后端，允许部分初始化后退出。

新依赖方向固定为：`audio_video` 依赖 `compat` 与 `resource_io` 的只读文件接口；
framebuffer/present、时钟/让出和生命周期都通过调用端口注入。SDL3 只负责最终音频
设备输出；具体音频/视频解码器通过独立端口接入，不把 Miles/Bink 或 SDL 类型放入
兼容核心。

## 4. 验证入口与开始条件

- 纯参数：锁定 `0x00486260` 的 signed clamp 和 `0x00486280` 的 32 位回绕加 63 后
  signed clamp，覆盖边界、负值和溢出。
- SND：用真实 `all.snd` 验证固定 3000 槽、一基访问、两位类型、当前 664 个 view、
  两个跨块长 view，以及原 RIFF 重叠复制缺陷；不得修复 ID 506/507。
- sample/sequence/stream：以 fake backend 记录每个 call、user-data、status、volume、
  pan、loop、链表移动和 buffer 引用，逐基本块比较事件序列。
- 视频：以 fake decoder 固定 wait/帧号/尺寸/失败结果，比较 framebuffer hash、居中和
  present 请求；原版 Bink/DirectDraw RECT 差分保持 `blocked_runtime_oracle`，需要时只
  准备 Frida 工具并等待用户运行原版。
- 平台输出：比较解码后的 PCM 与请求时序，不以不同宿主音频设备的模拟波形作为核心
  判定。

73 个 B5 地址、跨模块接口、状态 owner、启动/帧/停用/退出顺序、首批 UT 边界和真实
资产入口已经固定，达到单模块开始条件。sample 参数、SND、manager、输出 backend 和
游戏侧 sample wrapper 已逐层完成；当前沿相同依赖方向恢复 stream manager，不等待
sequence 与 Bink backend 一次性调研完毕。

## 5. 已有证据

- [`audio-video-entry-abi.md`](../evidence/audio-video-entry-abi.md)
- [`audio-frame-0040cdd0.md`](../evidence/audio-frame-0040cdd0.md)
- [`snd-archive-format.md`](../evidence/snd-archive-format.md)
- [`presentation-lifecycle.md`](../evidence/presentation-lifecycle.md)
- [`platform-backend-initialization-00424ef0.md`](../evidence/platform-backend-initialization-00424ef0.md)
- [`p4-dynamic-oracle-capture-protocol.md`](../evidence/p4-dynamic-oracle-capture-protocol.md)
- [`legacy-audio-parameters-00486260-00486280.md`](../evidence/legacy-audio-parameters-00486260-00486280.md)
- [`legacy-snd-runtime-004862b0-00486490.md`](../evidence/legacy-snd-runtime-004862b0-00486490.md)
- [`legacy-sample-manager-004859b0-00486430.md`](../evidence/legacy-sample-manager-004859b0-00486430.md)
- [`legacy-audio-output-004859b0-00485ca6.md`](../evidence/legacy-audio-output-004859b0-00485ca6.md)
- [`legacy-sample-commands-00485610-00485828.md`](../evidence/legacy-sample-commands-00485610-00485828.md)

## 6. 当前执行顺序

1. `[x]` B5.1：73 个 B5 自有地址、3 个 B10 移交、接口、状态 owner、生命周期和验证
   入口已固定，达到单模块开始条件。
2. `[x]` B5.2：`0x00486260/0x00486280` 已按完整 LST 实现 signed clamp 与 32 位
   回绕声像偏移；Linux `core` 65/65、Windows LLVM `app` 67/67 CTest 通过。
3. `[x]` B5.3：`0x004862B0/0x00486490` 的 3000 项索引、一基查找和四类 sample
   buffer 已实现；真实 `all.snd` 的 664 个 view 拼接 SHA-256 精确匹配既有证据，Linux
   `core` 67/67、Windows LLVM `app` 69/69 CTest 通过。
4. `[x]` B5.4：`0x004859B0` 的 pool 部分及 `0x00485C20–0x00486210`、
   `0x004862A0/0x00486430` 的 sample manager 核心已实现；fake backend 锁定两条单链表、
   播放/停止顺序、status 1/2、返回值及原始 buffer 泄漏语义。Linux `core` 68/68、
   Windows LLVM `app` 70/70 CTest 通过。
5. `[x]` B5.5：`0x004859B0` 的 wave 格式协商/回退已按 LST 恢复；SDL3 backend 已
   接通逻辑设备、PCM 转换、动态混音、loop/status 与 `0x00485C20` 设备关闭边界。真实
   `all.snd` 的 13 种 PCM 组合及异常 data 标签通过，Linux `core` 69/69、Linux/Windows
   `app` 73/73 CTest 通过。
6. `[x]` B5.6：`0x00485610/0x00485650/0x00485670/0x00485720/`
   `0x00485740/0x00485750` 已按 LST 恢复；锁定 ID 截断差异、固定返回、signed 缩放、
   512 距离门槛和空间 play→volume→pan 顺序。已恢复的两处 stop-all 调用点接入同一
   manager，Linux `core` 70/70、Linux/Windows `app` 74/74 CTest 通过。
7. `[>]` B5.7：恢复 `0x004865B0–0x00486A70` stream manager 核心，以及
   `0x004856C0/0x00485710/0x00485830/0x00485850/0x00485880/0x004858D0/`
   `0x00485910` 的剩余 stream wrapper 和过渡状态；不扩展 sequence 或 Bink。
