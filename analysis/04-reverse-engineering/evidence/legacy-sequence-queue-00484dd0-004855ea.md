# sequence/queue 与公共音频维护汇编证据

状态：B5.9 已实现并静态闭环

来源：`swd3.exe.lst`；LST 中折叠的 `0x004853E0` 构造块只用同次导出的原始内存字节
补足。汇编指令与机器码仍是唯一行为真值。

## 1. sequence manager

`0x00484DD0` 先以设备 `-1` 打开 MIDI output，失败后保留首条错误并以设备 `0`
重试；两次均失败时错误文本按 `first //second` 拼接。成功后只分配一个 24 字节节点
和一个 sequence handle，节点进入 `+0x43C` free 链。

`0x00484F60` 只在 initialized、enabled 且 ID 非零时从 free 链取节点。文件按
`OPEN_ALWAYS + GENERIC_READ` 打开，全部字节在 sequence 活动期间保持存活。Miles
初始化返回零时释放字节并回收节点；返回 `-1` 时记录错误但仍继续设置 user-data、音量、
循环次数并启动。这一异常分支没有被修复。

`0x00485180` 按下列状态维护 active 链：

- status `1/2`：结束 sequence、释放文件字节并把节点送回 free 链；handle 保留复用。
- status `4/8/16`：保留节点。
- 其他 status：保留，除非紧邻的前一节点刚被移除；该原始级联回收分支原样保留。
- 位 `0x80000000` 的淡出分支继续使用 4 位 fixed-point 音量。

`0x004852F0` 每检查一个节点会调用两次 `AIL_sequence_user_data(handle, 0)`，第一次结果
被丢弃；实现和 UT 保留了这次重复调用。`0x00485290` 的 ID 零路径只检查 active head。

## 2. queue coordinator

`0x004853E0` 的构造字节确定默认值：transition ticks 为 30，current/pending mode 都为
3，音量为 127；current 记录和两组 queue 记录清零。两组 queue 各有两条记录，每条
20 字节。`0x00485460` 按 selector `2` 清 sequence 组、按 `1` 清 stream 组，其他值
不修改状态。

`0x004854B0` 的一次维护顺序为：

1. 按 current playback type 查询现有 sequence/stream 是否已经消失；仍活动时立即返回。
2. 清空 current 20 字节记录，消费 pending mode；mode `1/2` 分别重置对应组索引，mode
   `3` 清除 current mode。
3. 从 current mode 对应的两条记录中复制下一条；即使 filename 为空也会推进索引。
4. playback type `2` 启动 sequence；type `1` 先 beep 再启动 stream；音量取全局字段，
   loop count 固定为 1。
5. repeat 字段等于 1 且第二条已消费时，把该组索引绕回零。

## 3. 公共顺序与平台边界

`0x0040CF10..0x0040CF3A`的机器顺序严格为queue → stream → sequence → sample，
并固定返回`AL=1`。现代入口直接调用实际队列和三个资源管理器；返回值不参与业务判断，
使用`void`，不把恒一结果描述为播放或回收成功。

`0x00485330`是Miles `AIL_serve`边界，不含游戏内部逻辑；替代实现复用实际音频
维护函数。`0x00485360` 的旧 DLL 启动和资源字符串读取属于平台替代边界，启动调用位置
保留，但不在核心伪造 Miles DLL。

## 4. 验证

- fake sequence backend 锁定默认/回退打开、单节点池、文件字节寿命、`-1/0` 初始化
  分支、重复 user-data 查询、status 分支和 shutdown 顺序。
- fake queue ports 锁定两组两条记录、busy 门控、beep→stream、sequence、repeat 和
  pending mode 迁移。
- 最初的maintenance UT通过Port调用记录锁定顺序；本次由实际资源测试替代，见第5节。
- Linux `core` 73/73、Linux `app` 77/77、Windows LLVM `app` 77/77 CTest 通过；未
  启动原版或重写版游戏 EXE。

## 5. 公共维护Port移除

删除`LegacyAudioMaintenancePorts`、SDL派生转发类及`service_audio`全局转发函数。
`LegacyAudioMaintenanceBindings`只借用实际队列、流、序列和音效管理器，
不提供虚函数或操作分派。窗口事件、显示维护、世界帧及战斗脚本使用同一个直接入口。
SDL各使用者按值保存借用关系，资源仍由原有主函数局部对象持有，析构顺序不变。

入口逐条对应LST的四次调用，不按某一次返回值跳过后续维护。流管理器关闭新播放时
仍维护已有流；样本维护保留其自身开关语义。队列自身的Port仍待后续独立迁移，
本批不宣称音频模块或全项目所有Port已经移除。

测试使用真实管理器和受控媒体backend，创建合成序列文件与SND索引，观察：

- 活动媒体按队列、流、序列、音效的顺序维护，仍保有实际节点与样本字节。
- 完成媒体按同一顺序回收节点和样本缓冲区、归还档案引用。
- 队列查询发生在流回收之前；当前轮保持等待，下一轮才消费待处理模式。
- 禁止新流播放仍不跳过已有流或后续资源维护。

验证：6并发core与ASan定向音频协调测试各1/1通过，SDL应用链接通过。
日志无warning/error，`git diff --check`通过；测试后仅补齐新测试方法间的空行。
日志保存在`build/tmp/runtime/audio-maintenance-direct-{core,asan,sdl}.log`。
第4节全量结果属于历史记录，不代表本批执行范围；没有启动游戏程序。
