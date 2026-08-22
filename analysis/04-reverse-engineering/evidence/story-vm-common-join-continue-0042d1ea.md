# 剧情 VM common join一次性续行 `0x0042D1EA`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042D1EA..0x0042D1FF`

opcode：1026

## 1. LST合同

表外special分派把1026归一化到`0x0042D1EA`。handler固定顺序为：

```text
0x0042D1EA  physical instruction pointer += 2
0x0042D1ED  logical u16 instruction offset += 2
0x0042D1F2  保存physical instruction pointer
0x0042D1F6  ESI = 1
0x0042D1FB  跳0x0042B0AE common join
```

common join先发布previous1026；`var_28|ESI`非零，因此不调用`_AIL_serve`并在同一次解释器调用中继续fetch。下一次fetch在`0x00427B59`重新清零ESI，所以该续行只对1026本身的common join生效，不像opcode1024那样形成调用期持久latch。1026也不清除已经存在的opcode1024 latch。

旧C++已有裸`case 1026U`并执行IP+2/same-call，但漏掉common join的previous1026发布。现代补齐previous并使用语义常量；不增加持久状态或audio。

## 2. 资产与测试

完整线性TALK目录含4141条唯一opcode1026记录、4150个entry probes，raw word全部为`0x0402`：

```text
TALK1.DAT  1244 records / 1245 probes
TALK2.DAT   749 records /  752 probes
TALK3.DAT  1005 records / 1007 probes
TALK4.DAT  1143 records / 1146 probes
```

九个多probe物理位置为TALK1 `0x00011FCD`，TALK2 `0x0003305E/0x00033078/0x000330D2`，TALK3 `0x0001D882/0x0001D884`，TALK4 `0x000364E4/0x000364FE/0x00036558`。真实资产测试读取四文件首尾和全部多probe样本；现有story transfer、map/item reload、global-bit branch与role/item reload真实链继续执行目标1026。

synthetic覆盖：

- 四个raw alias固定IP+2、previous1026、零audio并same-call测试typed-stop；
- 1026→opcode59证明下一fetch清ESI，opcode59恢复普通common audio-yield；
- 1024→1026→opcode59证明1026不清除既有持久latch；
- `IP=0x7FFE`精确尾先提交`0x8000`和previous1026，再由same-call后继fetch返回`instruction_out_of_range`。

补齐previous后，所有确实经过目标1026且后继未发布previous的既有合成/真实链均改为期望previous1026；顺序路、load failure和后继自行发布previous的路径不改。

Story VM synthetic/real/initial-session 3/3、Linux core 186/186与app 192/192通过。workpack/runtime-path双生成稳定hash分别为`556287b870175442a1f8e2738d8f7a71cc79ddc4e46a2c56c014bfc5de328ea8`与`7ab493544aebc7b82c75d98d58356c43847d23a1cca85427d654c54b2be2b5c7`。未启动原版或OpenSWD3游戏EXE。

## 3. 双向追溯

LST→C++：双指针+2、previous发布、一次性same-call和零audio均有直接映射；下一fetch自然重置一次性续行。

C++→LST：没有把1026建模为持久latch，没有清除opcode1024 latch，没有audio-yield，没有跳过previous，也没有把精确尾改成成功返回。
