# 剧情 VM common join same-call latch `0x0042D200`

状态：`platform_adapted`、`unit_tested`、`asset_absence_verified`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042D200..0x0042D218`

opcode：1024

## 1. 调用期状态

解释器每次进入`sub_427920`后在`0x00427B32`把局部`var_28`初始化为0。每次fetch又在`0x00427B59`把`ESI`清零，因此两种continue来源不同：

```text
var_28 = 调用期latch；一旦置1，后续fetch不清除
ESI    = 当前handler的一次性same-call来源；下一fetch清零
```

opcode1024固定执行：

```text
physical instruction pointer += 2
u16 IP += 2
var_28 = 1
保存physical instruction pointer
进入0x0042B0AE common join
```

common join先发布normalized previous1024；`var_28|ESI`非零，故不调用`_AIL_serve`并在同一次解释器调用中继续fetch。1024没有operand、helper、全局状态或跨调用持久状态。

`var_28`只在后续opcode1025入口`0x0042D49F`被清零，或随本次`sub_427920`返回销毁。1025的独立闭环见`story-vm-common-join-latch-clear-0042d49f.md`。

## 2. 后续common join

latch不是“只让1024自身same-call一次”。置1后，后续所有到达`0x0042B0AA/0x0042B0AE`的handler都必须：

1. 先发布该handler的normalized previous；
2. 跳过共同出口`0x0042D4D7 _AIL_serve`；
3. 同调用继续fetch；
4. 保持`var_28=1`供再后续共同出口使用。

现代把调用期bool放在`step_legacy_world_story_vm`栈内，并把全部64个common-join调用点统一接入同一窄helper。helper只在latch为0时执行previous→audio→yield；latch为1时只发布previous并返回same-call。switch内剩余两个裸yield均不经过共同join：入口对齐门，以及opcode96映射`0x0042D4B6`的CD preflight直返。

handler内部audio不受latch影响。例如opcode135在`0x0042C3ED`的内部audio仍执行，只有previous之后的common audio被抑制。opcodes186/187 taken loader的内部audio同理保留。

## 3. 非推进无限域与现代边界

若latch后遇到原地等待或默认非法handler，机器会在同一次调用中无限重复该handler：previous持续发布，common audio持续被抑制。现代沿用解释循环既有4096条dispatch guard，以`unsupported_opcode` typed-stop隔离该非终止域；不伪造IP推进、audio或跨帧yield。因此本handler归`platform_adapted`，合法推进链保持汇编行为。

## 4. 资产与测试

完整线性TALK目录对opcode1024为0条记录/0 probes，使用`asset_absence_verified`。全文件raw字样仅作反证导航：

```text
          0400  4400  8400  C400
TALK1     2369   147    50    37
TALK2     1244    65    55    81
TALK3     1219   100    22    22
TALK4     1323    71    16    31
```

这些字样均不位于已证明的线性记录入口。

synthetic覆盖：

- 四个raw alias；
- 1024→opcode59→测试专用负索引typed-stop，验证IP/previous、sound副作用和零audio；
- 两个连续opcode59共同出口，验证latch不是一次性ESI；
- 下一次`step`恢复普通common audio，验证latch不跨调用泄漏；
- 1024→opcode135，验证内部audio保留而common audio被抑制；
- 1024→非推进opcode53，验证原地same-call直到既有dispatch guard；
- `IP=0x7FFE`精确尾先提交IP=`0x8000`和previous1024，再由后继fetch返回`instruction_out_of_range`。

Story VM synthetic/real/initial-session 3/3和SDL app编译通过。1024闭环时workpack/runtime-path双生成稳定hash分别为`856190c62941e0c0af81d89381357dda47133ef4d131abb8f42aa1ad7d9d7f98`与`244f0d55c0ccd038e9391aba2d394161673d27b002864e4a08edf858f34daf3f`。Linux完整门通过：core 186/186、app 192/192。未启动原版或OpenSWD3游戏EXE。

## 5. 双向追溯

LST→C++：`0x00427B32`初始化、`0x00427B59`每fetch清ESI、`0x0042D200`双指针+2、`0x0042D208`置latch、`0x0042B0AE`发布previous/OR/分流均有直接映射。

C++→LST：没有把latch存入VM持久状态，没有把它退化成一次性continue，没有抑制handler内部audio，也没有让入口对齐门或opcode96 preflight经过common join。1025清除语义保持独立handler与证据。
