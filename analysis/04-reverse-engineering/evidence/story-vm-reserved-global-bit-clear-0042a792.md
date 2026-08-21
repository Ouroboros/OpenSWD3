# 剧情 VM 保留全局位清除 `0x0042A792`

状态：`platform_adapted`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042A792..0x0042A7C9`；bit helper `0x0040DCB0..0x0040DCD9`；公共退出 `0x0042D182..0x0042D193`

opcode：`93`

## 1. Handler顺序与位宽

机器块独立执行：

```text
selector_minus_one = zero_extend_u16(+2) - 1     // u32 wrap
if unsigned(selector_minus_one) > 3:
    nullsub_1(debug arguments)                    // 不改变业务状态
bit_index = selector_minus_one + 30               // u32 wrap
sub_40DCB0(bit_index)
IP += 4
ESI = 1
common join publishes previous93 and continues same call
```

因此1/2/3/4分别清bit30/31/32/33；selector0经32位dec/add回绕后清bit29。selector5及更大值即使经过invalid诊断也继续清除，不是skip。

## 2. `sub_40DCB0`与unsafe点

helper将完整32位bit index复制到ECX，以`bit & 7`读取单bit mask；ECX执行signed `sar 3`后访问`byte_4AB384[ECX]`。它从`DL=FF`减去单bit mask，随后执行`byte &= DL`并写回，即只清选中bit、保留同byte其余七bit。由u16 selector生成的bit index为29..65564且非负，所以modern逻辑右移与原sar等价。

modern owner为`0x400` bytes（bit0..8191）。机器owned域为selector0及1..8162；8162清最后一字节bit8191。selector8163起访问owner外，modern在该原始read/write点返回`global_bit_index_out_of_range`，不改flag、不推进IP、不发布previous。不能把完整bit index送入u16 helper后再检查，否则selectorFFFF的bit65564会错误截断。

完整四字节记录恰好到`0x8000`时，clear、IP和previous先完成，下一fetch才失败。handler无audio、callback或yield；正常路径same-call继续。

## 3. 资产锁与测试

线性TALK目录中opcode93为0条物理记录/0 probes，使用`asset_absence_verified`。四raw word全文件双字节候选共41处，但均不是线性指令入口：

```text
TALK1: 005D=9,  405D=0, 805D=0, C05D=1
TALK2: 005D=2,  405D=0, 805D=0, C05D=2
TALK3: 005D=16, 405D=0, 805D=0, C05D=3
TALK4: 005D=5,  405D=0, 805D=0, C05D=3
```

synthetic独立覆盖四raw alias精确尾、selector0/1/4/5、最后owned selector8162、首个越界8163、FFFF、operand截断、只清单bit、IP/previous和same-call。Linux Story VM三项为3/3。

分类：`platform_adapted`。owned域的u16零扩展、u32 dec/add回绕、invalid-safe继续清、`FF-mask`、+4、previous和same-call均保持；原版owner外裸read/write以原访问点typed-stop隔离，debug-only `nullsub_1`不建模。
