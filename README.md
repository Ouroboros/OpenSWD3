# swd3-rebuild

《軒轅劍參 雲和山的彼端》（SWD3 v1.02 DVD 版 `swd32.exe`）的 C++20 重建版。

本项目从原版可执行文件逆向还原全部 1626 个函数，按原汇编逐个重写为可读的 C++ 源码，在现代 Windows 上原生运行，并可在 Linux 上构建与测试。游戏逻辑按原版保持一致，包括原版的逻辑 BUG；不重现越界访问、未初始化读取这类会导致崩溃或内存损坏的问题。

本仓库不包含任何游戏数据，运行需要已安装的原版游戏目录。

## 相比原版的改进

### 运行环境

- 原生运行在现代 Windows 上，不依赖 DirectDraw、DirectInput、Miles Sound System、Bink 等旧组件：画面、输入、音频由 SDL3 提供，Bink 影片与 MP3 音乐由 FFmpeg 解码。
- 静态链接 CRT（`/MT`）与全部依赖，单个 `swd3r.exe` 即可运行，不需要安装 VC 运行库。
- 不检查光盘、不寻找光驱：地图、影片与地图动画一律从安装目录读取。
- 不需要安装程序写出的 Env.dat，也不需要安装命令行。

### 画面与窗口

- 窗口化运行：逻辑画面保持原版 640×480，按最近邻等比缩放填满窗口，比例不一致时居中留黑边。
- 窗口可以自由拉伸（最小 640×480），拖动改变大小时游戏继续运行；`Alt+Enter` 切换无边框全屏。
- 退出时记住窗口大小与最大化状态，下次按同样的窗口打开。
- 失去焦点时照常运行、绘制与播放声音；只有最小化时暂停。
- 文字使用由細明體生成的点阵字库（12／16／20 像素，Big5 含倚天扩充共 13960 字），不依赖系统是否装有该字体（原版用 GDI 调系统字体绘制，外观随系统而变）。

### 设置

- 设置改用 TOML 文件，放在可执行文件旁边：
  - `swd3r.toml`：用户的文件，程序只读不写，注释与顺序都保留。
  - `swd3r.default.toml`：程序的文件，保存游戏里改的设置、按键与窗口大小。
  - 两个文件按键合并，同一项以 `swd3r.toml` 为准。
- 可以单独指定游戏数据目录（`data_dir`）与存档目录（`save_dir`），exe 不必放进游戏目录。
- 第一次启动时从游戏目录的 Env.dat 导入一次原有设置与按键，之后不再读写 Env.dat。
- 修正原版保存按键设定后，下次启动全部设置与按键被重置的问题（原版把按键写在文件开头，覆盖了格式标记）。
- 完整的配置项与说明见仓库根目录的 [swd3r.toml](swd3r.toml)。

### 文件与路径

- 路径在程序内部统一为 UTF-8，Windows 上使用 Unicode（W 版）API，游戏目录、存档目录可以放在含中文或其他非系统代码页字符的路径下。
- 打开游戏文件时不独占：游戏运行期间，其他程序仍可打开、读取、写入、改名或删除这些文件（原版以独占或只读共享方式打开）。

### 计时与流畅度

- Windows 上以 1 毫秒精度计时（`timeBeginPeriod(1)` + `timeGetTime`）。原版不调用 `timeBeginPeriod`，在 Windows 10 2004 以后的系统上计时精度为 15.6 毫秒，35 毫秒的帧会被拉长到约 47 毫秒，移动明显变慢、不平稳。
- 启动界面的提示音只打开一次音频设备，鼠标悬停按钮时不再卡顿。

### 诊断

- 运行日志：每次启动写一个独立的 UTF-8 日志文件（`logs/`），记录启动参数、设置、路径与游戏中的错误信息，原版的调试输出以原文（转为 UTF-8）写入。
- 崩溃日志：未处理的异常会记录异常代码、C++ 异常的类型与消息、寄存器与带符号（PDB）的调用栈，并在 `logs/` 写出 minidump。
- 卡死监视：主循环停顿超过 5 秒时记录主线程的调用栈，便于定位卡住的位置。

与原版不同的每一处行为都逐条记录在 [deviations.md](reverse/analysis/deviations.md) 中，注明原版地址、原行为与重建行为。

## 运行

1. 准备原版游戏目录（例如 `E:\Game\swd3`）。
2. 把 `swd3r.exe` 与 `swd3font.bin` 放在同一目录，可以是游戏目录，也可以是任意目录。
3. exe 不在游戏目录时，在它旁边新建 `swd3r.toml`，写上游戏目录：

   ```toml
   [paths]
   data_dir = 'E:\Game\swd3'
   ```

4. 运行 `swd3r.exe`。

字库 `swd3font.bin` 由 `scripts/make_font_library.py` 从细明体（`mingliu.ttc`）生成，需自行提供字体文件；构建时会复制到 exe 旁边。

## 构建

在 WSL（Linux）中构建，Windows 版本由 clang-cl + lld-link 交叉编译为 32 位 x86。

依赖：LLVM（`/opt/llvm`）、CMake 3.25 以上、Ninja；Windows 目标另需 Visual Studio 2022 的 MSVC 工具集与 Windows SDK（路径见 [cmake/toolchain-windows-x86-clang-cl.cmake](cmake/toolchain-windows-x86-clang-cl.cmake)）。

第三方依赖（SDL3、FFmpeg、toml++）由脚本下载源码并静态构建，详见 [third_party/README.md](third_party/README.md)：

```sh
bash scripts/deps/fetch.sh
bash scripts/deps/build_sdl3.sh linux
bash scripts/deps/build_ffmpeg.sh linux
bash scripts/deps/build_sdl3.sh windows
bash scripts/deps/build_ffmpeg.sh windows
```

构建与测试：

```sh
cmake --preset linux && cmake --build --preset linux      # Linux：swd3_core、swd3r、swd3_tests
build/linux/swd3_tests                                    # 单元测试（使用游戏数据的测试读取 SWD3_GAME_DIR）
cmake --preset windows && cmake --build --preset windows  # Windows：build/windows/swd3r.exe
```

## 目录

| 目录 | 内容 |
| --- | --- |
| `src/` | 重建的游戏源码，每个函数前注明对应的原版地址 |
| `tests/` | 单元测试：与原版数据表、字符串逐字节比对，绘制与解码结果与参考输出比对 |
| `reverse/` | 逆向分析成果：函数表 `functions.tsv`、反汇编与反编译输出、数据布局说明、偏差记录与逐函数验证记录 |
| `scripts/` | 构建依赖、生成字库、登记函数与代码检查的脚本 |
| `cmake/` | 工具链文件 |
| `third_party/` | 第三方依赖的版本记录与构建说明 |

## 进度

全部 1626 个函数均已实现，游戏可以完整运行。逐函数对照原版汇编的二次验证正在进行，当前进度记录在 [GOAL.md](GOAL.md)。
