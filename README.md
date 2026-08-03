# OpenSWD3

OpenSWD3 是对《轩辕剑叁》旧版 Windows 可执行程序的现代 C++ 重写项目。项目目标不是修改玩法或制作增强版，而是在不依赖原 EXE 的前提下，继续读取原始游戏数据和存档，并在现代系统上复现原程序的可观察行为。

> 当前仍处于早期逆向与实现阶段。工程可以构建、运行 SDL3 启动界面并执行现有测试，但还不能完整游玩。

## 还原原则

- `swd3.exe` 的完整 x86 汇编是唯一行为真值。
- IDA 反编译 C 伪码只用于辅助阅读；与汇编冲突时一律以汇编为准。
- 初步还原追求 bug-for-bug 的 1:1 行为兼容，不修复游戏逻辑 BUG。
- 只有阻断启动或现代系统兼容的问题，才允许在平台层进行隔离且可验证的最小适配。
- 每个行为单元都按“逆向证据 → UT → C++ 实现 → 汇编逐块复核 → 真实数据验证”的顺序闭环。

## 技术栈

- C++20
- CMake 3.25+
- Ninja Multi-Config
- MSVC 或 LLVM Clang
- SDL3 作为窗口、输入、呈现和其他平台能力的边界
- toml++ 读取 OpenSWD3 自有配置

工程不依赖 Visual Studio IDE。CMake 会优先使用系统安装的 SDL3 3.4+ 和 toml++ 3.4+，找不到时获取仓库固定的上游版本。

## 当前进度

执行进度以 [`goal/execution-plan.md`](goal/execution-plan.md) 为准。目前：

- 顶层程序架构、模块职责、依赖方向和生命周期已经恢复。
- SDL3 启动界面、窗口缩放、拖动期间持续刷新和平台启动骨架已经实现。
- 支持通过命令行或 TOML 指定原始游戏数据目录。
- 公共 LZO1X 解压核心和调用包装器已经按汇编实现；20,091 个真实 TSW 压缩帧通过验证。
- `0x00438000..0x00438640` 旧文件、映射和视图对象已经实现并通过测试。
- 当前继续处理 `0x00438650` 起的虚拟页和内存对象。

后续仍包括输入与时间、软件渲染、音视频、资产运行时、地图与世界、剧情 VM、菜单、战斗和存档语义。完整里程碑与完成条件见执行计划。

## 构建

### Windows + LLVM

确保 `cmake`、`ninja`、`clang` 和 `clang++` 位于 `PATH`：

```console
cmake --preset core -DCMAKE_CXX_COMPILER=clang++
cmake --build --preset core-debug
ctest --test-dir build/core -C Debug --output-on-failure
```

构建 SDL3 应用：

```console
cmake --preset app -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build --preset app-debug
ctest --test-dir build/app -C Debug --output-on-failure
```

生成的程序位于：

```text
build/app/src/platform/sdl3/Debug/openswd3.exe
```

仓库还提供作者开发环境使用的快捷脚本：

```console
build.bat core
build.bat app
```

该脚本顶部保存了本机工具路径；其他环境应修改这些变量，或直接使用上面的标准 CMake 命令。

### MSVC

在已配置 MSVC 环境变量的终端中使用相同 CMake preset，并省略 Clang 编译器参数即可。工程不要求打开 Visual Studio 工程或解决方案。

## 指定游戏数据目录

OpenSWD3 不把原始游戏数据复制进构建目录。运行时可以直接指定现有安装目录：

```console
openswd3.exe --data-dir "E:\Game\swd3"
```

也可以把 [`config/openswd3.example.toml`](config/openswd3.example.toml) 复制到 EXE 同目录并改名为 `openswd3.toml`：

```toml
[paths]
data_dir = 'E:\Game\swd3'
```

数据目录选择优先级为：

```text
--data-dir > EXE 同目录 openswd3.toml > 启动时工作目录
```

相对路径的解析规则和旧命令行兼容细节见 [`data-directory-compatibility.md`](analysis/04-reverse-engineering/evidence/data-directory-compatibility.md)。

## 仓库结构

- `include/openswd3/`：公共 C++ 接口。
- `src/`：兼容核心、资源 I/O、应用逻辑和 SDL3 平台后端。
- `tests/`：按模块组织的单元测试和测试支持代码。
- `analysis/04-reverse-engineering/`：架构、模块工作包、汇编证据和机器生成目录。
- `analysis/tools/`：可重复生成或验证逆向结论的工具。
- `assets/ui/startup/`：从原程序 PE 资源中提取并登记来源的启动界面位图。
- `config/`：OpenSWD3 自有配置模板。
- `goal/`：唯一执行计划。

## 参与开发

开始工作前先阅读：

1. [`goal/execution-plan.md`](goal/execution-plan.md)
2. 当前模块工作包 [`analysis/04-reverse-engineering/modules/resource-io.md`](analysis/04-reverse-engineering/modules/resource-io.md)
3. 仓库工作约束 [`AGENTS.md`](AGENTS.md)

提交实现时需要同时给出对应汇编地址、可观察合同、边界 UT 和验证结果。不要根据函数名或反编译伪码猜测行为，也不要顺手整理原程序中看似不合理的逻辑。
