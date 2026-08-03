# A4 模块依赖、生命周期与平台边界

状态：A4 依赖方向、关键生命周期和旧平台边界复核完成

## 1. 依赖图口径

- [`module-dependencies.tsv`](../inventory/module-dependencies.tsv) 以“依赖方 → 被调用模块或状态所有者”为方向，聚合 A2 完整汇编直调关系和 A3 跨模块状态合同。
- [`build_module_dependencies.py`](../../tools/build_module_dependencies.py) 锁定 A2/A3 输入摘要并重新生成依赖表。
- 直调数量只统计可落到已知函数入口的完整汇编 `call`；状态关系只使用 A3 已人工复核的所有权、写入和借用记录。

当前图共有 87 条有向模块依赖，覆盖 1,648 个跨模块函数对和 3,566 个静态调用点；状态合同另外落到 28 条跨所有者写入边和 47 条跨所有者读取边。表内 `reverse_dependency_present=yes` 给出全部直接双向关系，共 34 对；`cycle_component` 给出长环归属。

11 个游戏模块全部位于同一个强连通分量 `cycle_01`：

```text
asset_runtime, audio_video, battle, input_time_rng, persistence,
rendering, resource_io, runtime_platform, special_modes,
story_scene, world_map
```

这是真实旧调用图，不是新工程允许的依赖图。旧代码用顶层编排函数、共享全局状态、加载器回调和跨模块工具函数形成大环；重写不能把 87 条边机械翻译成互相包含头文件。A5 必须把调用时点保留在兼容核心中，并以所有者接口、请求值、只读视图或平台端口切断构建依赖环。

当前依赖表 SHA-256 为 `f7cc5997acc5f9d7b92654e640ae29657a3737168b4a38a71047ec471a81dab9`。

## 2. 生命周期顺序

下列顺序已经由 [`program-architecture.md`](../program-architecture.md)、[`main-frame-0040a570.md`](main-frame-0040a570.md) 和 [`presentation-lifecycle.md`](presentation-lifecycle.md) 的完整汇编证据覆盖。A4 只固定跨模块顺序，不展开模块内部算法。

### 2.1 启动与初始化

```text
CRT/PE 入口
→ runtime_platform 建立进程、窗口、COM 和消息泵条件
→ resource_io 建立路径和目录
→ persistence 扫描存档槽
→ 启动模态分支
→ input_time_rng 建立输入并分别初始化两套 RNG
→ audio_video 建立 Miles 对象
→ rendering 建立 DirectDraw、公共源 surface 和字体/像素环境
→ resource_io 建立数据库与资源访问
→ story_scene、world_map、asset_runtime 建立游戏运行态
→ special_modes 接收初始模式请求
```

旧初始化不是统一事务：部分 DirectInput/DirectDraw 失败会同步请求销毁，但上层仍可能继续后续指令。重写允许平台层把“不支持的旧 API”替换成可启动的现代后端，但不得借机改变已经进入游戏核心后的状态顺序或游戏逻辑错误行为。

### 2.2 空闲循环与有效帧

消息存在时只分发消息，不执行游戏帧。空闲时按固定互斥优先级选择：视频、暂停/抑制、有效游戏帧。有效帧内部顺序是：

```text
显示与帧门控
→ u32 毫秒间隔判断
→ 输入设备快照与原始状态归一化
→ 高优先级模式
   或 战斗请求/活动战斗
   或 普通世界/特殊模式
→ 各分支在原位置完成软件绘制和呈现
→ 仅未提前返回的路径执行公共音频维护与关闭检查
```

不存在统一帧尾 `Present`。SDL3 后端可以提供统一 `present()` 端口，但调用仍由高优先级、暂停、世界、特殊模式、商店、战斗和视频各自的原分支触发。

### 2.3 世界、特殊模式与战斗切换

- 特殊模式请求非零时整帧跳过世界和剧情；模式内部处理输入、状态机、绘制及提交。
- 带最高位的战斗请求在高优先级模式未占用时消费：先关闭当前地图映射，再建立战斗并设置活动门。
- 活动战斗每帧只推进战斗。返回 `1` 保持战斗；`0`、`2`、`3` 都清活动门并分别执行既有地图恢复或特殊模式请求，然后立即结束本帧。
- 保存/读取位于高优先级互斥路径；`persistence` 装卸状态，但剧情、世界、物品和战斗仍保留各自业务所有权。
- 剧情 handler 是否同帧继续由原两个继续量和各 opcode 的 IP 推进规则决定；让出时只维护一次音频并等下一有效帧重入。

### 2.4 失焦与恢复

停用顺序固定为：保存/改变帧间隔状态，停用并维护音频，停用战斗显示，释放三套字体渲染资源，写显示无效和过渡抑制，隐藏窗口。停用期间不推进正常游戏帧。

恢复顺序固定为：恢复窗口位置和显示有效位，恢复呈现 surface，重建关联绘图状态与三套字体，解除过渡抑制，恢复战斗显示，完成显示对象恢复循环，最后把帧间隔设回 `35`。旧函数不重新初始化或 Acquire DirectInput；重写也不能把恢复事件伪造成一次新的输入初始化。

### 2.5 退出与销毁

正常关闭请求先在可到达的单帧尾转换成同步关闭消息；另有若干启动/资源失败路径直接同步进入销毁。总释放的跨模块阶段顺序是：

```text
字体/呈现附属资源
→ 模式、战斗和游戏对象
→ 运行时链表与缓存
→ 音频对象
→ 公共源画面与显示后端
→ 输入后端
→ 角色、路径、Talk、像素文件和映射
→ 公共堆缓冲与剩余链表
→ 最终绘图状态和光标
→ COM/事件循环退出
```

部分初始化路径不会建立全部对象；销毁实现必须按旧门控判断可用性，不能假设启动成功后才会退出。

## 3. 旧平台 API 的隔离边界

| 旧来源 | 旧程序可见合同 | OpenSWD3 边界 | 不得进入业务核心的内容 |
|---|---|---|---|
| Win32 窗口、消息、COM | 单线程事件泵、激活/停用、同步关闭和文本消息时点 | `runtime_platform` 的 SDL3 事件/窗口端口；Windows 专用代码只在后端 | `HWND`、窗口消息常量、COM 生命周期 |
| `timeGetTime`、`Sleep`、CRT `time` | `u32` 毫秒回绕、0/35/70 门槛、两次独立 RNG 种子采样、原阻塞点 | 可注入时钟/让出端口；规则仍归 `input_time_rng` | 宿主高精度时钟类型、平台 sleep 细节 |
| DirectInput | 每帧设备快照，之后执行原按键/鼠标边沿、连按和坐标规则 | SDL3 输入快照端口 | DirectInput 对象、宿主自动按键 repeat |
| DirectDraw | 640×480 16 位 CPU 画面、真实 pitch、分支内提交和失焦恢复边界 | `rendering` 持有兼容帧缓冲，SDL3 纹理上传/呈现后端 | COM surface、HRESULT、DirectDraw 恢复循环 |
| GDI 字形 | CP950 字节选择字形，临时像素压成 1-bit mask，再由软件 footprint 着色 | 确定性 glyph-provider 端口；软件 mask/footprint 仍归 `rendering` | `HFONT`、HDC、宿主字体解析细节 |
| Miles | sample/sequence/stream 请求、音量、循环、逐帧状态轮询和回收时点 | `audio_video` 的音频后端端口，SDL3 负责设备输出 | Miles 句柄、驱动对象和回调 ABI |
| Bink | 打开、等待、逐帧解码、拷贝、结束及视频与正常帧互斥 | `audio_video` 的视频解码端口；解码器与 SDL3 呈现分离 | Bink 对象和 DLL ABI |
| Win32 文件/映射 | 路径字节、打开/映射/关闭顺序和错误结果 | `resource_io` 的跨平台字节源/文件端口 | `HANDLE`、映射视图和 Windows 路径 API |
| Win32 ANSI/IME | 文本输入时点及原资源/脚本的字节长度、截断语义 | SDL3 文本输入转为明确的兼容字节合同 | `CharNextA`、IME 消息和 DBCS 指针运算 |

SDL3 只替代窗口、事件、输入、音频设备和画面上传等平台能力，不替代游戏规则。Bink 解码和确定性字形来源是独立端口，不应硬塞进 SDL3，也不构成依赖 DirectX 的理由。

## 4. A4 停止线

A4 已回答原模块依赖方向、全部环所属分量、启动/帧/切换/失焦/退出顺序，以及 Win32、DirectDraw、DirectInput、Miles、Bink 和 GDI 的隔离位置。它不负责为 87 条旧依赖逐条设计新 C++ 接口；该工作只在 A5 固定首版模块映射、允许方向、源码/测试目录和首轮实现顺序时进行。
