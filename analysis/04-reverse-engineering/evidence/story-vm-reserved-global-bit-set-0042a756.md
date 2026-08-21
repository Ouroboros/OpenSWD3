# 剧情 VM 保留全局位置位 `0x0042A756`

状态：`platform_adapted`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A756..0x0042A78D`；bit helper `0x0040DC80..0x0040DCA2`；公共退出 `0x0042D182..0x0042D193`

opcode：`92`

## 1. Handler顺序与位宽

机器块严格执行：

```text
selector_minus_one = zero_extend_u16(+2) - 1     // u32 wrap
if unsigned(selector_minus_one) > 3:
    nullsub_1(debug arguments)                    // 不改变业务状态
bit_index = selector_minus_one + 30               // u32 wrap
sub_40DC80(bit_index)
IP += 4
ESI = 1
common join publishes previous92 and continues same call
```

因此1/2/3/4分别映射bit30/31/32/33；selector0先`dec`到`FFFFFFFF`、再`+30`回绕到bit29。selector5及更大值即使经过invalid诊断也继续写入，不是skip或typed-invalid-selector。

## 2. `sub_40DC80`与unsafe点

helper将完整32位bit index复制到ECX，以`EAX & 7`选择`byte_4994EC` mask，并对ECX执行signed `sar 3`后访问`byte_4AB384[ECX]`，再OR写回。由u16 selector生成的bit index范围为29..65564，均非负，所以modern用`bit_index >> 3`与原sar等价。

modern的owned flag数组为`0x400` bytes，即bit0..8191。机器合法owned域为selector0以及1..8162；8162写最后一字节bit8191。selector8163起访问owner之外；modern在该原始数组访问点返回`global_bit_index_out_of_range`，不写flag、不推进IP、不发布previous。不能复用现有u16 flag helper处理越界前的值，否则selector65535的bit65564会错误截断为bit28。

完整四字节记录恰好结束在`0x8000`时，bit写入、IP=`0x8000`和previous92先完成，下一fetch才返回越界。handler无audio、callback或yield；正常路径same-call继续。

## 3. 资产锁与测试

线性TALK目录中opcode92为0条物理记录/0 probes，使用`asset_absence_verified`，不伪造real replay。四raw word的全文件双字节候选共102处，但均不是线性指令入口：

```text
TALK1: 005C=13, 405C=0, 805C=0, C05C=0
TALK2: 005C=39, 405C=0, 805C=0, C05C=0
TALK3: 005C=41, 405C=1, 805C=0, C05C=0
TALK4: 005C=8,  405C=0, 805C=0, C05C=0
```

synthetic覆盖四raw alias精确尾、selector0/1/4/5、最后owned selector8162、首个越界8163、FFFF、operand截断、flag隔离、IP/previous和same-call。Linux Story VM三项为3/3。

分类：`platform_adapted`。owned域的u16零扩展、u32 dec/add回绕、invalid-safe继续写、mask/byte计算、+4、previous和same-call均保持；原版owner外裸写以原访问点typed-stop隔离，debug-only `nullsub_1`不建模。
