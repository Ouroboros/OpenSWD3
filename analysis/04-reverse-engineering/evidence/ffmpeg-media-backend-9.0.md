# FFmpeg 9.0 音频流与视频后端

## 范围

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`linux_runtime_integrated`、`windows_runtime_integrated`。

该后端在既有平台无关合同后完成此前延期的媒体实现：

- BGM/MP3通过`LegacyStreamBackend`和`LegacyStreamManager`播放。
- BIK/OP通过`LegacyVideoBackend`和`LegacyVideoPlayer`播放。
- SDL3继续拥有最终音频设备与画面呈现职责。
- FFmpeg类型和函数不进入音视频兼容核心、剧情VM或应用编排层。

原版Miles/Bink行为合同继续由既有LST证据和fake backend测试定义。FFmpeg只是专有解码器的平台替代，不建立新的兼容合同。

## 依赖锁与许可

后端使用`dependencies/ffmpeg/9.0/SOURCE.md`记录的BtbN n9.0 Windows x64和Linux x64 `lgpl-shared`包。

CMake行为：

- 不联网，按平台选择`windows-x64`或`linux-x64`。
- 接受显式`OPENSWD3_FFMPEG_ROOT`覆盖。
- 校验本地头文件、导入库和运行库。
- 导入`avformat`、`avcodec`、`avutil`、`swresample`和`swscale`。
- 构建项目自有`openswd3_ffmpeg`共享库。
- 构建单一共享SDL3 fallback，使应用和媒体库使用同一SDL设备与运行时状态。
- 将项目媒体库、SDL3、五个FFmpeg运行库及上游`LICENSE.txt`复制到应用和真实媒体测试可执行文件旁。

Linux共享库RUNPATH优先使用`$ORIGIN`。Windows测试证明复制后的DLL集合无需依赖系统FFmpeg安装即可加载。

## 音频流后端

`make_legacy_stream_backend()`创建绑定到配置游戏数据目录的后端。

每个打开的音频流按以下顺序处理：

1. 规范化旧式反斜杠路径，不修改兼容层文件名合同。
2. 通过libavformat打开媒体并选择最佳音频流。
3. 通过libavcodec解码。
4. 通过libswresample转换为48 kHz双声道交错float。
5. 打开SDL3播放流，并实现Miles兼容的user data、音量、循环、启动、状态和毫秒位置操作。
6. 向未修改的`LegacyStreamManager`报告保持状态`4`、终止状态`2`和启动前状态`8`。

循环次数为零时无限重新排队；正数保持有限重播。manager拥有的淡入淡出及状态顺序不变。

真实媒体测试打开`Music/Map_Ca12.mp3`。该锁定资产为44.1 kHz单声道MP3，时长约4.023秒。测试通过真实FFmpeg和SDL dummy设备路径验证打开、user data、音量、无限循环保持状态，以及4,000至4,050毫秒的解码时长。

## 视频后端

`make_legacy_video_backend()`创建进程内单实例句柄后端，用于Bink容器。

每个打开的视频按以下顺序处理：

1. 选择并打开Bink视频解码器。
2. 从真实流time base推导帧节奏与帧总数。
3. 若存在内嵌Bink音频，则打开音频解码器，重采样为48 kHz双声道float，并排入SDL3队列。
4. 实现既有wait/copy/frame-count/frame-number/advance/service/close ABI，并增加明确的现代解码结果：`frame_ready`、`completed`或`failed`。
5. 通过libswscale将当前帧转换为RGB555或RGB565，直接写入居中的旧式目标span。
6. 使用SDL单调纳秒时钟实现既有帧等待合同。
7. demux到达EOF时flush解码器，发布实际解码帧数，并在不复制或呈现伪造黑帧的情况下结束。
8. 解码、packet提交或flush失败时立即关闭，不再重复推进不可用帧。
9. 音量与音频设备所有权保持在后端内部。

真实媒体测试打开`Video/firegod.bik`，验证640×480、176帧、首帧RGB565非黑、内嵌音频demux，并将全部176帧解码到明确EOF。测试另行打开真实OP资产`Video/opening.bik`，锁定其640×480和7,369帧，并将全部7,369帧解码到明确EOF。两个文件均能结束，不出现重复黑帧或末尾解码循环。现有剧情VM文件名适配选择的其他真实Bink资产复用同一后端。

## 应用接入

SDL主运行时不再实例化不可用的stream backend或立即完成型video backend。配置数据根目录解析后，主运行时构造两个FFmpeg factory，并只把其基类接口交给既有manager和剧情VM端口。

原接线存在运行时缺陷：剧情VM在已接受帧仍在执行时，把视频活动位写入`WindowEventState`；已接受帧尾随后用较旧的`FrameCoordinatorState::process_flags`覆盖它。结果是解码器句柄保持活动，但idle分派永远不会选择`step_video()`，脚本则一直等待未推进的黑帧。剧情VM端口现在写入当前帧协调状态，再由既有帧尾发布到窗口与idle owner。解码失败或EOF也会清除已发布的视频活动位。

验证期间未启动原版或OpenSWD3游戏EXE。

## 验证

- 链接的运行时版本以`n9.0`开头。
- Linux真实SDL媒体测试通过：MP3、`firegod.bik`完整176帧和`opening.bik`完整7,369帧均解码到EOF。
- Player fake backend测试证明：解码EOF不会复制或呈现黑帧；解码失败会关闭句柄。
- 帧运行时测试证明：剧情VM写入的视频活动位会保留在发布给idle分派的已接受帧状态中。
- 运行时修复后，Linux core无SDL/无FFmpeg配置保持独立并通过`186/186`。
- 运行时修复后，Linux app完整门通过`192/192`。
- 早先Windows LLVM app完整门通过`192/192`；运行时修复后的Windows复跑仍待完成，因为当前WSL会话丢失宿主`WSLInterop` binfmt注册且无权重新挂载。
- Linux ELF依赖从应用输出目录复制的文件中解析全部五个FFmpeg库，不存在缺失的FFmpeg依赖。
- Linux和Windows应用/测试输出目录均包含`openswd3_ffmpeg`、共享SDL3运行库、五个FFmpeg运行库及`LICENSE.txt`。
- ELF和PE依赖检查证明应用与`openswd3_ffmpeg`解析到同一个共享SDL3运行时实例。
