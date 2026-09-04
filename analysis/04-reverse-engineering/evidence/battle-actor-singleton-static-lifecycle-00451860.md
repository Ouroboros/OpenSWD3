# 战斗单例角色记录静态生命周期注册 `0x00451860`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整范围与外部chunk

权威LST主体为`0x00451860..0x00451869`，并声明外部`FUNCTION CHUNK AT 0x00451880 SIZE 0x0C`。完整函数合并13行主体与10行chunk，共23行。

主体无条件跳到chunk，真正返回位于`0x0045188B`；外部chunk必须计入审计。

## 2. 独立静态入口

函数无显式参数、无普通CODE XREF；唯一DATA XREF为`.data:0x0049E060`，位于组A、组B静态入口之后但保持独立。

主体调用`0x00451870`单例构造包装器。工作包不把`0x00451870`与`0x00451890`列为独立候选；两者各9行，均把固定对象token `0x00521598`装入ECX后分别尾跳元素构造`0x00478250`与析构`0x00478300`。因此本项同步审计并typed闭合两个附件wrapper，但只为主函数计一次工作包关闭。

构造返回不参与正常控制流；typed结果仅保存snapshot，最终返回由注册覆盖。公共元素构造`0x00478250`现已独立关闭并直接组合；析构本体`0x00478300`仍按其独立工作包顺序关闭。若构造在原写访问处typed-stop，静态入口保留已完成公共前缀并阻断`_atexit`注册。

## 3. 外部chunk与退出注册

chunk执行：

1. 压入单例退出包装器`0x00451890`；
2. 调用CRT `_atexit`；
3. caller回收一个参数；
4. plain返回。

最后EAX为`_atexit`完整结果。registration port显式接收单例退出token `0x00451890`，与组A `0x004517E0`、组B `0x00451840`均不同。测试覆盖0与`0xFFFFFFFF`。

调用顺序固定为单例构造一次、退出注册一次；注册失败不回滚构造。

## 4. 附件尾跳包装器

`0x00451870`完整9行：`mov ecx,0x00521598`后`jmp 0x00478250`，没有返回层；typed constructor helper直接对固定token的单例公共前部owner执行64次原始写入，正常原样返回该token，不再经过opaque object lifecycle port。

`0x00451890`完整9行：同样装入`0x00521598`后`jmp 0x00478300`；typed destructor helper继续使用相同token并原样传播尚未审计的析构EAX。

静态初始化器直接调用typed constructor，只有完整成功后才通过CRT registration port注册typed destructor token。不能把本单例误作组A/组B向量中的元素或复用向量参数。

## 5. 双向追溯

- `0x00451860..0x00451864`：调用单例构造包装器；
- `0x00451865..0x00451869`：无条件跳到外部chunk；
- `0x00451880..0x00451884`：压入单例退出函数地址；
- `0x00451885..0x0045188A`：调用`_atexit`并回收参数；
- `0x0045188B`：保留注册EAX并返回。

C++到LST反向追溯覆盖主函数23行、两个各9行附件尾跳包装器、外部chunk、固定对象token与退出token。

## 6. 验证与动态差分

定向测试覆盖：

- 单例公共前部64次typed写严格先于退出注册；
- 注册时可观察已完成公共owner；
- 构造正常返回固定对象token，析构helper向剩余端口传入同一token；
- 构造typed-stop阻断退出注册并保留精确寄存器；
- 构造EAX只记录且不成为正常最终返回；
- 注册收到精确单例退出token；
- 注册EAX 0与全1均原样返回；
- 组A/组B静态生命周期未回归。

battle聚合目标零warning构建及定向测试通过。

当前没有原版单例对象字节、元素构造/析构本体、CRT静态初始化表执行与退出注册结果联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。外部chunk已纳入完整静态闭环。
