# 战斗单例角色记录静态生命周期注册 `0x00451860`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整范围与外部chunk

权威LST主体为`0x00451860..0x00451869`，并声明外部`FUNCTION CHUNK AT 0x00451880 SIZE 0x0C`。完整函数合并13行主体与10行chunk，共23行。

主体无条件跳到chunk，真正返回位于`0x0045188B`；外部chunk必须计入审计。

## 2. 独立静态入口

函数无显式参数、无普通CODE XREF；唯一DATA XREF为`.data:0x0049E060`，位于组A、组B静态入口之后但保持独立。

主体调用`0x00451870`单例构造包装器。`0x00451870`与`0x00451890`各9行，均把固定对象token `0x00521598`装入ECX后分别尾跳公共构造`0x00478250`与公共析构`0x00478300`。两个公共本体均已按独立工作包typed关闭。

构造返回不参与正常控制流；typed结果仅保存snapshot，最终返回由注册覆盖。若构造在原写访问处typed-stop，静态入口保留已完成公共前缀并阻断`_atexit`注册。

## 3. 外部chunk与退出注册

chunk执行：

1. 压入单例退出包装器`0x00451890`；
2. 调用CRT `_atexit`；
3. caller回收一个参数；
4. plain返回。

最后EAX为`_atexit`完整结果。registration port显式接收单例退出token `0x00451890`，与组A `0x004517E0`、组B `0x00451840`均不同。调用顺序固定为单例构造一次、退出注册一次；注册失败不回滚构造。

## 4. 两个附件尾跳包装器

`0x00451870`完整9行：`mov ecx,0x00521598`后`jmp 0x00478250`。typed constructor直接对固定单例owner执行64次原始写入，正常原样返回该token，不再经过opaque对象生命周期端口。

`0x00451890`完整9行：`mov ecx,0x00521598`后`jmp 0x00478300`。typed destructor直接读取同一owner的`actor+0xB0`说明token；非零值先经固定`0x004885A0`释放，callee正常返回后才清token和说明owner。wrapper只替换ECX，入口EAX/EDX显式线程给基础析构；零token返回EAX零并保留EDX，非零token返回callee三寄存器残值。对象读、callee和对象写typed-stop均不伪造正常返回。

旧`destroy_object()`整函数端口已删除；现存析构端口只隔离固定CRT释放callee。静态初始化器只有构造完整成功后才注册typed destructor token。

## 5. 双向追溯

- `0x00451860..0x00451864`：调用单例构造包装器；
- `0x00451865..0x00451869`：无条件跳到外部chunk；
- `0x00451880..0x00451884`：压入单例退出函数地址；
- `0x00451885..0x0045188A`：调用`_atexit`并回收参数；
- `0x0045188B`：保留注册EAX并返回；
- `0x00451870..0x00451875`：装入固定this并尾跳公共构造；
- `0x00451890..0x00451895`：装入固定this并尾跳公共析构。

C++到LST反向追溯覆盖主函数23行、两个各9行附件wrapper、外部chunk、固定对象token、退出token及尾跳寄存器线程。

## 6. 验证与动态差分

定向回归覆盖公共构造先于退出注册、构造typed-stop阻断注册、注册EAX零/全一、单例公共说明正常释放、对象读typed-stop、固定ECX与入口EAX/EDX线程，以及组A/组B静态生命周期未回归。当前Linux core`198/198`和定向`2/2`已通过；完整release门见[`battle-actor-base-release-00478300.md`](battle-actor-base-release-00478300.md)。

当前没有原版单例对象字节、说明堆、CRT释放与静态初始化表执行、退出注册和尾跳寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
