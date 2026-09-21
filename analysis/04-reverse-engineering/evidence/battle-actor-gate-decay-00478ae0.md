# 战斗角色启动门与目标计数衰减（0x00478AE0）

## 1. 范围、边界与 ABI

权威 LST 把 `sub_478AE0` 锁定为 `0x00478AE0..0x00478B10`，半开区间为
`0x00478AE0..0x00478B11`。主体共 49 字节、13 条实际指令、0 个 callee、2 个条件分支、
2 个局部标签和 1 个普通 `ret`，没有外部 chunk 或中段入口。

```text
0x00478AE0  mov ax, [ecx+0x2A74]
0x00478AE7  xor edx, edx
0x00478AE9  cmp ax, dx
0x00478AEC  jz short loc_478AF6
0x00478AEE  dec eax
0x00478AEF  mov [ecx+0x2A74], ax
0x00478AF6  mov ax, [ecx+0x2A76]
0x00478AFD  cmp ax, dx
0x00478B00  jz short loc_478B0A
0x00478B02  dec eax
0x00478B03  mov [ecx+0x2A76], ax
0x00478B0A  mov [ecx+0x2AE0], edx
0x00478B10  ret
```

入口 ECX 是 actor token。函数没有显式参数。首字段读取成功后 EDX 被清零；EAX 高 16 位
始终保留入口值。普通返回时 RET 读取返回地址，ESP 相对入口增加 4，EIP 为物理返回地址。

## 2. 字段、算术、flags 与访问顺序

函数依次处理三个既有 canonical 字段：

```text
actor+0x2A74  start_gate
actor+0x2A76  target_selection_count
actor+0x2AE0  start_gate_latch
```

`start_gate` 与 `target_selection_count` 都先完整读取 word。原值为零时只保留 CMP 结果并跳过
对应写；原值非零时以完整 32 位 `dec eax` 递减，再把 AX 写回原 word。由于进入 DEC 时 AX
非零，EAX 高 16 位不会因借位改变，但 SF、ZF 与 OF 仍按完整 32 位结果计算。第二字段的
CMP/DEC 总会覆盖第一字段正常路径留下的算术 flags。

第二字段为零时，最终 flags 来自 `cmp ax,dx`：CF=0、ZF=1、SF=0、OF=0、AF=0、PF=1。
第二字段非零时，CF 保留该次 CMP 的 0；OF、SF、ZF、AF 与 PF 来自完整 32 位 DEC。
最后把已清零的 EDX 完整写到 `start_gate_latch`，MOV 与 RET 不修改 flags。

严格物理访问顺序为：

1. 读取 `actor+0x2A74` word；
2. 仅原值非零时写回 `actor+0x2A74` word；
3. 读取 `actor+0x2A76` word；
4. 仅原值非零时写回 `actor+0x2A76` word；
5. 写 `actor+0x2AE0` dword 0；
6. RET 从 `[ESP]` 读取返回地址。

现代实现保留六个独立 typed-stop 阶段：首字段读、首字段写、次字段读、次字段写、latch 写和
RET 读。每个停止点只保留此前已经提交的字段写、当前寄存器、flags、ESP 与物理 EIP。字段
原值为零时不存在对应写访问，不能因该字段不可写而提前停止。

## 3. canonical owner 与固定前一槽

Group-A 与 Group-B actor 继续复用既有 action-execution owner：

```text
Group-A  action.group_a_action_execution[index]
Group-B  startup.group_b_lifecycle[index].action_execution
```

`0x005229E0` 是 Group-B 数组基址前的固定槽，不是数组元素。其三个物理字段为：

```text
0x00525454  start_gate / packed dword low word
0x00525456  target_selection_count / packed dword high word
0x005254C0  start_gate_latch
```

现代实现只对精确 token `0x005229E0` 解析该固定槽。两个相邻 word 复用
`LegacyBattleGroupAFrameState::shared_value_525454` 的低、高半字，latch 复用唯一
`shared_value_5254c0`。低 word 写保留高 word，高 word 写保留低 word；其他越界 token 继续在
首次原字段读取点 typed-stop，不放宽 Group-B 数组范围。

## 4. 十二个已关闭父 CALL

完整 LST 只有十二个物理 CALL，全部位于已关闭父函数：

```text
0x00454A5D -> 0x00454A62  action dispatch case 22
0x00454B06 -> 0x00454B0B  action dispatch case 22
0x00456FA4 -> 0x00456FA9  Group-A frame fixed pre-Group-B
0x0045715A -> 0x0045715F  Group-A frame Group-B loop
0x004571AC -> 0x004571B1  Group-A frame selected side actor
0x0045720C -> 0x00457211  Group-A frame related Group-A actor
0x00457EEE -> 0x00457EF3  Group-B frame Group-A loop
0x00458002 -> 0x00458007  Group-B frame completed Group-A target
0x00458025 -> 0x0045802A  Group-B frame completed Group-B target
0x0045AEDF -> 0x0045AEE4  post-action queried Group-B actor
0x0045AF75 -> 0x0045AF7A  post-action queried Group-B actor
0x0045DC03 -> 0x0045DC08  debug Control+C retarget actor
```

十二处全部在原控制流位置直接组合 typed leaf。本轮没有延期 caller。trace 保留 CALL 地址、
返回地址、actor token、请求序号和 leaf 最终状态；typed-stop 立即阻断尚未执行的父级后缀。

### 4.1 Action dispatch：两处 case 22

两处 CALL 都以已选 Group-B 目标建立 token。入口 EAX 为 `1381*selected` 的地址算术残值，
EDX 为 `345*selected`，flags 来自末次 `sub eax,ecx`。成功后才继续 case 后缀；任一 leaf
停止都直接结束当前动作分支。

### 4.2 Group-A frame：四处 CALL

`0x00456FA4` 使用固定前一槽 token `0x005229E0`，并保留 `sub ecx,0x2B28` 的入口状态。
`0x0045715A` 位于 Group-B 全量循环。`0x004571AC` 按 action side 选择 Group-A 或 Group-B
actor。`0x0045720C` 处理关联 Group-A actor。

四处共享同一全局请求序号。若 `0x004571AC` 成功后继续到 `0x0045720C`，第二处必须消费下一
request，而不是从父 context 的旧 offset 重读首项。默认 active-actor selection-complete 路径实际
命中 `0x0045720C`；令 `0x00478B40` 返回 0 后才先到 `0x004571AC`。

### 4.3 Group-B frame：三处 CALL

`0x00457EEE` 位于 Group-A 全量循环。`0x00458002` 处理单个完成的 Group-A 目标，前序玩家
道具 helper 没有公开 flags，因此入口 flags 显式标为未知；leaf 的首个 CMP 后恢复确定 flags。
`0x00458025` 处理完成的 Group-B 目标，入口 EAX/EDX 与 flags 来自 Group-B token 地址算术。

嵌套 opponent-action dispatch 使用 `parent request offset + 已消费 calls` 建立独立 context，
防止嵌套调用从旧 offset 重读 gate-decay request。

### 4.4 Post-action 与 debug hotkeys

post-action 两处 CALL 都处理 queried Group-B actor，保留 `1381*index` 地址算术残值。成功后
才继续各自清理后缀。

调试 Control+C retarget 在 `0x0045DC03` 调用本 leaf。retarget 前缀已经把
`priority_actor_index` 清为 `0xFFFFFFFF`；leaf typed-stop 保留该前缀，但抑制后续 priority
重读、Group-A actor reset 与 action-block 后缀。

## 5. 双向追溯与测试范围

LST 到 C++ 已覆盖两个条件分支、两个 word 条件写、latch 完整 dword 清零、普通 RET、EAX
高字保留、EDX 清零、32 位 DEC flags、完整访问顺序和六阶段 partial commit。C++ 到 LST
反向追溯覆盖 canonical resolver、固定前一槽 packed owner、request offset、trace 和十二个物理
caller；没有无来源字段写或延期 caller。

汇编独立测试向量覆盖：

- Group-A、Group-B 与固定前一槽 resolver；
- packed `0x00030002 -> 0x00020001` 与 latch 清零；
- 三个固定物理字段 token；
- 两字段零/非零组合、条件写跳过与32位 DEC flags；
- EAX 高字、ECX、EDX、ESP、EIP 与 RET；
- 六个 typed-stop 阶段及逐阶段 partial commit；
- 十二组 CALL/return/actor 物理身份；
- action dispatch、Group-A frame、Group-B frame、post-action 与 debug hotkeys 的直接停止和后缀抑制；
- Group-A 连续 CALL 与 Group-B 嵌套 dispatch 的全局 request offset；
- production generic/raw `0x00478AE0` 调用归零。

## 6. 分类与动态差分状态

任意 32 位 actor token、三个字段页和 RET 栈页无法由现代 C++ 直接合法解引用。canonical token
resolver、固定前一槽精确解析和原访问点 typed-stop 是最小平台边界，因此本目标登记为
`platform_adapted`，不能以 owner、span 或测试通过标记为 `assembly_exact`。

当前缺少原版完整 Group-A/Group-B actor backing、三个字段异常页、RET 异常栈页，以及十二个
caller/callee 的联合寄存器、flags 与 SEH 捕获后端。原版动态差分登记为
`blocked_runtime_oracle`，不得写成 `original_diff_verified`。

## 7. 验证与关闭

最终定向 setup 测试静默通过；格式化后的 Linux core 为 `199/199`，Linux app 为
`205/205`，AddressSanitizer/UBSan 为 `199/199`。连续十轮 Linux core 均以 `199/199` 和
exit 0 完成。最终日志扫描没有 OpenSWD3 源码 warning、编译 error、测试失败、sanitizer
finding 或 runtime error。

三份新 leaf 文件全量 clang-format、旧文件 changed-range clang-format、`git diff --check`、
权限位、生产 raw/generic `0x00478AE0` 调用、十二个 caller 覆盖、TMP 分类和完整 release diff
审计均通过。未启动原版或 OpenSWD3 游戏程序。

LST 地址摘录 SHA-256 为
`d9ec9988fae8a606bceff6cd056a468aef7bf9f1e6f53d2e46d4c664ce6d2272`。inventory 生成器
连续双跑逐字节一致，关闭结果为
`310/422 = 300 platform_adapted + 10 assembly_exact + 112 pending_audit`，下一条为
`audit_order=311 / 0x00478B20 / sub_478B20`。最终 inventory SHA-256 为
`5d2cac8cfcd4a4f3475e611b3ebb7ad7ba9fa33ed352b3732676b56929d9c0b2`。
