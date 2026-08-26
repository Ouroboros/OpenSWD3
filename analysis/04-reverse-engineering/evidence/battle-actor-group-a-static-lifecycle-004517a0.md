# 战斗角色组A静态生命周期注册 `0x004517A0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整范围与外部chunk

权威LST主体为`0x004517A0..0x004517A9`，并明确声明外部`FUNCTION CHUNK AT 0x004517D0 SIZE 0x0C`。完整函数必须合并：

- 主体从地址标题到`endp`共13行；
- `0x004517D0..0x004517DB`外部chunk共10行；
- 合计23行LST，控制流从主体无条件跳到chunk后才返回。

只读取主体会漏掉退出清理注册和最终EAX来源，不能关闭本函数。

## 2. 入口与静态初始化身份

函数无显式参数、无栈帧、没有普通CODE XREF；唯一DATA XREF为`.data:0x0049E058`。它是CRT静态初始化表入口，而不是逐帧战斗函数。

主体唯一动作是调用`0x004517B0`组A构造包装器，然后无条件跳转外部chunk。构造callee返回值不被读取或传播。

## 3. 外部chunk与返回值

外部chunk执行：

1. 压入退出清理函数`0x004517E0`；
2. 调用CRT `_atexit`入口`0x00487BA0`；
3. caller清理一个栈参数；
4. plain `retn`。

最后一次写EAX的是`_atexit`，因此函数返回其完整32位结果。modern端口以`register_exit_cleanup()`表达平台CRT注册边界，并原样保留返回bit pattern；测试覆盖0和`0xFFFFFFFF`，不把非零改写成布尔值。

调用顺序严格为构造一次、退出注册一次。注册失败不回滚已完成构造，也没有额外清理。

## 4. 与相邻工作包的边界

本项只关闭静态初始化协调器：

- `0x004517B0`内的组A基址、元素尺寸、数量及构造/析构回调参数属于`audit_order=44`；
- `0x004517E0`内的向量析构调用属于`audit_order=45`；
- 本函数只证明先调用前者，再把后者的函数地址注册给CRT。

因此本项以typed lifecycle port保留两个尚未关闭callee边界；后续callee关闭后必须回收opaque端口。

## 5. 双向追溯

- `0x004517A0..0x004517A4`：调用组A构造包装器；
- `0x004517A5..0x004517A9`：无条件跳到外部chunk；
- `0x004517D0..0x004517D4`：压入退出清理函数地址；
- `0x004517D5..0x004517DA`：调用`_atexit`并由caller回收参数；
- `0x004517DB`：原样保留`_atexit` EAX并返回。

C++到LST反向追溯覆盖23行完整函数、唯一主体callee、外部chunk及CRT边界。

## 6. 验证与动态差分

定向测试覆盖：

- construct事件严格先于registration事件；
- 两个入口各调用一次；
- `_atexit`返回0原样返回；
- `_atexit`返回全1 bit pattern原样返回；
- 结果显式记录一次构造和一次注册。

battle聚合目标零warning构建及定向测试通过。

当前没有原版CRT静态初始化表执行与`_atexit`注册结果联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。外部chunk已纳入完整静态闭环。
