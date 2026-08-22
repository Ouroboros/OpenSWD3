# 剧情 VM 场景音乐stream停止 `0x0042C3F7`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042C3F7..0x0042C49D`

opcode：137 / `OP_137_STOP_SCENE_MUSIC_STREAM`

## 1. 记录与双音乐槽组

记录只含u16 opcode，固定长度2，无operand。`0x004B7C74..0x004B7C88`是两组三dword音乐槽：

```text
0x004B7C74  world/normal group request
0x004B7C78  world/normal group first music id
0x004B7C7C  world/normal group second music id
0x004B7C80  scene/alternate group request
0x004B7C84  scene/alternate group first music id
0x004B7C88  scene/alternate group second music id
```

逐帧`sub_40CDD0`先消费scene request的pending位，再消费world request；后消费的world request可覆盖transition mode。`sub_40CF40`维护world两个ID，opcode114维护scene三项。`sub_40E0B0`以`rep stosd`一次清六个dword。

现代VM state补齐world三项，与既有scene三项共同承接完整六槽进程owner；初始化严格清六槽。opcode137保持world两个ID不变，只发布world request并清scene三槽。

## 2. transition分流

入口首先测试`music_control_flags & 0x00800000`，发生在任何槽写、flags mask与IP推进之前：

- bit23置位：调用`sub_485880`。mode1对stream100发divisor1 fade并清mode/current fade；mode2按pending divisor发fade、复制到current fade且保留mode2；其他完整u32 mode不调用backend、不改字段。backend结果不参与handler流控。
- bit23未置位：不调用helper，直接把transition mode和pending fade divisor清零；current fade divisor保持原值。

现代复用opcode114已审计的`apply_music_stream_transition`窄port。SDL port继续以bit-preserving `u32↔i32`接实际`LegacyStreamManager`，有效mode与状态迁移不新增第二套实现。

## 3. 请求、flags、IP与yield顺序

transition分流结束后，机器严格执行：

```text
world request       = 0x80000001
scene request       = 0
scene first id      = 0
scene second id     = 0
music control flags &= 0xFF5CFF00
IP                  += 2
previous            = 137
yield
```

world两个ID不改，current fade只受前述helper合法分支影响。handler不调用`AIL_serve`，没有audio service、same-call continuation、operand读取或失败出口。

机器在请求写前执行`wsprintfA("StreamStartOut")→nullsub_1`，在IP提交后读取world两个ID并执行`wsprintfA("S_M_off[%d,%d]")→nullsub_1`。两次callee为空操作，唯一额外效果是覆盖共享Win32 `FileName`诊断scratch；现代没有该无消费者可变日志缓冲，不保留格式化副作用。业务槽、transition、flags、IP与common join顺序不变，此差异归入平台适配。

完整两字节记录可位于`IP=0x7FFE`并精确结束于窗口尾：状态写、IP=`0x8000`、previous137和yield全部完成，不读取后继字节。

## 4. 资产与验证

完整线性TALK目录锁定60条物理记录/60 probes，全部raw `0x0089`、长度2：

```text
TALK1  20
TALK2   8
TALK3  13
TALK4  19
```

四库基础raw `0x0089`字样总数为`69/20/20/35`；三个高位alias raw字样均为零。代表记录使用`TALK1.DAT@0x00005925`、`TALK2.DAT@0x0000DEA4`、`TALK3.DAT@0x00002E28`与`TALK4.DAT@0x00002F40`，四条均完成真实回放。

synthetic覆盖四raw alias、bit23两路、transition mode1/2/其他完整u32、false路current fade保留、helper调用时六槽/flags/IP/previous旧值、`0xFF5CFF00`精确mask、world ID保留、scene三槽清零、无audio、previous137/yield，以及`IP=0x7FFE`精确尾。reinitialization同时锁定六槽一次清零。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186与app 192/192完整门以exit0通过。未启动原版或OpenSWD3游戏EXE。
