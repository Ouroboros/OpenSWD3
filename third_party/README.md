# 第三方依赖

由 `scripts/deps/` 自行下载源码并静态构建（toml++ 只有头文件，直接包含），不复用本机其他位置的副本。

| 依赖 | 版本 | 来源 |
| --- | --- | --- |
| SDL3 | `release-3.4.18` | https://github.com/libsdl-org/SDL.git |
| FFmpeg | `n9.0` | https://git.ffmpeg.org/ffmpeg.git |
| toml++ | `v3.4.0` | https://github.com/marzer/tomlplusplus.git（MIT，解析 swd3r.toml） |

实际提交记录在 `SOURCES.tsv`。

## 步骤

```sh
bash scripts/deps/fetch.sh
bash scripts/deps/build_sdl3.sh linux
bash scripts/deps/build_ffmpeg.sh linux
bash scripts/deps/build_sdl3.sh windows
bash scripts/deps/build_ffmpeg.sh windows
```

产物安装到 `install/linux`、`install/windows`；主工程 `CMakeLists.txt` 通过 `SWD3_DEPS_PREFIX` 使用。

## FFmpeg 配置

LGPL 2.1+，静态库 avformat、avcodec、avutil、swresample、swscale；只启用 `file` 协议、`bink`／`mp3` 解封装、`bink`／`binkaudio_rdft`／`binkaudio_dct`／`mp3`／`mp3float` 解码与 `mpegaudio` 解析。启用 x86／x86_64 汇编（`--enable-x86asm`，另有内联汇编）：nasm 2.16.01（Ubuntu noble 包解压到 `/opt/nasm`，`/usr/local/bin/nasm` 链接；本环境 apt 无法写系统缓存，故用 `apt-get download` + `dpkg-deb -x`）。Windows 目标同样在 WSL 中调用该 nasm 生成 win32 目标文件，无需 Windows 版 nasm.exe。Windows 目标：clang-cl + lld-link 交叉编译 x64（`windows`）或 x86（`windows-x86`），`-MT`（静态 CRT，与工具链文件的 `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded` 一致，SDL3 同样由工具链文件得到 `/MT`），归档工具 llvm-ar；构建时用一个 `wslpath` 包装处理 configure 的依赖路径转换。

## 构建结果（2026-10-16）

| 平台 | SDL3 | FFmpeg | 链接冒烟测试 |
| --- | --- | --- | --- |
| Linux i386（WSL，clang 22.1.8，含 x86 汇编） | 成功，`install/linux-i386/lib/libSDL3.a` | 成功，5 个 `.a` | 构建并运行通过：SDL 3.4.18，bink／mp3 解码器与 bink 解封装可用 |
| Windows x86（clang-cl／lld-link，含 x86 汇编） | 成功，`install/windows-x86/lib/SDL3-static.lib` | 成功，5 个 `.lib` | 构建 PE32 i386 可执行文件，经 WSL interop 运行通过，输出与 Linux 相同 |

## 构建结果（2026-10-18，x64 改为默认）

| 平台 | SDL3 | FFmpeg | 结果 |
| --- | --- | --- | --- |
| Linux x86_64（WSL，clang 22.1.8，含 x86_64 汇编） | 成功，`install/linux/lib/libSDL3.a` | 成功，5 个 `.a`，`ARCH_X86_64`、`HAVE_X86ASM`、`HAVE_INLINE_ASM` 为 1 | 游戏与单元测试为 ELF x86-64 静态程序，159 个测试通过 |
| Windows x64（clang-cl／lld-link，含 x86_64 汇编） | 成功，`install/windows/lib/SDL3-static.lib` | 成功，5 个 `.lib`，`ARCH_X86_64`、`HAVE_X86ASM`、`HAVE_INLINE_ASM` 为 1 | `swd3r.exe` 与 `swd3_tests.exe` 为 PE32+ x86-64 |

主工程 `cmake --preset linux`／`cmake --preset windows` 均配置成功；`src/` 尚无源码，未定义可执行目标。

## 平台限制

- Linux SDL3：本机缺少 Xcursor、XInput2、XFixes、XRandR、XTest、Wayland、ALSA、PulseAudio、PipeWire、udev、dbus 等开发包，构建脚本用 pkg-config 检测后关闭对应后端（构建输出中的 `SDL3 backend` 行）。Linux 只要求编译、单元测试与代码验证，不要求音频输出或可游玩，当前配置满足要求。
- Windows：MSVC 14.44.35207、Windows SDK 10.0.26100.0，经 `third_party/winsdk/` 下的无空格符号链接引用。
