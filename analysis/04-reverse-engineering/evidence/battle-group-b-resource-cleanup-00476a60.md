# 战斗组B资源释放 `0x00476A60`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威主体为`0x00476A60..0x00476A7B`，从`proc`到`endp`共18行、11条实际指令、1个call、1个跳转、1个局部标签和1个返回点。没有外部`FUNCTION CHUNK`。

直接callee固定为CRT释放包装器`0x004885A0`。两个普通call caller固定为战斗运行时销毁`0x0045EA30`和已关闭组B元素析构`0x00475590`；后者自己的SEH外部chunk`0x004983B0..0x004983BD`也已重新核对。

## 2. 字段访问、零token与清零后缀

函数是thiscall，入口ECX为组Bactor token。它先保存ESI，把ECX复制到ESI，再无条件读取actor `+0x0C`的资源token到EAX。

- actor访问本身无效时，故障发生在首次`[ESI+0x0C]`读取；
- token为零时，跳过CRT释放与清零写入，EAX返回0，ECX保持actor token，EDX保持入口值；
- token非零时，先把该token传给`0x004885A0`，只有callee正常返回后才把actor `+0x0C`清零；
- callee异常、诊断陷阱或其他不返回路径均不能提前清零token。

清零写入不改EAX、ECX或EDX，所以非零路径的三个终端寄存器全部来自CRT释放callee。函数只恢复ESI，不规范化EAX，也不根据callee返回值分支。

## 3. CRT释放边界完整审计

`0x004885A0..0x004885B2`共16行、9条实际指令、1个call、0个跳转、0个局部标签和1个返回点且无外部chunk。它把固定值1和资源token传给`0x004885C0`，返回寄存器不再改写。

深层`0x004885C0`完整主体共502行、343条实际指令、20个call、49个跳转、30个局部标签和1个返回点，无外部chunk。其控制流属于MSVC调试CRT堆：包含client hook、heap pointer和block状态检查、诊断与debug break分支，以及最终宿主释放调用。这些宿主CRT内部行为不属于战斗核心owner，不能在现代C++中复制为业务逻辑。

因此modern只在`0x004885A0`保留窄typed释放端口，传递callee token、actor token/index、资源token、物理字段偏移和完整EAX/ECX/EDX。端口正常返回后才执行原清零后缀；端口抛出时保持token和资源内容不变。

## 4. 唯一typed owner

资源token与164-byte资源内容继续由`LegacyBattleActorGroupBElementState`唯一拥有，不建立第二份释放状态。原版释放的是动态164-byte块并清指针；modern使用内联数组承接该块，因此释放成功后除清token外还清空内联数组，表示已释放内容不再可访问。零token与释放失败路径均保留数组旧内容。

actor owner缺失或actor token为零只在原版首次字段访问点形成`actor_state_typed_stop`，保留入口寄存器并且不调用释放端口。没有添加资源长度、token范围或重复释放防护。

## 5. 两个caller回收

战斗运行时销毁仍先完成渲染清理和固定十槽组A双资源清理，然后对固定八槽组B对象逐一typed直连本函数。每槽caller先发布固定对象token，EAX和EDX从前一槽线程传入；本函数读取token后决定是否调用窄CRT端口。零token仍执行八次字段读取但不产生释放调用。组Bowner缺失时只完成到首槽真实字段访问并typed-stop，不伪造后续七槽完成。第八槽正常结果仍作为运行时销毁的完整EAX/ECX/EDX返回。

组B元素析构不再调用整函数opaque端口，而是在SEH状态0内typed直连本函数。资源释放正常完成后才进入公共基础析构；资源typed-stop仍按既有SEH等价规则执行一次基础析构并返回typed状态；释放端口抛出时外部cleanup chunk仍执行一次基础析构并传播原异常。基础析构自身抛出时不重复调用。

旧`release_group_b_object`整函数操作和`release_extension`整函数端口在production源码中为零。保留的唯一opaque边界是尚未业务化的CRT释放callee，而不是`0x00476A60`本身。

## 6. 双向追溯

- `0x00476A60..0x00476A61`：保存ESI并发布this；
- `0x00476A63`：无条件读取资源token到EAX；
- `0x00476A66..0x00476A68`：只以token是否为零决定跳过；
- `0x00476A6A..0x00476A70`：非零token调用窄CRT释放边界并回收参数栈；
- `0x00476A73`：仅在callee返回后清actor资源token；
- `0x00476A7A..0x00476A7B`：恢复ESI并返回。

C++到LST反向追溯覆盖全部11条目标指令、唯一跳转、唯一callee、零/非零/不返回三类路径、两个caller及元素析构SEH chunk。最后一轮正反向追溯不再产生差异或未决基本块。

## 7. 验证与动态差分

纯函数测试覆盖非零释放请求和三寄存器reply、成功后token/内联资源清理、零token跳过并留下EAX零、缺失owner、零actor token和释放异常阻断清零。元素析构回归覆盖typed直连、callee请求、成功顺序、资源异常cleanup、基础异常不重复和资源typed-stop时的基础清理。运行时销毁回归覆盖固定八槽非零释放、全零跳过、owner缺失首槽停止、组A到组B的EDX线程及第八槽终端寄存器。

战斗聚合定向测试、完整core AddressSanitizer `188/188`、Linux core `188/188`和Linux app `194/194`全部通过；最终日志零OpenSWD3源码warning、测试失败和sanitizer finding。

当前缺少原版八个组Bactor、动态资源token与164-byte块、CRT debug heap状态、client hook、诊断陷阱、两个caller和callee寄存器联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
