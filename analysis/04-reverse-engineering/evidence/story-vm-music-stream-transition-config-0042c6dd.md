# 剧情 VM 音乐stream transition配置 `0x0042C6DD`

状态：`assembly_exact`（有效窗口域）、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C6DD..0x0042C731`

opcode：141 / `OP_141_CONFIGURE_MUSIC_STREAM_TRANSITION`

## 1. 记录与分阶段写入

记录固定6字节：

```text
+0  u16 opcode
+2  u16 transition mode
+4  u16 pending fade divisor
```

机器严格按两个独立访问点执行：

1. 读取`+2`并零扩展为u32，写`0x004B7380` transition mode。
2. 读取`+4`并零扩展为u32，写`0x004B74F0` pending fade divisor。
3. u16 IP增加6。
4. 从两个全局读回值供无状态诊断使用。
5. common join发布previous141并same-call继续。

`0x004B7378` current fade divisor不读不写。handler只配置状态，不调用`sub_485880`、不访问stream backend、不service audio，也不yield。

旧C++在入口一次预验完整6字节，并且成功后漏发previous141。本轮改为逐访问点检查：`+2`成功、`+4`缺失时保留已经写入的mode，但pending、IP和previous保持原值。opcode后连`+2`都不可读时没有状态写入。

## 2. debug与平台边界

两项写入和IP提交后，机器执行：

```text
wsprintfA(FileName, "Fmode[%d,%d]", mode, pending)
nullsub_1(FileName, 0)
```

`nullsub_1`为空操作；额外效果仅为覆盖共享Win32 `FileName`诊断scratch。现代没有该无消费者可变日志缓冲，不复制格式化副作用。transition三项状态、IP、previous和same-call顺序保持不变，此差异归入平台适配。

完整6字节记录可位于`IP=0x7FFA`并精确结束于窗口尾：mode/pending先提交，IP=`0x8000`和previous141完成，随后same-call下一fetch才返回窗口越界。

## 3. 资产与验证

完整线性TALK目录锁定124条物理记录/124 probes，全部raw `0x008D`、长度6：

```text
TALK1  37
TALK2  17
TALK3  22
TALK4  48
```

真实mode仅为1或2，分布`1:9, 2:115`。pending divisor分布：

```text
1:1, 5:1, 10:7, 15:1, 20:12, 25:27, 30:10,
40:13, 45:40, 50:5, 60:2, 70:2, 80:2, 90:1
```

四库基础raw `0x008D`字样总数为`120/27/43/85`；三个高位alias raw字样均为零。四库代表记录为：

```text
TALK1.DAT@0x000044E9  mode2, pending25
TALK2.DAT@0x0000D38D  mode1, pending25
TALK3.DAT@0x0000264E  mode2, pending40
TALK4.DAT@0x00002DC8  mode1, pending45
```

四条均完成真实回放并same-call后继。synthetic覆盖四raw alias、u16零与最大值、current fade保留、无backend、无audio、mode读取截断、pending读取截断时的mode partial commit，以及`IP=0x7FFA`精确尾。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186与app 192/192完整门均通过。未启动原版或OpenSWD3游戏EXE。
