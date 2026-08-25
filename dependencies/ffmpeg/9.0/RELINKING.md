# openswd3_ffmpeg静态FFmpeg可重链接说明

OpenSWD3把五个FFmpeg 9.0静态库链接进项目自有的`openswd3_ffmpeg`共享媒体库。应用运行目录不再需要五个FFmpeg DLL或SO。此布局仍要求二进制发布者履行FFmpeg的LGPL 2.1-or-later义务，并让最终用户能够用修改后的FFmpeg重新链接媒体库。

本文不是法律意见。发布二进制前应由发布者按实际分发方式复核许可证义务。

## 1. 构建生成的合规包

应用构建会在构建树生成独立合规包，不复制到游戏运行目录：

```text
build/linux-app/compliance/<配置>/openswd3_ffmpeg-linux/
build/app/compliance/<配置>/openswd3_ffmpeg-windows/
```

包内包含：

- FFmpeg官方`ffmpeg-9.0.tar.xz`及detached signature；
- 官方release签名公钥、源码锁、构建脚本和验收器；
- 当前五个FFmpeg静态库、`BUILDINFO.txt`和LGPL文本；
- 构建`openswd3_ffmpeg`时产生的非FFmpeg目标文件；
- SDL3链接库；
- 本重链接说明和`relink.py`。

运行目录可以继续只放`openswd3`、`openswd3_ffmpeg`、SDL3运行库、许可证与配置，不需要把合规包展开到运行目录。发布者应把对应平台的合规包作为同一发行版的可取得附件一并提供。

## 2. 原样重链接

在Linux包目录运行：

```bash
python3 relink.py --platform linux
```

在带有项目锁定LLVM、Windows SDK和MSVC运行库的Windows终端，或能调用这些Windows工具的WSL环境中运行：

```bash
python3 relink.py --platform windows
```

输出位于包内`relinked/`。脚本只链接包内目标文件、五个FFmpeg静态库、SDL3链接库和操作系统库，不需要原应用源码即可替换FFmpeg部分并重建共享媒体库。验证或使用时，把重链接产物复制到应用运行目录并替换同名媒体库；该目录已有对应SDL3运行库。

## 3. 使用修改后的FFmpeg重链接

1. 解开包内`source/ffmpeg-9.0.tar.xz`。
2. 修改FFmpeg源码，但继续遵守实际启用组件的许可证。
3. 用包内`build-minimal.sh`和`BUILDINFO.txt`记录的参数生成同平台静态包；也可以使用接口兼容的自定义构建。
4. 指向修改后的包重新链接：

```bash
python3 relink.py --platform linux --ffmpeg-root /path/to/modified/linux-x64
python3 relink.py --platform windows --ffmpeg-root /path/to/modified/windows-x64
```

`--ffmpeg-root`目录必须保持项目静态包布局：Linux为`lib/lib<component>.a`，Windows为`lib/<component>.lib`。需要替换SDL3链接库时使用`--sdl-library`。

## 4. 验证替换结果

重新链接后应执行：

```bash
./dependencies/ffmpeg/9.0/verify-minimal.py \
  --linux-wrapper /path/to/libopenswd3_ffmpeg.so \
  --windows-wrapper /path/to/openswd3_ffmpeg.dll
```

还应运行真实MP3和Bink媒体测试，确认修改后的库仍满足发行版需要的ABI与媒体能力。验收器会拒绝仍动态依赖拆分FFmpeg运行库的媒体库。
