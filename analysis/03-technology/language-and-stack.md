# 语言与技术栈

## 推荐结论

使用 C++20。

基础设施：

- CMake
- Ninja Multi-Config
- SDL3
- FFmpeg
- 标准 C++ 测试框架，具体选择在建工程前再定

## 已固定的构建约束

- 语言标准固定为 C++20。
- 构建系统固定为 CMake。
- 默认生成器使用 Ninja Multi-Config。
- 配置、编译、测试和安装均可从命令行完成。
- 不依赖 Visual Studio IDE，不要求生成或维护 `.sln` / `.vcxproj`。
- Windows 编译器允许使用 MSVC `cl.exe` 或 LLVM。
- 编译器、SDK 和依赖路径不写死在项目 `CMakeLists.txt` 中。

“不依赖 Visual Studio”指不依赖其 IDE、解决方案和项目文件。使用 MSVC 编译器时，仍然需要相应的 MSVC Build Tools、Windows SDK 和命令行环境；这不构成 IDE 依赖。

## 参考项目构建环境结论

已只读核对 `D:\Dev\Source\d2tools\1.10f\goose\w.bat`、`goose/CMakeLists.txt` 和 `goose/bats/m2.bat` 等直接构建脚本。

可复用的做法：

- 用环境引导脚本集中发现 CMake、Ninja 和编译器。
- 用 `CMAKE_CXX_COMPILER` 显式选择编译器。
- 用 Ninja Multi-Config 同时承载 Debug 与 RelWithDebInfo。
- 用 `cmake --build` 和 `cmake --install` 驱动构建与部署。
- 打开 `CMAKE_EXPORT_COMPILE_COMMANDS`，便于 clangd 和静态分析工具使用。

不会直接照搬的部分：

- 个人机器的绝对工具路径。
- 当前活动编译器为 LLVM、但 `TEMP_COMPILER_NAME` 仍写成 `MSVC` 的不一致状态。
- 项目专用的全局 `-m32`、宏、运行库和部署路径。
- `git clean -xfd` 一类清理命令。
- 依靠非标准 `Clang` 变量判断编译器。新工程应检查 `CMAKE_CXX_COMPILER_ID` 和 MSVC 模拟前端信息。

正式建工程时，建议把共享构建配置放入版本控制，把机器路径放入用户级 CMake Preset 或工具链文件。至少保留两条 Windows 验证线：

- MSVC + Ninja Multi-Config
- LLVM + Ninja Multi-Config

LLVM 具体采用 `clang++` 还是 `clang-cl`，以及相应 C++ 运行库和第三方预编译库组合，需要在 SDL3、FFmpeg 依赖验证后固定。当前不凭空决定 ABI。

## 为什么是 C++20

### 与原程序的数据模型接近

原 EXE 是 32 位 C++ 程序。反编译结果中存在大量 `thiscall`、对象偏移、虚函数式调用和 C++ 运行库代码。

C++ 可以自然表达：

- 精确宽度整数
- 显式二进制读取
- packed 文件记录的解码结果
- 16 位像素和位运算
- 状态机和 opcode 分发
- SDL3、FFmpeg 的原生 C 接口

### 允许先恢复语义，再收紧类型

逆向早期经常只能确定“偏移 0x18 是一个 16 位状态”，还不能确定最终业务名。

C++ 允许先把二进制读取与业务模型分层，逐步把未知字段替换为强类型，而不需要在整个工程中传播裸内存布局。

### 能控制运行时行为

游戏需要稳定控制：

- 整数溢出和截断位置
- 随机数调用顺序
- 帧更新顺序
- 音视频缓冲
- 文件偏移和解压长度

C++ 对这些行为的控制直接，且没有托管运行时介入关键循环。

## 其他语言

### Rust

可行，是第二选择。

优点是内存安全和枚举/状态建模更强。

不足是当前数据结构和生命周期尚未恢复，早期会出现大量临时 `unsafe`、FFI 和借用边界调整，降低逆向验证速度。

### C#

适合快速制作工具和数据查看器，但作为最终引擎会增加 SDL/FFmpeg 原生互操作、固定布局和逐像素热路径的边界。

### C

与伪码最接近，但会继续复制原程序的全局状态、手工生命周期和裸指针问题，不适合作为长期维护目标。

### Go、JavaScript、Python

不建议作为最终游戏核心。它们可以用于离线分析工具，但在本项目的原生媒体依赖、二进制格式和像素兼容目标上没有优势。

## SDL3 的角色

SDL3 只负责平台能力：

- 窗口
- 键盘、鼠标和手柄
- 高精度计时
- 音频输出设备
- 最终纹理呈现
- 可选的跨平台支持

SDL3 官方文档说明它直接支持 C++，并提供 Windows、macOS、Linux 等平台的窗口、输入、音频和图形抽象：

- <https://wiki.libsdl.org/SDL3/FrontPage>
- <https://wiki.libsdl.org/SDL3/README-platforms>

第一版渲染不建议使用 SDL GPU API重写所有效果。

应创建一个与原逻辑一致的 640×480、16 位 CPU 帧缓冲，完成一帧后转换到 RGBA8888 流式纹理，再由 SDL 呈现。SDL3 官方文档也明确建议频繁更新的纹理使用 streaming texture 和锁定接口：

- <https://wiki.libsdl.org/SDL3/SDL_UpdateTexture>

## FFmpeg 的角色

暂定负责：

- Bink Video
- Bink Audio
- MP3

FFmpeg 当前源码明确包含 Bink Video 和 Bink Audio 解码器。

这条路线可以同时替代 `binkw32.dll`、`Mss32.dll` 中与压缩媒体解码相关的部分。

需要做一个独立技术验证，确认本目录 rev.h/rev.i Bink 文件的画面、音轨、时间戳和无音轨分支都可正常读取。

## 不应直接复刻的旧设计

本节只调整新工程的内部所有权和模块边界，不授权改变原程序的对外行为。初步还原必须保留游戏逻辑 BUG；只有启动和新系统兼容可在 `platform` 适配层作最小例外。

- 不保留全局可变状态作为主要架构。
- 不用对象内存偏移模拟原类。
- 不把 IDA 的 `int` 当作已经确认的类型。
- 不把 Windows 消息直接传入游戏逻辑。
- 不让资源解析器返回裸指针指向映射文件。
- 不让渲染、剧情和战斗共同写同一批无类型全局变量。

## 建议的目标模块

- `platform`：SDL3 封装
- `io`：文件、路径和字节读取
- `compression`：LZO1X 兼容解压
- `assets`：TSW、ACT、SND、DAT、LMF
- `video`：Bink 与 Ani
- `render`：16 位兼容软件渲染和最终呈现
- `audio`：音乐、音效和序列控制
- `script`：剧情 opcode 解释器
- `world`：地图、角色、移动和交互
- `battle`：战斗状态机
- `save`：旧存档解析与写入
- `game`：顶层状态和帧调度

这些是职责边界，不是当前阶段要创建的代码目录。
