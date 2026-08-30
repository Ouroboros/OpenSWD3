# 战斗角色组B向量析构包装器 `0x00451840`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x00451840..0x00451856`，从`proc`到`endp`共17行，没有外部`FUNCTION CHUNK`。函数无显式参数，其地址由组B静态初始化器外部chunk注册给CRT `_atexit`；唯一callee为MSVC向量析构迭代器`0x0048A560`。

## 2. 组B四个物理参数

LST按逆序压栈：

1. 组B析构回调`0x00475590`；
2. 元素数量8；
3. 元素尺寸`0x2B28`；
4. 全局基址`0x00525508`。

callee参数顺序为`base,size,count,destructor`。typed destruction request与组A同形，但使用组B独立常量。范围为`0x00525508..0x0053AE48`。

## 3. 编译器helper与返回

调用后无`add esp`，compiler helper清理四个参数；包装器直接`retn`。typed结果保留callee完整EAX，测试以`0x13572468`锁定，不因函数注释为void而归零。

组B元素析构callback`0x00475590`现已由typed helper关闭，并覆盖扩展→基础顺序、扩展异常时基础清理和旧SEH链ECX恢复。编译器helper自身的八对象逆序析构与异常展开仍由vector destruction port隔离。

## 4. 退出注册目标闭合

组B静态初始化器已显式向CRT registration port传递本函数token `0x00451840`。本项提供对应typed helper，因此组B注册目标行为边界已闭合：

- static initializer注册组B token；
- 独立helper生成同一组B四参数request；
- 组A退出token与四参数保持不同；
- CRT平台注册机制仍作为唯一外部边界。

## 5. 双向追溯

- `0x00451840..0x00451844`：压入组B析构回调；
- `0x00451845..0x00451846`：压入数量8；
- `0x00451847..0x0045184B`：压入尺寸`0x2B28`；
- `0x0045184C..0x00451850`：压入基址；
- `0x00451851..0x00451855`：调用向量析构迭代器；
- `0x00451856`：保留callee EAX并返回。

C++到LST反向追溯覆盖17行完整函数、四个参数、callee与返回寄存器。

## 6. 验证与动态差分

定向测试覆盖：

- destruction request组B基址、`0x2B28`和数量8；
- 组B析构回调token；
- 向量析构端口只调用一次；
- callee完整EAX返回；
- 组B静态注册目标精确；
- 组A构造/析构和组B构造请求未回归。

battle聚合目标零warning构建及定向测试通过。

当前没有原版编译器向量析构迭代器、八个完整全局对象字节与异常展开联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。元素析构callback已typed关闭；完整17行包装器LST已完成固定参数闭环。
