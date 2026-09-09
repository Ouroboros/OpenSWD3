# 战斗角色帧资源准备 `0x00478620`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed:5/5`。Workpack 289三个REVIEW已回收frame-input三处、Group-A目标演出一处及Group-B目标演出一处物理callsite；inventory row 289已由权威生成器关闭。

## 1. 完整LST边界与ABI

权威函数为`0x00478620..0x0047866C`，共77字节、29条实际指令、2个call、1个条件分支与2个普通`retn`，没有外部`FUNCTION CHUNK`或中段入口。两个callee按顺序为已关闭动作更新`0x004321E0`与帧资源查询`0x004315D0`。

入口为thiscall：`ECX`是actor token。函数依次保存EBX、ESI、EDI，把EBX绑定到actor，并以`actor+0x02A0`为源、`actor+0x0CB8`为目标执行38次正向`REP MOVSD`。平台合同要求DF=0；现代typed结果显式公开DF合同停止，不能把反向复制伪装成受支持路径。

## 2. 十八槽物理动作记录owner

actor从`+0x02A0`起存在18个连续`0x98`字节动作记录槽。`LegacyBattleActorActionRecordSlots`按物理顺序唯一持有这些记录；已知命名槽继续复用原字段，未知槽保留为reserved。第0槽是`frame_source_action_record`，第17槽是`frame_prepared_action_record`，结构总大小固定为`18 * 0x98`。

Group-A继续由`LegacyBattleActionDispatchState::group_a_action_execution[]`持有；Group-B继续由`LegacyBattleStartupState::group_b_lifecycle[].action_execution`持有。没有新增平行actor数组或第二套动作记录owner。

## 3. REP顺序、alias与部分提交

每个dword严格执行：source读取、destination写入、ESI加4、EDI加4、ECX减1。实现不预先snapshot源或目标，也不使用一次结构赋值。source fault保留此前已复制前缀且不读取当前dword；destination fault保留当前source读取，但不写入或推进当前dword。

底层view允许测试重叠字节窗口。目标位于源后4字节时，后续source读取会观察前一次destination写入，证明实现保留正向`REP MOVSD`的真实重叠传播，而不是从隐藏快照复制。

## 4. 两个callee、低word与寄存器

复制完成后先压入目标token，再由CALL隐式压入`0x00478640`，随后调用typed动作更新。callee返回后执行`add esp,4`与`test eax,eax`。完整EAX为0时直接恢复EDI、ESI、EBX并从早退`retn`读取caller返回地址；不读取两个目标word、不调用帧查询且不写frame token。

EAX非零时按机器顺序执行：

- `mov ax,[actor+0x0D04]`：只用目标`+0x4C`替换updater EAX低16位；
- `mov cx,[actor+0x0D02]`：只用目标`+0x4A`替换updater ECX低16位；
- EDX完整保持updater残值；
- 先push完整EAX，再push完整ECX，CALL隐式压入`0x00478660`；typed provider按resource低word、frame低word查询；
- `add esp,8`后，无论provider token为零或非零，都写入canonical `turn_frame_token`。

正常返回保留provider EAX/ECX/EDX，并恢复入口EBX/ESI/EDI。结果公开两个callee入口与返回残值、八项栈写、六项栈读、ESP、caller返回EIP、38项复制计数、两个word读取、frame-token提交和两个出口。每个typed-stop另公开当前故障EIP：三次入口push为`0x00478620/23/24`，DF/REP source/destination为`0x00478638`，updater参数push/CALL隐式返回地址push为`0x0047863A/3B`，早退pop/RET为`0x00478647/48/49/4A`，两个word读取为`0x0047864B/52`，provider参数push/CALL隐式返回地址push为`0x00478659/5A/5B`，token写为`0x00478663`，成功pop/RET为`0x00478669/6A/6B/6C`。

## 5. flags与typed-stop

REP及MOV不修改flags。REP source/destination停止保留入口flags。动作更新后的`add esp,4`会被紧随其后的`test eax,eax`覆盖，因此早退与两个word读取、帧参数push阶段都保留TEST结果：CF/OF清零、ZF/SF/PF来自完整EAX，AF未定义。provider正常返回后执行的`add esp,8`决定frame-token写入、三次pop与成功RET阶段的最终flags。

typed-stop独立覆盖：三次callee-saved push、38个source读、38个destination写、动作参数push、动作CALL返回地址push、两个目标word读、帧与资源参数push、帧CALL返回地址push、frame-token写、三次pop及两个leaf RET返回地址读。每个停止点保留当时ESP、栈内容、寄存器、flags、REP残值、已复制目标前缀、callee副作用与已提交actor写。

## 6. frame-input三个caller回收

`0x0045FC60`中的三处物理callsite已直接组合typed leaf：

- `0x004605D9`：Group-B逆序候选命中路径；返回地址`0x004605DE`；
- `0x004607F6`：Group-A actor-order路径；返回地址`0x004607FB`；
- `0x00460A0A`：Group-A直接逆序路径；返回地址`0x00460A0F`。

三处均位于已关闭`0x004784A0` snapshot之后。`0x004605D9`与`0x00460A0A`沿用snapshot EAX/EDX/flags；`0x004607F6`在snapshot后重算`EAX=actor_index*0xBCD`，入口flags来自最后一次`actor_index*0x3F0-actor_index`，其后两条LEA不改flags，EDX仍沿用snapshot残值。三处ECX都是显式actor；EBX/ESI分别为Group-B逆序索引/actor token、actor-order槽token/1、Group-A逆序索引/actor token。Group-B和Group-A直接逆序caller的EDI为0；actor-order首次及首像素前普通miss后的EDI为0，完整8×8扫描miss后的下一候选EDI为8，现代循环显式携带这一寄存器残值。只有虚拟ESP token由caller请求注入。leaf typed-stop保留actor动作记录、动作更新和可能的frame-token前缀，阻断surface解析、镜像、像素查询、目标可用标记、发布与当前/剩余actor后缀。

surface adapter只接收本leaf返回的frame token，不再以actor token替代资源结果。该adapter仅把现代资源对象解析成typed view，不对应原版CALL，因此不修改模拟EAX/ECX/EDX；Group-B首个mirror继续看到leaf返回EAX/EDX，Group-A对象解析后的下一mirror继续看到前一个mirror/pixel路径残值。Group-B在mirror前读取资源对象首dword；非零对象的首dword为零走普通未命中。Group-A先执行mirror，再在首个像素资源访问点读取资源对象；零token在该点typed-stop。provider返回零仍先由leaf提交零frame token，再由caller在真实对象访问点停止。

## 7. Group-A目标演出初始化caller回收

`0x004710D0`在`0x004710DF`以显式Group-B目标token为ECX直接组合本typed leaf，返回地址固定为`0x004710E4`。leaf通过startup `group_b_lifecycle[].action_execution` canonical owner读写目标的第0/17动作槽与`turn_frame_token`；Group-A source的target-phase owner保持独立。parent先执行`sub esp,10h`并保存入口EBX、EBP、ESI、EDI，再压入CALL返回地址。leaf typed-stop保留这五项parent栈前缀、目标动作记录部分提交、callee副作用与可能完成的frame-token写，且阻断source phase token、坐标、`0x58`记录清零及全部后缀。

leaf成功或早退完成后才把返回EAX写入Group-A source action-execution唯一owner的`target_phase_resource_token`。phase只借用该共享字段，不再复制`actor+0x255C`。provider返回零仍完成leaf并提交零`turn_frame_token`，caller随后执行基准坐标和`0x58`清零，直到`0x00471120`资源对象首dword读取停止。`0x00478470`入口EAX仍为Y输出地址、ECX为Group-B目标、EDX为leaf残值；flags来自leaf真实`test eax,eax`或`add esp,8`。解码`0x004019A0`与属性`0x0047CE70`继续保留为窄port，生产路径不再调用generic `0x00478620`。

## 8. Group-B目标演出caller回收

`0x00484020`前缀在`0x0048402F`以隐藏this的Group-B source、显式Group-A target token及目标索引直接组合本typed leaf，返回地址为`0x00484034`。outer caller为`0x00455D60` action 6的`0x00456458`；入口保持`EAX=target_index*0xBCD`、`ECX/ESI=source token`、`EDX=action-query residue`、`EBX=1`、`EBP=target token`、`EDI=target index`。

每个Group-B source按Group-A target索引持有唯一`0x58` phase槽；相邻source和target严格隔离。source `actor+0x255C`由lifecycle `action_execution.target_phase_resource_token`唯一持有，phase只借用；target的动作记录与`turn_frame_token`来自startup Group-A canonical owner。坐标查询后先清零对应`0x58` phase，再读取资源对象；可用时按Group-B常量发布emitter：lifetime divisor `0x28`、spawn divisor `0x3C`、remaining batches=`height/2`。解码`0x004019A0`入口保持`EAX=local_4`、`ECX=source共享资源token`、`EDX=资源对象首dword`；属性`0x0047CE70`入口保持`EAX=5`、`ECX=Group-A target token`，EDX高word继承source垂直调整值且低word为`height/2`。host-surface后直接恢复EDI/ESI/EBP/EBX、执行`add esp,10h`并`retn 8`，不执行Group-A尾部清零；正常残值为`EAX=surface_height`、`ECX=0x0051DF80`、`EDX=surface_width`。外层`0x0045645D`随后以这三项残值和target ECX调用真实`0x004787F0`，再以该callee返回残值和同一target ECX调用`0x0047D870`。

leaf或parent typed-stop保留已完成的target动作记录、target `turn_frame_token`、source共享token、坐标低word及phase清零前缀，并阻断set-target-mode、clear-mode、attack-order移除、暗化、刷新、phase/input word和`0x004841B0`完成阶段。生产路径对`0x00478620`与`0x00484020`均为零opaque调用；后者自身inventory row 419仍保持`pending_audit`，本轮只回收其前缀中的callee caller。

## 9. 测试与当前门禁

leaf测试覆盖38组source fault、38组destination fault、重叠正向复制、三次push、三次pop、两组参数push、两组CALL返回地址push、两个leaf RET读取、动作更新零/非零、两个word fault、provider零/非零、frame-token fault、DF合同、两个出口、updater高字与EDX、provider残值、ESP和TEST/ADD flags。所有故障均断言LST对应的当前EIP；十二类正常路径栈停止逐项锁定完整栈内容、寄存器、flags、callee副作用与复制前缀；updater零值早退另行覆盖三类POP fault和早退RET fault。

frame-input测试覆盖三个物理caller、caller-specific EBX/ESI/EDI、actor-order重算EAX与SUB flags、完整扫描后EDI=8的下一候选残值、两次动作更新与两次provider调用、typed token传给surface adapter、surface reply哨兵不污染Group-B首mirror和Group-A次mirror寄存器、Group-B首dword普通未命中、Group-B非零token/object-unreadable在action kind 6可达场景的mirror与action-six前停止、Group-A零token及非零token/object-unreadable保留reset/configure与一次mirror前缀后停止、REP中段fault对当前/剩余actor后缀的抑制、frame-token fault，以及reserved旧槽零调用。target-phase与action-dispatch测试另覆盖Group-B目标owner、source/target双token、五项parent栈写、leaf两出口、provider零token、REP中段fault、frame-token fault、基准坐标入口寄存器/flags、零token与非零token/object-unreadable在对象读取处的既有push、局部地址寄存器与XOR flags、演出后缀抑制和raw地址零调用。REVIEW 3再覆盖两个Group-B source与两个Group-A target的二维phase隔离、显式Group-A token、source共享token借用、正常emitter常量、parent栈与最终寄存器/flags、REP部分提交、零token、非零token/object-unreadable、canonical坐标停止、host-surface行表停止、完整外层后缀抑制，以及`0x004841B0`仅在正常完成检查路径保留。

Workpack 289最终注入式定向`1/1`、Linux core `199/199`、AddressSanitizer/UBSan `199/199`、Linux app `205/205`及连续十轮core `10/10`均通过；全部正式stderr为空，changed-range格式检查通过。inventory双生成、TMP分类和完整unstaged/staged发布审计在发布冻结阶段完成。

## 10. 动态oracle缺口

原版动态差分仍为`blocked_runtime_oracle`：缺少完整Group-A/Group-B actor十八槽动作记录、重叠与异常source/destination/resource/栈内存页、两个callee寄存器与flags、DF、SEH及五处caller联合捕获后端。该缺口不影响完整LST、静态访问顺序、typed部分提交或固定状态测试结论。
