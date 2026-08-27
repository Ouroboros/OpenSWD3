# 战斗逐帧双方完成数协调 `0x0045EC80`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 范围与调用图

权威LST完整主体为`0x0045EC80..0x0045EDEF`，从`proc`到`endp`共174行、108条实际指令、2个call、16个跳转、7个局部标签，没有外部`FUNCTION CHUNK`。

函数无参数，唯一caller是已关闭逐帧画面协调器`0x00453200`。两个静态call都指向尚未关闭的角色mask链查询`0x0047E580`，固定传mask 4；typed端口完整传递角色token、索引、组别及EAX/ECX/EDX，允许callee修改后续读取的共享数量和门。

## 2. 入口与组A扫描

入口读取当前角色完整dword；不是全1时立即走公共零返回，EAX清零而ECX/EDX保持caller快照。只有全1才继续。

组A数量完整u32为零时跳过。非零时按unsigned动态上界从索引0扫描，角色token为`0x005029D0 + index*0x2F34`。每个角色先直接读取对象内偏移`0x2B00`和`0x2B04`两个dword；任一完整值精确等于1都跳过mask查询。两项都非1时才调用查询，只有完整EAX精确等于1才把u8本地计数加1回绕，随后重读live组A数量。跳过查询的轮次不重读数量。

十组对象内字段由唯一frame-completion state持有。第11次真实字段读取才typed-stop，保留前十轮跳过/查询副作用和当前寄存器；对象token只作u32运算，不转宿主指针。

## 3. 组A阈值与发布

组A ready计数非零时，严格按byte宽度计算：

```text
required = u8(group_a_count.low - action_phase.byte2 - excluded_group_a.low)
available = zero_extend(removed_group_a) + zero_extend(ready_count)
```

比较是两个非负完整dword的signed `available >= required`。满足后再读取共享暗化门；门必须完整为0才提交：

1. removed-group-A u8加ready计数并回绕；
2. 共享message写`0x67`；
3. 返回EAX 1、ECX available、EDX ready计数。

数量、phase、排除值、removed值、暗化门和message全部复用既有唯一typed owner。

## 4. 组B回退与发布

组A未提交时先读取组B完成门；非零立即公共零返回。门为零后，组B数量完整u32为零也返回0。

非零时从`0x00525508`按`0x2B28`步长扫描，每轮无条件调用mask查询并在callee后重读live组B数量，以unsigned比较决定继续；不增加现代上限。只有查询完整EAX精确等于1才把u8 ready计数加1回绕。

ready为零时只用该byte替换最终callee ECX的CL并返回0，EDX保持最终callee值。ready非零时：

```text
available = zero_extend(packed_actor_counter.low) + zero_extend(ready_count)
```

`available`与live组B完整数量作signed比较；满足后仍要求共享暗化门完整为0。成功提交顺序为：

1. packed actor counter只替换低byte为原低byte加ready计数的u8回绕值；
2. 组B完成门写1；
3. terminal mode写0；
4. message写`0x63`；
5. 返回EAX 1。

成功尾ECX保留最终callee高24位、只把CL替换为ready计数；EDX为未回绕的低byte计数和。公共失败尾只清EAX，保留当时ECX/EDX。

## 5. caller回收与验证

逐帧caller在角色帧顺序之后直接组合本函数，旧opaque槽只保留reserved枚举值且不再调用。由于前一已关闭角色帧链尚未完整暴露嵌套callee的ECX/EDX，caller request显式携带post-actor-frame寄存器snapshot；本函数正常尾EDX再原样成为后续待执行动作提交的caller快照。子typed-stop阻断待执行动作、效果协调与全部后续绘制。

定向测试覆盖当前角色入口门、组A双字段精确1跳过、动态数量缩短/增长、查询精确1、u8 ready/removed回绕、组A阈值和message 103、第11次字段访问、组B双角色token、最终callee ECX高字、packed低byte回绕、signed完整数量、message 99、零ready寄存器、动态数量增长、暗化门、组B完成门、逐帧caller直连、旧opaque槽清零及子typed-stop阻断待执行动作与后续绘制。

当前缺少原版两组角色对象、mask链、动态数量/门轨迹、前一角色帧嵌套寄存器及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
