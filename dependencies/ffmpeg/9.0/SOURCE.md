# FFmpeg 9.0 最小LGPL静态依赖

状态：官方源码、签名、构建参数和工具链已锁定；Linux x64与Windows x64最小静态包由项目脚本独立构建，五个归档链接进单一`openswd3_ffmpeg`共享媒体库。CMake配置阶段不联网、不下载、不解包，也不现场编译FFmpeg。

## 1. 官方源码锁

- 上游：<https://ffmpeg.org/>
- release/tag：`9.0` / `n9.0`
- 源码URL：<https://ffmpeg.org/releases/ffmpeg-9.0.tar.xz>
- detached signature：<https://ffmpeg.org/releases/ffmpeg-9.0.tar.xz.asc>
- 字节数：`12032020`
- SHA256：`7f607a00dd0d28a729d5a4811205812eef01cf6ef6155025febb6f36a9062d52`
- release签名密钥：`FFmpeg release signing key <ffmpeg-devel@ffmpeg.org>`
- 完整指纹：`FCF986EA15E6E293A5644F10B4322F04D67658D8`
- `SOURCE_DATE_EPOCH`：`1785795290`

仓库保存官方ASCII armored公钥`ffmpeg-devel.asc`。构建脚本先核对源码字节数与SHA256，再在临时GnuPG home中核对完整指纹并验证detached signature；任一项不匹配即停止。

源码归档和签名缓存位于Git忽略目录：

```text
build/dependencies/ffmpeg/9.0/source/
  ffmpeg-9.0.tar.xz
  ffmpeg-9.0.tar.xz.asc
```

## 2. 构建入口与网络边界

统一入口：

```bash
./dependencies/ffmpeg/9.0/build-minimal.sh all
```

也可只构建一个平台：

```bash
./dependencies/ffmpeg/9.0/build-minimal.sh linux-x64
./dependencies/ffmpeg/9.0/build-minimal.sh windows-x64
```

只有该显式依赖构建脚本可在源码归档或签名缺失时访问上面的两个官方URL。CMake只读取下列已安装目录：

```text
build/dependencies/ffmpeg/9.0/self-built/
  linux-x64/
  windows-x64/
```

可用CMake cache变量`OPENSWD3_FFMPEG_ROOT`覆盖平台包根目录。Windows LLVM工具目录默认是`/mnt/d/Dev/Compiler/LLVM/x64/bin`，可用环境变量`OPENSWD3_WINDOWS_LLVM_BIN`覆盖。

## 3. 工具链锁

本次锁定并验证的工具链为：

- Linux C compiler：`gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`
- Windows C compiler：`clang-cl 21.1.8`
- Windows linker：`LLD 21.1.8`
- Windows archiver：`llvm-ar 21.1.8`
- GNU Make：`4.3`
- GnuPG：`2.4.4`
- GNU tar：`1.35`
- XZ Utils：`5.4.5`

Windows FFmpeg静态对象和OpenSWD3应用都使用`x86_64-pc-windows-msvc` ABI，不再把MinGW ABI归档直接交给Windows LLVM应用。Windows构建在Windows可见的项目`build/dependencies`路径中执行干净源码树内构建，避免Windows编译器跟随WSL符号链接。

脚本固定`LC_ALL=C`、`TZ=UTC`和`SOURCE_DATE_EPOCH`，每次清理对应源码树、build tree与install prefix后重新构建。FFmpeg安装prefix固定为`/`并通过`DESTDIR`落入本地包。Windows对象使用`/Brepro`，LLVM归档保持确定性成员元数据。

## 4. 最小组件白名单

两个平台共同使用：

```text
--disable-everything
--disable-autodetect
--disable-network
--disable-programs
--disable-doc
--disable-avdevice
--disable-avfilter
--disable-debug
--disable-x86asm
--disable-iconv
--enable-small
--disable-shared
--enable-static
--enable-pic
--enable-avcodec
--enable-avformat
--enable-avutil
--enable-swresample
--enable-swscale
--enable-demuxer=bink,mp3
--enable-decoder=bink,binkaudio_dct,binkaudio_rdft,mp3float
--enable-parser=mpegaudio
--enable-protocol=file
--pkg-config=false
--prefix=/
```

Linux额外使用：

```text
--cc=gcc
--extra-cflags=-Os -ffunction-sections -fdata-sections -fno-ident
```

Windows x64额外使用：

```text
--target-os=win32
--arch=x86_64
--enable-cross-compile
--toolchain=msvc
--cc=clang-cl.exe
--cxx=clang-cl.exe
--ld=lld-link.exe
--ar=llvm-ar.exe
--nm=llvm-nm.exe
--strip=llvm-strip.exe
--disable-pthreads
--enable-w32threads
--extra-cflags=/Brepro /Gy /Gw
```

未启用`GPL`、`version3`或`nonfree`。FFmpeg保持LGPL 2.1-or-later。`--disable-x86asm`移除NASM/YASM工具依赖，但保留C编译器支持的内联实现；真实MP3和640×480 Bink资产由端到端测试验证，不以组件名称推断可用性。

## 5. 静态产物布局

两个平台都安装完整公共头文件、`LICENSE.txt`、`BUILDINFO.txt`和pkg-config资料。静态组件固定为：

- `avformat`
- `avcodec`
- `avutil`
- `swresample`
- `swscale`

Linux输出：

```text
lib/libavformat.a
lib/libavcodec.a
lib/libavutil.a
lib/libswresample.a
lib/libswscale.a
```

Windows输出MSVC ABI COFF归档：

```text
lib/avformat.lib
lib/avcodec.lib
lib/avutil.lib
lib/swresample.lib
lib/swscale.lib
```

静态包内禁止出现FFmpeg DLL、SO或import library。当前Linux五个归档共`3743892`字节（`3.57 MiB`），Windows五个归档共`4570902`字节（`4.36 MiB`）；这些是链接输入，不复制到游戏运行目录。

## 6. 运行时布局与体积

CMake把五个静态归档链接进项目自有共享媒体库。媒体相关运行文件缩减为：

Windows：

```text
openswd3_ffmpeg.dll
SDL3.dll
```

Linux：

```text
libopenswd3_ffmpeg.so
libSDL3.so.0
```

应用构建会精确移除旧版五个FFmpeg DLL/SO文件名，但不删除应用输出目录，不读取、重写或清理用户`openswd3.toml`。

原BtbN Windows n9.0 `lgpl-shared`五个运行库基线为`99364864`字节，即`94.76 MiB`。当前Debug媒体DLL为`1278464`字节（`1.22 MiB`），相对该基线减少`98.71%`；此前自建五DLL共`3177472`字节，静态归并后媒体DLL再减少`59.76%`并把五个FFmpeg运行文件归零。Linux Debug媒体SO为`2485968`字节（`2.37 MiB`）。

Windows媒体DLL不依赖拆分FFmpeg DLL或MinGW运行时，只依赖SDL3、Windows系统库和当前构建配置对应的MSVC运行库。Linux媒体SO不依赖拆分FFmpeg SO。

## 7. 机械验收与复现

只验收静态包：

```bash
./dependencies/ffmpeg/9.0/verify-minimal.py
```

同时验收应用媒体库：

```bash
./dependencies/ffmpeg/9.0/verify-minimal.py \
  --linux-wrapper build/linux-app/src/platform/sdl3/Debug/libopenswd3_ffmpeg.so \
  --windows-wrapper build/app/src/platform/sdl3/Debug/openswd3_ffmpeg.dll
```

验收器直接检查：

- 源码尺寸与SHA256；
- 两个平台五个非空静态归档和完整头文件；
- Linux与Windows归档内嵌的实际configure字符串；
- 版本为`9.0`，白名单组件存在，GPL/version3/nonfree均未启用；
- 静态包中不存在拆分FFmpeg运行库或import library；
- Windows使用clang-cl/MSVC ABI、Win32线程和可复现编译参数；
- 最终媒体SO/DLL不再动态依赖任何拆分FFmpeg运行库；
- Windows最终媒体DLL不依赖额外MinGW运行时。

连续两次clean双平台构建必须产生相同的十个静态归档SHA256。验证报告写入Git忽略目录`build/dependencies/ffmpeg/9.0/self-built/verification-static.json`。

## 8. LGPL静态重链接交付

静态链接不能只分发最终媒体DLL/SO。应用构建会在构建树生成独立合规包：

```text
build/linux-app/compliance/<配置>/openswd3_ffmpeg-linux/
build/app/compliance/<配置>/openswd3_ffmpeg-windows/
```

合规包包含精确FFmpeg源码归档与签名、release公钥、构建脚本、当前静态归档、媒体库非FFmpeg目标文件、SDL3链接库、LGPL文本和`relink.py`。最终用户可替换五个FFmpeg静态库并重新链接`openswd3_ffmpeg`，无需原应用其余目标文件。

合规包不进入游戏运行目录。二进制发布者应把对应平台的合规包作为同一发行版的可取得附件提供。完整步骤见[`RELINKING.md`](RELINKING.md)。

## 9. 项目接入与媒体门

`cmake/OpenSWD3FFmpeg.cmake`默认选择`self-built/<platform>`，验证本地头文件和静态归档。FFmpeg C API只存在于平台媒体共享库，不进入音视频兼容核心、剧情VM或应用编排层。

`build.bat app`和`build.sh app`只链接已经存在的静态包，不下载或现场构建FFmpeg。应用和真实媒体测试输出目录只复制项目媒体库、SDL3和`LICENSE.txt`。真实资产门覆盖MP3、`Map_Ca12`/`Map_Eu08`循环、`firegod.bik`和`opening.bik`完整解码。
