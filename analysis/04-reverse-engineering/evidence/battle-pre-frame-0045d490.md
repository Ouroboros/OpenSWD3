# 战斗逐帧角色预处理 `0x0045D490`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围与调用图

权威函数为`0x0045D490..0x0045D685`，从proc到endp完整218行、181条实际指令、7个静态call、9个分支与6个局部标签，无外部FUNCTION CHUNK。

唯一caller是已关闭战斗逐帧协调器`0x00453200`。调用位于两个前置stage之后、已关闭角色metric和顺序重建之前；caller不读取正常返回寄存器。

四个唯一callee尚未关闭：`0x00478330`三处、`0x0047CE80`两处、`0x0047D7D0`与`0x00481FC0`各一处。modern保留四类窄预帧端口，发布完整EAX/ECX/EDX；资格查询可同步动态组B数量。

## 2. 三道入口门与首个共享写

函数依次执行：

1. terminal latch完整dword不等于1，直接返回该值；
2. active actor code为0，直接返回0；
3. message state完整dword等于3，返回active actor code。

三道门均不写状态、不调用callee。通过后先把action execution active写1，再以`active + 2`的32位回绕索引把事件工作区槽写1。

事件工作区是已关闭最终角色步进使用的同一126 dword数组。索引越界只在该首次真实store处typed-stop；action execution active已发布，其他状态不回滚。active为全1时索引回绕为1，原程序会写有效槽1，不能按signed actor范围提前拒绝。

## 3. source全1短路

首个工作区写完成后读取source actor code。完整值为全1时：

- pre-frame gate A写1；
- message state写3；
- active actor、terminal latch、source和工作区槽保持现值；
- 不调用任何callee；
- EAX仍为active actor，ECX为3，EDX为全1。

逐帧caller的常见短路路径保留该状态顺序。

## 4. 当前组A角色建立

source不是全1时，按顺序：

1. secondary actor code写active；
2. active actor code清零；
3. auxiliary gate写1；
4. 以`groupA_base + (actor-8)*stride`的u32回绕token调用配置callee，参数1；
5. published actor code写source；
6. message state、pre-frame gate A/B清零；
7. 对同一组A token调用当前状态查询。

actor code不在本函数夹到`8..17`。token算术只保留32位，callee负责原对象访问。

当前状态查询可改写secondary actor。完整EAX等于1时，函数先重读secondary：

- action execution active写5；
- 以重读值的`secondary+2`事件工作区槽写5；该store可在action execution发布后typed-stop；
- 依次调用通知callee与配置callee参数1；两者仍可继续改写secondary；
- 第二次typed availability写入后再次重读secondary，以`actor*5-40`的32位回绕索引把五dword角色记录的首项写1。

最后记录store是本路径第二个typed-stop。越界时保留两次availability写入、通知callee、工作区5及全部前序发布。成功返回EAX为secondary actor，EDX为线性记录索引，ECX保留第二次typed写入的角色token。

## 5. 组B资格扫描

当前组A查询不等于1时，先对`groupB_sentinel + published*stride`调用资格callee。published是1-based值，因此1正好定位首个组B对象。只有完整EAX等于1才继续；其他值直接返回callee寄存器。

随后读取共享metric组B数量并按signed语义处理：

- 数量小于等于0，直接进入收束；
- 正数时从物理组B首对象和零基索引0开始；
- 每项资格返回EAX为0时，先把索引加1并发布为新的1-based published code，然后立即返回0；
- 非零时在callee之后重读动态组B数量，再递增索引并作signed比较。

不缓存入口数量，不增加现代循环上限。callee收缩或扩张数量会立即影响下一次判断。

## 6. 全部可用后的收束

初始数量不正或扫描到动态末尾时：

1. 对secondary组A actor调用配置callee，参数0；
2. callee后重读secondary actor code到ECX；
3. action execution active清零；
4. published actor code写1；
5. source actor code写全1；
6. 以重读secondary的`+2`槽写0；该实际工作区store可typed-stop，保留前五项而不清terminal latch；
7. terminal latch清零。

返回EAX为末次完整参数0，ECX与EDX都为重新读取的secondary actor code。pre-frame gate A/B和secondary本身不清。

## 7. 单一typed存储与全局重置

D490涉及的active、secondary、published、action execution、auxiliary、事件工作区和五dword记录直接复用既有`LegacyBattleFinalActorStepState`与`LegacyBattleActionDispatchState`。terminal与message进一步从final/action、startup和效果状态的多个副本回收到唯一`LegacyBattleSharedPhaseStatePort`。只新增此前未命名的source actor与两个pre-frame gate字段，不建立第二套角色状态。

全局重置把对应标量与共享phase从未映射字节像回收，并按原固定写序只清事件工作区物理槽`0..9`与`16..95`；槽`10..15`及`96..125`保持入口值。五dword角色记录不在该重置写集合中，也保持不变。

## 8. caller回收、测试与动态差分

逐帧协调器删除第三前置stage opaque枚举，两个旧stage后直接组合typed预处理。子typed-stop保留预处理前缀并阻断metric、角色顺序、完成门、surface和全部后续帧逻辑。

定向测试覆盖三道入口门、工作区首次store时序、全1source短路、active全1索引回绕、callee改写source/secondary后的动态重读与后续store停点、组A成功、末记录typed-stop、1-based组B预查询、零基动态扫描、首个不可用发布、动态数量收缩、全部可用收束、尾寄存器、全局重置分段别名及caller typed-stop传播。

## 9. `0x00478330`三处预处理写入

工作包278关闭`0x0045D51C`、`0x0045D5BB`、`0x0045D647`三处物理call，参数依次为完整dword `1`、`1`、`0`。三处均在按actor code定位组A对象后直连共享availability owner；第一处EDX保留入口流程刚读取的source actor code，第二处保留组A通知callee返回EDX，第三处EDX是重新读取的secondary actor code。写停止时EAX已装参数、ECX为实际组Atoken，并阻断各自后续配置、记录、workspace及terminal尾部。

当前缺少原版角色对象、三类剩余callee、完整共享全局、动态数量修改和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

## 10. Workpack 316 的SDL实际入口前缀

用户第二次开场战斗日志`build/vm/wp316-sdl-frame-user-run-v2/openswd3-2026-10-03_19-14-31-66208.log`只到`0x0045FC5B`输入分派RET前：记录2／18均零、后续12项rapid零、脚本status=32，不能宣称已经执行本函数。此后SDL生产前缀在可证实的输入正常返回后，依LST顺序读`0x0053C018`、active actor及message；前两个门的早退不读后续状态，活跃路径在`0x0045D4C8`source首读前停止，早退路径按下一CALL`0x00453248→sub_45B0E0`清metric两张表，并借用启动owner读取`0x0045B0FE`组B计数；非零停在`0x0045B11F→sub_4783B0`前，零停在`0x0045B13E`组A读前。SDL的`0x0053C018`借用已存在的`battle_input_dispatch_.selected_actor_cleanup_gate`，不能再给`LegacyBattleSharedPhaseStatePort::battle_terminal_latch()`保留独立生产副本。新门、metric清表与组B计数首读各自定向core 1/1、Linux core／ASan各200/200及Linux／Windows app各206/206通过；尚无新SDL实际回调记录，也未进入角色坐标CALL或到角色帧。

## 11. SDL接线前的真实callee寄存器入口

再次完整读取本函数218行LST及三个callee：`00481FC0..00481FCD`读取`actor+26D0`低byte的bit 1；`0047D7D0..0047D7E9`依次写`+2AEC` dword 1、`+2A12` word 0、`+2670` dword 0；`0047CE80..0047CE9A`先读取`+2AB8`到EDX，精确等于1时短路，否则读取`+26D1` byte的bit 0x20。不能交换组A资格查询与组B资格查询的callee。

旧端口请求只携带actor token和逻辑参数，缺少真实callee保留的EAX/EDX。当前补充入口寄存器，actor token即物理CALL时的ECX：

- `0045D55E`：EAX为`(secondary-8)*3021`，EDX仍为secondary。
- `0045D597`：EAX为`(secondary-8)*1007`，EDX为该值乘3；此处LEA的目标是EDX。
- `0045D5F3`：EAX为一基published数量乘345，EDX保留前次callee结果。
- `0045D60E`：EAX为动态重读的组B数量，EDX保留上一调用结果，ECX为当前物理角色token。

所有算术保留u32回绕。另修正`0045D575..0045D57A`第二次工作区store停止时的寄存器前缀：EAX已为5，ECX已为`secondary-8`，不能保留上次callee返回值。

独立测试检查组A查询/通知的不同缩放值、组B预查询与扫描的不同EAX、跨调用非零EDX、0x80000000一基值的独立EAX/ECX回绕，以及第二次store停止前缀。`battle-pre-frame-register-binding-{core,asan,sdl}.log`确认core/ASan战斗聚合目标各1/1通过，SDL链接通过，三个日志无warning/error；这些改动补齐端口合同；当时尚未接入三个真实SDL回调或完整战斗帧，后续实现见下节。

## 12. 三个角色callee的生产借用与失败传播

再次逐指令核对三个短callee后，实现`invoke_legacy_battle_pre_frame_actor_call`，直接借用角色view中的progress与residual，不物化临时角色图像，不新增游戏字段副本。组A查询先清EAX，再读低byte bit 1，保留ECX/EDX；组B查询先把完整special-ready dword读到EDX，再把EAX置1，精确等于1时短路，否则检查`mode_gate`的0x2000位。通知先清EAX，再按`+2AEC`、`+2A12`、`+2670`顺序写入，进度只清低word。

回调请求缺少真实对象或已有访问门拒绝访问时，返回typed-stop和原物理指令。通知的第二、第三次写入失败分别保留第一写和前两写。预帧四个物理调用点均传播停止，不能继续第二次availability写、组B扫描发布、运行记录或最终清理；结果保存最后一次角色调用的诊断。

SDL现在继承预帧端口，并通过真实动作/启动状态解析角色token后调用上述实现。该方法已接线，但完整帧调用仍未替换手工入口，不能把新增override当作SDL运行证据。

跨层检查还发现角色图像缺少`+2AEC`字段；现已加入`transition_value`的序列化与按范围写回。测试用与SDL相同的角色解析器验证通知三次写入均出现在同一图像，再验证该dword写回原progress对象且不改进度字。其他新增测试覆盖byte选择、完整dword比较、入口寄存器、通知写入前缀，以及四个调用点失败后阻断后缀。

`battle-pre-frame-live-callback-{core,frame-core,asan,frame-asan,sdl}.log`确认战斗聚合和角色帧目标在core/ASan下分别1/1通过，SDL链接通过，五个日志均无warning/error。由于新增字段映射会影响角色帧快照，除战斗聚合外同时覆盖角色帧目标；`git diff --check`通过。完整SDL帧和实机生命周期仍未验证。
