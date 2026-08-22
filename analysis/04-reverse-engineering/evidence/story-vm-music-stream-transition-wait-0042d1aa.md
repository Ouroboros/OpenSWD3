# 剧情 VM 音乐stream transition等待 `0x0042D1AA`

状态：`assembly_exact`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042D1AA..0x0042D1D4`，共同join `0x0042B0AE..0x0042B0C8`，audio return `0x0042D4D7..0x0042D4F3`

opcode：192

## 1. owner与判断顺序

handler无operand，物理长度2。它复用scene music stream已闭环的完整u32 owner：

```text
0x004B7380  transition mode         LegacyWorldStoryVmState::current_first_stream
0x004B7378  current fade divisor    LegacyWorldStoryVmState::current_stream_fade_divisor
```

固定顺序：

1. 比较完整mode是否恰好等于2；相等时不读取current divisor，不推进；
2. mode不等于2才读取完整current divisor；非零时不推进；
3. mode不等于2且current divisor为0时`IP += 2`。

pending divisor `0x004B74F0`不读不写。handler不调用stream backend，也不修改任何音乐状态。

## 2. common join

函数入口`ESI=0`且首轮carry为0。三路都不设置`ESI=1`，均进入common join：

```text
publish previous192
service audio once
return/yield
```

因此即使完成路径已经推进2，也不会在同一次调用读取后继。精确尾`IP=0x7FFE`先推进到`0x8000`，发布previous并audio-yield；不产生successor fetch错误。

mode比较是完整u32 equality：例如`0x00010002`不等于2，在current divisor为0时会推进。current divisor只测试完整u32是否为零。

## 3. 资产与验证

完整线性TALK目录中opcode192为0条，使用`asset_absence_verified`。全文件双字节候选：

```text
00C0=23  40C0=0  80C0=0  C0C0=1
```

候选均无entry证明，不冒充真实record。

synthetic覆盖四raw alias、mode2短路、mode1/current非零等待、mode1/current零完成、完整u32 mode非相等、三项音乐状态保持、previous在audio前发布、三路恰好一次audio、完成仍yield不fetch后继及两字节精确尾。

LST→C++：mode短路、current gate、唯一IP写点、共同previous/audio/yield均逐块映射。

C++→LST：没有读取pending、调用backend、修改音乐状态、把mode截成低word、完成same-call或省略audio。零平台差异，归`assembly_exact`。

Story VM synthetic/real/initial-session 3/3、SDL app编译、Linux core 186/186与app 192/192完整门全部通过。workpack双生成稳定hash为`af1adc822dd7e246382ef33b72d0017ffbb5e501758d4bd4d8653d576dc8fe89`。接入192后，opcode134 synthetic的固定unsupported后继由数值192改为既有`OP_1025`，其134行为断言不变。未启动原版或OpenSWD3游戏EXE。
