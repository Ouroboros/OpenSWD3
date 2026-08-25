# FFmpeg 9.0 最小LGPL共享依赖

状态：官方源码、签名、构建参数和工具链已锁定；Linux x64与Windows x64最小共享包由项目脚本独立构建，CMake配置阶段不联网也不现场编译FFmpeg。

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

只有该显式依赖构建脚本可在源码归档或签名缺失时访问上面的两个官方URL。CMake只读取下列已安装目录，不下载、不解包、不编译FFmpeg：

```text
build/dependencies/ffmpeg/9.0/self-built/
  linux-x64/
  windows-x64/
```

可用CMake cache变量`OPENSWD3_FFMPEG_ROOT`覆盖平台包根目录。

## 3. 工具链锁

本次锁定并验证的工具链为：

- Linux C compiler：`gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`
- Windows x64 cross compiler：`x86_64-w64-mingw32-gcc-posix (GCC) 13-posix`
- MinGW linker/strip：`GNU Binutils 2.41.90.20240122`
- GNU Make：`4.3`
- GnuPG：`2.4.4`
- GNU tar：`1.35`
- XZ Utils：`5.4.5`

脚本固定`LC_ALL=C`、`TZ=UTC`和`SOURCE_DATE_EPOCH`，每次清理源码树、平台build tree与install prefix后重新构建。FFmpeg安装prefix固定为`/`并通过`DESTDIR`落入本地包，避免把仓库绝对路径写入configure字符串。Windows链接禁用PE时间戳。

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
--enable-shared
--disable-static
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
--extra-cflags=-Os -ffunction-sections -fdata-sections -fno-ident
--prefix=/
```

Linux额外使用：

```text
--cc=gcc
--extra-ldflags=-Wl,--gc-sections
```

Windows x64额外使用：

```text
--target-os=mingw32
--arch=x86_64
--enable-cross-compile
--cross-prefix=x86_64-w64-mingw32-
--cc=x86_64-w64-mingw32-gcc-posix
--disable-pthreads
--enable-w32threads
--extra-ldflags=-Wl,--gc-sections,--no-insert-timestamp -static-libgcc -static
```

未启用`GPL`、`version3`或`nonfree`。FFmpeg本体保持LGPL 2.1-or-later共享库；Windows只把GCC/MinGW运行时静态收进共享库，避免额外分发`libgcc_s_seh-1.dll`、`libatomic-1.dll`或`libwinpthread-1.dll`。

`--disable-x86asm`移除NASM/YASM工具依赖，但保留C编译器支持的内联实现。当前真实640×480 Bink与MP3资产仍由端到端测试验证，不以组件名称推断可用性。

## 5. 产物布局

两个平台都安装完整公共头文件、`LICENSE.txt`和`BUILDINFO.txt`。运行时组件固定为：

- `avformat`
- `avcodec`
- `avutil`
- `swresample`
- `swscale`

Linux输出五个带major SONAME的共享库及链接。Windows输出五个DLL，并同时输出：

- `bin/<component>.lib`：LLVM/MSVC可读取的COFF import library；
- `lib/lib<component>.dll.a`：GNU import library；
- `lib/<component>-<major>.def`：导出定义。

Windows DLL只依赖系统`KERNEL32.dll`、`bcrypt.dll`、`msvcrt.dll`以及包内FFmpeg DLL，不依赖其他第三方运行库。

## 6. 体积

原BtbN Windows n9.0 `lgpl-shared`五个运行库基线为`99364864`字节，即`94.76 MiB`。

自建结果：

- Linux五个运行库：`1838408`字节，`1.75 MiB`；
- Windows五个运行库：`3177472`字节，`3.03 MiB`；
- Windows相对基线减少`96.80%`。

头文件、许可证、导入库、`.def`和构建说明不计入运行库体积。

## 7. 机械验收与复现

运行：

```bash
./dependencies/ffmpeg/9.0/verify-minimal.py
```

验收器直接检查：

- 源码尺寸与SHA256；
- 两个平台五个共享运行库和完整头文件；
- Windows三类导入资料；
- Linux与Windows二进制内嵌的实际configure字符串；
- 版本为`9.0`，白名单组件存在，GPL/version3/nonfree均未启用；
- ELF/PE动态依赖没有未打包第三方库；
- Windows体积及相对基线降幅。

最终包必须由连续两次clean build产生相同的20个共享库及导入库SHA256。验证报告写入Git忽略目录`build/dependencies/ffmpeg/9.0/self-built/verification.json`。

## 8. 项目接入

`cmake/OpenSWD3FFmpeg.cmake`默认选择`self-built/<platform>`，验证本地头文件、共享库及Windows import library。项目仍只构建一个自有`openswd3_ffmpeg`共享媒体后端；FFmpeg C API不进入音视频兼容核心、剧情VM或应用编排层。

应用和真实媒体测试输出目录只复制项目媒体库、共享SDL3、上述五个FFmpeg运行库及`LICENSE.txt`。真实资产门必须覆盖MP3、`Map_Ca12`/`Map_Eu08`循环、`firegod.bik`和`opening.bik`完整解码。
