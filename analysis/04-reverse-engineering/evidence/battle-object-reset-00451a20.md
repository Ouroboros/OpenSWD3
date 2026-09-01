# 战斗对象批量重置 `0x00451A20`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`closed_callee_integrated`。

## 1. 完整LST范围

权威LST函数为`0x00451A20..0x00451A8C`，共58个物理行，无外部`FUNCTION CHUNK`。函数无显式参数，除两个固定计数循环外没有条件失败出口。

## 2. 固定调用前缀

入口保存ESI和EDI后严格执行：

1. 无参数调用`0x00478110`清理三条共享链；
2. 以`0x004B9F00`调用已关闭的五字清零helper；
3. 以`0x004ACBA8`调用同一helper；
4. 以`0x004B8A00`调用同一helper；
5. 三次调用完成后一次性回收12字节参数。

三个固定对象均由`LegacyBattleFixedObjectStatePort`中的唯一typed state持有。caller直接组合`reset_legacy_battle_fixed_object`，旧`LegacyBattleFixedObjectResetPort`及其生产opaque调用已删除。每次helper固定返回EAX零、ECX为本次对象token，并保留全局链清理reply中的EDX。

`0x00478110`仍属于独立全局链生命周期边界；caller端口现在返回完整EAX、ECX、EDX，使其EDX可被三个已关闭helper和后续角色重置准确线程。

## 3. 固定表清零

LST设置`ECX=0x60`、`EAX=0`、`EDI=0x004ACF50`，以`rep stosd`从低地址到高地址清零96个dword，共384字节。

modern用`std::array<u32,0x60>`建模并逐word正向清零。完成后显式令EAX和ECX为0，同时保留EDX；首个角色回调验证三个固定header和整张表均已清零。

## 4. 角色组B遍历

第一循环：

```text
base   = 0x00525508
stride = 0x2B28
end    = 0x0053AE48
count  = 8
```

每轮把当前token放入ECX调用`0x0047D350`，再增加stride，以signed `jl`继续。固定域内恰好八次，不夹索引、不跳空槽。

## 5. 角色组A遍历

第二循环紧随组B：

```text
base   = 0x005029D0
stride = 0x2F34
end    = 0x005201D8
count  = 10
```

同样每轮调用`0x0047D350`。该callee属于后续工作包，当前以typed actor reset port隔离。modern复用角色静态生命周期中已锁定的组A/B基址、尺寸与数量，按低32位token算术生成完全相同的18个地址，顺序固定为B八项后A十项。

每个actor request在调用前以当前token覆盖ECX，EAX和EDX继承前次callee reply。循环增量、比较和跳转不修改这三个寄存器；最终三寄存器结果均来自第十个组A对象的callee reply。

## 6. 双向追溯

- `0x00451A20..0x00451A45`：全局链清理、三个已关闭typed helper及统一栈回收；
- `0x00451A48..0x00451A54`：96个dword正向清零，结束时EAX/ECX为零；
- `0x00451A56..0x00451A6E`：组B八槽固定stride遍历；
- `0x00451A70..0x00451A88`：组A十槽固定stride遍历；
- `0x00451A8A..0x00451A8C`：恢复寄存器并保留末次callee的EAX/ECX/EDX。

C++到LST反向追溯覆盖58行完整函数、22次原callee调用、三个五字header、96次表写、18个物理actor token和最终三寄存器来源。

## 7. 验证与动态差分

定向测试覆盖：

- 全局链清理恰好一次且最先；
- 三个固定对象token、顺序、五字清零与共享owner；
- 三次helper均返回EAX零、当前token ECX及不变EDX；
- 96个dword在actor循环前全部清零；
- 组B八个地址与`0x2B28`步长；
- 组A十个地址与`0x2F34`步长；
- 首个actor入口EAX为零且EDX来自全局链清理；
- 18个actor reply的EAX和EDX逐次线程，ECX在每次调用前被当前token覆盖；
- 最终EAX、ECX和EDX来自末个组A对象callee。

当前没有原版全局链清理、384字节表、18个角色对象和后续角色callee的联合捕获后端，caller整体`original_diff_verified`仍为`blocked_runtime_oracle`。本次已关闭五字helper不依赖该动态oracle。
