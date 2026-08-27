# 战斗角色动作提交 `0x00462420`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00462420..0x004624BF`，从proc到endp共92行、53条实际指令、1个静态call、8个跳转、6个局部/返回标签、1个`retn`，没有外部`FUNCTION CHUNK`。

九个静态caller已经全部关闭：逐帧输入selected option路径一处，正向动作轮转四处，反向动作轮转四处。唯一callee为既有group-A角色状态查询边界。

## 2. 消息与队列入口

函数唯一栈参数为待提交actor code。入口先把共享message装入EAX；message非零且逐帧option word为全1时立即返回message，并保持入口ECX/EDX，不保存非易失寄存器，也不清任何缓存。

其余路径读取live group-A count，EAX减1。结果恰好为0时不扫描队列，直接进入缓存清理；count为0时减法回绕全1并进入原无界循环，不增加现代下限保护。

## 3. live十槽队列扫描

队列严格复用final-actor的十项物理actor order。从索引0开始，每项为0或不等于参数时跳过；每轮尾部都重新读取live group-A count，先把索引加1，再把count减1并作unsigned `index >= count-1`结束判断。

实现不把入口count固定为循环上限。count 12或count 0会在第十项前缀完成后，于第十一次真实队列读取typed-stop；此前所有角色查询返回和callee副作用保持可见，四项缓存不清。

## 4. 角色查询与交换

非零队列code匹配参数时，按`code-8`保留原乘移序列：调用前EAX为`index*0x3EF`，ECX为group-A对象token，EDX为入口值或上一轮live `count-1`。物理角色索引只允许0..9；code 7在完成地址计算后、首次真实对象call前typed-stop。

角色查询完整EAX等于1时继续扫描，callee ECX/EDX随即被循环尾的live count装载覆盖为`count-1`。其他EAX触发交换：先把当前queued code装入EAX，再把匹配code发布为queued，并把旧queued写回当前队列槽。交换后保留callee EDX，随后进入公共缓存清理。

## 5. 公共缓存与返回寄存器

所有普通扫描结束、count 1以及成功交换都按固定顺序把ECX清零，并清三个selection dword与一个selection word。普通返回EAX保留路径值：count 1为0，无匹配且无callee时为入口`count-1`，查询完成路径可为1，交换路径为旧queued code。EDX在无扫描时保持入口值，普通循环结束时为最后live `count-1`，交换时保留callee返回。

任何队列或角色typed-stop都发生在公共清理前，因此不能回滚前缀，也不能提前清缓存。

## 6. caller回收

逐帧输入旧direct commit槽保留reserved数值。record15把option word按i16符号扩展到EAX后直接提交；普通返回后才按message进入目标选择并把option恢复全1，typed-stop会保留option与pre-frame gate并阻断两项后续操作。

正向和反向动作轮转的旧nested commit槽也保留reserved数值。各自第一callee返回的EAX直接作为typed提交参数和入口EAX；typed-stop向上传播到逐帧输入，记录2不会恢复option或进入目标选择，记录3不会执行左向尾调用，记录4/6不会发布菜单动作，记录5不会继续后续输入阶段。

## 7. 验证与动态差分

定向测试覆盖消息/option组合门、count 1、无匹配扫描、查询完成、查询失败交换、两次匹配与live EDX、十槽一过尾、code 7角色停点、record15普通交换与typed-stop、正向/反向轮转typed提交及逐帧caller传播，三个reserved槽均为零调用。

当前缺少原版十槽角色队列、group-A对象、角色查询callee共享副作用、九个caller联合轨迹及EAX/ECX/EDX捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
