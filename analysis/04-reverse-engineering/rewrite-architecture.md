# OpenSWD3 首版重写架构基线

状态：A5 首版架构冻结，可开始模块实现

## 1. 固定原则

- 完整汇编是唯一行为真值；模块拆分只改变代码所有权和依赖方式，不改变调用时点、整数行为、状态迁移、错误结果或原始游戏逻辑 BUG。
- 旧程序 11 个逻辑模块全部处于一个依赖强连通分量，不能按旧 `call` 边建立 CMake target 依赖。
- 每份可变状态只有一个业务所有者。跨模块写入改为所有者 API、同步请求/结果或显式可变参数；`persistence` 只运输状态。
- SDL3 只存在于平台后端。游戏核心不得包含 Win32、DirectDraw、DirectInput、Miles、Bink 或 GDI 类型。
- 剧情脚本的控制流、操作数、长度和跳转仍保留原始字节域；只有 handler 已确认的文本载荷在载入边界按配置解码。解码后的内核文本统一为显式 UTF-16（`char16_t`/`std::u16string`），不使用 Windows `wchar_t` 作为跨平台 ABI。
- 本基线只冻结第一轮接口边界和工程结构，不提前建立无汇编证据的实体层次或通用 ECS。

逐模块输入、输出、所有权、允许依赖和首个验证入口见 [`rewrite-module-map.tsv`](inventory/rewrite-module-map.tsv)。

## 2. 目标模块与依赖方向

目标 CMake target 统一使用 `openswd3_` 前缀：

```text
openswd3_app
├─ openswd3_persistence
│  └─ openswd3_battle
│     └─ openswd3_modes
│        └─ openswd3_story
│           └─ openswd3_world
├─ openswd3_platform_sdl3
└─ 服务模块
   ├─ openswd3_assets ──────→ openswd3_resource
   ├─ openswd3_media ───────→ openswd3_resource
   ├─ openswd3_input
   └─ openswd3_render

所有 target 最终只向下依赖 openswd3_compat；
world/story/modes/battle 按各自行实际需要依赖上述服务模块。
```

这是允许依赖的上界，不表示每个 target 一开始就链接所有下层。新增依赖必须能落到映射表中的允许集合；下层向上层传递事件时返回生产者自己定义的结果值，不反向链接 `app`。

### 主要切环规则

| 旧关系 | 首版重写合同 |
|---|---|
| `world_map` 直接触发剧情、模式或战斗 | `world::step()` 返回带原始请求值的 `WorldStepResult`；`app` 在汇编对应位置同步消费 |
| `story_scene` 写顶层门控、模式或战斗请求 | opcode handler 返回/提交 `StoryStepResult`；`app` 保留同帧继续和让出顺序 |
| `special_modes` 直接进入战斗或存档 | `modes::step()` 返回请求；`app` 同步调度，模式模块不链接 `battle`/`persistence` |
| `battle` 直接改变战后顶层模式 | `battle::step()` 保留 `0/1/2/3` 原值；`app` 原样执行四条出口 |
| `persistence` 直接覆盖各模块全局内存 | 各所有者提供明确 snapshot/import 合同；导入顺序仍由存档汇编路径决定 |
| 资源/音频/渲染函数写进程标志 | 下层返回精确旧错误/完成结果；`app` 决定原关闭位或同步退出时点 |
| 渲染推进剧情拥有的 Picture/PicPaint 节点 | `story` 持有列表并调用 `render` 的表现记录更新函数；`render` 不包含剧情状态 |
| 业务模块直接读平台句柄 | 只使用 `compat` 中的端口和值；原生句柄封闭在 `platform_sdl3` |

同步返回不是异步事件总线。原汇编在同一调用栈内生效的请求，重写也必须在同一帧、同一顺序生效。

## 3. 模块职责冻结

- `compat`：固定宽度整数、字节视图、精确结果值和平台端口类型；禁止放可变游戏状态、万能 service locator 或业务 helper。
- `platform_sdl3`：SDL3 窗口/事件/输入快照/音频设备/纹理上传及宿主时钟实现；不解释按键业务、像素效果或模式状态。
- `app`：启动、帧门、互斥分支、场景/战斗/存档切换和销毁编排；不实现模块内部算法。
- `resource`：路径、字节所有权、公共解压和物理容器；格式的业务含义归消费模块。
- `input`：原设备状态归一化、时间门槛、等待规则和两套 RNG；不解释世界/菜单按键。
- `render`：16 位软件 framebuffer、blitter、文字 mask、效果和原分支提交端口；不选择游戏模式。
- `media`：Miles 可见音频状态和 Bink 可见视频时序的兼容实现；设备与解码器通过后端端口替换。
- `assets`：TSW/ACT/ANI/SND 运行时视图、缓存和通用 action 语义。
- `world`：地图、角色、移动、碰撞、寻路、交互和队伍根状态；只上报剧情/模式/战斗请求。
- `story`：Talk VM、剧情变量/位集、场景和异步表现意图；通过 `world` 的所有者 API 修改世界。
- `modes`：高优先级 UI、菜单、商店、数字模式、文本输入和 ItemNode 操作语义。
- `battle`：战斗对象、VM/帧状态机、AI/数值和四值返回合同。
- `persistence`：配置、槽、存档物理容器和 owner-mediated 装卸；不取得玩法状态所有权。

### 剧情文本编码边界

OpenSWD3 自有配置从 EXE 同目录的 `openswd3.toml` 读取：

```toml
[scripts]
encoding = "big5"
```

`encoding` 首版只接受 `big5` 和 `gbk`，分别采用 Windows CP950 与 CP936 兼容映射；字段缺失时默认为 `big5`，未知值必须报告配置错误，不回退到宿主区域设置，也不做启发式自动检测。配置文件本身仍是 UTF-8 TOML。

`resource` 只交付 Talk 文件原始字节，`app` 在任何剧情文本载入前读取配置并把编码枚举注入 `story`。`story` 继续以原始字节执行 opcode framing、指令长度、跳转和字节偏移，只在具体 handler 已确认某段是文本时解码；解码结果进入 `std::u16string`，之后 `story`、`modes`、`render` 等内核接口不得继续传递窄字符脚本文本。

原程序依赖 DBCS 字节数的固定缓冲、截断、光标和存档字段必须在兼容边界保留原始字节或显式的字节位置映射，不能把 UTF-16 code unit 数冒充旧字节数。无效源字节的处理要在对应脚本单元根据真实资产和汇编调用域冻结，不得无记录地替换字符或吞掉字节。编码门的 UT 至少覆盖同一文本的 CP950/CP936 输入、配置缺省与非法值、DBCS 字节边界到 UTF-16 的映射，以及跨内核接口不泄漏宿主 `wchar_t`。

## 4. 正式工程目录

阶段 B 创建以下目录，不再临时改名：

```text
OpenSWD3/
├─ CMakeLists.txt
├─ CMakePresets.json
├─ cmake/
├─ include/openswd3/
│  ├─ compat/  resource/ input/ render/ media/ assets/
│  └─ world/   story/    modes/ battle/ persistence/ app/
├─ src/
│  ├─ platform/sdl3/
│  ├─ resource/ input/ render/ media/ assets/
│  └─ world/ story/ modes/ battle/ persistence/ app/
├─ tests/
│  ├─ support/
│  ├─ unit/<module>/
│  └─ integration/
├─ analysis/
└─ goal/
```

公开头只放其他模块确实需要的值和接口；模块内部结构留在 `src/<module>/`。逆向生成器继续留在 `analysis/tools/`，不混入产品构建。

## 5. CMake、编译器和 UT 基线

- 根工程使用 CMake `3.25+`、`CXX_STANDARD 20`、`CXX_EXTENSIONS OFF` 和 Ninja Multi-Config。
- 提交的 `CMakePresets.json` 不写本机编译器绝对路径。MSVC 由已配置的命令行环境选择；LLVM 由 `CC`/`CXX`、`CMAKE_C_COMPILER`/`CMAKE_CXX_COMPILER` 或用户自己的 `CMakeUserPresets.json` 选择。
- 参考 `D:\Dev\Source\d2tools\1.10f\goose\w.bat` 的做法，只在仓库外的启动脚本中检查 CMake、Ninja 和编译器版本；不复制其硬编码路径、生成脚本和文件覆盖步骤。
- SDL3 后端使用 `find_package(SDL3 CONFIG REQUIRED)`。依赖路径通过 `SDL3_DIR`/`CMAKE_PREFIX_PATH` 或用户 preset 提供；配置过程不临时联网下载。纯核心和 UT 可用 `OPENSWD3_BUILD_APP=OFF` 在没有 SDL3 时构建。
- 首轮 UT 固定使用 CTest。`tests/support/` 只提供最小的等值、字节序列和调用序列断言；每个测试程序以非零退出码报告失败，不引入配置时下载的测试框架。
- 每个模块 target 同时接受 MSVC 和 LLVM；警告选项按编译器条件设置，不把 `/W4` 或 `-Wall` 泄漏给第三方 target。
- 构建目录固定在仓库外观上的 `build/<preset>/`；产物不写入源码目录。

## 6. 最终实现顺序

1. `compat + platform_sdl3 + app`：建立工程、假端口 UT、SDL3 窗口和空 16 位 framebuffer；先锁定顶层调度，不假装其他模块已实现。
2. `resource`：文件/内存、公共解压和物理容器。
3. `input`：设备快照归一化、时间、等待和 RNG。
4. `render`：16 位像素、文字、效果和 SDL3 提交。
5. `media`：音频状态和视频解码/时序边界。
6. `assets`：TSW/ACT/ANI/SND 与通用 action。
7. `world`：地图、角色、移动、碰撞和寻路。
8. `story`：剧情 VM、场景和异步 action。
9. `modes`：高优先级 UI、菜单、商店和特殊模式。
10. `battle`：战斗状态机、AI 和数值。
11. `persistence`：旧存档字段语义和全所有者装卸验收。

世界先于剧情，是为了先固定剧情 opcode 调用的世界所有者 API；世界触发剧情时只产生请求，因此不会形成反向构建依赖。存档的 LZO/容器可在第 2 步实现，业务字段到第 11 步统一验收。

## 7. 第一个模块工作包入口

阶段 A 结束后的唯一下一项是第 1 步，不再继续全局调研。其接口级逆向只覆盖：

- `0x0048A740`、`0x00409EC0`、`0x0040A0D0`、`0x0040A570`、`0x0040AB50`、`0x00424B90`、`0x004251B0`；
- 帧门、模式/战斗请求、显示/抑制/关闭状态；
- 启动、接受帧分派、战斗四出口、失焦/恢复和部分初始化销毁顺序；
- Win32/DirectX 行为映射到平台端口时唯一允许的启动/新系统兼容隔离。

首批 UT 使用假时钟、假事件源和记录调用顺序的假模块，覆盖分支优先级、提前返回、同帧请求消费和销毁顺序；SDL3 smoke test 只验证窗口、事件和 framebuffer 上传，不作为游戏逻辑正确性的替代。

到此首版架构冻结。以后只有新的完整汇编证据证明模块边界错误时才修改；函数内部细节在对应模块工作包中边逆向、边测试、边实现。
