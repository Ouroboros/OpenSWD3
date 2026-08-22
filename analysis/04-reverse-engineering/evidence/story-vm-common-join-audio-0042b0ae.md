# 剧情 VM common join 音频出口校正 `0x0042B0AE`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

共同出口：`0x0042B0AE..0x0042B0C8`

音频返回：`0x0042D4D7..0x0042D4F3`

## 1. 机器合同

每轮fetch前`ESI=0`。共同出口先把normalized opcode发布到`dword_4CF6D8`，再计算`var_28 | ESI`：

```text
(var_28 | ESI) != 0 -> same-call next fetch，不调用_AIL_serve
(var_28 | ESI) == 0 -> 调用_AIL_serve恰好一次，随后返回1
```

因此“进入`0x0042B0AE`且两项continue来源均为0”不是抽象的裸yield；它固定包含previous publication、一次最终audio service和跨调用返回。handler内部先行调用的`_AIL_serve`不替代这次最终service。

## 2. 已提交差异与校正范围

special opcode1024预审暴露旧C++把若干共同出口直接写成`yielded`，漏掉最终audio。独立LST复核确认以下25个handler组/30个opcode受影响：

```text
0x0042ADB7  20,169
0x0042949D  53 waiting
0x004295F3  55-57
0x0042B1F1  58,153
0x0042967B  59
0x00429693  60-61
0x00429A1B  63 overflow
0x00429AE8  65
0x00429B14  66
0x00429B62  67 waiting
0x00429BB5  68
0x00429C37  69
0x00429CBC  71
0x00429D0F  72
0x0042A611  85
0x0042A727  88
0x0042A7CE  94
0x0042A7EE  95
0x0042A80E  96 common paths only
0x0042AD3C  97 waiting
0x0042C7EA  98
0x0042AD75  99 waiting
0x0042B4CA  106,154 waiting
0x0042B50F  107 waiting
0x0042C234  133
```

现代新增单一`yield_from_common_join`窄helper，固定previous→audio→yield。只把上述确实到达共同出口的路径接入；same-call、typed-stop和handler内部audio不改。

两个多audio合同特别锁定：

- opcode85：清屏/提交后的显式audio仍保留，video helper返回后共同出口再audio一次，成功及modern preflight拒绝均合计2次；
- opcode96：成功路径两个显式audio后共同出口再一次，合计3次；modern `prepare_story_ani()==false`映射机器CD helper `0x0042A9C6 -> 0x0042D4B6`直接返回0，仍只1次audio且不发布previous，不被误纳入共同出口。

opcodes186-187 taken reload原实现已经是loader audio一次加共同出口audio一次；状态判断后的裸`yielded`不代表缺第三次audio，本轮明确不增加。

## 3. 验证

现有synthetic逐handler锁定至少一条共同yield的`direct_audio_service_count==1`；opcode85/96锁定2/3次以及callback事件顺序；opcode96 preflight锁定1次、旧previous不变。same-call和typed-stop断言继续锁定0次。

真实回放更新并锁定59、55-57、58/153、85、96、133以及以59/67为same-call后继的15/16/17/23/111/161长链累计计数。Story VM synthetic/real/initial-session 3/3通过。

LST→C++：逐入口确认最终jump为`loc_42B0AA/0x0042B0AE`或等价共享尾，且该路径`var_28=0, ESI=0`。

C++→LST：没有给same-call完成路、typed-stop或opcode96 CD preflight增加audio；没有把内部audio误删或把186/187变成三次。

SDL app编译通过；workpack双生成稳定hash为`87545fd372eb8f8ccf326bbac925b235ecaa0eb8c2c52fc88ee74a1c84d219a4`。Linux完整门通过：core 186/186、app 192/192。未启动原版或OpenSWD3游戏EXE。
