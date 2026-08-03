# 可配置游戏数据目录

状态：`platform_adapted`、Windows LLVM 已验证

## 1. 目的与边界

原程序默认以进程工作目录解释 `TSW`、`LMF`、`Data`、`Save`、`Video` 等相对路径。OpenSWD3 仍保留这套相对路径合同，但允许在启动最前端选择游戏数据根目录并把它设为进程工作目录。因此后续逐模块恢复的原始相对路径不需要混入本机绝对路径，也不需要为每个资源调用增加不同的路径规则。

启动界面的 OpenSWD3 自有位图继续通过 `SDL_GetBasePath()` 从 EXE 旁的 `assets/ui/startup` 读取，不受游戏数据目录切换影响。

## 2. 配置合同

配置文件固定为 EXE 同目录的 `openswd3.toml`，使用正式 TOML 1.0 解析器。当前读取字段为：

```toml
[paths]
data_dir = 'E:\Game\swd3'
```

选择优先级固定为：

```text
--data-dir > openswd3.toml > 启动时的工作目录
```

命令行支持两种等价形式：

```text
openswd3.exe --data-dir "E:\Game\swd3"
openswd3.exe --data-dir="E:\Game\swd3"
```

命令行中的相对目录以启动工作目录为基准；TOML 中的相对目录以 EXE 目录为基准。选中的目录必须已经存在且确实是目录。TOML 语法错误、字段类型错误、空路径、缺少命令行值或无法访问的目录都在 SDL 初始化前明确失败。

仓库模板为 `config/openswd3.example.toml`，应用构建后复制为 EXE 旁的 `openswd3.example.toml`。用户将其复制或改名为 `openswd3.toml` 后启用所需路径。

## 3. 旧命令行兼容

`--data-dir` 只在参数列表最前面作为现代平台前缀识别。未出现该前缀时，Windows 仍直接读取 `GetCommandLineA()` 并逐字保留原程序需要的非空命令行尾。

出现现代前缀时，Windows 按命令行引号和反斜杠边界跳过 EXE 与数据目录参数，再把剩余原始尾部交给旧命令行门。非 Windows 平台只拼接前缀之后的剩余 `argv`。因此仅设置数据目录不会误触发原程序的非空命令行提前退出，后续旧参数仍保留处理位置。

## 4. 实现映射

- `include/openswd3/resource_io/data_directory.hpp`：选择来源、状态、UTF-8 路径和激活接口。
- `src/resource_io/data_directory.cpp`：命令行优先级、TOML 读取、相对路径解析、目录验证和工作目录切换。
- `src/platform/sdl3/main.cpp`：在启动门和 `SDL_Init` 前选择并激活数据目录。
- `src/platform/sdl3/legacy_command_line.cpp`：跳过现代前缀并保留剩余旧命令行尾。
- `tests/unit/resource_io/data_directory_test.cpp`：回退、优先级、两种命令行形式、TOML、错误路径与目录激活 UT。
- `tests/unit/app/platform_startup_adapters_test.cpp`：Windows 原始命令行前缀剥离边界 UT。

TOML 解析使用固定提交 `30172438cee64926dc41fdd9c11fb3ba5b2ba9de`，对应 toml++ v3.4.0。CMake 优先使用已安装的 toml++ 3.4+，缺失时才获取固定源码。

单测夹具统一写入 `build/<preset>/tests/runtime/` 下的独立目录并在结束时删除，不使用系统 TEMP。

## 5. 验证结果

- Windows LLVM `core-debug`：构建通过，25/25 CTest 通过，无新增编译警告。
- Windows LLVM `app-debug`：构建通过，25/25 CTest 通过，生成 `openswd3.exe`。
- 命令行窗口 smoke：EXE 位于 `build/app/src/platform/sdl3/Debug`，启动目录位于 `build/runtime-smoke/launch`，参数为 `--data-dir E:\Game\swd3`；成功出现标题为 `OpenSWD3` 的窗口并正常退出。
- TOML 窗口 smoke：隔离 EXE 与配置位于 `build/runtime-smoke/toml`，启动目录位于 `build/runtime-smoke/launch`，未传命令行参数；成功从 EXE 同目录 TOML 选择数据目录、显示窗口并正常退出。

当前 smoke 证明了配置选择、旧命令行门绕过、任意工作目录启动以及 EXE 自有启动资源定位。真实 `TSW`、`LMF` 和存档调用将在对应 B2 调用层接入时继续验证，不能把本项平台验证升级为全部资源路径已经恢复。
