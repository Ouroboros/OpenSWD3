# 编译

所有编译都在 WSL（Ubuntu 24.04）中进行。默认编 64 位：Windows 版由 clang-cl + lld-link 交叉编译为 x64 程序，Linux 版为 x86_64 静态程序，只用于编译检查与单元测试。32 位版本保留为可选的 `windows-x86`、`linux-i386` 预设。

## 需要的工具

| 工具 | 用途 | 默认位置 |
| --- | --- | --- |
| LLVM（clang、clang-cl、lld-link、llvm-ar 等） | 编译与链接 | `/opt/llvm`，可用 `SWD3_LLVM_ROOT` 指定 |
| CMake 3.25 以上、Ninja | 构建 | `PATH` |
| git | 下载第三方源码 | `PATH` |
| nasm | FFmpeg 的 x86／x86_64 汇编 | `PATH`，或 `/opt/nasm/usr/bin/nasm`，可用 `SWD3_NASM` 指定 |
| Python 3 | 生成字库（只需标准库） | `PATH` |
| Visual Studio 2022 的 MSVC 工具集 | Windows 版的 C++ 标准库与 CRT 头文件和库 | `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207`，可用 `SWD3_MSVC_ROOT` 指定 |
| Windows SDK | Windows 版的系统头文件与库 | `C:\Program Files (x86)\Windows Kits\10`，版本 `10.0.26100.0`，可用 `SWD3_WINSDK_ROOT`、`SWD3_WINSDK_VERSION` 指定 |

上表中的变量，对第三方依赖脚本是环境变量，对 CMake 是缓存变量（`cmake --preset windows -DSWD3_MSVC_ROOT=...`）。Windows 路径按 WSL 的形式填写，例如 `/mnt/c/Program Files/...`。

## 1. Linux 的 i386 sysroot（只用于 32 位）

32 位的 `linux-i386` 预设需要 Ubuntu 的 32 位开发库。下面的脚本下载所需的 deb 包并解压到 `/opt/sysroot-i386`，不改动系统的软件包状态（可用 `SWD3_I386_SYSROOT` 指定其他位置）：

```sh
bash scripts/deps/setup_i386_sysroot.sh
```

默认的 64 位版本使用系统的 x86_64 静态库（libc6-dev、libstdc++-13-dev），不需要这一步。

## 2. 第三方依赖

SDL3、FFmpeg 与 toml++ 由脚本下载固定版本的源码并静态编译，安装到 `third_party/install/<平台>`。平台名 `windows`、`linux` 为 x64，`windows-x86`、`linux-i386` 为 32 位。FFmpeg 启用 nasm 汇编优化。版本与 FFmpeg 的配置见 [third_party/README.md](../third_party/README.md)。

```sh
bash scripts/deps/fetch.sh
bash scripts/deps/build_sdl3.sh windows
bash scripts/deps/build_ffmpeg.sh windows
bash scripts/deps/build_sdl3.sh linux
bash scripts/deps/build_ffmpeg.sh linux
```

32 位版本把平台名换成 `windows-x86`、`linux-i386`。

Windows 版的依赖与程序本身都使用静态 CRT（`/MT`），编出的 exe 不需要 VC 运行库。

## 3. 资源文件

字库和启动画面等资源放在 `third_party/asset`，不放进仓库，需要自行生成。构建时整个文件夹复制到 exe 旁边的 `asset`；不存在时程序仍能编译，但运行时缺少文字、图标、启动画面和这些音效。

字库 `font.bin` 由細明體（`mingliu.ttc`）生成。字体属于微软，需要从 Windows 的 `C:\Windows\Fonts\mingliu.ttc` 自行取得：

```sh
mkdir -p third_party/asset
python3 scripts/make_font_library.py /mnt/c/Windows/Fonts/mingliu.ttc third_party/asset/font.bin
```

窗口图标、启动画面的图片与音效、开场音效和作弊对话框取自原版 `swd32.exe`，需要从自己的原版游戏中提取：

```sh
python3 scripts/make_resources.py /mnt/e/Game/swd3/swd32.exe third_party/asset/launcher
```

提取到 `launcher` 子文件夹，是普通的 `.bmp`、`.wav`、`.ico`、`.cur` 文件和对话框模板 `.dlg`，以原版的资源编号命名。运行时不需要 `swd32.exe`。

## 4. 编译

```sh
cmake --preset windows
cmake --build --preset windows
```

产物在 `build/windows/`：

| 文件 | 内容 |
| --- | --- |
| `swd3r.exe` | 游戏程序 |
| `swd3r.pdb` | 调试符号，放在 exe 旁边时崩溃记录能显示函数名与行号 |
| `asset/` | 字库，以及从原版提取的图标、启动画面、音效与对话框 |
| `swd3_tests.exe` | 单元测试 |

Linux 版：

```sh
cmake --preset linux
cmake --build --preset linux
```

产物在 `build/linux/`：`swd3r`、`swd3_tests`。

32 位版本使用 `windows-x86`（产物在 `build/windows-x86/`）与 `linux-i386`（产物在 `build/linux-i386/`）预设。所有预设都是 `RelWithDebInfo`。

在 Windows 上直接编译并打包：

```bat
scripts\package.bat
```

需要 LLVM（`clang-cl`、`lld-link`、`llvm-lib`、`llvm-rc`、`llvm-mt`，默认在 `D:\Dev\Compiler\LLVM\x64\bin`，可用环境变量 `SWD3_LLVM` 指定），以及装有 C++ 工具的 Visual Studio 2022（提供 Windows SDK、MSVC 库和自带的 CMake、Ninja）。第 2、3 步的依赖和资源文件要先准备好。脚本在 `build\windows-native` 编译 `swd3r.exe`，再把 `swd3r.exe`、`asset` 文件夹和设置模板 `swd3r.toml` 放进一个 `OpenSWD3` 文件夹，打成 `build\OpenSWD3-<日期>-<提交>.zip`。

## 5. 单元测试

```sh
build/linux/swd3_tests
```

部分测试读取原版游戏资料，与原版的数据表、文字和画面逐字节比对。游戏目录默认为 `/mnt/e/Game/swd3`（Windows 版测试为 `E:/Game/swd3`），可用环境变量 `SWD3_GAME_DIR` 指定；找不到游戏资料时，这些测试会跳过。

Windows 版测试在 Windows 中运行 `build\windows\swd3_tests.exe`。

## 代码检查

提交前运行：

```sh
scripts/format.sh        # clang-format
scripts/check_style.sh   # 命名与写法检查
```
