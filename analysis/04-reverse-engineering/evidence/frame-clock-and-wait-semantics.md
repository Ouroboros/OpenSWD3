# 帧时钟、等待与阻塞语义

最后更新：2026-08-02

状态：主帧门控、间隔变更、`timeGetTime`/CRT `time` 调用、显式等待与 Sleep 调用已闭环

完整汇编是唯一真实依据。IDA 伪码只用于导航；本结论以完整汇编、导入表和机器调用点为准。

## 结论

当前 EXE 没有建立多媒体 timer callback。导入表中与 WINMM 时间有关的入口只有 `timeGetTime`；不存在 `timeBeginPeriod`、`timeEndPeriod`、`timeSetEvent` 或 `timeKillEvent`。

主循环使用 32 位毫秒采样和“最短帧间隔”门槛：默认 `35`，剧情路径可改成 `70`，若写成 `0` 则每次主帧调用都通过门控。它没有固定步长累计器，也不补跑丢失帧；一次延迟无论跨过多少个门槛，都只执行一次更新，然后把 gate 基准直接写成当前时间。

旧字符串 `mmTimer init Failed.` 不能覆盖汇编事实。它对应的 `0x0040DD20` 只写一个全局 dword 并恒返回一，当前失败分支不可达。

## 主线程调用模型

活动、未暂停且没有窗口消息时，`WinMain` 反复调用 `0x0040A570`。如果帧间隔尚未达到，函数直接返回，外层立即继续轮询；拒绝帧路径没有正时长 Sleep。

`WinMain` 与主帧各有一处 `Sleep(0)`：

- 外层门控阻止正常更新时让出当前线程剩余时间片；
- `0x0040A570` 发现显示 active 字段为零时让出后返回。

`Sleep(0)` 不是“睡到下一帧”。暂停且窗口仍 active 时，外层调用暂停绘制函数，而不是主帧；Bink 活动位 `0x20` 置位时，外层只推进 Bink 和音频维护。两种情况下主帧毫秒快照、输入归一化和按帧计数都停止。

## 主帧毫秒门控

`0x0040A596–0x0040A5CC` 的顺序为：

```text
time(NULL)             -> [0x004A99FC]
timeGetTime()          -> now
[0x004AAECC]           = now
elapsed                = u32(now - [0x004CC2B0])
if elapsed < [0x004B7BCC]: return
[0x004CC2B0]           = now
```

关键合同：

- 减法和 `jb` 是 32 位无符号比较，等价于模 `2^32` 的毫秒差；一次 `timeGetTime` 回绕仍按原 unsigned subtraction 工作。
- `[0x004B7BCC]` 是最短间隔，不是精确周期。只有 `elapsed >= threshold` 才接受帧。
- 接受帧后保存的是 `now`，不是 `previous + threshold`。例如门槛 35、实际经过 100 ms，只更新一次并丢弃多出的 65 ms，不补跑第二、第三帧。
- interval 为零时，`elapsed < 0` 永远不成立；即使 `timeGetTime` 同一毫秒返回相同值，也会接受帧。
- `[0x004CC2B0]` 不因暂停重置。恢复后通常立即接受一帧，但仍只推进一次。
- `[0x004AAECC]` 在门控判断前已写入，所以 busy poll 的拒绝尝试也会刷新它；游戏消费者只在接受帧后执行。

默认 35 ms 对应理论上限约 `28.57` 个接受帧/秒，70 ms 对应约 `14.29`；这只是门槛倒数。旧系统 `timeGetTime` 分辨率、调度、绘制和阻塞会使实际间隔更大，代码没有调用 `timeBeginPeriod` 请求特定分辨率。

跨平台兼容核心应接收截断到 `uint32_t` 的整毫秒样本并执行同样的无符号比较，不能直接把高精度浮点秒、SDL 自动 tick 或固定步长 catch-up 塞进游戏逻辑。

## interval helper 不是 timer API

两个 helper 的完整机器合同是：

```text
sub_40DD20(value):
    [0x004B7BCC] = value
    EAX = 1
    retn

sub_40DD30():
    [0x004B7BCC] = 0
    EAX = 1
    retn
```

`0x0040DD20` 有一个普通 32 位栈参数，调用者清栈。`0x0040DD30` 没有参数、也不清栈；六个调用点都在调用前把旧 interval 压栈，但 helper 完全不读取它，调用者随后只丢弃该栈槽。它不是“保存并恢复旧 timer”。

完整 12 个变更点只有三个结果：

- `0`：六次 `sub_40DD30`；
- `35 / 0x23`：初始化、显示恢复、地图/状态收尾和剧情 opcode 97；
- `70 / 0x46`：剧情 opcode 96。

显示恢复总是写 literal 35，不恢复停用前压栈的值。剧情 opcode 96 先清零再写 70，opcode 97 先清零再写 35。所有调用及其栈操作见 `inventory/frame-interval-mutations.tsv`。

总初始化 `0x00424E0C` 调用 setter(35)，随后测试 EAX；setter 恒返回一，所以 `mmTimer init Failed.` 消息分支在当前汇编中不可达。重写不得因字符串名称而虚构 timer thread 或 callback。

## 接受帧内的时间快照

通过门控后，`0x0040A725–0x0040A73F` 写：

```text
[0x004C844C] = now
[0x0049E1C8] = u32(now - [0x004C8444])
[0x004C8444] = now
```

随后才采样键盘并归一化输入。因此输入记录、剧情等待和对象时间都读取同一份 `[0x004AAECC]` 接受帧时钟，不在各自函数里重新查询墙钟。

`[0x0049E1C8]` 在完整汇编中只有一个消费者：调试画面的 `1000 / delta` 无符号整数 FPS 显示。第一次接受帧使用零初始化的 previous 值；暂停/失焦期间 previous 不更新，恢复帧的 delta 包含整段停顿。interval 为零时连续两次接受帧可得到 delta 零，调试除法没有防零；这是原始调试路径风险，不得在核心中顺手 clamp。

普通世界尾部又计算：

```text
[0x004C8448] = [0x004AAECC] - [0x004C844C]
```

两项在本帧前部已被写成同一个 `now`，中间没有其他写入，所以正常到达该点时结果严格为零。调试画面在尾部写入之前读取上一普通世界帧的该值，初始同样为零；字段不是 CPU frame time。

`[0x004CB224]` 是输入状态机对 `[0x004AAECC]` 的镜像，驱动严格 `>150` 的连按链过期；`[0x004CB23C]` 每次归一化再镜像一次，但完整汇编没有读取者。

## 使用接受帧时钟的等待规则

已确认的毫秒消费者使用相同的模 32 位差值，但边界并不统一：

- 输入连按链：仅在 raw 为 released 时检查 `u32(now-release_time) > 150`；等于 150 不过期。
- 剧情 opcode 67：第一次进入保存 16 位 duration 和起点，并把命令参数高位置为活动标记；以后只有 `u32(now-start) > duration` 才清标记并推进，等于 duration 仍等待。
- `0x0042ED40` 的对象/UI 状态：先算 `floor(u32(now-start)/100)`，再与 16 位门槛作严格大于比较；`0xFFFF` 是特殊哨兵。

这些等待只在接受帧被调用。暂停、显示停用和 Bink 外层分支不会按墙钟异步触发；恢复后由下一次调用用跳变后的 `now` 一次性重新判断。精确谓词见 `inventory/time-wait-rules.tsv`。

## 直接 `timeGetTime` 的 CD/文件检查循环

除主帧外，另外两条 `timeGetTime` 调用都在 `0x004118B0` 的同步 CD/文件检查流程中。它不依赖主帧快照：

- 文件轮询间隔为严格大于 1000 或 3000 ms，取决于候选字节序列长度；初始 last-scan 为零，通常立即执行第一轮。
- 轮询等待期间严格大于 100 ms 才调用一次 `AIL_serve`。
- 成功画面用 busy loop 保持到严格大于 500 ms。
- 成功 busy loop 中严格大于 90 ms 才调用一次 `AIL_serve`。

四处都使用无符号减法和 `jbe/ja`，不是大于等于。循环不调用 Sleep，会持续查询时钟并手动服务 Miles。

## CRT `time` 是另一时间域

完整汇编有八次 `_time`：

- 两次连续调用分别播种 CRT-compatible random state 和独立的 250-dword xor 随机状态；跨秒边界时两种种子可以不同，调用顺序必须保留。
- 存档路径用 `difftime(now, Time2)` 累加游戏时长，再刷新 `Time2`；载入和一条模式路径会重置 `Time2`。
- 一条资源路径再次播种 CRT-compatible random state。
- 主帧每次尝试在毫秒门控之前把 32 位结果写入 `[0x004A99FC]`；完整汇编没有读取该全局，是当前构建的死存储，但调用和写入位置已经记录。

机器只传递/保存 32 位值。现代 64 位 `time_t` 不能未经显式截断和兼容策略直接渗入随机种子、存档游玩时间或旧状态字段。

## Sleep 与按帧倒计时

完整汇编有 27 个 Sleep 调用：2 个 `Sleep(0)`，25 个正时长阻塞。正时长立即数范围为 20、50、100、150、200、250、300、350、500、600 和 1024 ms，分布在开发热键、高优先级状态、初始化错误提示、剧情 ANI 错误、战斗流程等路径。逐调用点见 `inventory/sleep-callsites.tsv`。

Sleep 阻塞整个游戏主线程；在此期间没有主帧输入、剧情、战斗或按帧计数推进。不能把它们普遍改成“异步延迟但逻辑继续跑”。是否将某个阻塞点隔离为新系统兼容外壳，需要对应子系统证据单独批准。

另一类所谓“时间”完全是接受帧/归一化调用计数，例如：

- 帧前 `[0x004CAD20]`、`[0x004A9918]` 的递减；
- 普通世界 20/1000 循环计数；
- 输入连续按住计数和各消费者 repeat；
- 鼠标静止第 451 个归一化样本。

它们不能乘以 35 ms 后替换成 wall-clock duration。interval 从 35 改为 70、发生卡顿或暂停时，这些计数的现实时间会随之改变；这是原行为。

## SDL3 / C++20 兼容合同

1. 平台层提供单调时间，进入兼容核心前截为模 `2^32` 的整数毫秒；核心只用 unsigned subtraction。
2. 保留门槛 `0/35/70`、`elapsed >= interval`、接受后 `previous=now` 和不 catch-up。
3. 保留暂停、失焦、Bink 分支期间不调用主帧的冻结边界；恢复只推进一帧。
4. 保留每个等待点自己的严格 `>` 或 `>=`，不可统一为一个 duration helper 的默认边界。
5. 把设备/时钟采样注入兼容核心，差分测试使用固定 `uint32_t` 时间序列；宿主刷新率不能自行决定逻辑帧数。
6. SDL 可在平台外壳中减少无意义 busy spin，但必须证明送入核心的接受帧时间、调用次序和 Miles/媒体服务边界不变。
7. 不建立原 EXE 不存在的多媒体 timer callback，也不把 `mmTimer init Failed.` 当作 API 规格。

## 可复现产物

- `tools/build_time_flow_inventory.py`
- `inventory/time-source-callsites.tsv`
- `inventory/frame-interval-mutations.tsv`
- `inventory/time-global-accesses.tsv`
- `inventory/sleep-callsites.tsv`
- `inventory/time-wait-rules.tsv`

生成器锁定原 EXE、完整汇编和导入表哈希，硬校验 11 个时间源调用、12 个 interval 变更、42 条时间全局访问、27 个 Sleep 和 9 类关键等待规则。伪码不参与生成。
