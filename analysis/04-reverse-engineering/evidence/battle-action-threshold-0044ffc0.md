# 战斗行动计时阈值发布 `0x0044FFC0`

状态：`assembly_exact`、`unit_tested`、`fixed_state_tested`。

## 1. 范围与ABI

权威LST范围为`0x0044FFC0..0x0044FFD9`，入口`proc`至`endp`共15行，没有外部`FUNCTION CHUNK`，没有callee。

ABI为cdecl单参数：

```text
arg0 = 32位战斗速度设置
EAX  = 发布后的行动计时阈值
```

函数把同一结果写入旧全局`0x004A74CC`，随后直接`retn`，由caller清理参数。

唯一caller是战斗初始化入口`0x00451B10`内的`0x00451B2E`。caller从战斗设置全局取得参数，调用后立即改写ECX，不消费EAX；发布的全局状态供后续战斗行动逻辑读取。

## 2. 精确计算

指令链为：

```text
value = 20 - speed_setting
value = value + value * 4
value = value + value * 4
value = value << 2
```

即模`2^32`意义下的：

```text
(20 - speed_setting) * 100
```

所有步骤都是32位寄存器算术，没有有符号溢出检查、夹值或非法设置早退。typed实现用u32执行相同的减法、两次乘5和左移，再以bit pattern发布为i32，避免C++有符号溢出未定义行为。

旧数据初值`0x00000384`即900，对应设置11。设置20发布0，设置21发布-100；输入`INT_MIN`和`INT_MAX`分别按低32位发布2000和2100。

## 3. 下游消费

旧全局共有四类读取：

- `0x0046E56B`：把对象u16计时值与阈值比较；
- `0x0047560E`：另一行动路径执行同类比较；
- `0x00478352`：以signed dword阈值参与x87整数除法；
- `0x00478370`：把阈值低16位写入对象`+0x2A12`；
- `0x0047DB06`：把对象u16计时值与阈值比较。

这里共有五处读取指令、四类消费方式。它们尚未进入现代实现；关闭各caller时必须直接读取`LegacyBattleTimingState::action_threshold`，不得重新推导、夹值或改成无符号合同。

## 4. 双向追溯

LST到C++：

- `mov eax, 20`与`sub eax, ecx`对应u32回绕减法；
- 两次`lea eax, [eax+eax*4]`对应两次乘5；
- `shl eax, 2`对应乘4；
- 全局写对应typed状态发布；
- EAX残值对应typed函数返回值。

C++到LST：

- typed状态只有一个行动阈值字段；
- 函数没有额外门、分支、callee、除法或资源操作；
- 默认900来自原数据字节；
- 所有实现语句均有唯一LST指令或旧数据来源。

完整正向与反向追溯未发现未解释指令、字段、出口或调用边。

## 5. 验证与动态差分

定向测试覆盖原默认、设置11、20、0、21以及`INT_MIN`、`INT_MAX`，同时锁定typed字段与返回值相同。battle聚合目标零warning构建及定向测试通过。

当前没有原版战斗速度设置、阈值全局与对象计时字段的联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整15行LST、typed实现和固定状态已经闭环。
