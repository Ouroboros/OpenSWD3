# FFmpeg 9.0 预编译依赖来源

状态：已下载、已校验、尚未接入CMake或运行时后端。

## 上游

- 仓库：<https://github.com/BtbN/FFmpeg-Builds>
- release页面：<https://github.com/BtbN/FFmpeg-Builds/releases/tag/latest>
- release tag：`latest`
- release发布时间：`2026-08-22T13:24:06Z`
- 本次获取时间：`2026-08-22T15:49:51Z`
- 版本系列：`n9.0`
- 包类型：`lgpl-shared`
- 架构：Windows x64与Linux x64

BtbN只在滚动的`latest` release提供这两个n9.0包；tag本身不是不可变版本。为避免后续`latest`漂移，本项目同时锁定GitHub asset ID、asset更新时间、原始文件名、字节数与SHA256。任何重新获取都必须全部匹配，不得只依赖`latest` URL。

## Linux x64

- GitHub asset ID：`525064837`
- asset更新时间：`2026-08-22T13:23:53Z`
- 原始文件名：`ffmpeg-n9.0-latest-linux64-lgpl-shared-9.0.tar.xz`
- 本地文件名：`ffmpeg-n9.0-linux64-lgpl-shared-9.0.tar.xz`
- 字节数：`54325956`
- SHA256：`1857bfb5781d82e6f402be251a5019b24f20ed340084951fbc2cdaa69c197bb4`
- asset API：<https://api.github.com/repos/BtbN/FFmpeg-Builds/releases/assets/525064837>

## Windows x64

- GitHub asset ID：`525064920`
- asset更新时间：`2026-08-22T13:23:59Z`
- 原始文件名：`ffmpeg-n9.0-latest-win64-lgpl-shared-9.0.zip`
- 本地文件名：`ffmpeg-n9.0-win64-lgpl-shared-9.0.zip`
- 字节数：`67196309`
- SHA256：`80fa3acdcf73b8810a0aa2567674b12523ce6311651f80aca9797e88ccefd3f9`
- asset API：<https://api.github.com/repos/BtbN/FFmpeg-Builds/releases/assets/525064920>

## 本地目录

二进制包与解压结果位于Git已忽略的构建依赖目录，不纳入源码提交：

```text
build/dependencies/ffmpeg/9.0/
  SOURCE.json
  archives/
    ffmpeg-n9.0-linux64-lgpl-shared-9.0.tar.xz
    ffmpeg-n9.0-win64-lgpl-shared-9.0.zip
  linux-x64/
  windows-x64/
```

两个解压目录均包含上游`LICENSE.txt`、`include/libavcodec/avcodec.h`和对应平台的共享库。Linux二进制报告`ffmpeg version n9.0.1-6-g9d4ca21220-20260822`；验证时使用同包`lib/`作为`LD_LIBRARY_PATH`。

本清单只记录已验证的预编译依赖。不得在CMake configure阶段联网下载或源码编译FFmpeg；后续后端接入必须继续隔离FFmpeg API，不得扩散到兼容核心。
