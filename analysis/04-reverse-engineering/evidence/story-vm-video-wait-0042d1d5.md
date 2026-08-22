# 剧情 VM 视频等待 `0x0042D1D5`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原Bink backend动态差分仍为`blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

handler：`0x0042D1D5..0x0042D1FF`

query helper：`0x00484DA0..0x00484DB2`，chunk `0x004846D0..0x004846E6`

opcode：193

## 1. helper合同

`sub_484DA0`先把返回值置为-1：

1. process video wrapper `dword_53D070`为空，返回-1；
2. wrapper首指针为空，返回-1；
3. 读取inner `+0x0C` frame number，再读取`+0x08` frame count；
4. unsigned `frame_number <= frame_count`时按原bit pattern返回frame number；
5. 否则执行u32 wrapping negation后返回。

现代窄port调用actual `LegacyVideoPlayer::legacy_progress()`：inactive返回-1；active按frame-number→frame-count顺序查询，使用相同unsigned关系与wrapping negation。该player/backend已有独立UT固定调用顺序与正负结果。Bink wrapper由typed backend替代属于既有平台适配。

## 2. handler控制流

handler无operand，固定物理长度2。它调用query恰好一次并执行signed结果判断：

```text
progress >= 0  -> 不推进，发布previous193，service audio一次，yield
progress < 0   -> IP += 2，发布previous193，same-call，无audio
```

原旧C++已调用正确port并按符号决定等待/完成，但两路均漏发common previous，非负等待路还漏掉`_AIL_serve`。本轮恢复完整共同出口。

非负包括0和`INT32_MAX`；负值包括inactive -1和wrapping negation可能形成的`INT32_MIN`。handler不主动step/close视频，不修改video owner。

精确尾`IP=0x7FFE`：active留在原IP、previous/audio/yield；inactive先推进到`0x8000`并发布previous，再由same-call successor fetch返回`instruction_out_of_range`。

## 3. 真实资产与验证

线性目录锁定唯一记录：

```text
TALK1.DAT@0x0000450E  raw=0x00C1  length=2  probes=1
```

该record紧随`OPENING.bik`视频启动。真实回放覆盖progress0 active等待和progress-1 inactive完成。全文件候选为`00C1=512`、`40C1=41`、`80C1=1`、`C0C1=4`；除上述唯一线性入口外均不冒充opcode记录。

synthetic覆盖四raw alias、progress `0/1/INT_MAX/-1/INT_MIN`、query时IP/previous未改、active previous先于audio、inactive noaudio same-call、query恰好一次和两类精确尾。Story100真实长链继续验证OPENING启动后查询一次并完成。

LST→C++：helper signed结果、两路IP、common previous、active audio和continuation逐块映射。

C++→LST：没有把0视为完成、主动step/close、active推进、inactive audio或漏发previous。现代视频backend是既有明确平台边界。

Story VM synthetic/real/initial-session与audio-video依赖4/4、SDL app编译通过；Linux core 186/186与app 192/192完整门通过。workpack双生成稳定hash为`df4cc9cff50b34a98ebaef859f54b241ddb27a58ad43cf7f42df13e1e7af7ccd`。未启动原版或OpenSWD3游戏EXE。
