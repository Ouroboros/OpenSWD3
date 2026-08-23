# 标准模式数据库清理 `0x0043D880`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与callback边界

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043D880..0x0043DA2E`，195行。direct code caller是`0x0043E770`尾跳转；`0x00444FC0`另以callback地址绑定。直接callee为尚未关闭F080一次和release owner的21个静态调用点。

本helper与已关闭D530共享全部typed owner。F080保留精确port边界，可同时改写forward head和adjustment head；release边界区分u32 token、forward node storage及15类固定database storage。清理顺序、条件、悬空storage内容与最终EAX均在helper内。

## 2. 动作常量与F080顺序

入口只写原字段：

- primary action ID=`0x232A`、base variant从D530的`0x3B`改为`0x39`。
- cleanup action ID=`0x232A`、base variant=3。

其他action字段保持。随后立即调用F080。F080真实语义会耗尽forward head：text index非`0xFFDC`的节点移入adjustment head，缺失节点释放token和node；D880必须观察F080返回后的head。UT分别覆盖port保留残留节点及port完全耗尽两种结果。

## 3. 可选token与inline/runtime records

F080之后依次处理两个外部heap token：仅非零时release并把owner写0。随后无条件清零两个inline `0xB0` records。

两个D530 runtime record各读取`+0xAC` token：仅非零时release并把四个token字节写0；record其余字节保持。UT锁定第一项非零、第二项零的条件分支。

## 4. F080后残留forward节点

若F080返回后forward head仍非空，D880逐节点：

1. 先把共享head推进到next。
2. 无条件release节点`+0xAC` token，包括0。
3. release节点storage。

typed `LegacyStandardModeForwardNode`统一表达forward与adjustment链所共享的text index、`+6/+8/+A`字段和release token；既有只读链API保持const next，只有真实owner写点显式转换。

UT留下两个节点，锁定token `0x33333333,0`与node storage交替release；另一路让F080耗尽，证明D880不会重复释放。

## 5. 15类固定storage顺序与EAX

D880随后无条件按LST顺序release：

1. 两个runtime records。
2. 四个1200项字段表：`+5E,+60,+2C,signed +A7`。
3. 四个0xF0 buffers。
4. 四个0x1B8 buffers。
5. 0x400 mirror表。

原指针owner不写0，形成悬空指针；typed arrays因此不清内容。UT预置table/buffer/mirror字节并确认release后保持。最终EAX只取mirror storage release结果，锁定为`-321`。最后独立的`word_4FC900`生命周期phase由2写为u16 1，不改DA30读取的dword `FCD20`。

## 6. 验证

定向UT锁定F080第一事件、可选heap/runtime token条件、inline全清、残留node双release、15 storage精确枚举顺序、动作字段保持、悬空storage内容、最终EAX和生命周期phase1。

`special_modes.legacy_initial_menu`定向测试通过。workpack连续两轮稳定为`45/227`，SHA256均为`f8c52a97a3ac329afa39acee09d78e2a1863d3f6b9eef2fa179a0637381d508e`；下一独立单元为`0x0043DA30`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
