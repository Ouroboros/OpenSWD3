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

后端使用`dependencies/ffmpeg/9.0/SOURCE.md`记录的官方签名`ffmpeg-9.0.tar.xz`，由项目脚本构建Windows x64和Linux x64最小LGPL静态包。源码字节数、SHA256、release签名密钥完整指纹、configure白名单、`SOURCE_DATE_EPOCH`和双平台工具链版本均已锁定。Windows归档使用与应用一致的clang-cl/MSVC ABI，不再跨ABI链接MinGW归档。

CMake行为：

- 不联网，按平台选择`self-built/windows-x64`或`self-built/linux-x64`。
- 接受显式`OPENSWD3_FFMPEG_ROOT`覆盖。
- 校验本地头文件和五个静态归档，拒绝拆分FFmpeg运行库布局。
- 把`avformat`、`avcodec`、`avutil`、`swresample`和`swscale`静态链接进项目自有`openswd3_ffmpeg`共享库。
- 构建单一共享SDL3 fallback，使应用和媒体库使用同一SDL设备与运行时状态。
- 应用和真实媒体测试目录只复制项目媒体库、SDL3及上游`LICENSE.txt`；构建后精确清理旧五个FFmpeg运行库文件名，不删除输出目录或用户TOML。
- 在独立`compliance`目录生成精确源码、静态归档、媒体对象、SDL3链接库和重链接脚本，不增加游戏运行目录文件。

Linux媒体库RUNPATH固定为`$ORIGIN`。Windows媒体DLL使用Win32线程和clang-cl/MSVC ABI，只依赖SDL3、系统库和当前配置对应的MSVC运行库；Linux与Windows媒体库均不再依赖拆分FFmpeg运行库或额外MinGW运行时。

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

世界BGM运行时直接复用剧情VM中与原全局一致的六个音乐槽、请求控制位和stream transition字段。主帧从MAPS payload的根目录读取8字节地图音乐表，并从相邻的四字节偏移目录把音乐ID解析为源文件名；例如真实map214解析为ID102和`Map_Ca12.`，最终构造`Music\\Map_Ca12.mp3`并建立用户数据编号100的stream。BGM请求、启动、目录解析失败和媒体打开失败均写入运行日志。

逐帧消费末尾的`and ecx, 0xFFDFFFFF`只清`0x00200000`，不得清场景组循环位`0x00020000`。初次主帧接线误把掩码解释为后者，导致`Story_10`与`Map_Eu08`场景音乐对进入第二槽后不能循环；现已按LST修正，场景第二槽在真实MP3 EOF后重新建立stream100。

Windows真实音频设备复测进一步暴露：完整解码样本经`SDL_PutAudioStreamData()`入队后未调用`SDL_FlushAudioStream()`，因此设备侧可能保留尚未提交的输入尾部，`SDL_GetAudioStreamQueued()`在`Map_Eu08`播放结束后仍不归零，stream100不会进入Miles状态2。后端现于每次完整文件入队后显式flush；SDL3允许flush后继续入队，循环重开路径保持有效。

原版没有曲内loop point合同。`0x004856C0`每次都以loop count `1`调用`0x00486730`，后者执行`AIL_open_stream(driver, filename, 0)`并把该`1`传给`AIL_set_stream_loop_count`；单次stream从文件offset零播放一次。EOF后由`0x0040CDD0`依据普通组`0x00080000`或场景组`0x00020000`重新选择音乐槽并再次打开文件，因此重复从曲首开始就是原版静态控制流。当前`Music`目录77个MP3全部没有ID3，也没有`LOOPSTART`、`LOOPEND`、`LOOPLENGTH`、`LOOPPOINT`或`SMPL`标记。

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

原视频接线存在运行时缺陷：剧情VM在已接受帧仍在执行时，把视频活动位写入`WindowEventState`；已接受帧尾随后用较旧的`FrameCoordinatorState::process_flags`覆盖它。结果是解码器句柄保持活动，但idle分派永远不会选择`step_video()`，脚本则一直等待未推进的黑帧。剧情VM端口现在写入当前帧协调状态，再由既有帧尾发布到窗口与idle owner。解码失败或EOF也会清除已发布的视频活动位。

原BGM接线的`update_background_music()`为空，因此FFmpeg MP3后端虽然可以独立打开并播放真实文件，主帧却从未消费地图音乐请求。该入口现已接入MAPS音乐表、剧情VM音乐状态、stream transition和`LegacyStreamManager`，并保持原`0x0040CDD0`在普通帧分支前执行的顺序。

验证期间未启动原版或OpenSWD3游戏EXE。

## 验证

- 链接的运行时版本为`9.0`系列。
- 机械验收直接读取Linux与Windows静态归档内嵌configure字符串，确认只启用Bink/MP3解复用、Bink视频、两种Bink音频、MP3 float解码、mpegaudio parser、file协议、swresample和swscale；GPL、version3、nonfree、网络、程序、设备和滤镜均未启用。
- Linux五个静态归档为`3743892`字节（`3.57 MiB`），Windows五个MSVC ABI静态归档为`4570902`字节（`4.36 MiB`）；归档只作为链接输入，不进入运行目录。
- Windows Debug媒体DLL为`1278464`字节（`1.22 MiB`），相对原`94.76 MiB`五DLL基线减少`98.71%`；Linux Debug媒体SO为`2485968`字节（`2.37 MiB`）。
- 连续两次clean双平台静态构建的十个归档SHA256逐文件一致。
- 自建最小包的Linux真实SDL媒体测试通过：真实MAPS map214经世界音乐状态机启动`Music\\Map_Ca12.mp3`的stream100；普通组`0x00080000`和场景组`0x00020000`均在真实MP3 EOF后重开stream100；真实`Map_Eu08.mp3`播放到EOF后同样重开；`firegod.bik`完整176帧和`opening.bik`完整7,369帧均解码到EOF。
- Player fake backend测试证明：解码EOF不会复制或呈现黑帧；解码失败会关闭句柄。
- 帧运行时测试证明：剧情VM写入的视频活动位会保留在发布给idle分派的已接受帧状态中。
- 自建依赖接入后，Linux core无SDL/无FFmpeg配置保持独立并通过`188/188`。
- 自建依赖及`Map_Eu08`循环补证接入后，Linux app完整门通过`194/194`。
- 不删除`build/app`，直接执行`cmd.exe /c build.bat app`后，Windows LLVM app完整门通过`194/194`；首次构建前后用户`openswd3.toml`的SHA256完全一致。用户实机正常退出后只调整TOML段落顺序，全部配置值语义相等；后续Windows构建未再修改该文件时间。
- Windows应用`Debug`输出目录不再包含五个FFmpeg DLL，只保留单一`openswd3_ffmpeg.dll`和SDL3媒体运行依赖；媒体DLL不依赖拆分FFmpeg DLL或额外MinGW运行时。测试聚合进程继续单独保留16 MiB栈，生产应用与库未调整栈设置。
- 场景音乐循环掩码及SDL音频输入flush修复后，Windows LLVM app完整门分别通过`192/192`。
- Windows真实设备日志中，`Map_Eu08.mp3`分别于`21:58:29.021`、`21:59:11.621`和`21:59:54.226`报告启动，相邻重开间隔为42.600秒与42.605秒；与该MP3约42.53秒的解码时长一致，证明EOF后连续循环已实际生效。
- ELF和PE依赖检查证明两个平台的`openswd3_ffmpeg`都不再引用拆分FFmpeg运行库，并与应用解析到同一个共享SDL3运行时实例。
- Linux和Windows应用/测试输出目录均包含`openswd3_ffmpeg`、共享SDL3运行库及`LICENSE.txt`，不再复制五个FFmpeg运行库。
- 两个平台都生成运行目录之外的LGPL合规包；使用包内媒体对象、SDL3链接库和五个静态归档实际重新链接出替代`openswd3_ffmpeg`成功，证明最终用户可用修改后的FFmpeg重链接。
