# 剧情 VM 场景音乐stream请求 `0x0042B739`

状态：`platform_adapted`、有效运行域`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042B739..0x0042B7FB`

opcode：`114` / `OP_114_STAGE_SCENE_MUSIC_STREAM_REQUEST`

## 1. 记录与分阶段副作用

记录固定为8字节：

```text
+0  u16 opcode
+2  u16 first stream id
+4  u16 second stream id
+6  u16 request flags
```

handler入口先快照当前transition mode，再严格按以下顺序执行：

1. `0x004B7C80 = 0x80000001`；
2. 读取`+2`并零扩展写入`0x004B7C84`；
3. 读取`+4`并零扩展写入`0x004B7C88`；
4. 若入口快照mode为0，把mode写1；
5. 调用`sub_485880`应用现有stream transition；
6. 对`0x004ACDBC`先置bit23；
7. 才读取`+6` flags并派生请求位。

机器不预验完整记录。缺`+2`时只保留request marker；缺`+4`时还保留first id；缺`+6`时还保留second id、transition副作用和control bit23。现代实现逐访问点检查，只在原裸读取处停止，不回滚已提交状态。

## 2. 现有stream transition同步

`sub_485880`读取三个独立dword：

```text
transition mode          0x004B7380
current fade divisor     0x004B7378
pending fade divisor     0x004B74F0
```

行为为：

- mode 1：对stream 100提交divisor 1的fade，然后把mode与current divisor都清零；
- mode 2：对stream 100提交pending divisor，并把current divisor复制为pending，mode保持2；
- 其他完整32位mode：不调用backend，不改三字段。

backend返回值不参与handler控制流。现代窄port以bit-preserving `u32↔i32`转换复用`audio_video::apply_legacy_stream_transition`和实际`LegacyStreamManager`，再把mode/current divisor写回同一VM owner。新增current divisor字段不是占位：原opcode192直接读取`0x004B7378`。

Miles全局stream对象由typed manager替代，属于平台适配；有效mode和状态迁移保持机器语义。

## 3. control flags

transition调用返回后，handler先把`0x004ACDBC` bit23置一；因此flags读取失败也保留该写入。flags成功读取后，从该中间值：

1. 清bit16/17；
2. 若flags bit15置位，跳过两种派生；
3. 否则bit14置位时OR `0x00030000`；
4. bit13置位时再OR `0x00020000`；
5. 最后`AND AL,0`清完整dword低八位，其余位保持。

bit14与bit13是独立测试；两者同时置位的结果仍为`0x00030000`。bit15优先抑制两者。

## 4. 推进与common join

成功路径把物理指针与u16 IP各加8，设置continuation `ESI=1`，再经共享join发布normalized previous114。该路径不执行`_AIL_serve`，而是在同一次解释器调用中读取后继。

精确尾记录先完成全部状态、IP=`0x8000`和previous114，随后下一次取指才返回窗口越界。全局common-join继承carry仍属于独立runtime path；本handler自身已固定`ESI=1`，不受zero-carry让出分支影响。

## 5. 资产锁与验证

线性TALK目录锁定157条物理记录/159 probes，全部raw `0x0072`、长度8：

```text
TALK1  45
TALK2  31
TALK3  38
TALK4  43
```

共有90种不同operand三元组。flags只出现：`0x2000` 62条、`0x4000` 82条、`0x8000` 13条。代表记录`TALK1.DAT@0x000044EF`为`114,25,25,0x2000`，已完成真实回放。

synthetic覆盖四raw alias、stream id 0/`0xFFFF`、mode 0/1/2/`0xFFFFFFFF`、current/pending divisor分离、flags `0x2000/0x4000/0x6000/0x8000`、transition调用时快照、无audio、same-call后继、三个分阶段截断和完整精确尾。既有stream command测试覆盖实际manager的mode 1/2/其他分支。

Story VM synthetic、real及initial-session三项通过；Linux core 186/186、app 192/192完整门通过。未启动原版或OpenSWD3游戏EXE。
