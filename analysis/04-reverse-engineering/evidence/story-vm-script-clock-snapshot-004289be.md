# 剧情 VM 脚本时钟快照 `0x004289BE`

## 结论

`sub_427920` 主分派 opcode 37 的唯一入口是 `0x004289BE`。物理记录只有两字节raw opcode，没有operand：

```text
script_clock_origin = script_clock
instruction_offset += 2
previous_opcode = 37
goto same-call fetch
```

复制宽度为完整32位。handler不修改script clock本身、不重置21帧divider、不service audio、不调用callback，也不yield。

## 唯一汇编边界

完整opcode-specific本体为`0x004289BE..0x004289DD`，下一独立handler opcode38位于`0x004289DE`：

```text
004289BE  mov eax,dword_4ACDB0
004289C3  add ebx,2
004289C6  mov dword_4BA42C,eax
004289CB  add word ptr [ebp+0],2
004289D0  mov [esp+var_50],ebx
004289D4  mov esi,1
004289D9  jmp loc_42B0AE
```

`mov eax`与`mov dword`证明源和目标都是32位。没有`mov ax`、mask、范围判断或饱和处理。`ESI=1`送入common join，后者发布effective opcode并继续同一次解释器调用。

## 状态owner

```text
LegacyWorldStoryVmState::script_clock        -> dword_4ACDB0
LegacyWorldStoryVmState::script_clock_origin -> dword_4BA42C
```

opcode37只写origin。source clock与`script_clock_frame_counter`保持原值。刚保存的origin被opcode36的`origin + u16 delta`阈值使用，也与PATH runtime共享同一个typed owner；不得复制出第二份快照状态。

## 长度与窗口尾

主循环抓取raw opcode时已要求当前IP至少有两个字节。opcode37没有额外operand，因此handler内不得再要求后续字节。

当IP=`0x7FFE`且窗口恰好只剩opcode本身时，机器仍完成snapshot、IP+2与previous发布，然后同调用下一次fetch才以IP=`0x8000`停止。现代checked边界必须保留这个副作用顺序，不能把“下一条不存在”前移到snapshot之前。

## 真实资产审计

锁定的线性记录与entry-probe inventory均未观察到opcode37：

```text
unique_physical_records = 0
entry_probe_instances   = 0
coverage                = not_seen_in_linear_prefix_probe
```

直接字节扫描仅得到非入口候选：

| raw word | TALK1 | TALK2 | TALK3 | TALK4 | 合计 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `0x0025` | 94 | 17 | 10 | 81 | 202 |
| `0x4025` | 0 | 0 | 0 | 0 | 0 |
| `0x8025` | 0 | 0 | 0 | 0 | 0 |
| `0xC025` | 0 | 0 | 0 | 0 | 0 |

202处双字节候选都没有被锁定CFG证明为opcode37入口。本组使用`asset_absence_verified`，不伪造real replay。

## 测试覆盖

synthetic测试覆盖：

- 四种raw alias；
- `0x89ABCDEF`完整32位snapshot，隔离低16位错误实现；
- script clock与frame counter均保持不变；
- IP+2、previous发布和同调用抓取下一条；
- IP=`0x7FFE`只剩两字节时仍先完成snapshot，再在下一fetch报告越界；
- 无audio service或其他端口副作用。

## 双向收敛与分类

LST→C++：完整32位copy、固定IP+2、`ESI=1`与common previous join均有一一映射。

C++→LST：case没有新增operand check、callback、yield、clock/frame-counter写入或平台分支。所有valid-domain行为都由typed内存owner直接表达。

本handler不需要平台替代，归类为`assembly_exact`：

```text
assembly_exact;unit_tested;asset_absence_verified;sdl_runtime_integrated
```
