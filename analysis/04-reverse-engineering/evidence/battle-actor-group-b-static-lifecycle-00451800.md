# 战斗角色组B静态生命周期注册 `0x00451800`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整范围与外部chunk

权威LST主体为`0x00451800..0x00451809`，并明确声明外部`FUNCTION CHUNK AT 0x00451830 SIZE 0x0C`。完整函数合并：

- 主体从地址标题到`endp`共13行；
- `0x00451830..0x0045183B`外部chunk共10行；
- 合计23行LST。

主体无条件跳到chunk，只有chunk中的`retn`结束函数。忽略chunk会漏掉退出注册和返回EAX。

## 2. 独立的组B静态入口

函数无显式参数、无普通CODE XREF；唯一DATA XREF为`.data:0x0049E05C`，紧邻但不同于组A的`.data:0x0049E058`。它是组B自己的CRT静态初始化表入口。

主体调用`0x00451810`组B构造包装器，然后无条件跳入外部chunk。构造callee EAX不用于分支；typed结果仅作诊断snapshot，最终由后续注册覆盖。

组B使用独立construction entry port，禁止把组A的基址、尺寸、数量或回调偷用于尚未审计的组B构造callee。

## 3. 外部chunk与退出目标

外部chunk严格执行：

1. 压入组B退出清理函数`0x00451840`；
2. 调用CRT `_atexit` `0x00487BA0`；
3. caller清理一个参数；
4. plain返回。

最后EAX为`_atexit`完整结果。typed registration port显式接收组B退出token `0x00451840`，与组A `0x004517E0`不混用。测试覆盖返回0和`0xFFFFFFFF`。

注册失败不回滚已完成的组B构造；调用顺序固定为构造一次、注册一次。

## 4. 与相邻工作包的边界

- 本项只关闭组B静态协调器；
- `0x00451810`内的组B向量构造参数属于`audit_order=47`；
- `0x00451840`内的组B向量析构参数属于下一相邻工作包；
- 当前construction entry保持opaque，下一项关闭后必须改为typed helper直连；
- CRT注册端口已typed标识退出目标，但平台注册机制仍隔离。

## 5. 双向追溯

- `0x00451800..0x00451804`：调用组B构造包装器；
- `0x00451805..0x00451809`：无条件跳到外部chunk；
- `0x00451830..0x00451834`：压入组B退出清理函数地址；
- `0x00451835..0x0045183A`：调用`_atexit`并由caller回收参数；
- `0x0045183B`：保留`_atexit` EAX并返回。

C++到LST反向追溯覆盖23行完整函数、主体callee、外部chunk、独立退出token与CRT结果。

## 6. 验证与动态差分

定向测试覆盖：

- 组B构造事件严格先于组B退出注册；
- 两个入口各调用一次；
- 构造返回snapshot不成为最终返回；
- 注册收到精确组B退出token；
- `_atexit`返回0与全1均原样返回；
- 组A构造/析构及其独立退出token未回归。

battle聚合目标零warning构建及定向测试通过。

当前没有原版组B构造状态、CRT静态初始化表执行与退出注册结果联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。外部chunk已纳入完整静态闭环。
