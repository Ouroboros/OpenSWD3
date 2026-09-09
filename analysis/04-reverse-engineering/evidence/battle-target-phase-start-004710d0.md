# 战斗目标演出阶段初始化 `0x004710D0`

状态：`platform_adapted`。完整LST、typed实现、action dispatch caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x004710D0..0x0047126B`，proc至endp共171行、104条实际指令、7个call、3个跳转、3个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。五个函数callee为`0x00478620`、`0x00478470`、`0x004019A0`、`0x0047CE70`与已关闭host-surface设置`0x00433F30`；另有两次`GetSystemMetrics`导入调用。

## 2. 资源、演出记录与算术

函数先以显式Group-B目标token直接组合已关闭typed `0x00478620`，把返回EAX写入Group-A source actor `+0x255C`语义槽，再查询两项坐标差，随后把source actor `+0x0E14`起始的`0x58`字节演出记录清零。leaf typed-stop保留目标actor动作记录复制、动作更新与可能提交的`turn_frame_token`，但阻断source phase token及全部caller后缀；leaf正常返回零则仍发布零token并完成坐标与记录清零，直到`0x00471120`真实资源对象首dword读取点停止。

资源可用时调用图像解码窄port，把返回token写入演出记录；resource `+0x0C/+0x0E`两个word分别发布为宽高。坐标差按signed low-word扩展，第一项减一；演出X按`source_x - source_y + actor_x - 0x32`的32位回绕算术生成，演出Y按`actor_y - vertical_adjustment + 0x28`生成。固定宽高为`0x14/0x1E`，两个active dword为1，三项step为5，spacing为`0x28`。

resource height按unsigned word分支：小于100时16位减10，否则逻辑右移一位。flags初值`0x56`；目标属性查询严格等于1时并入bit0。actor `+0x2A87` byte再并入bit3。

## 3. host surface、清零与owner

原版用系统宽高重建host surface。OpenSWD3复用startup中的唯一`LegacyBattleRenderGeometry` owner，并以当前SDL raster surface宽高完成平台适配，直接调用已关闭`set_legacy_battle_host_surface`。其后按原顺序把actor `+0x2680`清零，并清空`+0x0DF4`的8个dword、`+0x0500`的38个dword和`+0x2BC8`的190个dword。

action dispatch为每个group-A行动者持有唯一`LegacyBattleTargetPhaseState`；其中复用既有`LegacyBattleImageParticleEmitter`作为actor `+0x0E14`物理记录owner，并持有资源token、mode byte与三段清零区，不复制第二份状态。隐藏this是Group-A source actor，显式参数是Group-B目标token。资源准备通过startup `group_b_lifecycle[].action_execution` canonical owner解析；`0x00478470`已关闭并在原`0x004710F6`站点直接组合typed基准坐标查询。生产路径不再调用`0x00478620` generic port，仅保留后续尚未关闭的`0x004019A0`解码与`0x0047CE70`属性窄port，也不保留整个`0x004710D0` opaque adapter。

## 4. caller回收与验证

唯一caller为action dispatch `0x004539B0`的action 6阶段零分支。production现在按group-A index定位行动者的typed target-phase owner，以group-B token作为显式目标参数并直接调用初始化，失败映射到caller typed-stop；后续演出门、暗化、阶段word与刷新保持原顺序。

parent按`sub esp,10h`后依次保存入口EBX、EBP、ESI、EDI，再由CALL隐式压入返回地址`0x004710E4`。typed leaf入口EAX/EDX沿用caller残值，ECX为Group-B目标，ESI保留Group-A source token；leaf两个RET均回到`0x004710E4`。leaf成功或早退完成后才发布EAX。基准坐标X输出复用原目标参数dword槽并只覆盖低16位，Y输出使用独立零初始化local；入口EAX仍为Y输出指针token，ECX为Group-B目标，EDX与flags来自typed leaf真实返回，其中成功路径flags由`add esp,8`决定、早退路径flags由`test eax,eax`决定。Y侧typed-stop保留资源token和X低word提交，并在演出记录清零、解码、属性查询、host surface及全部尾部动作之前停止。正常尾部恢复四个callee-saved寄存器，并公开`add esp,10h`后的flags及`retn 4`返回EIP/ESP。

Workpack 289 REVIEW 2扩展测试覆盖Group-B目标owner与Group-A source phase双token、五项parent栈写、callee-saved恢复、typed leaf成功与早退出口、provider零token仍完成leaf、REP中段部分提交、frame-token写故障、leaf EDX与真实ADD/TEST flags转交、基准坐标typed-stop、零token及非零token/object-unreadable均在`0x00471120`停止并公开两次既有push后的ESP、局部EAX/ECX、资源EDX、演出EBX/EDI与XOR flags、演出后缀抑制、action 6完整后缀及`0x004710D0/0x00478620/0x00478470`生产零opaque调用。工作包284的既有门禁保持历史有效；Workpack 289 REVIEW 2另通过注入式定向`1/1`、Linux core `199/199`、AddressSanitizer/UBSan `199/199`、Linux app `205/205`及连续十轮core `10/10`，全部正式stderr为空且无源码warning。动态差分因原版group-B资源对象、图像解码分配、坐标/属性callee和caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
