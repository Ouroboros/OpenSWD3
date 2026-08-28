# 战斗角色组A向量构造包装器 `0x004517B0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x004517B0..0x004517CB`，从`proc`到`endp`共19行，没有外部`FUNCTION CHUNK`。函数无显式参数，由`0x004517A0`静态初始化器调用；唯一callee为MSVC编译器向量构造迭代器`0x0048A4C0`。

包装器不建立栈帧，不读取共享可变状态，只按固定顺序压入五个参数、调用迭代器并直接返回。

## 2. 五个物理参数

LST按逆序压栈：

1. 析构回调`0x0046E4D0`；
2. 构造回调`0x0046E490`；
3. 元素数量10；
4. 元素尺寸`0x2F34`；
5. 全局基址`0x005029D0`。

按callee参数顺序即`base,size,count,constructor,destructor`。typed request逐字段保存，不把地址token换成导航名称或主机指针。

范围独立复核：`0x005029D0 + 10 * 0x2F34 = 0x005201D8`，与既有角色组A结构边界一致；计算不扩展或夹值原常量。

## 3. 编译器向量迭代边界

调用后没有`add esp`，说明编译器helper清理五个参数；包装器紧接`retn`。最后一次写EAX来自`0x0048A4C0`，因此typed结果保留其完整32位返回snapshot，尽管权威注释把包装器标为void且唯一caller忽略该值。

元素构造与析构回调现均已关闭；编译器向量迭代器仍承担十对象的前向构造、失败时逆向回滚和全局数组字节映射，因此本包装器暂时保留单一`LegacyBattleActorVectorConstructionPort`。测试以`0x12345678`证明返回不改写。

## 4. caller边界回收

上一项静态初始化器不再调用opaque `construct_group()`端口，而是直接调用本项typed helper，再调用CRT退出注册端口。测试同时锁定：

- 构造helper收到五个固定参数；
- 构造返回`0xAABBCCDD`被caller记录但不成为最终返回；
- 随后的`_atexit`结果覆盖最终EAX；
- 构造事件严格先于退出注册事件。

元素回调已不再阻塞；construction port只隔离尚未建立typed全局十对象存储的MSVC向量异常回滚边界，不再包含未知业务callee。

## 5. 双向追溯

- `0x004517B0..0x004517B4`：压入析构回调；
- `0x004517B5..0x004517B9`：压入构造回调；
- `0x004517BA..0x004517BB`：压入数量10；
- `0x004517BC..0x004517C0`：压入尺寸`0x2F34`；
- `0x004517C1..0x004517C5`：压入基址；
- `0x004517C6..0x004517CA`：调用向量构造迭代器；
- `0x004517CB`：保留callee EAX并返回。

C++到LST反向追溯覆盖19行完整函数、全部五个参数、callee与返回寄存器。

## 6. 验证与动态差分

定向测试覆盖：

- request基址精确为角色组A全局token；
- 元素尺寸精确为`0x2F34`；
- 元素数量精确为10；
- 构造/析构回调token精确；
- 向量构造端口只调用一次；
- callee EAX完整返回；
- 静态初始化caller直接调用typed helper并保留构造先于注册。

battle聚合目标零warning构建及定向测试通过。

当前没有原版编译器向量构造迭代器、全局十对象数组字节和异常展开联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整19行LST已完成固定参数闭环，元素构造与析构行为各有独立证据。
