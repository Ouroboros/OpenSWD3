# 战斗对象批量重置 `0x00451A20`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威LST函数为`0x00451A20..0x00451A8C`，共58行，无外部FUNCTION CHUNK。函数无显式参数、无条件分支之外的失败出口。

## 2. 固定调用前缀

入口保存ESI/EDI后严格执行：

1. 无参数调用`0x00478110`全局重置；
2. 向`0x004776F0`传固定对象`0x004B9F00`；
3. 传固定对象`0x004ACBA8`；
4. 传固定对象`0x004B8A00`；
5. 最后三次调用完成后一次性回收12字节参数。

三个固定对象调用均为cdecl单参数；旧caller让三个参数暂时连续留在栈上，但每个callee只消费自己的首参数。modern typed port保留调用顺序、token和完整返回snapshot，不暴露旧物理栈。

`0x004776F0`属于后续`audit_order=265`，当前只关闭本caller固定序列，不提前计入callee。`0x00478110`同样保留独立全局重置端口。

## 3. 固定表清零

LST设置`ECX=0x60`、`EAX=0`、`EDI=0x004ACF50`，以`rep stosd`从低地址到高地址清零96个dword，共384字节。

modern用`std::array<u32,0x60>`建模，并逐word正向清零；结果记录`table_dword_writes=96`。测试在入口填充`0xDEADBEEF`，并由首个actor回调证明actor循环开始前全表已清零。

## 4. 角色组B遍历

第一循环：

```text
base   = 0x00525508
stride = 0x2B28
end    = 0x0053AE48
count  = 8
```

每轮把当前token放入ECX调用`0x0047D350`，再加stride，以signed `jl`继续。固定域内恰好八次，不夹索引、不跳空槽。

## 5. 角色组A遍历

第二循环紧随组B：

```text
base   = 0x005029D0
stride = 0x2F34
end    = 0x005201D8
count  = 10
```

同样每轮调用`0x0047D350`。该callee属于后续`audit_order=337`，当前以typed actor reset port隔离。

modern复用角色静态生命周期中已锁定的组A/B基址、尺寸与数量，按低32位token算术生成完全相同的18个地址，顺序固定为B八项后A十项。

## 6. 返回EAX

函数不在循环后改写EAX；`pop edi`与`pop esi`不影响EAX。因此正常返回严格来自第十个组A对象的`0x0047D350`完整EAX。全局重置与三个固定对象返回只作snapshot，组B和前九个组A返回均被后续callee覆盖。

modern每轮保存最新actor reset返回，并最终原样发布最后一个组A返回bit pattern。

## 7. 双向追溯

- `0x00451A20..0x00451A45`：全局重置、三个固定token调用与统一栈回收；
- `0x00451A48..0x00451A54`：96个dword正向清零；
- `0x00451A56..0x00451A6E`：组B八槽固定stride遍历；
- `0x00451A70..0x00451A88`：组A十槽固定stride遍历；
- `0x00451A8A..0x00451A8C`：恢复寄存器并保留末次callee EAX返回。

C++到LST反向追溯覆盖58行完整函数、22次callee调用、96次表写、18个物理token和最终返回来源。

## 8. 验证与动态差分

定向测试覆盖：

- 全局reset恰好一次且最先；
- 三个固定对象token与顺序；
- 三个callee返回snapshot完整保留；
- 96个dword在actor循环前全部清零；
- 组B八个地址与`0x2B28`步长；
- 组A十个地址与`0x2F34`步长；
- 事件总数22，顺序为全局→固定三项→B八项→A十项；
- 最终EAX来自末个组A token对应callee返回；
- battle聚合目标零warning构建、普通定向与独立ASan定向均`1/1`通过。

当前没有原版三类reset callee、384字节表、18个角色对象和后续状态联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。本caller固定协调语义已由完整LST与端口snapshot闭环。
