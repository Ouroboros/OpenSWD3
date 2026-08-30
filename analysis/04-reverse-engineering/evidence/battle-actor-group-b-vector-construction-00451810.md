# 战斗角色组B向量构造包装器 `0x00451810`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x00451810..0x0045182B`，从`proc`到`endp`共19行，没有外部`FUNCTION CHUNK`。函数无显式参数，由`0x00451800`组B静态初始化器调用；唯一callee为MSVC编译器向量构造迭代器`0x0048A4C0`。

包装器只压入五个固定参数、调用迭代器并直接返回。

## 2. 组B五个物理参数

LST按逆序压栈：

1. 组B析构回调`0x00475590`；
2. 组B构造回调`0x00475560`；
3. 元素数量8；
4. 元素尺寸`0x2B28`；
5. 全局基址`0x00525508`。

callee参数顺序为`base,size,count,constructor,destructor`。typed request与组A使用同一结构，但常量全部独立，禁止混用组A的`0x2F34`、数量10或回调。

边界复核：`0x00525508 + 8 * 0x2B28 = 0x0053AE48`。

## 3. 编译器helper与返回

调用后无`add esp`，编译器helper清理五个参数；包装器直接`retn`。typed结果保留helper完整EAX，测试以`0x55667788`锁定；唯一caller虽忽略该值，modern不擅自归零。

组B元素构造callback`0x00475560`与析构callback`0x00475590`现均已由typed helper关闭。构造迭代器自身仍负责八对象前向构造、失败逆向回滚与异常传播，当前继续以共享vector construction port隔离未闭合的MSVC/EH边界；request本身区分组A与组B。

## 4. caller边界回收

组B静态初始化器不再调用临时`LegacyBattleActorGroupBConstructionEntryPort::construct_group()`，而是直接调用本项typed helper。测试锁定：

- caller收到组B五项常量；
- 构造helper事件先于退出注册；
- 元素构造callback的基础构造、固定164-byte分配、token发布、全清零与零分配停止点由独立typed测试锁定；
- 构造EAX只记录，最终返回仍为`_atexit` EAX；
- 注册目标仍为组B退出函数；
- 组A请求未受影响。

## 5. 双向追溯

- `0x00451810..0x00451814`：压入组B析构回调；
- `0x00451815..0x00451819`：压入组B构造回调；
- `0x0045181A..0x0045181B`：压入数量8；
- `0x0045181C..0x00451820`：压入尺寸`0x2B28`；
- `0x00451821..0x00451825`：压入基址；
- `0x00451826..0x0045182A`：调用向量构造迭代器；
- `0x0045182B`：保留callee EAX并返回。

C++到LST反向追溯覆盖19行完整函数、五个参数、callee与返回寄存器。

## 6. 验证与动态差分

定向测试覆盖：

- request组B基址、`0x2B28`尺寸与数量8；
- 组B构造和析构回调token；
- 向量构造端口单次调用；
- callee完整EAX返回；
- 静态caller直接调用typed helper；
- 组B构造先于组B退出注册；
- 组A参数、构造、析构和注册未回归。

battle聚合目标零warning构建及定向测试通过。

当前没有原版编译器向量构造迭代器、八个完整全局对象字节与异常展开联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。构造与析构callback均已typed关闭；完整19行包装器LST已完成固定参数闭环。
