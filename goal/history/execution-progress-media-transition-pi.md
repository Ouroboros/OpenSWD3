# 执行历史：FFmpeg与模块9过渡

状态：冻结历史；不得作为当前执行状态或行为真值。

来源：重构前`execution-plan-pi.md` v853第2986..3072行，FFmpeg、媒体修正和模块9过渡。

完整性与当前资料入口见[`../execution-history-index-pi.md`](../execution-history-index-pi.md)。当前状态见[`../execution-state-pi.md`](../execution-state-pi.md)。

---


- 按用户决定完成FFmpeg 9.0预编译依赖落盘。BtbN当前只在滚动`latest` release提供n9.0包，
    因此除tag外锁定同批次asset ID/更新时间/文件名/字节数/SHA256。Linux x64与Windows x64
    `lgpl-shared` archives及解压结果位于Git已忽略的
    `build/dependencies/ffmpeg/9.0/`；SHA256分别为
    `1857bfb5781d82e6f402be251a5019b24f20ed340084951fbc2cdaa69c197bb4`与
    `80fa3acdcf73b8810a0aa2567674b12523ce6311651f80aca9797e88ccefd3f9`。可追踪来源清单为
    `dependencies/ffmpeg/9.0/SOURCE.md`。两个解压目录的license、头文件与共享库均存在，Linux
    二进制报告`n9.0.1-6-g9d4ca21220-20260822`。未接入CMake或运行时后端，且未在configure
    阶段联网下载/源码编译。

- 按用户决定先拆分剧情VM超大单元测试。原
    `tests/unit/world_map/legacy_world_story_vm_test.cpp`为36092行/1607263字节，现保留298行main，
    把1424行共享fixture/ports/helper移入support头、282个test函数声明移入cases头，并按完整函数
    边界分成6个约5555–5905行/235850–275179字节的`.cpp`。仍只生成
    `openswd3_world_map_legacy_world_story_vm_tests`一个二进制，不改变三条CTest注册、同二进制锁或
    `legacy_real_assets`全局锁。原HEAD与拆分后使用同一仓库clang-format基准的282个函数体逐函数
    SHA256完全一致，main的282个调用顺序一致；单目标编译、Story VM
    synthetic/real/initial-session 3/3、Linux core 186/186与app 192/192通过。
    该结构提交不改变198个opcode的实现/验收进度。

- 剧情VM P2第一百四十四组`0x0042D49F` / special opcode1025完成独立闭环。handler固定
    双指针+2、清ESI与调用期`var_28`，再经common join发布previous1025、audio一次并yield，
    不fetch后继。现代清除step栈内latch后复用统一common helper，未增加持久状态，归
    `assembly_exact`。四raw alias、无latch、1024持久latch清除、audio callback顺序、后继
    不读和`IP=0x7FFE`精确尾通过；线性资产0条/0 probes。旧测试中417处“未实现1025”标识
    引用已迁为明确typed-stop，其中读取下一指令`+2`的文件操作链使用保留lookahead字节的
    专用无previous typed-stop，既有IP/previous/audio断言全部保持。Story VM 3/3、SDL app
    编译、Linux core 186/186与app 192/192通过。workpack/runtime-path双生成hash分别为
    `85bd6d1f7549bb7877d1e4a2ba53db98e60dccfd314f6fce2d90604faba10df6`、
    `d4ef8d464ed37b2321e6ad5a9705cd5bce10ea0b06281f247b3fd705a74e1285`。本地进度为
    已实现197/198、已验收196/198；内部workpack144/146，即`25 assembly_exact +
    119 platform_adapted + 2 pending_audit`。

- 剧情VM P2第一百四十五组`0x0042D1EA` / special opcode1026完成独立闭环。handler固定
    双指针+2、设置一次性ESI，经common join发布previous1026、跳过audio并same-call；下一fetch
    清ESI，因此不同于1024持久latch，且1026不清已有1024 latch。旧bare case已补previous并
    使用语义常量，归`assembly_exact`。四alias、one-shot successor audio恢复、1024组合链、
    精确尾通过。完整线性资产4141条记录/4150 probes，覆盖四TALK文件与九个多probe物理位置；
    真实边界样本及所有现有reload/transfer链通过。Story VM 3/3、Linux core 186/186与app
    192/192通过。workpack/runtime-path双生成hash分别为
    `556287b870175442a1f8e2738d8f7a71cc79ddc4e46a2c56c014bfc5de328ea8`、
    `7ab493544aebc7b82c75d98d58356c43847d23a1cca85427d654c54b2be2b5c7`。本地进度为
    已实现198/198、已验收197/198；内部workpack145/146，即`26 assembly_exact +
    119 platform_adapted + 1 pending_audit`。

- 剧情VM P2第一百四十六组`0x0042D24E` / special opcode16383完成独立闭环。恢复source
    selector、角色path completion、one-shot/action update、bit19原始quirk、全角色与72对象槽清理、
    dialog/window/context/movement收尾和common join；3743条记录/3756 probes及17条真实边界/多probe
    回放通过。P2独立Windows LLVM app门经单独提交的MAPS资产门禁修复后192/192通过。workpack关闭为
    `146/146 = 26 assembly_exact + 120 platform_adapted`，hash为
    `f442d46d2d9179ee51c2c26da24e469cfba7ae1d2e9ac94882d612489b8075b5`。

- 剧情VM P3全VM验收完成。17条runtime path全部关闭；严格线性探针覆盖3992入口、58782记录、
    198行opcode目录且零中途解码错误；权威LST锁定CFG覆盖138988节点、137207边、零issue，九份生成
    库存连续两轮hash一致。Story VM 3/3、Linux core186/186、Linux app192/192及独立P3 Windows
    LLVM app192/192全部通过，未启动游戏EXE。公开进度为已实现198/198、已验收198/198；剧情VM
    追加PLAN完成并解除优先级覆盖。

- FFmpeg 9.0媒体后端完成。BtbN n9.0 `lgpl-shared`包通过项目自有`openswd3_ffmpeg`
    共享库接入，C API只存在于平台实现；BGM/MP3经既有stream ABI解码、重采样并交SDL3播放，
    BIK/OP经既有video ABI逐帧解码、定时、RGB555/565拷贝并处理内嵌音频。Linux/Windows真实
    `Map_Ca12.mp3`与`firegod.bik`测试通过，Linux core186/186及Linux/Windows app192/192完整门
    通过；应用和测试目录均复制项目库、五个FFmpeg运行库及LGPL许可，未启动游戏EXE。

- 模块9 scope lock完成。机械所有权库存锁定227个候选：高优先级5项、共享对话/队伍helper8项、
    标准模式1/3/4/5/6共204项、商店mode2共10项；全部重新置为`pending_audit`，不继承IDA名或
    既有mode3切片完成结论。`special-modes.md`固定owner、跨B10/B11边界、入口顺序和测试合同；
    workpack连续两轮稳定hash为`dff92f9a105168ebc6503c38c51f4de7b8201ad22080254f4526e7268f4e7a3d`。

- FFmpeg视频运行时修复完成Linux验证。Story VM原先把视频活动位写入窗口状态，随后被同帧旧
    协调状态覆盖，导致解码器已打开但idle永不执行`step_video()`；现改为写当前协调状态并由
    既有帧尾发布。解码后端增加明确EOF/失败终止，禁止重复呈现不可用黑帧。`firegod.bik`
    176帧和`opening.bik`7369帧均全量解码到EOF；Linux core186/186、app192/192通过。用户
    已确认Windows实际OP播放正常；修复后Windows LLVM app192/192完整门已复跑通过。

- FFmpeg BGM运行时修复完成Linux验证。原SDL主帧`update_background_music()`为空，导致MP3
    后端可独立播放但地图音乐请求从未被消费；现将剧情VM六槽音乐状态、真实MAPS地图音乐表、
    ID文件名目录、stream transition和`LegacyStreamManager`接入主帧。真实map214解析ID102并
    启动`Music\\Map_Ca12.mp3`的stream100；新增请求、启动、目录失败和打开失败日志。首次接线
    将LST掩码`0xFFDFFFFF`误读为清`0x00020000`，导致`Story_10`到`Map_Eu08`的场景第二槽不能
    循环；现修正为只清`0x00200000`，保留场景循环位。普通组与场景组真实MP3 EOF重开均通过。
    Windows真实设备仍因完整输入入队后未`SDL_FlushAudioStream()`而不报告EOF；现为每次完整文件入队
    显式flush，使设备队列可归零并触发stream100重开。Linux core186/186、app192/192及两轮修复后
    Windows LLVM app192/192通过。Windows实际日志证明`Map_Eu08.mp3`以42.600秒和42.605秒间隔
    连续重开两轮。权威LST证明原版每轮以offset零、loop count一重开stream；77个MP3均无曲内loop
    point元数据，因此从曲首重复属于原版行为，媒体复测正式关闭。
