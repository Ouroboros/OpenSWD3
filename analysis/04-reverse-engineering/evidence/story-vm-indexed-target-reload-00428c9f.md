# 剧情 VM 索引目标重载 `0x00428C9F`

## 结论

`sub_427920` 主分派 opcode 41 的唯一入口是 `0x00428C9F`。物理记录是以dword sentinel结束的变长目标表：

```text
+0                 u16 raw opcode
+2                 u32 target[0]
+6                 u32 target[1]
...                u32 target[count-1]
+2 + 4*count       u32 0xFF00FF00 sentinel
physical length    6 + 4*count
```

handler先读取完整32位`dword_4CAE7C` selector，再从`+2`开始逐个比较完整u32，直到遇到`0xFF00FF00`。原版scan没有窗口边界。

计数与选择保留一个明确原BUG：

- selector按无符号u32比较；
- `selector > target_count`时只向`nullsub_1`输出诊断，然后回退index 0；
- `selector <= target_count`时直接使用selector；
- 因此`selector == target_count`会把sentinel本身`0xFF00FF00`当成reload target，而不是回退或报错。

选定target后调用`sub_42E430(current_talk_context, target)`重载同一TALK文件。helper返回后，机器把`dword_4CAE7C`清零，设置`ESI=1`并进入common previous-opcode join；不顺序推进当前窗口IP，而是在新窗口offset 0同一次VM调用继续。

`dword_4CAE7C`不是VM私有副本。`sub_427300`世界交互choice链在`0x0042746F`把命中序号写入该全局；现有typed owner是`LegacyWorldInteractionState::selected_choice_index`。`LegacyWorldStoryVmRuntime::indexed_target_selector`只借用该字段，SDL绑定到同一个owner；opcode41完成后清零的正是世界交互与方向输入共享的selector。

## 唯一汇编边界

opcode-specific入口为`0x00428C9F..0x00428D13`；下一独立共享handler opcodes42/43位于`0x00428D18`。

selector读取、sentinel scan及越界回退：

```text
00428C9F  mov eax,dword_4CAE7C
00428CA4  xor edx,edx               ; target_count = 0
00428CA6  lea ecx,[window+2]
00428CAD  cmp dword ptr [ecx],0FF00FF00h
00428CB3  jz loc_428CC5
00428CB5  inc edx
00428CB6  add ecx,4
00428CBD  cmp dword ptr [ecx],0FF00FF00h
00428CC3  jnz loc_428CB5
00428CC5  cmp eax,edx
00428CC7  jbe loc_428CE8            ; equality is accepted
00428CC9  push eax                  ; pure diagnostic only
00428CCA  xor eax,eax
...       nullsub_1(opcode, requested selector)
00428CE6  xor eax,eax               ; fallback index 0
```

目标读取、reload及caller尾：

```text
00428CE8  mov ecx,[window+eax*4+2]
00428CEC  push ecx                  ; selected u32 target
00428CED  push offset talk_context
00428CF2  call sub_42E430
00428CF7  mov ebx,dword_4B8860      ; reload window base
00428D04  mov dword_4CAE7C,0
00428D0E  mov esi,1
00428D13  jmp loc_42B0AE
```

`loc_42B0AE`随后在`0x0042B0BD`发布effective opcode到`dword_4CF6D8`，并从reload后的window offset 0抓取下一条。

## `sub_42E430`调用合同

独立复核`0x0042E430..0x0042E47B`：

```text
0042E431  call AIL_serve
0042E436  mov ecx,[arg_0]           ; talk context
0042E43A  mov eax,[arg_4]           ; target
0042E43E  mov [ecx+14h],eax         ; publish talk data offset
0042E441  add eax,200h              ; physical TALK header bias
0042E446  mov word ptr [ecx+20h],0  ; new-window IP = 0
0042E44C  push eax
0042E452  call sub_4384B0            ; seek
0042E457  mov ecx,dword_4B8860      ; destination window
0042E468  mov size,8000h
0042E470  call sub_438380            ; read
0042E475  mov eax,1
0042E47B  retn
```

modern复用`load_same_file_story_window`：先`service_audio`，发布target/IP0，再用当前file number读取0x8000-byte typed window。成功后继续解释；typed I/O failure保留audio、target与IP0副作用，标记window未加载，然后caller仍按机器helper-return顺序清selector、发布previous，最后返回`load_failed`。

## Unsafe点与平台适配

原版`dword_4CAE7C`是始终存在的进程全局。modern runtime pointer缺失时在该首次全局读取点返回`runtime_unavailable`，不扫描脚本、不服务audio、不修改selector/IP/previous。

原版sentinel scan无边界。modern逐个dword检查；若完整`0xFF00FF00`未在当前0x8000-byte窗口内出现，则在原始下一dword访问点返回`operand_out_of_range`。此前只有无副作用的selector读取和目标计数，因此selector、IP、previous及audio均保持。完整表可恰好结束在window末尾，不要求后续字节，因为handler必然reload。

原版超界selector的格式化输出只进入`nullsub_1`，无业务consumer；modern保留index-0回退而省略纯诊断。selector和target均保持完整u32，不能截成u16。

同文件typed loader新增可报告I/O失败，且借用世界交互owner取代裸全局；因此本handler分类为`platform_adapted`。

## 真实资产审计

锁定inventory观察到：

```text
unique_physical_records = 64
entry_probe_instances   = 65
raw opcode               = 0x0029 (64/64)
encoding class           = dword_target_list_ff00ff00 (64/64)
```

文件分布：

| 文件 | 记录数 |
| --- | ---: |
| `TALK1.DAT` | 19 |
| `TALK2.DAT` | 20 |
| `TALK3.DAT` | 8 |
| `TALK4.DAT` | 17 |

目标数/物理长度分布：

| target count | physical length | 记录数 |
| ---: | ---: | ---: |
| 2 | 14 | 50 |
| 3 | 18 | 4 |
| 4 | 22 | 4 |
| 5 | 26 | 2 |
| 6 | 30 | 3 |
| 10 | 46 | 1 |

64条记录的166个target共有134种，范围`0x00004100..0x00058438`；所有记录均在声明长度末尾包含完整`0xFF00FF00` sentinel。

真实回放使用`TALK1.DAT@0x000042E6`，该记录有5个target并有2个entry probes：

```text
29 00
00 41 00 00
18 41 00 00
24 41 00 00
0C 41 00 00
30 41 00 00
00 FF 00 FF
```

selector 3精确选择`0x0000410C`，调用同文件reload，清selector并在新窗口offset 0继续。

四文件raw `0x0029`字节候选分别为`115/85/58/99`，合计357；`0x4029/0x8029/0xC029`均为0。只有inventory证明的64条作为指令记录，不把其他byte-word候选扩张为入口。

## 测试覆盖

synthetic与real测试覆盖：

- 四种raw opcode alias；
- target table正常index选择、同文件number、audio、target/IP0发布、selector清零、previous发布及same-call continuation；
- 完整u32大selector的无符号`>`比较与index-0回退；
- `selector == target_count`选择`0xFF00FF00` sentinel的原BUG；
- typed load failure后的audio/target/IP0/window状态、selector清零及previous顺序；
- 缺少共享selector owner时在首次全局读取点checked stop；
- 有target但缺sentinel的窗口截断，以及target+sentinel恰好结束于`0x8000`边界；
- SDL runtime指针绑定`LegacyWorldInteractionState::selected_choice_index`；
- `TALK1.DAT@0x000042E6`真实26字节记录以selector 3回放。

## 双向收敛与分类

LST→C++：共享u32 selector、完整dword sentinel scan、unsigned `>` fallback、equality sentinel bug、indexed u32 target、`sub_42E430`顺序、selector reset、无顺序IP推进与common previous均一一映射。

C++→LST：新增runtime指针只借用已经由`sub_427300`证据锁定的interaction owner；没有新增VM私有selector、顺序advance、范围clamp、sentinel拒绝、业务诊断、额外callback或yield。checked table/runtime/I/O失败仅隔离原版裸全局、越界scan与不可报告I/O域。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
