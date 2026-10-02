# 战斗角色逐帧动作与效果呈现（0x00479850）

状态：`pending_audit`；以下为独立 LST 审计基线，不是实现或验收通过声明。

316 阶段进度采用等权八项里程碑，只在该项全部判据有证据时计数；
2026-09-27 为 `2/8 = 25%`，不代表指令覆盖率、生产兼容率或最终验收。

已完成：(1) 锁定主体范围、四处调用及 249 块／98 CALL／22 RET 的机械索引；
(2) 四处真实调用在合成快照、无物理 owner 条件下的子函数首读停点。

尚未完成：(3) 图像解码其余命令、尺寸与全部读写路径；
(4) 主函数其余分支、调用、返回及异常栈；
(5) 生产数据源、内存所有权和跨 owner 别名；
(6) 四处调用正常返回与故障后缀；
(7) 全部 249 块、98 CALL、22 RET 与四处调用的双向 REVIEW 收敛；
(8) 最终门禁、inventory／模块文档／PLAN 同步、提交推送和完成 TG。

某项只有前缀或条件化测试时计为未完成；后续只在整项闭合后递增。
B10 已关闭工作包仍为 `315/422 = 74.64%`，316 仍是 `0/1` 验收。

## 1. 唯一机器证据与 ABI

权威 `swd3.exe_export_for_ai/swd3.exe.lst` 的主体为 `0x00479850..0x0047B9CA`，
半开区间到 `0x0047B9CB`，没有外部 FUNCTION CHUNK。其后 `0x0047B9CC..0x0047BA17` 是 19 项跳转表，
`0x0047BA18..0x0047BA7B` 是 100-byte 间接表；`0x0047BA80` 才开始下一函数。主体含 2289 条实际带字节和助记符的指令、98 个物理 CALL、22 个普通 `retn`。

ECX 为隐藏 actor token，无显式栈参数；四个入口保存的寄存器均在正常返回时恢复，ESP 相对入口加4。纠正标签解析后，
导航脚本从原指令机械划出 249 个基本块、98 个单独 CALL 块及 22 个 RET；按原始机器字节与标签联合解码 19 项间接跳表和 100-byte selector 表，
检查每个表项的四字节小端目标地址、全部控制流后继、跳表边缘，结果 249/249 块可达且无未解析边。原脚本曾把符号名 `def_4799D5` 误当成地址 `0x004799D5`，
实际标签位于 `0x0047A80B`；旧版 250 块和旧边均作废。机械校验不替代 FLAGS、寄存器与每条读写的人工作证。

导航材料（不提交）：`build/workpack316/function-and-tables.lst`、`calls.txt`、`basic-block-ledger.txt`、`branch-vectors.md`；摘要文件中有各件 SHA-256。所有语义仍以原 LST 字节和指令为准。

## 2. 已关闭父函数中的四个物理 caller

```text
动作主分派 case 7       0x004554F6 -> 0x004554FB  group B；EBX=1，完整 EAX 等于1才执行后缀
对手动作分派 case 7     0x0045650F -> 0x00456514  group A；完整 EAX 等于 1 才进入后缀
最终角色步进 group A    0x0045AA33 -> 0x0045AA38  group A；完整 EAX 等于 1
最终角色步进 group B    0x0045ACBF -> 0x0045ACC4  group B；完整 EAX 等于 1
```

组 A 物理 token 为 `0x005029D0 + index*0x2F34`，组 B 为 `0x00525508 + index*0x2B28`，
按 LST 的低 32 位计算，不提前限制索引。主动作分派在 `0x004539FB` 把 EBX 置1，case7 `0x004554D8..0x004554FB` 无改写 EBX；
`cmp eax,ebx; jnz default` 因此只在 EAX 恰为1时继续。四处 caller 均为完整 EAX==1 门；
之前把第一处当成 EAX 非零的结论错误，已撤回。四处 LST 调用前缀分别为：主分派 `0x004554D8..0x004554F6` 从栈取 group-B 索引，
按低32位乘 `0x2B28` 加基址后以 ECX 调用；EAX==1 后才在 `0x00455503` 压零调用 `sub_478710`、随后 `sub_45EFB0(index)`。

对手分派 `0x004564F3..0x0045650F` 从栈取 group-A 索引，按低32位乘 `0x2F34` 后调用，
`0x00456514` **在回复后**置 EBX=1，只有 EAX==1 才清 EBP、调用 `sub_478710(0)` 及 `sub_45EFB0(index+4)`。

最终角色步进组A `0x0045AA07..0x0045AA38` 先按上一操作 EAX==1 门进入，再取组A索引，发布局部 actor token 后调用；
EAX!=1 跳 `0x0045ADE6`，恰为1才读 `dword_5054D0[index*0x2F34]` 并执行后缀。组B `0x0045AC98..0x0045ACC7` 先检查索引是否全1，
再计算组B token；EAX!=1 同样跳 `0x0045ADE6`，恰为1才准备两处局部槽、清局部输入并调用 `sub_475870`。

EAX!=1 的三类原 LST 后缀也已锁定：主分派跳 `0x00455CBB..0x00455CCF`，从局部恢复 fs:0 的 SEH 链、恢复寄存器并强制 EAX=0 返回；
对手分派跳 `0x0045661B..0x0045662F`，同样先恢复 SEH 链再 EAX=0 返回；最终角色步进两处都跳 `0x0045ADE6..0x0045ADEC`，
只恢复保存寄存器并令 EAX=0 返回。故 target 返回0时四处 parent 都不调用各自前述成功后缀；现代 typed-stop 不得误走这些**正常** EAX0 返回路径。

主分派的成功后缀已逐指令核到 `0x00455538`：目标 EAX==1 后压0、以 ECX=先前算出的 EDI actor token 调 `sub_478710`，
再压 ESI 源 Group-B 索引调 `sub_45EFB0`、`add esp,4`；随后按 **signed** `0<=ESI<=3` 门只递增 `dword_53BF00` 低 byte，
恢复寄存器、EAX=入口时设好的 EBX=1、恢复 fs:0 的 SEH 链、`add esp,0x10` 并 `retn`。任何两个后续 CALL 的 typed-stop 都不得执行它们之后的自增/正常 return。

未绑定显式 caller 快照的 C++ `case 7` 在进入 `sub_479850` 前仍调用 `require_group_b()`；
显式快照现绕过此现代数组门，原 LST `0x004554D8..004554F6` 对 ESI 按低32位构造 actor token 并先发 CALL。

仅证明无物理 owner 时停在 child 首读；不能将静态 `.data` 覆盖冒充实际页面值或完整 caller 等价。
对手分派成功后缀也已逐指令核到 `0x00456580`：原 `0x00456514` **先**写 EBX=1，再以回复 EAX==1 门继续；
清 EBP=0、以目标 token EDI/压0 调 `sub_478710`，将 EDI 改为 `ESI+4` 后调 `sub_45EFB0(EDI)`、`add esp,4`；
按 signed `0<=ESI<=3` 才 `inc byte_53BEFF`。随后独立比较 `dword_53BD54==EDI` 则清0、`dword_53AE70==EDI` 则写全1并清 `dword_53BF74`，
EAX=EBX=1，**先**恢复 SEH `fs:0` 再 pop 保存寄存器、`add esp,0x0C` 并返回。两处后续调用停点与三处条件全局写的前缀须分开；
现有 C++ 的 `validate_group_a()` 也在物理 `0x0045650F` 之前，异常 target 索引不能直接视作已证明等价。

最终角色步进的 Group-B 成功后缀 `0x0045ACCD..0x0045ADE5`：先获取两处原栈输出地址，**先压地址再清原槽**为零，
以 ECX=先前 EDI actor token 调 `sub_475870`；正常回复后读两个输出 word，依次加到 `word_53BF14`、`dword_53BF10+2` 高 word。

调用 `sub_480AD0` 取 actor 嵌套对象 token，**先**写 `dword_5028FC` 再读其 `+0x20` byte bit5；
据此 `word_53BF16` 加0x14或3。设 EBP=1，压1调 `sub_47F910`，用其 EAX 与共享 `dword_4B9F00` 调 `sub_477710`，
再压 ESI 调 `sub_45EFB0`，其后只一次 `add esp,0x10`。按 signed `0<=ESI<=dword_53BCE0` 时递增 `dword_53BF00` 低 byte；
比较该低 byte 减 `dword_53BF00` 第三 byte 与 bound，若不少于 bound，依序写 `53BCEC=0x63`、`53C018=0`、`53BFC4=1`、`53BFC0=1`。

接着 `word_53BF28` 非零才调用 `sub_47CE80`；该回复 EAX==0 且 `dword_53BCE0 - lowbyte(dword_53BF00)==1` 才写 `53BCE0=1`、清计数低 byte、清 `word_53BF28` 并依次调 `sub_45B0E0/sub_45B190/sub_45B5A0`。
其他分支跳过这段，最后 EAX=EBP=1、恢复四寄存器返回。该后缀最多九处物理 CALL/嵌套 `+0x20` 故障必须截断其后缀，
不可把目标函数正常回复1直接视为 parent 已返回1。

最终角色步进 Group-A 成功后缀 `0x0045AA41..0x0045AC97` 也按原 LST 复核：其物理 CALL 前在 `0x0045AA2F` 已把栈上原 `arg_4` 改写成当前 actor token。
子函数 EAX==1 后才读取 `dword_5054D0[ESI]` 完成 flag；等于1时递增 `dword_53BF00` 第二 byte，
以 `dword_53BF24+2` unsigned word 阈值决定是否在 `dword_53BCE4` 指示的尾部用 `rep stosd` 按每项32 byte 清工作区、再清阶段高 word 并减 Group-A count；
随后按 `sub_45B0E0/sub_45B190/sub_45B5A0` 三 CALL。flag 不等于1仅 `inc byte_53BEFF`。

两路汇合后以 EBP actor token 调 `sub_4750C0`，压 `EDI+8` 角色编号调 `sub_45EFB0`，
**随后先清** `dword_524268[EDI*4]`，再按 `dword_53BD54==EDI+8` 判断是否扫描 Group-A 数量 `53BCE4`、Group-B 数量 `53BCE0`，
逐 actor 调 `sub_47C660(0)`；匹配时依序发布七项共享状态（`53BD54/53BCE8/4A754C/53BCEC/53BFC4/53BFC0/53BF74`）。

之后独立比较 `dword_53AE70` 与该角色编号、按当前 Group-A 数量扫描 `dword_520DD0` 十槽顺序并左移命中的角色；
用 `53BCE4 - lowword(53BF0C) - highword(53BF24)` 的32位结果与 `byte_53BEFF` 的零扩展值作 **unsigned** 比较。

达到阈值时清126 dword 工作区、发布 `53AE70=all1` 与双 gate/message0x67 返回1；未达到则以原改写后的栈 `arg_4` actor token 调 `sub_47F340`，
仅完整 EAX==1 时先写 `4A754C=53AE70+1`、`53BD50=角色编号` 再调 `sub_478330(1)`，
随后重读可被该 callee 改写的 `53BD50`，依次写 pending、事件槽、辅助门与五 dword 记录清零；两路正常都返回1。

此前任一 REP 写/单次 CALL/动态数组访问故障须在实际指令截断，不能拿 parent 既有 typed 结果或全函数成功值充当 LST 故障证据。
四处 caller 的**正常**非1及成功后缀现已有逐段原指令索引，但跨 child typed-stop 的 FLAGS/DF/ESP/EIP、全部寄存器和个别故障前缀仍待逐点验证。

四个 parent→child CALL 入站寄存器/栈别名仍有区别，按原 LST 独立验收：主分派 `0x004554D8..0x004554F6` 以 ESI=index，
经算术使 EAX=`index*1381`（低32位），EDI/ECX=`0x525508+8*EAX`，EBX在远处 `0x004539FB` 已置1；
最后改 FLAGS 的是 `0x004554E2 sub eax,esi`，后续 LEA/MOV 不改 FLAGS。对手分派 `0x004564F3..0x0045650F` 使 EAX=`index*3021`、EDI/ECX=`0x5029D0+4*EAX`；
最后改 FLAGS 的是 `0x00456501 sub eax,esi`，**EBX=1 要等 child 正常返回后的 `0x00456514` 才执行**，
child typed-stop 时不得提前发布该寄存器值。最终步进组A `0x0045AA11..0x0045AA33` 保存 EDI=index、EAX=`index*1007`、ESI=`index*0x2F34`、EBP/ECX=`0x5029D0+ESI`，
且 **CALL 前 `0x0045AA2F` 已把原栈 arg_4 改写成 token**；最后 FLAGS 来自 `0x0045AA24 shl esi,2`，
其后 LEA/MOV 不改。最终步进组B `0x0045AC98..0x0045ACBF` 先比较 index==全1，命中则无 child CALL；
否则 ESI=index、EAX=`index*1381`、EDI/ECX=`0x525508+8*EAX`，最后 FLAGS 来自 `0x0045ACAB sub eax,esi`。

四处的低32位乘法不事先作 bounds check，且分别保留原 parent 保存寄存器与 locals；child 内途停止时并无返回回复，
parent 的下一条 cmp、EBX重置、正常 EAX0 清理尾和成功后缀均未执行。首个 child 读 `0x0047985B cmp [esi+0x2ABC],ebx` 若故障：
设各 parent CALL 前 ESP=`P`，CALL 已压返回地址、child 已 `sub esp,0x14` 并依次压 EBX/EBP/ESI/EDI，
故四处均停在 EIP=`0x0047985B`、ESP=`P−0x28`、EBX=0、ESI=该次 ECX actor token、FLAGS来自 child `0x00479858 xor ebx,ebx`（ZF1/CF0/OF0/SF0/PF1，
AF不作断言），DF仍为该次 parent CALL 时的入站方向位，EBP/EDI 尚未被 child 改写。EAX仍分别是主分派 `index*1381`、对手分派 `index*3021`、最终组A `index*1007` 与最终组B `index*1381` 的低32位，
而非 child 的正常0/1回复。`0x0045AA2F` 对原 arg_4 的写已发生，其余三个 parent 在该 child 读障前的写前缀以各自的真实前缀为准；
所有 parent post-call 指令尚未执行。仅 child 正常返回后，四处才以完整 EAX==1 作各自门；不能把“可观察停止”折算为 EAX0。

## 3. 入口、分派与副作用顺序

- `0x0047985B..0x00479861`：actor `+0x2ABC` 完整 dword 为零时直接走 default，返回 EAX0，没有 CALL 和 actor 写入；任何非零进入并先置 `+0x2AAC/+0x2AB8=1`，以 `+0x2A0C` 低 word 写 `+0x3D0`，写 `+0x3D8=0x24`。
- `0x00479893..0x0047991F`：`+0x2AA0==1` 或（`+0x2AA0!=1` 且 `+0x2AF8==1`），
  并且 `+0x2B00/+0x2B04` 均完整为零时立即重置。`+0x2AA0!=1` 时先 OR `actor[+4]->+0x25` 的 byte bit7。
  后续先写清零前缀与 38 次 `rep stosd`，在 `0x004798F7` 直接调用已关闭 `sub_478850`，再写 `+0x2AAC/+0x2ABC/+0x2AB8`，
  调用 `0x00479911 sub_47E950`，返回 EAX1。无完整门匹配时不清理，走下一条更新。

- `0x00479920..0x004799D5`：以先前的 EDI（`actor+0x3D0`）作参数调用 `sub_4321E0`；
  更新器七个正常 `retn` 中 `0x00432449` 令 EAX0、其余六处均以 EBP=1 令 EAX1；EAX0 时立即返回 EAX1。
  否则 `mov cx,[actor+0x41A]` **只替换 ECX 低16位**，先压零再压完整 ECX 调 `sub_4315D0`；
  其高16位沿用更新器的 ECX 回复，不可随意清零。随后先写 `+0x2548`，再读 `+0x2B20` 和 `+0x3E0`；
  精确等于1时清 `+0x2B08`。只有 `+0x2B08==1` 且 `+0x3E0!=0` 时读取 frame `+0x0C` 并改写 EBP。

  之后写 `+0x2694 &= 0x80000003`；source record `+0x20` bit5 命中时把 selector byte 写100；
  非零 `+0x2A95` byte 覆盖前一 selector，最后才进入跳表。

- `0x004799C3` 对零扩展的 selector 减1，再按无符号与99比较。主表 19 项，byte 间接表 100 项；18 项非 default 精确为 `1..15 -> 0..14`、`50 -> 15`、`51 -> 16`、`100 -> 17`，其余 82 项指向索引18的 default。0 与大于100均在间接表之前进入 default；无效 selector 返回0。

分派的两个物理表读取和停止寄存器不能被高层 switch 合并：`0x004799BB xor eax,eax; 0x004799BD mov al,[esi+2A94h]` 零扩展完整 selector，
`0x004799C3 dec eax` 生成低32位 selector−1，`0x004799C4 cmp eax,0x63; 0x004799C7 ja default` 在 selector0或>=101时不读两表。

只有1..100时，`0x004799CD xor edx,edx` 把 EDX全清并设置 FLAGS，再由 `0x004799CF mov dl,byte_47BA18[eax]` 读第一表的单 byte 到 EDX低8，
最后 `0x004799D5 jmp dword ptr jpt_4799D5[edx*4]` 读第二表的完整四字节目标。首表读障时 EAX=selector−1、EDX=0、FLAGS来自 XOR EDX；
次表读障时 EDX=已取的跳表索引、FLAGS仍来自该 XOR，两个异常 EIP/已读字节与直接 `ja default` 都不同。

表值的19/100项机器字节机械核对仍只是结构导航，实际内存读障与目标分派要以原指令为准。

当前未提交 typed WIP 实现入口零门真实 default、非零门五次逐次访问与四项短路条件；更新路由停在 `0x00479920`。
重置路由按 `0x004798B3/BC` 写 phase1000/selector0、`0x004798C3` 复读 source；
不等于1时 `0x004798CB` 重读 actor+4、`0x004798CE` 对 nested `EAX+0x25` 分别读/写 byte，
写成功才 OR bit7 与发布新 FLAGS。然后 `0x004798D2/D8/DE` 按顺序清 `+0x2C4/+0x2C8/+0x2958`，
`0x004798E5/EA` 置 ECX=38、XOR EAX0，`0x004798EC` 清 `+0x2A12`，`0x004798F3` 按原入站 DF 逐 dword 清38次并逐次同步 actor/slot scalar 别名，
正常经 `mov ecx,esi` 停在**尚未执行**的 `0x004798F7 sub_478850` CALL；每次 REP 写障保留 EIP=`0x004798F3`、剩余 ECX、当前 EDI 和已提交前缀。

Group-A nested 0x38 byte backing 有 canonical owner；Group-B `legacy_battle_group_b_action_configuration.cpp:72` 的 live token 指向外部 0x20 步长 source 池，
不能以本地单条0x20 record 冒充 `+0x25`。未提交 WIP 将 resolver 指向 `group_b_lifecycle` 八条 canonical `action_record`，
按原地址跨条映射相邻 source 的 `+5`；末条在未提供相邻全局owner时仍 typed-stop。跨条、无owner与末条边界向量已加入；
受管`proc_a13b`编译通过但CTest`198/199`（battle setup三项断言失败，退出码8），该外部映射**尚未经验证**；
受管诊断`proc_8fca`再现CTest`198/199`：三项均停在真实更新短路`0x00479920`（status=0、访问12次），
因测试把`source_runtime_value=2`却把`script_binary_state`留为0；按`0x00479893..0x004798A1`必须置script=1才走后续重置。

修正script后受管`proc_f72f`仅剩一项向量期望错误：真实重置路径`reset_call_ready`，相邻source byte已从0x25变成0xA5，
但可观测访问数为62而非source==1跳过两读两RMW的58；将独立断言改为62后于2026-09-23 22:42 UTC受管`./build.sh core --test`与CTest`199/199`通过（`proc_a52f`，
退出码0）；覆盖Group-B跨条写、缺owner与末条越界三个向量。当时尚未接入相邻全局owner。入口/phase/nested 各版core/CTest`199/199`已通过（`proc_a495/proc_6e74/proc_8f1a/proc_f336/proc_64a1`，
2026-09-23 UTC）；**新增**四项清零与 REP 的 DF=0/1、第29/30次访问向量首次受管`proc_77e8`构建成功但CTest`198/199`（battle setup失败，
退出码8）：断言误期望 DF=1 会清 `+0x3D8`；LST逆向38次从`+0x3D0`向低址，实际保留先写的`+0x3D8=0x24`，
修正该独立向量后于2026-09-23 22:00 UTC受管重跑`./build.sh core --test`成功，CTest`199/199`（`proc_546d`，
退出码0）；这只验证至`0x004798F7`的前缀。即使通过，也只证明局部前缀，不代表18 selector、全部98处CALL、四parent caller或最终双向REVIEW；
inventory保持`pending_audit`。

新增 `continue_legacy_battle_actor_frame_reset()` 只接受入口 helper 停在`0x004798F7`的状态：
先独立触摸 CALL 返回地址写，再用**现有** typed `reset_legacy_battle_actor_runtime()` 和当前 canonical owners 组合子调用，
保留全局物理访问 ordinal、子函数完整停止回复、寄存器/FLAGS/ESP与随机port。child 仅正常返回后重新物化其写回后的 actor image，
按 `0x004798FC` 清 `+0x2AAC`、`0x00479902` 清 `+0x2ABC`、`0x00479908` 压 EBP=1、`0x0047990B` 写 `+0x2AB8=1`；
暂停在**尚未执行**的 `0x00479911 sub_47E950`，不伪称返回1。CALL 栈写障、child 首访问障与成功尾部测试于2026-09-23 22:08 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_a313`，
退出码0）；只证明至`0x00479911`的组合与这些向量。后续核查发现 child `random_call_typed_stop` 无物理 actor 访问站点，
已单独映射成 `callee_call` 并保留 `0x00439070` EIP 而非默认 actor_read；新增随机 CALL 停止向量于2026-09-23 22:14 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_8b07`，
退出码0）。新增 `continue_legacy_battle_actor_frame_release()` 在`0x00479911`压返回地址后物理执行`sub_47E950`的实参读、四次callee寄存器压栈、`0x0047F0BF`读`+0x2584`、`0x0047F0C5`按读/写拆分AND`+0x26D0 & 0xFEBD`，
再清`+0x26C0/+0x2584`。

**仅空链**完成callee四次出栈、`retn 4`及parent `0x00479916..0x0047991F`还原与返回1；

非零头已提交清理前缀但明确停在**未执行**`0x0047F0DE mov esi,[eax]`。空链、非空链节点门、CALL压栈与RMW写障向量已加入；
受管`proc_9ff4`编译失败（测试fixture误写`base_initialization`而实际变量是`base`，另新增enum触发switch警告），
均已修复；2026-09-23 22:23 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_3ab2`，
退出码0）；另将`0x0047F0F3 retn 4`与外层`0x0047991F RET`的可读性障碍拆成独立参数，新增两处逐次栈读障向量，
于2026-09-23 22:58 UTC受管`./build.sh core --test`与CTest`199/199`通过（`proc_8abc`，
退出码0）。非空链owner和`sub_4885A0`深层CRT释放、Group-B末条池外owner接线在该轮仍未完成；其它跨调用同步仍未闭合。

生产者已核：`legacy_battle_startup.cpp:250..251`及`legacy_battle_opponent_action_dispatch.cpp:101..102`均传`0x005213A0+index*0x20`作为Group-B外部source token；
`token+0x25`横跨到下一条0x20记录的`+5`（末项甚至超出当前八条数组边界），不等同于`LegacyBattleGroupBActionRecord`局部0x20字节。

LST `.data:005214A4..005214A9` 随后定义`dword_5214A4/word_5214A8`，末条source token `0x00521480` 的 `+0x25` 等于 `0x005214A5`，
即已存在 `LegacyBattleLevelAdvancementState::growth_delta_primary[0]` 高byte；
补足该全局 owner 后可解锁末项，仍不得读取短局部对象的越界位置。

更新路由另新增 `continue_legacy_battle_actor_frame_update()`：从`0x00479920`独立压EDI，
再在`0x00479921`压返回地址并通过显式 typed port 对 canonical slot2 `reserved_action_record_02 / actor+0x3D0`调用`sub_4321E0`。

port 未正常返回时停于callee并保留两次栈写及已提交record写；正常返回经`0x00479926`清实参、`0x00479929` TEST EAX，
EAX=0才按`0x0047992D..0x00479936`逐次出栈并返回1；EAX≠0则停在**尚未执行**的`0x00479937`读取`actor+0x41A`，
后续18 selector不声称已实现。两次压栈写障、port停止、EAX=0/1及record别名向量于2026-09-23 23:04 UTC受管`./build.sh core --test`与CTest`199/199`通过（`proc_f527`，
退出码0）；opaque callee内部物理访问仍为部分边界。

更新第二段 `continue_legacy_battle_actor_frame_lookup()` 已从`0x00479937`逐次读 slot2 `+0x4A`，
保留 ECX 高16位，按 LST 压 EBX/ECX 并在`0x00479940`调用 adapted `sub_4315D0`；
正常后先于`0x00479945`写 canonical `+0x2548`，再依次读`+0x2B20/+0x3E0`，清两实参并比对帧启动位，
仅等于1才于`0x0047995F`清`+0x2B08`，停在**未执行**的`0x00479965`读。首字读障、call/写前栈、+2548写障FLAGS、+2B08写障别名与正常路径独立向量于2026-09-23 23:09 UTC受管`./build.sh core --test`与CTest`199/199`通过（`proc_4f88`，
退出码0）；callee内部与下一批selector未闭合。

`0x00479965` 后续资源门先读`+0x2B08`；若其为1且`+0x3E0`非零，重读`+0x2548`、清EBP，再按 frame token 读取`[EDX+0x0C]`的低16位，
作32位`SUB EBP,EAX`；其余路线跳过 frame 读。然后逐次读`+0x2694`、AND`0x80000003`并写回、读取`actor+0x0C`。

Group-B `actor+0x0C`映射构造函数的 `LegacyBattleActorGroupBElementState::resource_token` 与0xA4-byte owner；
Group-A 由已关闭 `0x0046E890/0x0046E9C0` 资料物化时的物理 `actor+0x0C` 写，对应现行 `LegacyBattleGroupAConfigurationState::profile_token/profile_record`（0xA4 byte），
同样接入image与逐次写回。只有人为缺 owner 的 view 在读站点typed-stop。早期仅Group-B接线和手工view缺owner向量于2026-09-23 23:19 UTC受管`./build.sh core --test`和CTest`199/199`通过（`proc_8a0c`，
退出码0）；Group-A真实resolver与跨阶段资料字节门测试于2026-09-23 23:50 UTC受管`./build.sh core --test`和CTest`199/199`通过（`proc_ddd7`，
退出码0）。

后续 `continue_legacy_battle_actor_frame_selector_header()` 从`0x0047999F`物理读取资源`+0x20` bit5，
为真时先于`0x004799A5`写selector100；再独立读 actor`+0x2A95` byte，非零时于`0x004799B5`覆盖`+0x2A94`，
最后`XOR EAX`、重读selector，停在**未执行**的`0x004799C3 DEC`与跳表。LST另有`0x0047D53C/0x0047F3A4`对`+0x2A95`的物理写；
仓库尚无其它typed owner，WIP将该byte单独纳入actor residual image，并保留`+0x2A94`原canonical owner。

Group-A/B image写回与Group-B bit5、override、两处故障向量已加入，2026-09-23 23:28 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_182b`，
退出码0）；该轮只验至selector DEC 前；后续双表见下段。

`0x004799C3..0x004799D5` 双表分派 WIP 严格分开：32位DEC、无符号`>99`直接跳`0x0047A80B`且不读表；
其它索引先从`0x0047BA18+(selector-1)`读byte，后从`0x0047B9CC+4*EDX`读dword，不合并读障。

byte索引0..14对应selector1..15，15/16/17对应50/51/100，18为默认（16..49及52..99）；
函数只返回**未执行**的目标case/default EIP，并未实现case体或默认POP/RET。有效/无效索引与两处独立读障向量已加入，
2026-09-23 23:30 UTC受管`./build.sh core --test`和CTest`199/199`通过（`proc_8a4b`，
退出码0）。

frame token 精确等于上述 Group-B allocation token 时，现在可借同一0xA4-byte backing 读取 resource`+0x0C/+0x0D`；
异 token、无 owner 或 ordinal 故障均在未读取的`0x00479980`停下，不借用错误 allocation。

little-endian BP、32位SUB、写掩码与异 token/ordinal 故障测试已加入，2026-09-23 23:41 UTC受管`./build.sh core --test`和CTest`199/199`通过（`proc_5c27`，
退出码0）。

default `0x0047A80B..0x0047A814` 共尾另续逐次POP EDI/ESI/EBP、XOR EAX0、POP EBX、ADD ESP0x14、RET；
区分selector直接越界和表映射默认的不同 FLAGS/EDX 入站，在同一共尾消解。入口第一个POP及最后RET故障向量已加入，
2026-09-23 23:37 UTC受管`./build.sh core --test`和CTest`199/199`通过（`proc_c362`，
退出码0）；正常RET不得替代尚未执行的18条case。

Group-B末条额外 alias 的 WIP 从可选 `LegacyBattleLevelAdvancementState` 指针读取/写回 `growth_delta_primary[0]` 的高byte：
`0x1234→0x9234` 保留低byte；未传入该全局 owner 仍停在 `0x004798CE` 读站点，不把默认为0或相邻主机对象当作原映像。

LST `0x005214A4..0x005214A8` 静态相邻关系、全局 typed 状态地址、两侧向量已加入；受管`proc_384d` core构建通过但CTest`198/199`（新末项alias断言失败，
退出码8），`proc_1b4f`复测仍为CTest`198/199`（退出码8）：status=21/`0x004798CE`、growth=0x1234、owner存在。

实现将八条池尾`0x005214A0`误加1得`0x005214A1`，而last token `0x00521480+0x25`实际为`0x005214A5=池尾+5`；
比较改为`+5`后`proc_78d8`仍为CTest`198/199`（退出码8），但诊断已见`growth=0x9234`、status=13/`reset_call_ready`、phase=0；
原断言错误要求phase仍为1000，而真实 `0x004798DE` 成功清零。修正测试期望后于2026-09-24 00:04 UTC受管`./build.sh core --test`和CTest`199/199`通过（`proc_3818`，
退出码0）；只证明显式提供growth owner时该别名与reset前缀，未证明四处真实caller已接入。真实四处 caller 接入时仍需把共享 growth 状态接给resolver，
不能只在UT里提供。

case1 (`0x004799DC..0x004799E7`) 以u16相减的SF/OF/ZF作signed `JG`：481进 `0x0047B801`，
480和0x8000留在**未执行**的`0x004799ED`（绘制、音频尚未实现）；共用reset尾暂执行`0x0047B801/08`两次word清、ECX38/XOR EAX0、按现有DF逐次`0x0047B816 REP`，
止于**未执行**的`0x0047B81A sub_478850`。同一 EIP 可从其它case以不同 EDI/ECX 进入，不把case1前缀冒充全部共享reset。

独立phase、首读障、第二word写障、REP首写障及正常38项向量于2026-09-24 00:10 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_02fc`，
退出码0）；case1其余路径尚未执行；共享reset CALL/RET下一段待核。

共享reset另一出口 `0x0047B81A..0x0047B83D` 的 WIP 组合现有 typed `sub_478850`：
CALL压返回地址`0x0047B81F`，child正常后才MOV EAX1并依序写`+0x2AAC=0/+0x2AB8=1/+0x2ABC=0`，
独立POP四项、ADD ESP0x14、RET；**不调用**入口重置路线的`0x00479911 sub_47E950`链表释放。

CALL压栈/child首障与成功返回向量于2026-09-24 00:15 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_4f69`，
退出码0）；其它case入口使用同一尾前仍须核对各自 EDI/已提交前缀。

2026-09-24 00:21 UTC对当前未提交WIP额外执行受管`./build-asan.sh --test`：linux-asan Debug构建和CTest`199/199`通过（`proc_fb18`，退出码0）。此为当前代码的阶段性sanitizer结果；后续编辑后需重跑，不能替代完整case、caller与最终 REVIEW。

case1活动路由另从`0x004799ED`按word比EBX：phase0独立读共享`dword_4AB784`样本句柄，逐次压句柄与`0x31`，
停在**未执行**的`0x004799FA sub_485610`；非零phase逐次读`actor+0x2548`、将EAX改为`0x7BDEF7BD`、压EBX零，
停在**未读取**的`0x00479A0E [ECX]`。global `sample_handle` 复用现有 `LegacyBattleGroupAActionExecutionSharedState`，
Group-A/B resolver共享同一action state；缺owner、首PUSH障和frame token读障均保留入站寄存器/FLAGS/ESP。

向量于2026-09-24 00:27 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_d97a`，
退出码0）。音频port、frame首dword owner、后续四global/绘制及case1活动返回仍未实现；先前`proc_fb18`的sanitizer结果不覆盖本轮新增代码。

case1 `0x00479A0E mov edx,[ecx]` 的首dword owner 复用已关闭动作呈现 `0x00478FF1 sub_4315D0 / 0x00478FFC mov edx,[eax]` 的同类资源首读。
给现有 `LegacyBattleGroupAActionResourceRecord` 加`value_00`及**显式已知**标记：
已有动作呈现port的`outputs[0]`填入该值；actor`+0x2548`换成另一token时失效；316的lookup port仅在回复明确给出资源头且actor指针写成功后发布新缓存。

case1源读只在token非零、与canonical资源记录相等、首dword已知时读取并抵达**未写**`0x00479A10 dword_4CD730`，
否则在`0x00479A0E`typed-stop且保留先前EBX压栈；不借Group-A/B资料`actor+0x0C`假冒frame header。

现有动作呈现、Group-A/B token失效、316资源头端口和两侧读障向量于2026-09-24 00:36 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_940c`，
退出码0）。port对原始加载器未有完整文件/缓存动态差分，首dword只代表显式提供的已知值。

case1 phase0 的音频边界继续到`0x004799FA`：CALL返回地址单独压栈，窄typed port仅代表`sub_485610`正常回复或**callee入口**停止；
正常回复后恢复返回地址、由caller `0x004799FF add esp,8` 清两个样本参数并重入`0x00479A02`源token读。

port若要表示Miles/CRT深层中止，当前reply缺深层ESP/栈追踪，因此不宣称deep stop已等价；只允入口typed-stop。
CALL压栈障、callee入口stop、正常合流三向量于2026-09-24 00:42 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_d37c`，
退出码0）。

case1 `0x00479A10..0x00479A71` 现按物理序写`dword_4CD730`、三次**独立**重读有符号`actor+0x2958`并按`imul/sub/sar/shr/add/shl`写`0x4CD71C/0x4CD30C/0x4CD304`；
在下一条`0x00479A77`源token读取前停止。case1 phase480的三个结果各为`0xFFFFFFE2`、phase`0xFFFF`按有符号-1得0；
七处source/phase/global访问各自可障并检查前缀已提交写。音频入口停止改为保留CALL入站寄存器/FLAGS，不采信未返回reply的伪寄存器。

于2026-09-24 00:49 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_af2c`，
退出码0）；尚未覆盖本轮ASan及case1高度/标志位后缀。

case1 `0x00479A77..0x00479AA1` 再次读`actor+0x2548/+0x2958`、独立取同token资源`+0x0E`（必须明确已知）、将无符号word与有符号phase按32位加到`dword_4CD75C`，
再读`actor+0x2694`、掩`0x80000023`、OR低byte bit5、写回。现分别标记首dword与资源`+0x0C/+0x0E`的有效性，
换token时都失效；未标记的`+0x0E`不能借已知首dword臆读。六个独立物理访问与缺owner障向量于2026-09-24 00:55 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_f6fe`，
退出码0）；其后绘图参数/CALL仍未执行。

case1 `0x00479AA7..0x00479AEB` 将再次读取actor源token、slot2`+0x14`绘图Y偏移、同token资源`+0x0E/+0x0C`两个独立word、源坐标Y/X、共享motion A与phase，
再按LST的五个PUSH次序保存FLAGS/高度/宽度/计算后Y/X。13个物理资源、actor、global与栈访问各可单独停止；
停在**未执行**的`0x00479AEC sub_4170E0`前。负motion(-30)、phase480、坐标(250,300)、slot2 Y=9、EBP=17应产生参数`[0x80000021,7,5,-69,233]`。

资源宽度未知的故障必须在第一项FLAGS已压栈、资源高度已读后发生。于2026-09-24 01:01 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_d2ff`，
退出码0）；绘图CALL/返回、phase增加31与case1返回零尚未实现。

case1 `0x00479AEC sub_4170E0` 现引入六个cdecl参数的typed绘图port：已压的零尾参数来自`0x00479A0D PUSH EBX`，
其上依次压flags/高/宽/Y/X；CALL返回地址单独可障，入口停止保留该地址及六参数而不加phase，正常回复仅弹返回地址。

`0x00479AF1` 以**当前**phase的word RMW分开检查读/写故障，正常写入后只加`0x1F`并停于尚未执行的`0x00479AF9`栈清理。
三路CALL、phase读写障与正常写回向量于2026-09-24 01:07 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_d657`，
退出码0）；该绘图port只验入口停/正常回复，不代表完成sub_4170E0内部像素路径、深层故障或四处真实caller。case1 POP/RET后缀仍未实现。

case1 `0x00479AF9..0x00479B05` 现于phase已按word加31后由caller `ADD ESP,0x18` 清除零尾参及五绘图参数，
`XOR EAX,EAX`、逐次POP EDI/ESI/EBP/EBX、`ADD ESP,0x14`及单独RET读；五个POP/RET读障保留各自ESP和已写phase。

新增正常activity出口应返回零、ESP=入口+4、DFlag沿用入站。2026-09-24 01:11 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_53c3`，
退出码0）。该局部出口不等于完整316：其它17条case、非空链释放、四处真实caller与最终REVIEW均仍未完成。

case2先推进`0x00479B06/+0x0E14`两次独立actor读取：phase与入站CX=100按word比较，相等直接到`0x00479C6C`，
不访问粒子owner；否则读取token并与EBX0比，零到粒子初始化`0x00479B1F`、非零到已有粒子共尾`0x0047B6B4`，
均只作为待续站点。Group-A的`+0x0E14`取既有`group_a_target_phases[index].decoded_resource_token`；
Group-B的既有`group_b_target_phases[group_b_index][group_a_index]`由目标索引选出，
且其产生路径（`legacy_battle_action_dispatch.cpp`）把发射器放在`actor+0x0E6C`，**不是此处所读`+0x0E14`**；
不能借用作本物理字段owner。所以Group-B resolver保持空指针并在该读typed-stop，测试中仅显式注入本字段owner验证后续前缀，
不宣称真实Group-B caller已接线。两次独立读障/三向量与resolver映射于2026-09-24 01:19 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_e2fb`，
退出码0）。此处不代表case2的初始化/释放及粒子共尾完成。

case2 phase100从`0x00479C6C`继续按序写phase word0、将ECX设38且EAX清零、写`+0x2A12` word0，
然后由入站EDI=`actor+0x3D0`依真实DF逐次REP清38 dword；**此后**才读`+0x0E14` token并将EDI改为该字段地址。

DF0覆盖slot2的0x98字节，DF1先清slot2首dword、再逆向清slot1的最后37 dword；缺Group-B目标相关owner时停止在`0x00479C83`且保留全部40项先前写。
零token停在待清22 dword的`0x00479C9C`；非零token停在待压外参的`0x00479C93`，并未执行`sub_4885A0`。

正反DF、第二次REP障、缺owner与零/非零token向量于2026-09-24 01:25 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_c143`，
退出码0）。

case2 非零token从`0x00479C93`再独立压外参、`0x00479C94`压CALL返回地址，窄释放port只能模拟`sub_4885A0`入口停止或正常回复：
入口停止保留全部前置38写、原token及两个栈项；正常回复只弹CALL返回槽、由父`0x00479C99 add esp,4`清外参，
MOV ECX22后到仍**未执行**的共享`0x0047B814`。零token不经过port直接汇到该站。实参/CALL双栈障、wrapper入口停、正常合流与零token向量于2026-09-24 01:29 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_73a7`，
退出码0）；wrapper内部9条及`sub_4885C0`深层CRT尚未建模，22次粒子REP、reset/返回均未完成。

case2共享尾`0x0047B814/0x0047B816`现在把Group-A已证实的target-phase emitter 22个dword物化到`actor+0x0E14..+0x0E6B`，
对逐次清写独立同步`decoded_resource_token`、宽高、位置、flags与计数字段；当首dword换token时不继续借用旧pixel span。

DF0且完整Group-A owner可逐次清22项，抵达**待执行**的`0x0047B81A sub_478850`；Group-B仅显式token owner能清第一项，
第二项`+0x0E18`因没有已证物理owner停住；DF1第二项`+0x0E10`越出该发射器区亦停住，不用零填充伪造上游区域。

Group-A/Group-B缺口及逐写向量于2026-09-24 01:39 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_14db`，
退出码0）；完整case2仍取决于callee与后续particle尾。

case2 Group-A显式owner的零token共享清写后，现将同一`case_reset_call_ready`交给已经typed的`0x0047B81A sub_478850`共享返回尾；
正常子调用应返回EAX1、恢复原CALL栈，原22项已清不会被子调用复活。该**局部组合**向量于2026-09-24 01:42 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_a818`，
退出码0），不能替代从四处真实caller经phase100全路由的最终审计。

case2 非终结零token初始化分支另从`0x00479B1F`开始设ECX22、XOR EAX、EDI=`actor+0x0E14`、EDX=`ESP+0x18`，
在不同于终结尾的`0x00479B30`逐次清发射器22 dword；Group-A有完整owner可到`0x00479B32`三个解码输出栈槽LEA之前，
Group-B DF1的第二项`+0x0E10`仍因未证owner停止。token后/宽高前故障与完整清写向量于2026-09-24 01:45 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_4972`，
退出码0）；`sub_4019A0`、后续字段初始化与粒子尾尚未实现。

case2 清写后另从`0x00479B32`按物理顺序求三个栈输出指针，压第一个、读`actor+0x2548`、压剩余两个、读取同token资源首dword、压该值，
停在尚未执行的`0x00479B46 sub_4019A0`；若缺首dword owner，则已有三次PUSH而未压第四项。六处物理栈/actor/资源访问及缺源向量于2026-09-24 01:50 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_149c`，
退出码0）。decoder返回的栈输出字段和后续emitter发布尚未宣称覆盖。

case2 `0x00479B46 sub_4019A0` 现独立压CALL返回槽，窄解码port仅支持callee入口停或正常回复；
正常回复不清四参，随后`0x00479B4B`才写`actor+0x0E14`，并把port显式提供的decoded pixels span发布到已证Group-A phase owner。

CALL压栈障、callee入口停、正常回复、后置actor写障及写后owner向量于2026-09-24 01:56 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_955b`，
退出码0）。该边界并未核定`sub_4019A0`内部输出栈写/异常，也尚未执行`0x00479B51`源token重读及后续初始化。

case2 解码token写后现严格在四个cdecl实参仍在栈时执行`0x00479B51`第一次`+0x2548`重读，**随后**`0x00479B57 add esp,0x10`才清参；
从同token资源`+0x0C`读word并写emitter`+0x0E18`，第二次重新读`+0x2548`、资源`+0x0E`后写`+0x0E1A`。

六项独立访问均可停，若后续token变化则不会借前一header；正常到`0x00479B76`几何段前。于2026-09-24 02:00 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_a647`，
退出码0）。pixel span是由port显式提供的已知资源，不能仅由非零token推断。

case2 `0x00479B76..0x00479BC7` 几何前缀现按十二处独立访问读取X、写`+0x0E1C=X-EBP`，
读取slot2 Y偏移/Y、写`+0x0E20=Y-Oy`，重读资源token/X/宽、写`+0x0E24=X+(W>>1)-EBP`，
再重读资源token/Oy/高，保留EBP已变为Oy并停在尚未执行的`0x00479BCB` flags byte RMW。宽度缺owner的停止发生在前两项坐标写后。

十二序障、三次写前缀与数值向量于2026-09-24 02:05 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_63a5`，
退出码0）。

case2 `0x00479BCB..0x00479C13` 先对emitter`+0x0E3C`低byte独立读/写OR`0x16`，
再读Y、半高减Oy、按源序写`+0x0E30=2/+0x0E2C=Y+(H>>1)-Oy/+0x0E28=2/+0x0E36=50/+0x0E34=250/+0x0E38=1/+0x0E3A=10`，
其中非offset升序。正常到尚未执行的`0x00479C1C sub_47CE70`，十个物理读/写障点逐项检查已提交flags/字段；
于2026-09-24 02:09 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_9088`，
退出码0）。不能把此处OR字节当作完整word覆盖。

case2 `0x00479C1C` 的`sub_47CE70`只有三个LST指令（`0x0047CE70 movsx eax,byte ptr [ecx+2694h]`、`0x0047CE77 and eax,1`、`0x0047CE7A retn`），
现用已显式映射的render flags低byte及独立CALL压栈/child单byte读/RET栈读来执行；parent `cmp eax,1`后仅1读/写OR emitter`+0x0E3C`低byte1，
0跳过，停在尚未读取的GetSystemMetrics IAT`0x00479C2C`。五处逐访问障与清bit0路径于2026-09-24 02:16 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_1c29`，
退出码0）；不以任意EAX回调伪作该三指令callee。

case2 `0x00479C2C..0x00479C3B` 引入显式IAT指针owner及Win32 `GetSystemMetrics`窄port：
一次IAT读取、`push 1`/间接CALL压返回槽、stdcall正常清index与返回槽、保留第一次结果、`push EBX`/第二次CALL及第二次结果，
ECX装全局矩形对象`0x0053B0B8`，停在尚未执行的`0x00479C40 sub_433F30`。七项LST访问和第二次callee入口停止前缀已测向量；
未知IAT立即停，无声明Win32内部实现。于2026-09-24 02:20 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_446a`，
退出码0）。

case2 非零既有emitter直接进公共`0x0047B6B4`：现先从明确owner读取`dword_4CD76C`，LEA actor`+0x0E14`、独立PUSH发射器地址和刚读全局，
ECX设`0x0053B0B8`，停在尚未执行的`0x0047B6C7 sub_434790` CALL。三处读/栈写障及缺全局owner停止向量于2026-09-24 02:25 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_0df9`，
退出码0）。尚未表示粒子callee真实副作用或本case的最终返回。

case2 `0x00479C40 sub_433F30` 现仅附窄host-surface port：两个先前真实栈槽分别保留width/height，
CALL返回槽PUSH故障不进入callee，入口停止保留两参+返回槽；port确证正常返回时才应用`retn 8`清两参并继承EAX/ECX/EDX/FLAGS，
停于尚未执行的`0x00479C45 actor+0x428`动作码写。**这不是host helper的内部逐访问/共享几何状态门禁**；
该处仍partial。于2026-09-24 02:30 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_a316`，
退出码0）。`actor+0x428`实际别名slot2 `+0x3D0` 内 `LegacyActionRecord::field_58`（`0x3D0+0x58=0x428`），
已由action-record image的通用同步承载；不得额外创建第二份owner。

case2 `0x00479C45..0x00479C54` 现先写slot2 `LegacyActionRecord::field_58=0x31`（actor`+0x428`），
之后重新读`dword_4AB784`显式shared owner，分别压样本handle和0x31，停在尚未执行的`0x00479C56 sub_485610`。

四项独立故障中第二、三、四处均应保留已写动作码。于2026-09-24 02:36 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_98bb`，
退出码0）；未把音频callee内部伪作已执行。

case2 `0x00479C56` 狭义音频port现在单独执行CALL返回槽PUSH，入口停止保留动作码word与样本两参但尚未写phase；
正常返回后caller于`0x00479C5B add esp,8`才清参并置flags，`0x00479C5E`再单独写phase word1，
转与非零既有emitter相同的`0x0047B6B4`粒子尾。CALL/phase写前后状态向量在`proc_6008`首次因测试局部变量重名编译失败，
修复后于2026-09-24 02:41 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_7ba2`，
退出码0）；音频wrapper及内部Miles行为未由此port证明。

当前WIP至case2音频/phase接线的**临时**sanitizer：2026-09-24 02:44 UTC受管`./build-asan.sh --test`成功，CTest`199/199`（`proc_5ac2`，退出码0）。后续新代码需重新跑最终sanitizer；此局部成功不等于完整WP316或全Pi目标验收。

case2 公共`0x0047B6C7 sub_434790`现单独CALL返回槽，缺canonical emitter phase owner在callee入口typed-stop；
窄port可针对已接typed phase应答正常`retn 8`，按已关闭callee正常结果限制EAX∈{0,1}，`cmp eax,1`时0进原default epilogue、1停在尚未执行的`0x0047B6D5 phase=100`。

传入全局值来自先前真实PUSH槽，不从变动request再次快照。CALL障、owner缺失、port入口停与0/1/无凭据2向量已加，
于2026-09-24 02:49 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_2334`，
退出码0）。此port尚不证明内部node/随机数/屏幕访问路径。

case2 粒子正常回复1现在独立在`0x0047B6D5`写phase word100后按`0x0047B6DE/DF/E0/E3/E7`逐站点POP寄存器、XOR EAX0、清20 byte局部并RET；
粒子正常0复用`0x0047A80B`默认尾而不写100。phase写与五项独立栈读取故障前缀于2026-09-24 02:52 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_9e0f`，
退出码0）。此结果只覆盖case2条件性尾；其余selector及四处真实caller仍未完成。

case9 `0x0047A752` 现独立按signed phase与45比较：44/-1/-32768停在尚未执行的活动前缀`0x0047A763`；
45/32767先于`0x0047A253`写phase=BX、`0x0047A25A`写motion word=BX，再进入共用reset的**第二写点**`0x0047B808`，
不可重复执行`0x0047B801`。该共用前缀按路径选择跳过首写，其后progress与38次REP次序不变。读、两次字写故障及共用REP共39项检查的首轮`proc_ea93`因测试缺u16别名编译失败，
修复后于2026-09-24 02:59 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_d43b`，
退出码0）。

case9 `0x0047A763..0x0047A776` 现比较AX与BX：非零phase立即停在尚未执行的`0x0047A779`帧源读取；
phase0才独立读实时sample global入EDX、先压EDX后压0x31并经窄音频port保留CALL返回槽与正常清8 byte实参。

正常回复与入口typed-stop各保留完整EAX高16位（至真正callee回复前），不预读帧源。于2026-09-24 03:03 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_ec71`，
退出码0）；仍不是case9完整活动绘制。

case9 `0x0047A779..0x0047A7AB` 现独立读`+0x2548`、压BX完整dword、从匹配token读资源首dword、写canonical `turn_frame_source_token`，
再次实读当前signed phase并执行原`0x55555556`单操作数IMUL的EDX:EAX高字算法，先写`draw_opacity(4CD724)`后写`special_render_mode(4CC2F0)`。

向量phase2/3/44/-1/-3的等级应依次1/2/15/1/0；七处内存访问故障分清首global与后两项提交。停在尚未执行的`0x0047A7B1` render flags读，
于2026-09-24 03:07 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_0ce5`，
退出码0）。

case9 `0x0047A7B1..0x0047A7EF` 现分别读render flags、重读frame token与资源高/宽、signed Y/源Y offset/slot0`+0x2B4`/signed X，
五次物理PUSH依序冻结(flags|0x14,H,W,Y−offset,X−offset)，另保留上段已压EBX=0作为第六参，
停在未执行的`0x0047A7F0`绘图CALL。八次读与五次栈写共13独立故障向量、缺资源高owner停点已加，于2026-09-24 03:12 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_9b14`，
退出码0）；绘图callee内部不在此段证明。

case9 `0x0047A7F0 sub_4170E0` 现把前述六项真实栈参数按callee顺序传入绘图窄port，CALL返回槽失败或callee入口停时六参及phase原值均不清/不改；仅正常回复后于`0x0047A7F5 add esp,0x18`清参，停在尚未执行的`0x0047A7F8`首绘图全局清零。入口/回复寄存器及两类栈故障已测，受管`proc_1b7a`待核；此port不替代`sub_4170E0`的像素表间接CALL深层审计。

case9 首绘图正常返回后的`0x0047A7F8/FE`现依原序分别清`special_render_mode(4CC2F0)`和`draw_opacity(4CD724)`，
`0x0047A804`对phase执行独立word读/写INC，保留原CF，写成后只进入既有default POP/RET返回零。

两全局及INC双访问的四处障点、`0xFFFF→0`回绕与默认尾已测，于2026-09-24 03:18 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_e6fa`，
退出码0）。完整case9仍依赖绘图callee深层路径及真实caller接线审计。

case8 `0x0047A5FE/0x0047A60B` 现按原指令两次分别CMP actor phase word与CX=100、固定emitter token dword与EBX=0，
phase100直接进入已实现case2共享释放`0x00479C6C`而不读emitter，非终结非零token直接进共享粒子尾`0x0047B6B4`，
零token停在case8独立初始化`0x0047A617`；两项CMP均不修改EAX高低位。两处故障、三出口及Group-B缺owner向量已加，
于2026-09-24 03:21 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_2103`，
退出码0）。case8初始化还未实现。

case8 零token初始化的22次独立REP已在同一物理写owner/DF逻辑中加入口区别：`0x0047A617`设ECX22、EAX0、EDI固定emitter、EDX栈var_C；
写障EIP必须是case8独有`0x0047A628`，成功停在`0x0047A62A`，不冒充case2的`0x00479B30/32`。

Group-A canonical owner DF0第1/末项与DF1第二项越界前缀已加；于2026-09-24 03:24 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_9141`，
退出码0）。case8 decoder参数及其后初始化仍未执行。

case8 decoder入参段`0x0047A62A..0x0047A63D`现沿用经过证明的四个局部输出槽地址和物理访问次序，
但六处PUSH/actor token/source head站点全部使用case8自己的EIP，正常只停在未执行的`0x0047A63E sub_4019A0`。

四参数栈值、六处逐访问停止于2026-09-24 03:28 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_7c76`，
退出码0）；不可把case2的`0x00479B3A..45`故障地址照搬。

case8 `0x0047A63E` decoder CALL现含return slot故障、callee入口停与正常回复三路，返回地址`0x0047A643`单独区别case2；
CALL正常回复后四实参仍在栈，独立actor `+0x0E14` token写于`0x0047A643`，正常停在`0x0047A649`待frame token重读。

callee内部仍只窄port，不证明深层副作用；已加零提交/写故障/正常发布和显式像素span回归，受管`proc_9333`因测试局部变量重名编译失败，
已改名重启`proc_ccbf`，于2026-09-24 03:34 UTC受管`./build.sh core --test`及CTest`199/199`通过（退出码0）。

case8 `0x0047A649..0x0047A667`现分别读actor来源token、执行16字节栈清理、读资源`+0x0C`/写emitter宽、再读actor来源token与资源`+0x0E`/写emitter高；六个访问以case8 EIP单独判停，提交前缀覆盖两项尺寸和栈时序。局部回归已加，于2026-09-24 03:38 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_a9ad`，退出码0）。

case8 `0x0047A66E..0x0047A6AF`现单独实现：D66有符号X减EBP→E1C、3E4高度偏移与D68有符号Y之差→E20、E24先写-30、CMP 2B08==1再可选覆盖670。
七处常规访问、可选第八处与写提交前缀/FLAGS的测试已加，于2026-09-24 03:42 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_e40c`，
退出码0）。其后`0x0047A6AF`算术/粒子字段配置仍未实现。

case8 `0x0047A6AF..0x0047A705`续接E20源Y读、减10→E2C、E28=2、source token与已知资源高、E3C低字节OR 0x16独立RMW读写、E30高+10、E36/E34=100、E38=1、E3A=10；
共12个物理访问按站点故障。正常只停在未执行的`0x0047A70E sub_47CE70`。局部正常与12处故障前缀已加，于2026-09-24 03:46 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_304d`，
退出码0）。

case8 `0x0047A70E sub_47CE70`采用同一三指令属性叶子但保留独立return地址`0x0047A713`及CMP EAX,EBP=1；
条件真时`0x0047A717 OR word [E3C],BP`是16位RMW，不是case2的8位OR。零/一属性、CALL栈故障、word写故障及高字节保留向量已加，
于2026-09-24 03:51 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_91f4`，
退出码0）；正常停在case8 Metrics IAT读`0x0047A71E`。

case8 `0x0047A71E..0x0047A72B`复用两次Metrics窄port但各IAT/PUSH/CALL/返回值PUSH使用case8各自的七处地址；
Win32 stdcall返回清参数加CALL槽，保留高度、宽度两实参，正常只停在`0x0047A731 sub_433F30`之前。

独立调用顺序1/0与七个故障点向量已加，于2026-09-24 03:55 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_5ecd`，
退出码0）。

case8 `0x0047A731 sub_433F30`现保留CALL栈故障、入口停和正常RET8的三类边界，独立返回地址为`0x0047A736`，正常立即停在全局音频样本读取，不加入case2独有的actor `+0x428`动作码写。窄port不证明callee内部；局部回归已加，于2026-09-24 03:58 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_ae56`，退出码0）。

case8 rectangle之后全局样本读`0x0047A736`、两个PUSH`0x0047A73B/3C`、`0x0047A73E sub_485610`返回地址`0x0047A743`、参数清8、`0x0047A746` actor phase=1现均按单独EIP推进；
正常跳公共粒子尾`0x0047B6B4`，不会执行case2特有的+428字段写。callee只为窄port。样本读故障、CALL入口/返回、phase写故障和共享尾向量已加，
于2026-09-24 04:02 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_c71e`，
退出码0）。

case8 零token新建粒子已从前缀一直接到共享`0x0047B6B4`实参、`0x0047B6C7 sub_434790`窄port及EAX0/1两条真实栈返回：
EAX1写phase100，EAX0默认不写；测试亦核对decoder token的canonical owner。于2026-09-24 04:04 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_25ff`，
退出码0）；不能以该窄port证明粒子callee内部、副作用及跨branch完整审计。

case8另外两出口已串联验证：非零token从`0x0047A60B`直接进入共享粒子CALL并沿EAX0默认返回；phase100在`0x00479C6C`执行两word写、38次REP、旧token读取、`sub_4885A0`窄释放、22次固定emitter REP直至共享reset CALL `0x0047B81A`。两出口及canonical owner回归已加，2026-09-24 04:04 UTC 受管
`./build.sh core --test`及 CTest `199/199`通过（`proc_a347`，退出码0）；
重置callee内部及完整release异常路径仍属另行审计。

case6 `0x0047A1A0..0x0047A1AB` 先从 actor `+0x2958` 读取完整 word，
保留 EAX 高16位，按有符号 word 比较 `0xFFE0`。phase `<=-32`（包括
`-32768`、`-33`、`-32`）在共享 `0x0047A253/0x0047A25A` 顺序清
主/辅 phase，再自 `0x0047B808` 清进度与38个 dword；`-31/0/-1/32767`
保留 active 入口 `0x0047A1B1`。三处独立读/写故障与共享 reset
前缀测试于2026-09-24 04:11 UTC受管 `./build.sh core --test`及
CTest `199/199`通过（`proc_a5ac`，退出码0）。

case6 活动期 `0x0047A1B1` 单独比较 AX 与 BX；非零直接进
`0x0047A1C6`，不需要共享 sample owner。phase0 在 `0x0047A1B6`
读当时的样本句柄进入完整 EAX，再按 `0x0047A1BB/BC` 分别压
句柄与 `0x31`；`0x0047A1BE sub_485610` 的返回槽为
`0x0047A1C3`。CALL 栈故障、callee入口停及正常回复后清两参，
于2026-09-24 04:25 UTC核实受管 `./build.sh core --test` 与
CTest `199/199`通过（`proc_6fc1`，退出码0）。窄 sound port
不代表 callee 内部访问。

case6 源首段 `0x0047A1C6..0x0047A1F5` 先读 actor 源 token、
压 EBX=0，再按匹配 token 读取资源首 dword 发布全局 `4CD730`。

三次分别从 `+0x2958` 重新读 signed word，按序写全局
`4CD71C/4CD30C/4CD304`，保留原 phase 而非取负。十个物理
访问故障及缺共享 owner 的栈前缀测试，于2026-09-24
受管 `./build.sh core --test`及 CTest `199/199`通过（`proc_ba78`，
退出码0）。下一站是 `0x0047A1FB` 的绘制参数读取。

case6 `0x0047A1FB..0x0047A239` 另以独立访问读取
`actor+0x2694/+0x2548/+0x2B4`；仅把 render flags 的参数副本
OR4，不写回 actor。以零扩展读取匹配 frame 的 H/W，分别读取
signed `+0x29B2/+0x0D68/+0x0D66`，借已压的 EBX=0 形成
`(X-29B2,Y-2B4,W,H,flags|4,0)` 六参。13处读/PUSH 故障及缺来源高 word 的停止前缀，于
2026-09-24受管 `./build.sh core --test`与 CTest `199/199`
通过（`proc_0b55`，退出码0）。

case6 `0x0047A23A sub_4170E0` 的 CALL 返回槽独立故障、
callee入口停止和正常回复均按窄绘图 port 建模；正常回复才
清除六个栈参数，停在 `0x0047A242` phase DEC 前。

该口不证明绘图子函数深层访问；局部回归于2026-09-24
受管 `./build.sh core --test`及 CTest `199/199`通过（`proc_79b5`，
退出码0）。后续 `0x0047A242` 为真实 word DEC 独立读/写：保留
CF 后依次 POP EDI/ESI/EBP、XOR EAX=0、POP EBX、清20字节
局部与 RET `0x0047A252`。七处读/写/栈故障和 phase word 回绕
向量于2026-09-24受管 `./build.sh core --test`及 CTest
`199/199`通过（`proc_e19b`，退出码0）；整条case6活动路径
仍受绘图 callee 深层副作用边界约束。

case11 `0x0047A94D` 单独从 actor `+0x2958` 读 signed word，
门限同为 `<=-32` 但活动 successor 是 `0x0047A95E`；终结
分支复用 `0x0047A253/0x0047A25A` 顺序写与公共 reset。

三个独立读/写故障向量于2026-09-24受管 core/CTest
`199/199`通过（`proc_56e3`，退出码0）。phase0 的音频
`0x0047A963/0x0047A968/0x0047A969/0x0047A96B`
分别读 sample/PUSH/PUSH/CALL，入口停不冒充返回，正常回复
才在 `0x0047A970` 清参、汇到 `0x0047A973`；局部
core/CTest `199/199`通过（`proc_3e84`，退出码0）。

case11 源首段先写 `4CC2F0=15`、`4CD730=frame[0]`，
再比较 signed phase 与 `-16`；`<=-16` 时写
`4CC2F0=phase+31`。六处读/写故障与边界回归于
2026-09-24受管 core/CTest `199/199`通过（`proc_743b`，
退出码0）。第一绘图 `0x0047A9A3..0x0047A9E2` 的
14处 actor/frame/栈访问及六参前缀回归于2026-09-24
受管 core/CTest `199/199`通过（`proc_1ada`，退出码0）。

第一次绘图 `0x0047A9E3` CALL返回槽与 callee入口停止，
正常回复后不清六参、改在 `0x0047A9E8` 继续；callee FLAGS
若未知不得假借调用前的 FLAGS。局部回归于2026-09-24
受管 core/CTest `199/199`通过（`proc_c52a`，退出码0）。

两绘图之间 `0x0047A9E8..0x0047AA0F` 先清全局
`4CC2F0`，在前六参仍留栈时再压 EBX=0，随后按三次
`+0x2958` signed 读写 `4CD71C/4CD30C/4CD304`；八处
故障前缀于2026-09-24受管 core/CTest `199/199`通过
（`proc_9747`，退出码0）。第二次绘图的14处独立读/PUSH
含 phase×10 的 Y 坐标构造，于2026-09-24受管
core/CTest `199/199`通过（`proc_eabe`，退出码0）。

第二次 `0x0047AA62 sub_4170E0` CALL入口停与正常回复
清除两组共48字节栈参数、抵达 `0x0047AA6A` phase DEC
的局部回归于2026-09-24受管 core/CTest `199/199`通过
（`proc_9083`，退出码0）。`0x0047AA6A` 物理 DEC word
保持 CF，四处 POP 与 `0x0047AA7A` RET 的七处独立
读/写/栈故障于2026-09-24受管 core/CTest `199/199`
通过（`proc_35fb`，退出码0）。case11 的 caller 窄路径
已到返回；`sub_4170E0` 两次调用的 callee 深层行为仍须
最终 REVIEW。

case15 `0x0047B2E8` signed `<=-32` 仍复用共享终结
`0x0047A253/0x0047A25A`，活动 successor 独立为
`0x0047B2F9`；局部 core/CTest `199/199`通过
（`proc_a295`，2026-09-24，退出码0）。phase0 音频
`0x0047B2FE/B303/B304/B306` 的三访问与 CALL 边界
单独测试，于2026-09-24受管 core/CTest `199/199`通过
（`proc_7953`，退出码0）。源首段只在 signed phase `>=-15`
时写 `4CC2F0=phase+15`，`-31` 时保持旧值，而非套用
case11 的预先写15；五处故障和阈值向量于
2026-09-24受管 core/CTest `199/199`通过（`proc_a0e8`，
退出码0）。

case15 首绘图的14处 actor/frame/栈访问于
2026-09-24受管 core/CTest `199/199`通过（`proc_2c68`，
退出码0）。`0x0047B374` CALL 保留首组六参及绘图前的
模式值；`proc_c4fd` 的局部断言因 mock 默认回传
`flags_known=true`、却要求未知而失败（CTest `198/199`）。

测试已将该向量的 mock 改为未知；首CALL与中间段八处
故障于2026-09-24受管 core/CTest `199/199`通过
（`proc_38b8`，退出码0）。第二次绘图以 signed phase×2
作用于 X，而非 case11 的 phase×10 作用于 Y；14处访问
与六参前缀于2026-09-24受管 core/CTest `199/199`
通过（`proc_4723`，退出码0）。`0x0047B3F0` 第二次
CALL 的物理入口与48字节清栈于2026-09-24受管
core/CTest `199/199`通过（`proc_3f92`，退出码0）。

`0x0047B3F8` DEC word 及 `0x0047B401..0x0047B408`
四 POP/RET 的独立七处故障、CF 保留于2026-09-24
受管 core/CTest `199/199`通过（`proc_d760`，退出码0）。

case11 的 -17/-16 与 case15 的 -16/-14 模式边界于
2026-09-24受管 core/CTest `199/199`通过（`proc_b3ed`，
退出码0）；两 case 的 callee 深层访问仍不在上述局部测试覆盖内。

case50 `0x0047B747` signed phase `<15` 活动、`>=15`
进入共享 `0x0047B801` 先清主 phase 而不清辅 word；
读故障、14/15/-1 及共享 reset 前缀于2026-09-24
受管 core/CTest `199/199`通过（`proc_4654`，退出码0）。

phase0 的动作码音频 `0x0047B75D..0x0047B775` 先读
slot2 `actor+0x428` word 到 AX（保留高16）、再写 slot1
`+0x390=0x31`、读共享样本并压样本/完整EAX；五处
读/写/PUSH 故障与独立 CALL 停止于2026-09-24
受管 core/CTest `199/199`通过（`proc_c1bd`，退出码0）；
高16位 `0xCDEF` 是合成寄存器向量，不证明真实调用者会
产生该值。源首段 `0x0047B77D..0x0047B79A` 的零
辅助 PUSH、frame[0] 及 signed `15-phase` 全局发布，
六处访问故障于2026-09-24受管 core/CTest `199/199`
通过（`proc_3a77`，退出码0）。

`0x0047B7A0..0x0047B7E7`
的表是 LST 实字节：15项以及紧邻的 -5..-1 映像已
有明确 dword；超过已知映像不臆造值。14处独立读/PUSH
故障、phase14 的 table=-32、-1=0、-3=2、-6 未知
读障于2026-09-24受管 core/CTest `199/199`通过
（`proc_bbef`，退出码0）。`0x0047B7E8 sub_4170E0`
窄 CALL入口及清六参于2026-09-24受管 core/CTest
`199/199`通过（`proc_e88f`，退出码0）。case50 的
`0x0047B7F0..0x0047B800` word INC、XOR/POP、
`ADD ESP,0x14` 与物理 RET 的初测 `proc_8150`
为198/199，暴露已有 case6/11/15 测试将 XOR
`eax,eax` 后的 CF 误判为 DEC 保留的 CF；对照
LST 四条路径均在 POP 前 XOR，修复测试与执行
顺序后于2026-09-24由 `proc_6321` 验证 core/CTest
`199/199`、退出码0。

case51 `0x0047B83E..0x0047B851`
signed phase 门初测 `proc_4e46` 为198/199：测试
入口漏置 LST 前驱加载的 CX=100；修正向量后于
2026-09-24由 `proc_1d0f` 验证 core/CTest
`199/199`、退出码0。case51 的 `+0x0DD8/0x0DDC`
两处 dword 现由 actor action execution 唯一持有，并
显式映射物理镜像及逐字节写同步；`0x0047B857..0x0047B873`
逐访问故障、源 token、初始值0x400及资源宽读于
2026-09-24受管 core/CTest `199/199`通过
（`proc_8c0f`，退出码0）。

`0x0047B877..0x0047B8CA`
的13处物理读写、资源重复读取、signed 坐标与
后续字段的唯一 owner 映射于2026-09-24受管
core/CTest `199/199`通过（`proc_88aa`，退出码0）。

`0x0047B8D0 sub_47CE70` 的 CALL/子首读/RET、
比较 EAX==1 和 `+0x0DE0` 单字节 OR 的双访问前缀
于2026-09-24受管 core/CTest `199/199`通过
（`proc_e45f`，退出码0）。`0x0047B8E0..0x0047B907`
的四次 decoder 参数 PUSH、`+0x0DF0=0x5A`、
资源首 dword、子 CALL停点与回址由受管 `proc_21b5`
于2026-09-24受管 core/CTest `199/199`通过
（`proc_21b5`，退出码0）。`0x0047B907..0x0047B92C`
的 token、动作码、音频句柄、子 CALL 入口和六参清栈
于2026-09-24受管 core/CTest `199/199`通过
（`proc_879d`，退出码0）。

`0x0047B92C..0x0047B97F`
两处 scale 读取、phase 加2的 RMW、三处5值、两处
scale 加4、`4CD76C` 读取及双参数压栈由受管
于2026-09-24受管 core/CTest `199/199`通过
（`proc_8fac`，退出码0）。`0x0047B97F` 已明确
CALL返址0x0047B984，子 `sub_4344E0` 内部尚未
复原，不能声称已正常返回。case51 phase100 入口
`0x0047B98E..0x0047B9A7` 两次 word 写和38次
REP 写前缀由受管 于2026-09-24受管 core/CTest `199/199`通过
（`proc_e0ad`，退出码0）。

`0x0047B9A7..0x0047B9CA`
重置子 CALL、与共享尾不同的三写顺序、物理 POP/RET
初测 `proc_f830` 为198/199；`proc_10bb` 拆分
断言确认 CALL 停点与子首访问停点无误，失败限于
正常返回向量。测试用例手工构造的 selector ESP
是入口 ESP-0x28，不能套用另一个真实入口向量
`entry_esp+4`；已改为从该前缀物理栈加40，
于2026-09-24受管 core/CTest `199/199`通过
（`proc_5753`，退出码0）。

case3 `0x00479CA6` 的
signed phase `>32`、相等、负值和零音频入口
同步核测。`0x00479CBC..0x00479CCD` 的音频双参、
子停点与清栈于2026-09-24受管 core/CTest
`199/199`通过（`proc_6329`，退出码0）。

`0x00479CCD..0x00479CD7` 资源首读与共享 draw 源
发布于2026-09-24受管 core/CTest `199/199`通过
（`proc_f90c`，退出码0）。case3
`0x00479CDD..0x00479D39` 的两次局部栈读写、
资源宽高、signed 坐标和首组矩形四参的16处独立
访问于2026-09-24受管 core/CTest `199/199`通过
（`proc_34eb`，退出码0）。`0x00479D39 sub_416FF0`
仅建模真实 CALL 栈写与子入口停点，由受管
于2026-09-24受管 core/CTest `199/199`通过
（`proc_2756`，退出码0）；子内部未审完不得
伪作正常回复。

case4 `0x00479EAA..0x00479EC0`
的 signed phase 门于2026-09-24受管 core/CTest
`199/199`通过（`proc_2d75`，退出码0）。

`0x00479EC0..0x00479ED0` 音频的全32位 EAX
句柄、CALL 与双参清栈于2026-09-24受管 core/CTest
`199/199`通过（`proc_998d`，退出码0）。

`0x00479ED0..0x00479EE0` 资源首 dword 的 EDX
发布于2026-09-24受管 core/CTest `199/199`通过
（`proc_78e7`，退出码0）。case4
`0x00479EE0..0x00479F37` 15处资源、actor、
局部栈与矩形四参访问，以及首个 `sub_416FF0`
真实 CALL 子停点于2026-09-24受管 core/CTest
`199/199`通过（`proc_6439`，退出码0）。

case3/4 的 phase=-1、signed 坐标和两套不同矩形
几何于2026-09-24受管 core/CTest `199/199`通过
（`proc_7f2c`，退出码0）。case5
`0x0047A083..0x0047A0A6` 的 signed phase 与资源
高度门于2026-09-24受管 core/CTest `199/199`
通过（`proc_4729`，退出码0）。case5
`0x0047A0A6..0x0047A0B7` 音频参数与真实 CALL
子停点于2026-09-24受管 core/CTest `199/199`
通过（`proc_4105`，退出码0）。

case5
`0x0047A0B7..0x0047A0C6` 的 EBX 实参栈写、资源
首读与共享 draw 源发布于2026-09-24受管
core/CTest `199/199`通过（`proc_4292`，退出码0）。

`0x0047A0C6..0x0047A0FB` 的12处资源/actor/
PUSH 前缀及首个六参绘图入口于2026-09-24
受管 core/CTest `199/199`通过（`proc_e069`，
退出码0）。`0x0047A0FB sub_4170E0` CALL、
绘图后再次读 actor+2548、清六参和不同停点
于2026-09-24受管 core/CTest `199/199`通过
（`proc_38ac`，退出码0）。`0x0047A109..0x0047A128`
的资源高16保留、signed phase 再比较与条件 AX 写
于2026-09-24受管 core/CTest `199/199`通过
（`proc_751d`，退出码0）。

`0x0047A128..0x0047A13D`
的 EBX 辅助 PUSH 和三次分立 `-24` global 写由
于2026-09-24受管 core/CTest `199/199`通过
（`proc_4629`，退出码0）。`0x0047A13D..0x0047A175`
的第二绘图六参和12处单独访问由受管
2026-09-24受管 core/CTest `199/199`通过
（`proc_b995`，退出码0）。`0x0047A175`
第二绘图 CALL 的回址和子入口停点由受管
于2026-09-24受管 core/CTest `199/199`通过
（`proc_01c3`，退出码0）。

`0x0047A17A..0x0047A18B` 的裁剪四实参和
`sub_416FF0` 子入口停点由受管 `proc_81ba`
于2026-09-24受管 core/CTest `199/199`通过
（`proc_81ba`，退出码0）；不把子入口停点
冒充正常返回。case10 `0x0047A815..0x0047A838`
的 signed phase/资源高度门由受管 `proc_3f4d`
于2026-09-24受管 core/CTest `199/199`通过
（`proc_3f4d`，退出码0）。case10 phase0
`0x0047A838..0x0047A849` 的音频参数、CALL 与
子停点于2026-09-24受管 core/CTest `199/199`
通过（`proc_6aae`，退出码0）。

case10
`0x0047A849..0x0047A858` 的 EBX PUSH、资源首读与
共享绘图源发布于2026-09-24受管 core/CTest
`199/199`通过（`proc_8631`，退出码0）。

case10 `0x0047A858..0x0047A88D` 首绘图12处独立
访问于2026-09-24受管 core/CTest `199/199`通过
（`proc_36d0`，退出码0）。case10 首绘图
`0x0047A88D` 的 CALL 与 `0x0047A892` 返回后
再次读取 actor+2548 于2026-09-24受管
core/CTest `199/199`通过（`proc_4ccb`，退出码0）。

case10 首绘图后 `0x0047A89B..0x0047A8BA`
重读高度、signed phase 夹值及 full32 DEC 的 OF
于2026-09-24受管 core/CTest `199/199`通过
（`proc_2448`，退出码0）。case10
`0x0047A8BA..0x0047A8D0` 的第二绘图 `0/0/16`
三次分立 global 写与不提前压 EBX 由受管
2026-09-24受管 core/CTest `199/199`通过
（`proc_e026`，退出码0）。case10
`0x0047A8D0..0x0047A90A` 的第二绘图13处访问、
`phase+1` 高度参数于2026-09-24受管 core/CTest
`199/199`通过（`proc_69d7`，退出码0）。

`0x0047A90A` 第二绘图 CALL、`0x0047A91B`
全屏裁剪 CALL 的真实子入口与栈由受管
于2026-09-24受管 core/CTest `199/199`通过
（`proc_ffda`，退出码0）；不声称裁剪子内部
已返回。case5/10 共享 `0x0047A935..0x0047A94C`
的 selector2/phase0 双写和物理 POP/RET 由受管
于2026-09-24受管 core/CTest `199/199`通过
（`proc_0931`，退出码0）。裁剪子函数
`0x00416FF0..0x00417047` 已以 canonical raster
owner 逐条保留15处读写/POP/RET，含负宽高和四项
global 的故障提交前缀于2026-09-24受管
core/CTest `199/199`通过（`proc_6e0e`，退出码0）。

case5/10 在裁剪实际返回后的 phase word 加14/4、
40字节清栈、保存寄存器 POP 与 RET 的首轮受管
`proc_35cb` 仅 `198/199`：新增共尾断言失败；
`proc_ce09` 分拆后仅正常出口失败。`proc_12f5`
定量确认两支均物理返回且 EAX0、phase 正确，
但 synthetic selector 前缀相对 entry_esp 少4字节：
测得 returned ESP=1244928，旧测试期待1244932。

断言改为从实际 caller 提供的 clip return ESP
增80字节，不伪造独立前缀的入口栈归一；
2026-09-24受管 `proc_e6e4` core/CTest `199/199`
通过。case12 的 signed 30门和 phase0 经 ECX 装载
音频样本于2026-09-24受管 `proc_0a5f` core/CTest
`199/199`通过。case12 `0x0047AAA2..0x0047AAF6`
四个 scaled-RLE canonical global 的逐访问发布、
32位有符号乘法低位回绕、极值故障向量由受管
2026-09-24受管 `proc_10a4` core/CTest `199/199`
通过。

case12 `0x0047AAF6..0x0047AB14/4A` 的
资源首 dword 发布、低 byte bit0 分流与两次 actor
源 token 读取首轮 `proc_1227` 在编译阶段失败：
新状态序号达到256，原 `LegacyBattleActorFrameEntryStatus`
的 `u8` 底层类型无法表示；仅将 typed host 诊断
状态改为 `u16`，2026-09-24受管 `proc_ca57`
core/CTest `199/199`通过。case12 forward/reverse
`0x0047AB14/4A..0x0047AB43/79` 的十处访问、
四个实际 writer 实参于2026-09-24受管
`proc_9bdf` core/CTest `199/199`通过。

两条
`sub_422C70/sub_423020` CALL 的真实回址和
子入口停点于2026-09-24受管 `proc_8ecd`
core/CTest `199/199`通过；尚不冒称 writer 已从
此 caller 正常返回。case12 phase30 时两个 word
按 `0x0047AB9A/A1` 清理再到 `0x0047B995` 的
故障前缀于2026-09-24受管 `proc_bb33`
core/CTest `199/199`通过。case12 专有
`0x0047B99C/0x0047B9A3` 的首 word 与38次 REP
单写前缀于2026-09-24受管 `proc_f45d`
core/CTest `199/199`通过。

共享 reset 现按 case12
独立回址 `0x0047B9AC`、实际写序 `+2AAC/+2ABC/+2AB8`
和 `0x0047B9C3..CA` 物理 POP/RET 接入。首轮
`proc_ce3f` 及诊断 `proc_d54a` core/CTest 均
`198/199`：测试错误地把子函数首访问设为
`0x00478856`；LST 首访问是 `0x00478850 PUSH EBX`，
已纠正断言；2026-09-24受管 `proc_85c5`
core/CTest `199/199`通过，仅验证此局部 reset 路线。

活动 case12 的 `0x0047AB43/79` scaled-RLE 窄port、
`0x0047AB7E/89` 两次 word RMW、
`0x0047AB92..99` 物理返回现按两支分离；
2026-09-24受管 `proc_67fd` core/CTest
`199/199`通过。port 仅在显式返回时允许父尾
继续；其正常回复需实际 canonical RLE 源和 framebuffer
后端，不能将入口停点视作像素执行完成。case7 新核
`0x0047A266/26D/271` 的 signed word `>=32` 共享reset，
phase0 `0x0047A27C..28D` 独立全局音频读、两参PUSH、
CALL入口停/正常返回；0、31、32、`0x8000/0xFFFF`
向量于2026-09-24受管 `proc_4f98`
core/CTest `199/199`通过。

case7 从`0x0047A28D`
独立读 actor 源token、XOR EBX、资源首dword和
`0x004CD730`发布，匹配token/缺owner及三处故障
于2026-09-24受管 `proc_2349` core/CTest
`199/199`通过。case7 `0x0047A29D..0x0047A2F0` 首矩形的
13处物理访问、H/W半宽高截断、signed坐标与phase、
四项PUSH和缺资源宽度时已留存一参由受管
`proc_0e05`于2026-09-24 core/CTest `199/199`
通过；首个矩形`0x0047A2F0 sub_416FF0`现含真实CALL栈写、
callee入口和复用已逐站还原的15访问clip子函数，
回址为`0x0047A2F5`；2026-09-24受管`proc_bcf6`
core/CTest `199/199`通过。

`0x0047A2F5..0x0047A323`
三次分别重读 signed phase、NEG并逐次发布三个canonical
motion global；首写后额外压零构成首绘图第六参数。

-1与七处故障前缀于2026-09-24受管`proc_6336`
core/CTest `199/199`通过；此后新增第六绘图参数标记。

`0x0047A323..0x0047A368` 13次访问建立首绘图五参，
先前零尾参数保持于栈，`sub_4170E0` CALL 仅入口停/窄port
正常回复且不清累计栈；2026-09-24受管`proc_2d58`
core/CTest `199/199`通过。

第二矩形按LST `0x0047A36D..0x0047A3CB` 在首绘图返回后
独立读Y、source、Oy、signed phase、资源H，再读X并压
`Y-Oy+H+P`；重读source和W，将`X+floor(W/2)-P-Ox`
压栈；重读Oy/Y后压`Y+floor(H/2)-Oy+P`和`X-P-Ox`。

第二矩形的参数/14处访问及`0x0047A3CB`真实clip
CALL、回址`0x0047A3D0`于2026-09-24受管
`proc_a635` core/CTest `199/199`通过；
`0x0047A3D0..0x0047A3FE` 另三次signed phase重读
与canonical motion写、`0x0047A3DE` 第二次压零
于2026-09-24受管`proc_d7e0` core/CTest
`199/199`通过。`0x0047A3FE..0x0047A441` 第二绘图
高为资源完整H（不右移）、Y为`Y-Oy+P`，
13次物理访问与六参数CALL回址`0x0047A446`已接入；

2026-09-24受管`proc_ce00` core/CTest
`199/199`通过。下一组LST
`0x0047A446..0x0047A49B`第三矩形在读取source/H/W、
Oy/Y后先于`0x0047A470 add esp,0x50`清前两组
矩形与绘图的20个dword，再计算并压
`(X+W/2-Ox+P, Y-Oy-P, X+W-Ox+P, Y+H/2-Oy-P)`；
phase=-1、X43/Y27、W5/H7、Oy11时按物理PUSH
应为`[20, 47-Ox, 17, 44-Ox]`。12处独立访问、
累积栈清与`0x0047A49B`真实clip CALL/回址`A4A0`
于2026-09-24受管`proc_8da4` core/CTest
`199/199`通过。

第三次clip后`0x0047A4A0..0x0047A4CC`
三次独立phase读/NEG/三个canonical global写，共六个可障站，
**无**前两次全局段的PUSH0，尾部首绘图参数尚未压入；

2026-09-24受管`proc_727e` core/CTest
`199/199`通过。第三绘图`0x0047A4CC..0x0047A511`
按LST在读取flags/source并OR bit2后才`0x0047A4DB`
压第六参数零，随后压五参数
`(flags|4, H, W, Y-P-Oy, X-Ox+P)`，
14处访问及真实CALL回址`0x0047A516`由受管
`proc_49be`于2026-09-24 core/CTest
`199/199`通过。第四矩形`0x0047A516..0x0047A572`
13处访问、四参数PUSH、真实clip CALL/回址`0x0047A577`
于2026-09-24受管`proc_80a3` core/CTest
`199/199`通过；第四次clip后三次phase重新读取/NEG/
三个canonical global写及`0x0047A585`真实PUSH0
于2026-09-24受管`proc_4731` core/CTest
`199/199`通过。

第四绘图`0x0047A5A5..0x0047A5EA`
13处访问、LEA `Y-Oy+P+16`（不改FLAGS）、五参
与现有第六零参数的真实CALL/回址`0x0047A5EF`
于2026-09-24受管`proc_1434` core/CTest
`199/199`通过。case7活动路径的`0x0047A5EF`
累计栈清80、`0x0047A5F2` word INC两次物理访问、
`0x0047AF04..0x0047AF12`四参全屏clip CALL及
`0x0047AF17..0x0047AF23`物理POP/RET
于2026-09-24受管`proc_394b` core/CTest
`199/199`通过；但case7其他出口、生产caller与整函数
逐块追溯仍未完成。

case3/4首clip返回的
`0x00479D3E..0x00479D6B`与`0x00479F3C..0x00479F6B`
各七处phase/全局写与真实PUSH EBX的故障前缀已接入，
于2026-09-24受管`proc_2f7e` core/CTest
`199/199`通过。case3 `0x00479D6B..0x00479DAE`
及case4 `0x00479F6B..0x00479FAA`各13个独立故障前缀、
资源宽高和phase双倍表达式、真实六参绘图CALL及回址
于2026-09-24受管`proc_dbc3` core/CTest
`199/199`通过。

case3第二矩形`0x00479DB3..0x00479E07`
13处逐访问故障、四参顺序、真实clip CALL与回址`0x00479E0C`
于2026-09-24受管`proc_0526` core/CTest
`199/199`通过。case4第二矩形
`0x00479FAF..0x0047A009`的14处独立物理访问、
宽高与Oy重复读、真实clip CALL/回址`0x0047A00E`
于2026-09-24受管`proc_a90f` core/CTest
`199/199`通过。case3/4第二clip返回段
`0x00479E0C..0x00479E39`及`0x0047A00E..0x0047A03B`
各三次独立phase NEG和canonical motion写、物理PUSH EBX
于2026-09-24受管`proc_bd84` core/CTest
`199/199`通过。

case3
`0x00479E39..0x00479E7C`与case4
`0x0047A03B..0x0047A07E`分别依LST构建
13次故障访问/五绘图参数，后者通过物理JMP汇聚
`0x00479E7C`共享真实CALL/回址`0x00479E81`；

于2026-09-24受管`proc_ff64` core/CTest
`199/199`通过。case3/4共享`0x00479E81` word ADD2
**先**逐次读写phase，`0x00479E89`才清80字节，
四参全屏clip的真实CALL/回址`0x00479E9D`及
`0x00479EA0..0x00479EA9`物理POP/RET
于2026-09-24受管`proc_15c0` core/CTest
`199/199`通过；仅证明case3/4局部活动分支，
整函数/真实caller/双向REVIEW仍未闭合。case13
`0x0047ABAD` signed phase32门、phase0真实
`sub_485610`及`0x0047ABD4..0x0047ABE4`匹配source首项
发布的独立故障前缀于2026-09-24受管`proc_b370`
core/CTest `199/199`通过。

case13第一矩形
`0x0047ABE4..0x0047AC2F`按LST保留
`H>>2`缓存EDI、物理`[ESP+0x14]`二倍phase
先写再读、13处逐访问及clip CALL回址`0x0047AC34`；

于2026-09-24受管`proc_8f0e` core/CTest
`199/199`通过。第一次clip返回的
`0x0047AC34..0x0047AC64`三项canonical motion独立发布、
`0x0047AC43`压立即零，以及`0x0047AC54 XOR EDX`
在末次读前的逐访问前缀于2026-09-24受管
`proc_5980` core/CTest `199/199`通过。

case13首绘图`0x0047AC64..0x0047ACA8`的
13处逐访问、`sub_4170E0`物理CALL及正常返回，经受管
`proc_e401`于2026-09-24 core/CTest `199/199`通过。

case13第二矩形`0x0047ACA8..0x0047ACEB`利用首组缓存EDI=H>>2，
按10处逐访问和clip物理CALL返回核对，于2026-09-24受管`proc_94e2`
core/CTest `199/199`通过；第二clip后
`0x0047ACEB..0x0047AD19`的三项motion全局、零实参及逐访问故障
于2026-09-24受管`proc_06cc` core/CTest `199/199`通过。

case13第二绘图`0x0047AD19..0x0047AD61`的
13处逐访问、实参顺序和物理`sub_4170E0`返回，
于2026-09-24受管`proc_9be6` core/CTest `199/199`通过。

case13第三矩形`0x0047AD61..0x0047ADBD`按LST保留三次actor读后
`ADD ESP,0x50`、两个不同栈局部写/读、14处逐访问故障与物理clip CALL；
于2026-09-24受管`proc_e5d8` core/CTest `199/199`通过。

case13第三clip后`0x0047ADBD..0x0047ADEB`三项motion与立即零
各物理访问于2026-09-24受管`proc_f0b9` core/CTest `199/199`通过。

case13第三绘图`0x0047ADEB..0x0047AE33`保留13处逐访问、
物理`sub_4170E0`入口与正常返回，于2026-09-24受管`proc_2b32`
core/CTest `199/199`通过。case13第四矩形`0x0047AE33..0x0047AE83`清EDI后分别读取高宽、
保留第三矩形的三倍quarter缓存EBX、12处逐访问及物理clip CALL，
于2026-09-24受管`proc_9e28` core/CTest `199/199`通过。

case13第四clip后`0x0047AE83..0x0047AEB1`三项motion发布与
立即零压栈，经受管`proc_074c`于2026-09-24 core/CTest `199/199`通过。

case13第四绘图`0x0047AEB1..0x0047AEF9`的13处访问与物理
`sub_4170E0`返回于2026-09-24受管`proc_6b5f` core/CTest
`199/199`通过。case13尾部在`0x0047AEF9`清80字节、`0x0047AEFC`读写分离的
word `ADD 2`（与case7的INC保留CF不同）后汇入
`0x0047AF04..0x0047AF23`全屏clip与物理POP/RET；
读写障、四次PUSH障、child CALL/返回、五次POP/RET障以及
`0xFFFF+2 -> 1`携带CF的回绕向量，于2026-09-24受管
`proc_8abc` core/CTest `199/199`通过。以上仅是case13静态路径局部验证；

其他selector、生产caller和完整双向REVIEW仍未完成。
case14 `0x0047AF24`有独立signed word phase读：
0进入音频、32进入资源、33进入共享reset、`0x8000/0xFFFF`
均不走高phase门；actor首读障在原指令停止。该局部头部于2026-09-24
受管`proc_2057` core/CTest `199/199`通过；高phase在
`0x0047A253/+0x2958`与`0x0047A25A/+0x2954`
两次独立word写（首写已提交的第二写障）于2026-09-24受管
`proc_5604` core/CTest `199/199`通过，下一物理点为
`0x0047B808`。

phase0 的`0x0047AF3A..0x0047AF4A`
canon sample-handle读、压0x31、物理音频CALL/typed-stop与返回，
于2026-09-24受管`proc_1826` core/CTest `199/199`通过。

`0x0047AF4A..0x0047AF69/B11F`重读当前源token、源首dword发布
`4CD730`后才重读signed phase，以9分流；四处逐访问障与
0/8/9/32/负word两侧，于2026-09-24受管`proc_ede1`
core/CTest `199/199`通过。早段首矩形`0x0047AF69..0x0047AFBE`
含原phase局部P与14处读写/四次PUSH，于2026-09-24受管
`proc_0127` core/CTest `199/199`通过；物理裁剪CALL入口停点、
返回地址`0x0047AFC3`及子函数正常返回于2026-09-24受管
`proc_1494` core/CTest `199/199`通过。后裁剪读取局部P、
先压EBX=0、再将同一个`-P`依次写三处motion global及五个障点，
于2026-09-24受管`proc_9c7f` core/CTest `199/199`通过；

早段首绘图`0x0047AFDE..0x0047B01B`从当前actor重读
flags/source/宽高/坐标，五次物理PUSH与12个读写障于2026-09-24
受管`proc_7e4f` core/CTest `199/199`通过；`0x0047B01B`
真实绘图CALL、子入口typed-stop、回复`0x0047B020`与六参数
于2026-09-24受管`proc_bc36` core/CTest `199/199`通过。

早段第二矩形`0x0047B020..0x0047B083`
两次资源token重读、局部P的读→倍增→物理写回与17个访问障，
于2026-09-24受管`proc_f8bc` core/CTest `199/199`通过。

第二次物理裁剪CALL`0x0047B083`及正常返回`0x0047B088`
于2026-09-24受管`proc_1df2` core/CTest `199/199`通过。

`0x0047B088/8E/94`复用裁剪返回保留的EDI将同一个`-P`
逐次发布三处motion global，于2026-09-24受管`proc_59d4`
core/CTest `199/199`通过。第二绘图`0x0047B09A..0x0047B0D9`
逐处重读资源、先PUSH EBX作六参末项、在`0x0047B0D1`
从局部读`2P`写EBP，再压X/Y，于2026-09-24受管`proc_e945`
core/CTest `199/199`通过；`0x0047B0D9`真实绘图CALL、六参数、
子入口typed-stop与`0x0047B0DE`带高16位EAX的正常回复，
于2026-09-24受管`proc_9f1f` core/CTest `199/199`通过。

早段尾部`0x0047B0DE..0x0047B11A`先重读phase再清80字节，
phase=8时aux word INC独立读/写、重读比较后仅相等才写主phase9；
其他phase保留绘图回复EAX高16位做32-bit ADD4，再发布低word。

五处故障与回绕携带CF向量，于2026-09-24受管`proc_5d30`
core/CTest `199/199`通过。共享`0x0047B2CA..0x0047B2DB`
四次全屏PUSH、真实裁剪CALL及回复，于2026-09-24受管
`proc_9538` core/CTest `199/199`通过；
`0x0047B2DB..0x0047B2E7` 清四参、清EAX、四次物理POP及RET
五处独立故障向量，于2026-09-24受管`proc_622d`
core/CTest `199/199`通过。以上仅是case14早段静态路径局部闭合；
晚段首矩形`0x0047B11F..0x0047B174`从旧AX相位
先算局部2P、在首次PUSH之后物理写/读局部槽、14处逐访问障，
于2026-09-24受管`proc_616c` core/CTest `199/199`通过。

首矩形真实裁剪CALL`0x0047B174`与返回`0x0047B179`于
2026-09-24受管`proc_4f53` core/CTest `199/199`通过。

首裁剪后`0x0047B179/B189/B19A`三次独立signed phase读、
三处motion global逐次发布，首写后才物理压EBX与七处访问障，
于2026-09-24受管`proc_1d91` core/CTest `199/199`通过。

首绘图`0x0047B1A8..0x0047B1E7`当前phase逐访问重读，
验证了glob写时phase9、绘图时phase10产生不同motion/坐标，
13个读写障于2026-09-24受管`proc_ca32` core/CTest
`199/199`通过。`0x0047B1E7`真实绘图CALL、六参数、子入口与正常回复
`0x0047B1EC`于2026-09-24受管`proc_2a39` core/CTest
`199/199`通过。第二矩形`0x0047B1EC..0x0047B246`重读phase与两次源token、
14个物理访问及四次PUSH，于2026-09-24受管`proc_63c0`
core/CTest `199/199`通过。`0x0047B246`第二次物理裁剪CALL与`0x0047B24B`
正常返回，于2026-09-24受管`proc_39b0` core/CTest
`199/199`通过。

第二次`0x0047B24B/B25B/B269`逐项重读phase与三处motion写，
物理`B25A` PUSH EBX及七处读写障于2026-09-24受管
`proc_ca3a` core/CTest `199/199`通过。

第二绘图`0x0047B278..0x0047B2BB`绘图时phase11与
此前glob phase10分离，十三处逐访问障于2026-09-24受管
`proc_1462` core/CTest `199/199`通过；`0x0047B2BB`真实绘图CALL与`0x0047B2C0`
正常返回于2026-09-24受管`proc_3d23` core/CTest
`199/199`通过。晚段尾部`0x0047B2C0`清80字节后，`0x0047B2C3`主phase
word INC独立读写、回绕并保留先前ADD ESP的CF，
于2026-09-24受管`proc_95f9` core/CTest `199/199`通过。

晚段正常回复汇入同一`0x0047B2CA..0x0047B2E7`
全屏裁剪及物理POP/RET，受管`proc_e3fa`于2026-09-24
core/CTest `199/199`通过。以上为case14静态早/晚分支局部验证；
其他selector、四处真实caller及整个316双向REVIEW仍未完成。

其余 selector、
`sub_4344E0` 子内部与双向 REVIEW 仍未完成。

下一段 `0x004798B3` 的重置不能因路由通过就直接清零。`legacy_battle_actor_runtime_reset.cpp` 已新增 `+0x2958`→`LegacyBattleGroupAActionExecutionState::turn_threshold`、`+0x2A94`→`LegacyBattleActorBaseInitializationFields::field_2a94` 的 image 双向逐次映射，
Group-A/B 分离写回测试及受管`./build.sh core --test`于2026-09-23 UTC通过，CTest`199/199`（`proc_fa3f`，
退出码0）。`+0x2C4/+0x2C8` 的 image 来自 slot0 record `field_24/field_28`；
当前新增 image 单次写→slot0 与 `LegacyBattleActorProgressState::cache_x/cache_y` 双向 typed 视图同步及 Group-A/B 分离写回测试，
受管`./build.sh core --test`及CTest`199/199`于2026-09-23 UTC通过（`proc_f051`，
退出码0）。跨调用 owner 缺口已定位：LST `sub_46E520` 在`0x0046E5AC/0x0046E5B2`依次写`actor+0x2C4/+0x2C8`；
先前`legacy_battle_actor_progress.cpp:198..199`只更新`LegacyBattleActorProgressState::cache_x/cache_y`，
未同步slot0 `frame_source_action_record` 的 `+0x24/+0x28`。未提交 WIP 为 `advance_legacy_battle_actor_progress()` 增加借用slot0 port，
分别在两次物理cache写后同步record字节；`legacy_battle_group_a_frame.cpp:1594`和`legacy_battle_transition.cpp:847`的已知owner路径现已接入，
独立及两处caller向量于2026-09-23 22:48 UTC受管`./build.sh core --test`与CTest`199/199`通过（`proc_74ac`，
退出码0）。

进一步新增无slot0 owner但要求物理一致时在`sub_46E520 / 0x0046E5AC`的首个cache写**之前** typed-stop，
保留先行`+0x2AB0/+0x2AEC/+0x2B20/+0x2B08`已提交前缀；Group-A frame和Transition分别检查child状态，
后者保留停止回复，不把EAX=1误作已返回。独立无owner停止向量于2026-09-23 22:55 UTC受管`./build.sh core --test`及CTest`199/199`通过（`proc_fa9a`，
退出码0）。两处生产caller的可达owner路径之外，仍不代表所有alias或316整条路径完成。

## 4. 18 个有效 case 与共用尾的控制流

```text
case   首地址       signed phase 出口/动作
1      0x004799DC   >480 清零/重置；否则绘制后 +31
2      0x00479B06   ==100 清零/释放/重置；否则初始化或推进粒子
3      0x00479CA6   >32 清零/重置；否则多次几何绘制、+2
4      0x00479EAA   >32 清零/重置；否则另一组多次几何绘制、+2
5      0x0047A083   >= frame 高度切换 selector=2/phase=0；否则绘制、+14
6      0x0047A1A0   <=-32 清零/重置；否则绘制后 -1
7      0x0047A266   >=32 清零/重置；否则成对矩形/绘制后 +1
8      0x0047A5FE   ==100 复用 case2 清理；否则另一种粒子初始化/推进
9      0x0047A752   >=45 清零/重置；否则绘制后 +1，直落 default
10     0x0047A815   >= frame 高度切换 selector=2/phase=0；否则绘制、+4
11     0x0047A94D   <=-32 清零/重置；否则双层绘制后 -1
12     0x0047AA7B   >=30 清零/重置；否则按 mode bit0 二择一 RLE，双 phase 步进
13     0x0047ABAD   >32 清零/重置；否则多层绘制后 +2
14     0x0047AF24   >32 清零/重置；<9 / >=9 两套几何，phase8 另查辅助 phase
15     0x0047B2E8   <=-32 清零/重置；否则绘制后 -1
50     0x0047B747   >=15 清零/重置；否则查十五项偏移表后 +1
51     0x0047B83E   >=100 清零/重置；phase0 初始化，之后更新效果 +2
100    0x0047B409   ==100 释放粒子并重置；phase0 计数/绘制，非零粒子推进
```

以下按原 LST 核对了入口和各 case 主路径的已列指令、部分几何与共享尾；并非 249 个块的全访问、CALL 回复及故障前缀均已审完：

- 入口重置 `0x004798B3..0x0047991F` 先将 `+0x2958` 写1000、`+0x2A94` 写零；仅 `+0x2AA0==1` 时读取 `actor[+4]` 再对 nested `+0x25` OR 0x80。
  随后清 `+0x2C4/+0x2C8/+0x2958/+0x2A12`，以原 EDI=`actor+0x3D0` 连续清 38 dword（半开范围 `+0x3D0..+0x468`，
  覆盖刚写的 `+0x3D8=0x24`），调用 `sub_478850`，依次写 `+0x2AAC=0/+0x2ABC=0`，压入1、写 `+0x2AB8=1`、调 `sub_47E950`；
  忽略其 EAX 回复，返回1。更新器返回0的另一条路径只返回1，不执行这组清零。

本函数唯一 `0x00479911 sub_47E950` CALL 的实参由 `0x00479867 mov ebp,1` 和 `0x00479908 push ebp` 固定；
中间 `sub_478850` 在正常回复时恢复 EBP。因此对**这处 caller**，callee `0x0047E959 cmp 1,0` 必走 `0x0047F0BF`，
其502条完整主体仅入口9条及该分支18条（合计27条）可达，不等于 `sub_47E950` 在其他调用域的零实参路径不可达。`build/workpack316/list-release-audit.tsv` 记录该分支17个独立可障指令地址、18个物理读/写/栈/CALL站点（`0x0047F0C5` 的 word RMW 分读、写两站）。

入站 ESP=C=`S−0x2C`、ECX=actor、arg0=1；它先读原 `actor+0x2584` 链表头，再对 `+0x26D0` word AND `0xFEBD`，
依次把 `+0x26C0` 与 `+0x2584` 两个 dword 清零。**这些写发生在遍历原头指针之前**；非零链表每轮先从当前节点读 `next=[node]` 到 ESI，
才压当前节点调用 `sub_4885A0` 释放，正常回复后以原先保存的 next 决定继续，不新增环检测或预快照。如果节点与 actor 字节别名，
下一节点读取必须看到前三次写的当时内存；故障时已提交前缀不得回滚。有限链或空头返回 EAX0、ZF1，callee `retn 4` 清自身一参，
caller `0x00479916` 随即 POP EDI 并 `mov eax,ebp` 恢复最终返回1。若循环调用的释放 wrapper `0x004885A5` 原参重读或深层 CRT `0x004885C7` 首全局读故障，
二者相对 actor 入口 S 的 ESP 分别是 `S−0x4C`、`S−0x68`，**不同于**本函数内两处直接资源释放的 `S−0x34/S−0x50`；
这只是条件性嵌套停止前缀，CRT 深层及链节点生命周期尚未完成全路径验收。现有 canonical `LegacyBattleActorBaseInitializationFields::linked_action_head_token` 承载 `actor+0x2584`，
当前 `legacy_battle_actor_runtime_reset.cpp` 已将该字段接入 image 物化及每次写回。

局部新增 `LegacyBattleActorFrameLinkedNode` 显式借用token→首dword(next) owner；`continue_legacy_battle_actor_frame_release_node` 每次执行一个 `0x0047F0DE..0x0047F0ED` 物理迭代，先读 next、再独立PUSH参数和CALL返回地址，对现有 `sub_4885A0` 适配port调用，正常返回后只在next非零时继续读下一节点；next为零才POP4/RET4并让外层 `0x00479916..1F` 物理返回。缺节点、逐访问预算故障、wrapper未返回及末尾callee RET读障均保持原地前缀；若节点token位于actor映射区，则**不使用外借快照覆盖前序 `+0x2584` 写**，在 `0x0047F0DE` typed-stop，真实actor/链节点别名仍待后端。`proc_a49d/proc_cf8e/proc_c19b/proc_1f75/proc_29ea` core/CTest 各`199/199`覆盖双节点正常返回、缺首/第二节点且保留已释放首节点的前缀、释放回调修改后继节点后现时重读而非整链预快照、actor边界部分重叠停点、wrapper子停点和末尾RET读障；每轮借用的node span只能保证本轮有效，释放使其失效时下轮必须重新解析，不得复用悬空视图；最新`proc_1962` ASan core/CTest `199/199`，`proc_b89f` Linux app/CTest `205/205`。`proc_678f` 因误向已内置`core`的`build-asan.sh`追加`core`参数而usage退出，已用`./build-asan.sh --test`修正重跑通过。上述只证明显式非别名节点owner局部路径，不证明深层CRT与别名路径全验收，也非316最终门禁。
最新局部回归：`proc_41d6` Linux app/CTest `205/205`；
`proc_67fc` ASan core/CTest `199/199`。两者均不替代316最终门禁。

返回点局部审计：`return-audit.tsv` 的22个物理RET地址均在实现与测试中出现；
为此前缺失的7处增加独立RET读障断言，`proc_ce38` core/CTest `199/199`。

地址提及与局部向量均不能替代全函数双向REVIEW。
CALL站点局部审计：`call-audit.tsv` 的98个物理CALL地址均在实现与测试中出现；
为此前缺失的14处增加CALL前栈写故障向量，`proc_3aeb` core/CTest `199/199`。

这仍不是98处CALL逐指令双向REVIEW，也不证明深层callee异常。
新增CALL/RET向量后的局部回归：`proc_20f9` app/CTest `205/205`；
`proc_762b` ASan core/CTest `199/199`。四处真实caller仍未接线。

`proc_fec7` LST静态栈检查：249/249可达块、98/98 CALL、22/22 RET、
9处REP的额外栈字数为零；CALL/RET/CFG汇合栈错误均为零。

此检查只证明索引与栈深一致，不证明C++逐块语义相等。
入口组合路由已串联已核对的reset、链节点逐轮解析、update、lookup和default；
缺端口或未接入的selector保持未执行continuation，不发布正常返回。

`proc_4619` core/CTest `199/199`；含双节点正常返回及释放后改变后继的读障。
该组合尚未接入四处真实caller，不能标为全函数完成。

组合路由当前局部回归：`proc_5108` app/CTest `205/205`；
`proc_5730` ASan core/CTest `199/199`。

四处caller入站准备已按LST分开计算低32位索引、保存寄存器、FLAGS与DF；
组B最终步进的全1哨兵不进入child；首个actor读障保留CALL压栈与child PUSH。

`proc_5a5a` core/CTest `199/199`。此处仅为入站向量，尚未替换真实parent的旧调用；
四个parent的真实栈别名、异常索引与正常后缀仍待双向验收。

selector1组合路径补齐：相位481经共享38-dword REP/reset后物理返回1；
相位0按源header执行音频、绘制、phase+31并物理返回0；缺端口保留CALL待执行。

`proc_1cb8` core/CTest `199/199`；组合后的局部app `proc_1597` `205/205`、
ASan core `proc_0ddd` `199/199`。其他selector与parent仍未完成。

selector2 组合路径继续按原站点接入：相位100 的零 emitter 经独立
`0x00479C9C` 比较直接到22次 REP 与共享 reset；非零 emitter 必须先经
`0x00479C6C` 窄释放 CALL，正常回复后才执行同一 REP/reset。

未提供 release port 时停在 CALL 前，不能清零后宣称成功。
相位非100时，有 token 经 `0x0047B6C7` 粒子 CALL，完整 EAX0
回共享 default 物理 RET、完整 EAX1 才写 phase100 后走独立 RET；
零 token 先清22个 dword，经 decoder、资源宽高/几何、属性、两次
Win32 metrics、宿主矩形、音频、phase1 再进入同一粒子 CALL。

所有缺失的 decoder/metrics/rectangle/sound/particle port 保留
尚未执行的 continuation；窄 port 的正常回复只模拟被调用函数已完成，
并非其深层 owner/异常已验证。`proc_7f27/proc_ef11/proc_2aaf/
proc_f5bf/proc_a6ea` 是按上述分支逐次扩展后的局部 Linux core/CTest
`199/199`，最后一次截至2026-09-25（UTC+08）。

只覆盖显式 Group-B owner 与窄回调向量；真实四处 parent 未接线，
其余 selector、callee 深层及双向 REVIEW 均未完成，inventory 不变。

selector8 也已接入同一组合路由：header 于 `0x0047A5FE` 将
`+0x2958` 的低16位与当时 CX=100 比较；相等进入 case2 共用释放/REP/reset，
不等时按固定 emitter token 是否为零选择粒子复用或22-dword 清零、
自身几何/字段、共用 decoder/metrics/rectangle/sound/particle 后缀。

已有token、零token CALL前、零token多port正常回复及phase100相等
四条局部向量于2026-09-25（UTC+08）受管 core/CTest
`proc_772a` `199/199` 通过；先前`proc_a22c`为`198/199`，
仅测试误把 CX 当作 slot2 `+0x4A` 的0x12，实际 `0x00479986` 后
`mov ecx,0x64` 已覆盖该值；改用原 LST 的100后通过。

上述正常回复均依赖显式 mock port，不能证明深层callee和真实caller接线。
selector9 组合路径按 signed phase>=45 的两次 word 写进入共享
`0x0047B808` reset 前缀，再执行 reset/物理返回1；phase0 依次音频、
当前 frame 源、三项共享绘图全局、六参数绘制、全局清理、INC word
与 default 物理返回0。缺 draw port 停在 `0x0047A7F0`。

`proc_f937` core/CTest `198/199` 首次暴露缺少
`case_reset_progress_write_ready` 的组合 continuation；补接后
`proc_e740` 于2026-09-25（UTC+08）core/CTest `199/199`。

此为 Group-B 显式 owner 和窄 callback 的局部向量，非全函数验收。
selector6/11/15 已组合各自 header：signed phase=-32 经两次 word 写
进入共享 reset 并物理返回1；phase0 停在各自未执行的 active 入口，
不冒充绘制/返回。`proc_cc24` 于2026-09-25（UTC+08）局部
core/CTest `199/199`；三条活动后缀及真实caller仍未完成。

后续条件化音频入口已组合至三者各自源读前缀；phase0 经独立
`sub_485610` 窄回复才进入 source，缺sound port不执行CALL。

selector6 另接入 source/global逐写、六参数绘制、DEC word 与独立
物理返回0；phase0 的0→0xFFFF及音频/绘制次数由
`proc_1536` 于2026-09-25（UTC+08）局部 core/CTest `199/199` 验证。

selector11/15 后续已分别组合 source/global、首次绘制六实参、
第一次窄绘制CALL、两次绘制间的清栈/全局访问、第二次绘制实参/
窄CALL、phase word DEC和各自物理RET；缺 draw port 留在第一次
CALL 前。Group-B phase0 的两次绘制计数和0→0xFFFF由
`proc_264f/proc_b5c4` 于2026-09-25（UTC+08）局部
core/CTest 各 `199/199` 覆盖；真实callee内部和parent仍未验收。

selector50 也已组合：phase15 经 selector 自身 header 进入共享 reset；
phase0 按物理 slot2 音频索引、共享源/绘图全局、绘制六参、
INC word及独立物理RET。缺音频或绘制 port 停在各自未执行的 CALL；
`proc_ddcb` 于2026-09-25（UTC+08）局部core/CTest `199/199`
覆盖该两类相位和正常窄回调。其它相位/故障与caller仍待双向REVIEW。

selector100 header 组合后，phase0/runtime gate6 经三次逐写清 gate、
置phase1后物理短RET0；phase100/零emitter 先读固定token，
再写word、依序38与22次REP、共享reset CALL，走自身物理RET1。

非零emitter的release缺port时仍在CALL前；其它 phase 进入
未执行的粒子 gate或音频/source分支。`proc_6b50` 首次局部
core/CTest `198/199` 因把独立RET误断言为共尾状态；
`proc_84c9` 再次 `198/199` 因旧测试仍假设组合路由停在
case100跳转目标，实际保存的phase1000使其继续到粒子gate。

同步改正测试后`proc_89a2` 于2026-09-25（UTC+08）
core/CTest `199/199`。这仍非全部case100和parent的双向验收。

selector100非终结phase1000的emitter gate也已局部组合：已有token
经源位置读写进入共用粒子CALL，窄回复EAX0才走default物理RET；
零token按物理22-dword清零停在未执行 decoder 实参站点。

`proc_d5fc` 于2026-09-25（UTC+08）core/CTest `199/199`。
phase0/runtime gate0/motion word0 的源/global、绘制实参、
`0x0047B4C0` 窄CALL和偶数运动word加4后物理default RET
由`proc_74ed` 同日局部core/CTest `199/199` 覆盖；
零token decoder 后缀仍需组合和跨owner复核。随后 phase0/runtime gate0/
`motion_aux_word=24` 的音频窄CALL、源、绘制、运动word
24→28、runtime gate0→1与独立物理RET由`proc_654b`同日局部
core/CTest `199/199` 验证；不据此认定音频callee内部或其他相位等价。

selector100的零emitter后续现在也条件化组合 decoder、资源宽高、
几何/配置、属性、两次metrics、宿主矩形、后段音频、phase101写
及粒子步进；测试用完整mock正常回复EAX0时phase最终102，
随后物理default RET0；资源header未提供时独立停在
`0x0047B568` 的读点，不伪造 decoder CALL。

`proc_0197/proc_64de` 初次core/CTest各 `198/199`，
一处期望误漏粒子步进的再次phase INC，另一处旧向量在缺header
时错误期望走到 decoder CALL；诊断确认为 default_returned/
phase102与typed-stop `0x0047B568`。修正后`proc_ac73` 于
2026-09-25（UTC+08）局部core/CTest `199/199`；
窄mock不能替代子调用真实owner及故障双向REVIEW。

selector51 组合增加 phase100 的38-dword清零/shared reset与专用物理RET1，
以及phase0方向记录初始化、几何/属性、decoder和音频窄CALL、
按原顺序phase+2与source比例写。缺`0x004CD76C`全局时先在
`0x0047B96C` read typed-stop，保留phase2；提供该全局后才
PUSH方向扫描双参/CALL返回地址，缺像素owner于
`0x004344E0` callee-entry typed-stop，不伪称返回。

`proc_99ef` core/CTest `199/199` 验证reset/初始化前缀；
`proc_03b7/proc_1caf` 两次`198/199` 暴露测试误以为无需
全局读取即可进入扫描CALL；按LST增设该缺全局与显式backing
双向量后`proc_c1a0`于2026-09-25（UTC+08）core/CTest
`199/199`。真实扫描pixel/global owner尚未生产接线。

selector5/10组合在各自入口读取phase与resource高度：相等时
`0x0047A935`切 selector2/phase0、经物理RET；phase0按物理
PUSH/CALL音频、写全局源、准备首绘、再按原汇编序列执行
两次绘制和裁剪CALL。缺draw端口停在首绘CALL前；显式窄
mock可验至`0x00416FF0` callee-entry typed-stop，未借窄mock
伪造矩形callee返回。`proc_78a3/proc_76d9/proc_f6eb`于
2026-09-25（UTC+08）各自core/CTest `199/199`；另显式
raster backing 经`sub_416FF0`几何写、物理RET、phase按5/10
分别+14/+4的组合向量在`proc_9cbd` core/CTest `199/199`。

这些只是局部路径验证，不是两个selector所有故障前缀或全函数REVIEW。
selector12按signed phase30分支：终结时清phase/motion、
38-dword REP与共享reset，再物理RET1；phase0音频后缺
`0x004A0698` transform 先在`0x0047AAA9`全局写停点，
显式transform才经source、正向`0x00422C70`或反向
`0x00423020`光栅CALL入栈；显式窄光栅回复后motion+2、
phase+1、物理RET0。`proc_052e/proc_b167`于
2026-09-25（UTC+08）局部core/CTest各`199/199`。

窄端口未覆盖两条光栅callee的内部像素写或真实资产差分。
selector3/4/13的signed phase33经各自header进入38-dword
shared reset/物理RET；phase0经独立音频CALL、source全局写和
首个矩形参数前缀，未提供矩形callee时停在各自CALL前。

`proc_bf45/proc_e1e7`于2026-09-25（UTC+08）局部
core/CTest各`199/199`；随后首矩形CALL物理入栈，显式
raster backing 经callee、三个全局motion写、首绘、第二矩形
callee与第二绘制参数，缺draw端口停在第二绘制CALL前；
`proc_4c96/proc_f664/proc_f8f9/proc_1c0f`同日局部
core/CTest各`199/199`。继而3/4的第二绘制、phase+2、
共享矩形callee和物理RET，以及13的四次绘制、五次矩形、
phase+2与物理RET已在显式窄端口组合；`proc_20da`于
2026-09-25（UTC+08）core/CTest `199/199`。

窄端口不证明真正draw callee内部、真实栈alias及故障后缀。
selector7 signed phase32经共享reset，selector14 signed phase33
先写phase/motion再共享reset；selector14恰phase32仍走晚分支。

selector7 phase0音频后，显式raster与窄draw端口组合四次绘制、
五次矩形、phase+1与物理RET；selector14早期phase0双绘制/
三矩形后phase+4，晚期phase32双绘制/三矩形后phase+1，
均经共享矩形callee后物理RET。缺裁剪raster时CALL入栈
后停在真实callee入口，未伪造正常返回。

`proc_9eef/proc_4def/proc_5789/proc_0421/proc_6b2e/
proc_c0e2/proc_76b3`于2026-09-25（UTC+08）局部
core/CTest均`199/199`；仍未覆盖draw内部、alias与故障后缀。

另两写的 `+0x26C0` 对应 action-execution/progress 双镜像，`+0x26D0` 是 progress `mode_gate` 低 word 且需同步 action-execution 的 `retreat_ready_flags`；
这些 owner 关系已由当前源码字段核对；显式链节点借用span仅覆盖非actor别名的首next读与释放回调，真实分配/释放的物理所有权和深层wrapper仍须另行核定。

原 LST 的节点产生路径 `0x0047DCB9..0x0047DCD7` 为 `sub_487C10` 请求 **0xAC byte**、以 `rep stosd` 清43个 dword、先将旧 `actor+0x2584` 头写入新节点 `+0`、再把新节点 token 写入 actor `+0x2584`；
本处释放环读的正是该节点 `+0`。现有 `LegacyBattleImageParticleNode` 则经 `sizeof` 静态断言为 **0x38 byte**，
其 `next_token` 在 `+0x34`；两种节点物理布局不同，不能借粒子节点池替代此链表。若分配 token 为0，原产生路径的 `rep stosd` 仍访问该地址，
不能把它概括为提前正常跳过；该产生函数不属于本 Workpack 316 的实现范围，仅用于确定当前清理路径的输入形态。

- 入口五个早期块 `0x00479867..0x004798D2` 的异常顺序：`xor eax,eax` 后 `mov ax,[actor+0x2A0C]` 形成**零扩展**的 dword，
  再 `lea edi,[actor+0x3D0]` 并 `mov [edi],eax` 写完整4 byte，随后写 `+0x3D8=0x24`；
  `+0x2AA0==1` 时跳过 `+0x2AF8` 访问，否则仅 `+0x2AF8==1` 才查两个独立完整 dword `+0x2B00/+0x2B04`。

  两门均零才在 `0x004798B3/BC` **先**写 phase word1000 **再**写 selector byte0。
  `0x004798C3 cmp [esi+2AA0h],ebp(1)` 为 nested OR 的最后算术 FLAGS：仅该值**不等于1** 才 `0x004798CB` 读 actor+4 token、`0x004798CE or byte [eax+0x25],0x80`；
  nested token 读障时 EAX尚为零扩展的 `+0x2A0C`，nested byte OR 障时 EAX已是 nested token，
  二者都已发布 phase1000/selector0，尚未清 `+0x2C4/+0x2C8`，FLAGS仍由 `+0x2AA0` 的比较决定。

  OR 正常才改 FLAGS 与 nested byte；之后再执行两项清零/相位清零/REP/reset。先前“立即重置”只描述最终状态，
  不能把 OR 或相位1000 的瞬时已写前缀省掉。

- 更新公共入口 `0x00479920..0x004799D5` 的正常零与停止前缀：先 `push edi; call sub_4321E0; add esp,4`；
  回复 EAX0 时 `0x00479929 test eax,eax` 后直接恢复寄存器，以 EBP=1 返回，**不**读 `+0x41A/+0x2548/+0x2B20`。
  非零时 `0x00479937 mov cx,[esi+0x41A]` 仅替换 ECX低16位，依次 `push ebx=0; push ecx; call sub_4315D0`；
  后者即使正常返回 token0，也要在两个 cdecl 参数**尚留栈上**时按 `0x00479945` 写 `+0x2548=token`，
  `0x0047994B` 读完整 `+0x2B20`，`0x00479951` 读完整 `+0x3E0`，到 `0x00479957` 才清8 byte。

  `sub_4315D0` callee LST `0x0043174E..0x0043175A` 在两级解析均返回0时可把 `dword_4FB0C8=0` 再正常返回 token0；
  其唯一正常 RET `0x0043175A` 先在 `0x00431753` 重读 token 全 dword 到 EAX，而 FLAGS 来自较早的两条 nested 回复 `test eax,eax` 之一（`0x0043173C/0x0043174A`），
  可能与新读 token 值无关。父 `0x00479945` 写 token 不改 FLAGS，后续 `+0x2B20` 读障须携带该 callee 回复 FLAGS。

  另其初始化门 `dword_4CF840==0` 可执行两处没有 `cld/std` 的 `0x004316F0/0x004316FC rep stosd`：
  分别从 `0x004DAD28` 写33000 dword、从 `0x004CF988` 写11475 dword；DF=1 时写集合为 `[0x004BA98C,0x004DAD2C)` 和 `[0x004C4640,0x004CF98C)`，
  第二写集合**完整包含于**第一写集合，仍执行两次共44475次物理写而非一次去重。两处 REP 前的第六个 `sub_433270` 只普通 `retn`、尚未清其 `FileName` 参数：
  设本函数入口 ESP=`S`，`0x004316F0/0x004316FC` 入站 ESP 均为 `S−0x3C`，EAX=0，算术 FLAGS 来自 `0x004316E9 xor eax,eax`，
  第二段入口 `mov ecx/edi` 不改 FLAGS。DF=1 的首段第8次写落在 `dword_4DAD0C`（首项索引7），
  第11579次写落在初始化门 `dword_4CF840`（首项索引11578）；第二段第83次又写该门（索引82）。两 REP 正常完成后 `0x00431704 call sub_431A50` 的 Win32 `ReadFile` 接收 `lpBuffer>=0x004DAD28`，
  按所传44-byte长度不触及前侧 `0x004DAD0C`；随后 `0x00431716` 会重读该全局参与分支，因此不能按 DF=0 假设原值保持；
  在两次 REP 正常完成、无外部越界/异步写前侧地址的条件下该重读 ECX=0，`0x0043171C mov eax,dword_4A6020; 0x00431721 cmp ecx,eax; 0x00431723 jl 0x0043172A` 仅当后者有符号为正才跳过 `0x00431725 call sub_431EA0`，
  后者为0或负数则仍调用。

该嵌套 CALL 的副作用/停点没有因这一条件推导完成审计。此处依赖外部 API 按所传缓冲界限写入，IAT 内部指令及其 DF 出站状态仍不在 LST；

  到站 DF=1 是条件，不能仅由 child 入口 DF=1 保证；不是纯 token 查询。该条件性深层路径的 DF/逐字故障和跨全局别名尚未验收；
  不能把零回复折算成 child typed-stop 或在 `+0x2548` 写后提前跳过这些读取。若 `frame_started(+0x2B20)!=1`、`+0x2B08==1` 且 `+0x3E0!=0`，
  `0x00479978` 又从已写的 `+0x2548` 取 token，`0x0047997E xor ebp,ebp` 后才 `0x00479980 mov bp,[token+0x0C]`；
  token0 下该读是否故障应取决于物理零页映像，此时两参已经清掉、EDI/ESI等需按真前缀保留，不能以 decoder 零值自动终止整函数。

  若 `+0x2B20/+0x3E0` 实读障，参数仍是8 byte且先前 token 写已提交；读障本身不更新算术 FLAGS。

该一次 `0x00479940 sub_4315D0` 的初始化支路另按 `build/workpack316/loader-init-audit.tsv` 核对六条独立文件序列：
`all_char.tsw`→`dword_4FB0CC`，`all_item.tsw`→`4FB0D0`，`all_magic.tsw`→`4FB0D4`，
`all_sys.tsw`→`4FB0D8`，`all_map1.tsw`→`4FB0DC`，`all_map2.tsw`→`4FB0E0`。

每项先经过外部 `lstrcpyA/lstrcatA` 构建文件名，再由 `sub_433270` 通过 Win32 `CreateFileA` 取 handle；
该15条指令 wrapper 正常接到 `INVALID_HANDLE_VALUE` 时用 `inc/neg/sbb/and` 将 EAX 规范成0，
调用者**仍按原顺序继续后续文件与两段 REP**，不得把失败0改写成 typed-stop。前五项在 CALL 后各 `add esp,4` 才提交相应全局，
最后一项 `0x004316DA` 的文件名参数却一直保留在栈上：提交第六个全局、执行两次 REP、重读第三个全局 handle 并调 `sub_431A50` 后，
才由 `0x00431709 add esp,8` 与新压的 handle 一起清理。设本函数入站 ESP=S，两段 REP 停点均为 `S−0x3C`，
留存第六项参数，前六个 handle 全局已提交；第二段停止还须保留第一段的全部逐次写。DF=0 两段写集合分别为 `[0x004DAD28,0x004FB0C8)` 与 `[0x004CF988,0x004DACD4)`，
DF=1 为前述两个可重复重叠集合；两段写到站的 DF 仍取决于前置外部调用，不能用入口 DF 的单一假设代替。`loader-init-audit.tsv` 的8行只是顺序/栈/范围索引，
Win32 外部细节、`sub_433270/sub_431A50` 的完整深层障点和后续 token 解析仍未验收。

`sub_4315D0` 后段首先以 `0x00431732/0x00431733` 压 `(arg4=0, arg0=完整 ECX)` 调 `0x00431734 sub_431DF0`。
该无嵌套 callee 共57条指令、27个栈/全局/节点实际访问指令地址，逐站见 `build/workpack316/cache-lookup-audit.tsv`。

本 caller 的 key 只取 `arg0&0xFFFF=actor+0x41A` 当前 word、高16来自 `arg4&0xFFFF=0`，
但源参高16仍由 updater 回复保留到调用入口；桶按前者 `u16%10`，**唯低16位恰为 `0xFFFF` 时改用第二参 `u16%10=0`**。

桶头读取地址 `0x004CF84C+(bucket*0x20)`；先 `0x00431E12` 把结果全局 `4FB0C8` 写0，
再读桶头、`0x00431E4C` 写游标 `4DACDC=head`，逐节点先读 `+0` packed key、再读 `+4` next，
并逐次改写游标。无命中正常 EAX0、FLAGS 来自 `0x00431E69 xor eax,eax`。命中头节点不改链，非头命中则按 `[found+4]` 读 next、`[previous+4]=next` 写、**重读游标**、重读 bucket 当前头、`[found+4]=current_head` 写、重读游标、`[bucket]=found` 写的原顺序前移节点；
任一读/写故障不得回滚更早的全局和链表写。最后 `0x00431E88 add eax,8` 生成可回绕的 payload token，
`0x00431E8D` 才写 `4FB0C8` 并正常 `retn`；若节点地址的低32位为 `0xFFFFFFF8`，成功匹配仍可返回0，
父 `test eax,eax` 会走 fallback，此为位宽条件向量而非原运行时命中事实。此 lookup 的节点 key 在 `+0`、next 在 `+4`、payload 在 `+8`，
与清理分支的 0xAC-byte 动作链节点 `+0=next` 不是同一结构，不能复用物理 owner。DF=1 的初始化两段 REP 均覆盖 cache bucket 十项及 `4DACDC` 游标，
必须保留第二段的重复清写与随后该 callee 的重新读取；Win32/缓存 fallback 与异常别名仍为 partial。

首层 lookup 回复 EAX 精确0 时，`sub_4315D0` 才于 `0x00431740/0x00431741` 重压同两参调 `0x00431742 sub_431C50`；
可能是缓存未命中，也可能是命中后 `found+8` 回绕0，不可把 fallback 的启动原因改写成仅“未命中”。`build/workpack316/cache-fallback-prefix-audit.tsv` 在该124条指令 callee 的前47条中标明26个独立访问指令地址/27个物理读写站（`0x00431C93 inc word` 分读、写两站）。

本 caller 下其入站 ESP=F=`S−0x44`，首先把桶 `word_4DACE0[bucket]` **低16位递增**、写游标 `4DACDC=bucket_base`、读当前桶头；
随后 `0x00431CA4 mov [esp+0x14+arg_0],edx` 将**旧桶头覆盖进本 callee 的 caller 实参栈槽 `F+4`**，
原 arg0 另存 ESI/EBX。这个栈写发生在 `0x00431CA8 sub_487C10(size=0x20)` 之前，分配深层停点不可遗漏该提交；
callee `0x00431CE6` 在清节点之后又重读同一栈槽作 old head。分配正常回 EAX0 时没有测试：`0x00431CB6` 仍将桶头写0，
紧随 `0x00431CCE rep stosd` 从此0地址逐次尝试8个 dword，若实际零页不可写才停住；不能提前返回并保留旧桶头。

到站 ESP=`F−0x10=S−0x54`、EAX=0、ECX=8、EDI=分配 token，DF 取分配 callee 出站实际值，
算术 FLAGS 来自 `0x00431CC9 cmp si,0xFFFF`，REP 本身不改算术 FLAGS。REP 正常后还要重读桶/全局，
再顺序写新节点 `+0` 的原 arg0 低 word、`+2` 的 arg4 低 word 和 `+4` 的**栈槽 old head**；
任一写障保留计数、游标、实参栈和更早节点写。由 `0x00431C81 push 0x20` 与这些字段可核其缓存节点最少 0x20 byte，
物理结构 `+0=packed key/+4=next/+8=payload`，不同于 0xAC-byte 动作节点和 0x38-byte 粒子节点。

低16为 `0xFFFF` 时 REP 后保留先前 CMP 的 ZF，转 `sub_40AD10` 特殊加载，其他值则进入多分支文件加载；
后77条与六次嵌套 CALL 尚未全审，此前缀也未运行测试。

fallback 余下77条的分支索引见 `build/workpack316/cache-fallback-suffix-audit.tsv`，
不替代深层 callee 审计。若 `arg0` 低16恰0xFFFF，先压 `node+8` 与 `(arg4&0xFFFF)=0` 调 `sub_40AD10`，
正常后忽略其回复 EAX、清两参并汇入共同尾；其他值先写 `4DACD4=0`，仅在低16位闭区间 `0x1771..0x2328` 内改写为1并使用 `4FB0D4` 的 magic handle 调 `sub_431AA0`，
再通过计算的普通文件 handle 调 `sub_433380`。该 `0x00431D7D` 的物理读地址是 `dword_4FB0CC[index*4]`；
按 LST 的 `imul 0x057619F1/sar 6/shr 31/add` 在全部65536个 u16 输入上机械比对 `index=floor(value/3000)` 为零差异，
非 sentinel 的索引可达0..21，**不限制于**先前六个 TSW 全局，6..21 仍会读取后续实际物理地址，不能自动夹紧/报错。

`sub_433380` 正常回0时先调两指令诊断 callee、再 EAX0 提前返回，桶计数/旧实参栈/已分配节点及先前魔法分支均不回滚；
正常非零时再调 `sub_401C70`，与 sentinel 分支汇入 `0x00431DBF`。共同尾顺序为重读游标 `4DACDC`、重读累计值 `4DAD0C`、恢复 EDI/ESI、读节点 `+0x18` dword 长度、`0x00431DD4` **先发布** `4FB0C8=node+8` token、恢复 EBP、`0x00431DDA` **后写** `4DAD0C=old+length`，
最后 EAX设1、恢复 EBX 并 `retn`；长度写障时 token 已发布，但 `sub_4315D0` 尚未取得正常回复。

`sub_40AD10/sub_431AA0/sub_433380/sub_401C70` 的深层可障读写与回调仍未完成，因此不宣称本 callee 或其上层 CALL 已全语义验收。
- case1 `0x004799DC..0x00479B05`：signed phase `>480` 转公共重置尾；零 phase 先播0x31。
  读取 frame `+0x00` 发布 `dword_4CD730`；把 i16 phase 符号扩展后用有符号常量 `0x7BDEF7BD` 做三次 `imul`，
  每次取 EDX 高32位减 phase，再算术右移4、加结果的无符号最高位、左移1，按顺序发布 `dword_4CD71C/4CD30C/4CD304`，
  不得把 `imul` 低32位当结果。`dword_4CD75C=zero_extend(frame.height)+sign_extend(phase)`，
  `+0x2694=旧值&0x80000023 | 0x20`。绘制 `sub_4170E0` 六参数依次为 `x=sign_extend(actor+0x0D66)-EBP`、`y=sign_extend(actor+0x0D68)-actor[+0x3E4]-4*dword_4CD71C-sign_extend(phase)`、`width=zero_extend(frame.width)`、`height=zero_extend(frame.height)`、`flags=actor[+0x2694]`、`0`；
  回复被忽略，再将 phase 按 word 加31，返回0。CALL 前帧指针的重复读取不得与初次 `+0x2548` 查询折叠。

- case1 Draw 前单块 `0x00479A02..0x00479AEC` 的实际物理访问顺序：读 `actor+0x2548`，
  再解引用源首 dword 发布 `dword_4CD730`；三次分别重读 `actor+0x2958` 的 signed word，
  以 `imul ecx` 的**有符号 EDX 高乘积**减 phase、`sar 4`、无符号 `shr 31` 符号补偿、`shl 1` 依次发布 `dword_4CD71C/4CD30C/4CD304`。

  再次读源 token、phase word、源 `+0x0E` 的 unsigned word 并发布 `dword_4CD75C`；
  随后读 `actor+0x2694`，AND `0x80000023` 再 OR 低 byte `0x20` 写回；第三次读源 token，
  然后读 `actor+0x3E4`、源高/宽 `+0x0E/+0x0C`、actor `+0x0D68`、刚发布的 global `4CD71C`、phase word、actor `+0x0D66`，
  按原 push 顺序调用 `sub_4170E0`。只在调用正常返回后才对 `actor+0x2958` 的 **word** 加 `0x1F`，
  并清 EAX=0 返回；源首 dword 或源宽高无法读取时故障前缀不同于调用失败后缀。对正 phase 30/31，该高乘积算法给出偶数位移分别为0/-2，
  不可用简化的普通比例四舍五入替代。按同一 LST 有符号高乘积计算，phase `-32768` 和 `+32767` 的双倍位移分别为 `+2114` 与 `-2114`；新增两端向量在定向 `battle.actor_frame_316` 1/1（`proc_8ece`）通过。该块的绘制子调用及其故障仍为 partial。

- case1 故障栈与物理写 `0x00479A02..0x00479B05`：首次读 `actor+0x2548` 后、读源首 dword **之前** `0x00479A0D push ebx=0`，
  所以源首读障已有4 byte参数停在 ESP；三次各自读 phase 并依序写 `4CD71C/4CD30C/4CD304`，再**第二次**读源 token、phase、源 unsigned H，
  写 `4CD75C=H+phase`，随后 full dword `+0x2694` AND 掩码、只在 AL OR0x20并写回。

  **第三次**读源 token，再读 `+0x3E4`、压已改写 actor flags、读源 H/W、压 H、读坐标、**重新读刚写的 `4CD71C`**、压 W、重读 phase 等计算 Y，
  压 Y/X 调绘制。若 CALL 在入站停，6 dword仍在 ESP、flags已写、全局四字段已发布；正常绘制后 `0x00479AF1` **先对 phase word加31**，
  到 `0x00479AF9` 才清累计24 byte，所以 phase 加法写障仍留下这六参。此处不能把 `4CD71C` 计算时的 EDX 局部值传给后续坐标，
  而跳过 global 的实际重读及可能故障。

- case2 `0x00479B06..0x00479CA1`：phase 精确等于 CX=100 时，先清 phase、`+0x2A12`，
  以保留的 EDI=`actor+0x3D0` 清38 dword；然后读取 `+0x0E14` 并仅在非零时调用释放，设置 EDI=`actor+0x0E14`、ECX=22，
  进入 `0x0047B814` 清 `[+0x0E14,+0x0E6C)`，调 `sub_478850` 后依次置三个 latch 并返回1。

  非100、`+0x0E14` 非零者不重新初始化；为零时先清粒子区22 dword，再以 frame 源指针及三个栈槽调用 `sub_4019A0`，
  发布 token/宽高/源目标位置/大小/计数/flags，调用 `sub_47CE70`，仅 EAX 恰为1 时 OR `+0x0E3C` 的低 byte。

  再按顺序调用 `GetSystemMetrics(1)`、`GetSystemMetrics(0)`、`sub_433F30`、声音0x31，
  写 `+0x2958=1`，最后进入公共粒子尾。两个 `GetSystemMetrics` 是独立的间接 CALL；上述调用均可能按各自结果留下不同前缀，
  不得把 release 和初始化交换顺序。初始化 LST `0x00479B1F..0x00479C1C` 的具体重读顺序为：清22 dword → 第一次 `+0x2548` 与源首 dword → `sub_4019A0` → 先写 `+0x0E14`，
  再重读 `+0x2548`/源宽并写 `+0x0E18` → 重读 `+0x2548`/源高并写 `+0x0E1A` → 读 `+0x0D66` 写 `+0x0E1C` → 读 `+0x3E4/+0x0D68` 写 `+0x0E20` → 重读 `+0x2548`/`+0x0D66`/源宽写 `+0x0E24` → 再读 `+0x2548/+0x3E4`/源高，
  先 OR `+0x0E3C` 低 byte 0x16，才读 `+0x0D68` 并写 `+0x0E30`、`+0x0E2C`、`+0x0E28` 等余下字段。

  若 decoder 或中途 owner 改变 `+0x2548`，不能把五次源 token 访问归并为入口 snapshot；源宽高/`+0x3E4` 在不同写点前出现物理读障。
- case2 初始化/停止细化 `0x00479B1F..0x00479C67`：22 dword `rep stosd` 后压 decoder 四参，
  回复先写 `+0x0E14`、读第二次源 token，**再**清16 byte，以第二次 token 读宽写 `+0x0E18`；
  第三次 token 读高写 `+0x0E1A`。写 `+0x0E1C=X-Ox`、`+0x0E20=Y-Oy` 后第四次读源 token/宽并写 `+0x0E24=X+(W>>1)-Ox`；
  第五次读 token 后把 EBP **覆盖为 `actor+0x3E4` 的 Oy**，读源高、OR `+0x0E3C` 低 byte0x16，
  再读当前 Y，依次 `+0x0E30=2`、`+0x0E2C=Y+(H>>1)-Oy`、`+0x0E28=2`、`+0x0E36=50`、`+0x0E34=250`、`+0x0E38=1`、`+0x0E3A=10`；
  次序不得从字段 offset 排序推导。`sub_47CE70` 以 actor 为 ECX 且 EBP仍为 Oy；属性 bit0回复1才再 OR低 byte1。

  两次 `GetSystemMetrics` 的 IAT 函数地址在 EDI，`sub_433F30` 正常 `retn 8` 清两个 metric 结果；
  **先写 `actor+0x428` word0x31**，再压声音样本与0x31调用 `sub_485610`。声音正常返回后才清其8 byte、写 phase word1，
  再进入粒子尾。若声音停在 CALL，`+0x428` 已写、phase尚未写1、EBP=Oy；如果 decode 后第二次源 token 读障，
  token 已写但其16 byte参数未清。非空 `+0x0E14` 不执行以上初始化并复用原记录，也不强写 phase1。

- case2 释放 `0x00479C6C..0x00479CA1`：phase word精确100时先写 phase0、`+0x2A12=0`，
  用原 EDI从 `actor+0x3D0` 清38 dword，**之后**读 `+0x0E14` token 并置 EDI=`actor+0x0E14`，
  仅非零压 token 调 `sub_4885A0`、正常后清4 byte，再按共享 `0x0047B814` 清22 dword 并 reset；
  case100 的释放顺序恰好相反（先 release、后38+22次清），不可共用事务。释放 CALL 停住时前38 dword已清、后22 dword未清；
  DF方向/部分 REP 故障另按原逐 dword 序列保留。

两处 `sub_4885A0` 资源释放 CALL（`0x00479C94/0x0047B6F9`）的外层 wrapper 只有9条指令，
但并非无副作用的 token 回调。`build/workpack316/release-wrapper-audit.tsv` 按 LST 登记七处可障栈/CALL站点：
若 wrapper 入站 ESP=C，依次保存 EBP、压内层 arg4=1、在 `0x004885A5` 从 `C+4` 读原 actor `+0x0E14` token、压内层 arg0=token、调 `sub_4885C0`；
仅内层正常普通 `retn` 后，wrapper `0x004885AE add esp,8` 抛弃两内参并**重算 FLAGS**，
弹 EBP、自身普通 `retn`，父级之后才清外参4 byte。该 wrapper 所调 `sub_4885C0` 是342条指令、20个嵌套 CALL、10处 `int 3` 与 CRT heap/调试 hook 分支，
不可把 `free(token)` 的成功或 typed-stop 当作已经完成全部深层路径。若其首个非栈读 `0x004885C7 dword_4A82F4` 故障，
已发生 wrapper 两 PUSH、嵌套 CALL 返回地址写、CRT 的五次栈写（EBP/ECX/EBX/ESI/EDI，ECX 是局部槽而非寄存器恢复保存）；
相对本函数入口 S 的 ESP 为 `S−0x50`，EAX=原 token、EBP=CRT 局部帧、FLAGS 仍是释放入口前的 `cmp token,EBX`，
释放后的 parent REP 均不能在这一停点执行（case2 的前置38项已执行，case100 的38与22项均尚未开始）。

现将该条件性首非栈读落实到 case2 `0x00479C94`、case100 `0x0047B6F9` 与链节点内 `0x0047F0E1` 三个生产续段：外层参数及 CALL 返回槽写入后，依次执行 wrapper `PUSH EBP`、`PUSH 1`、原参数栈读、`PUSH EAX`、内层 CALL 返回槽、CRT `PUSH EBP/ECX/EBX/ESI/EDI` 共十次访问；窄 release port 不返回时不再回滚到 wrapper 入口，而在首次未拥有的 `0x004885C7` 全局读前保留真实 ESP、两层 EBP、EAX、末次 EDI 保存值、FLAGS/DF 和先前 actor／链节点写入。逐访问序号 0–10 验证每一步停点 IP、kind、物理 token、ESP 和访问计数；序号恰停全局读前或全局 owner 缺失时不调用 release port，另对 case100/链节点也核不调用 port 的全局前障；正常回包的父级已知访问数追加十项。定向 `battle.actor_frame_316` 于 `proc_15ce` 1/1 通过（首次 `proc_2d87` 因两处旧测试仍用入口停止前访问数而失败，已按新物理前缀修正）。格式化后阶段快照的串行完整门禁 `proc_65bc` 通过 Linux core 200/200、ASan core 200/200、Linux app 206/206。这仍只是当时受控回包与首非栈读的局部前缀；`0x004885C7` 及后续 CRT hook/全局/链表/`int3` 异常和真实原版差分当时仍未核定，不能当作 Workpack 316 最终门禁。

随后按相同全局 `0x004A82F4` 的已持有 decoder debug-flags owner，
在三处 release 续段先对 `0x004885C7 MOV EAX,[dword_4A82F4]`
单独设读前障；缺 owner 或显式不可读时不调用 release port。
成功读后 `AND EAX,4`、`TEST EAX,EAX`：掩码结果为零则停在
下一个 `0x00488603 CMP [EBP+8],0` 栈实参读前，EAX=0、
ZF/PF=1、CF/OF=0、AF 未定义，ESP 不变，前缀新增一次真实访问；
掩码结果非零则先检查 `0x004885D3 CALL sub_488BB0` 返回槽写，
可于写前停；写入后在调试子调用入口 `0x00488BB0` 停住，
不伪造其返回或跳至非调试路径。case2 的 owner 缺失、
debug 掩码0x4 的两停点及原有 C7 逐序号，链节点每次释放
新增的一次访问，定向 `proc_d25a` Linux core 200/200。
该阶段尚未读取 488603 栈字或执行 CRT hook／heap／int3；
阶段快照 `proc_9a04` Linux core／ASan core 各200/200、
Linux app 206/206，不能代替316最终门禁。

三处已知非零释放 token 的非调试分支随后于 `0x00488603`
从已压入的内层 `arg_0` 栈槽读 token 并 `CMP token,0`；
非零分支按物理次序在 `0x0048860E/10/12` 压三个零、
`0x00488614` 读 wrapper 先前压入的 `arg_4=1`、
`0x00488617` 压1、`0x00488618` 压0、`0x0048861A`
重读同一 `arg_0`、`0x0048861D` 压 token、`0x0048861E` 压3。
在间接 `CALL ds:off_4A8360` 的内存操作数 `0x00488620`
读取前停下，**尚未压该 CALL 的返回槽**，不调用 release port；
ESP 相对父 CALL 前减72，EAX=token、EDX=1、末次 PUSH=3，
FLAGS 来自 `CMP token,0`。无 hook 全局 owner 或显式不可读
同样在操作数前停；零 token 只到 `0x004889A5` 的未建模
CRT 尾部，不错误调用 hook。case2 C603–C620 共11处前障
核 IP、token、ESP、计数及无提前端口调用，链节点每次正常释放
相较上一阶段新增十项访问；阶段 `proc_5f1c` Linux core 200/200。
间接钩子、CRT 深层和真实父路径仍 partial；阶段快照 `proc_da14` Linux core／ASan core 各200/200、Linux app 206/206，不是316最终门禁。

继续区分间接 CALL 的两次物理访问：在 `0x00488620`
从持有的 `decoder_heap_alloc_owner` 读取 `0x004A8360` 中的
目标 guest 地址，先检查返回槽 `ESP−4` 的写前障；成功后
压 `0x00488626` 并以**实际读取的**目标地址为 EIP，
非返回的窄 release port 停在该入口，保留 EAX=token、
EDX=1、ESP 相对父 CALL 前减76和源于 CMP token,0 的 FLAGS。
原有11个读前站不前移，新增返回槽独立站，三处正常受控
回包的计数再加两项。测试中的 owner 是合成地址；当前源码
只在测试构造中显式赋值该字段，实际 caller 通过
`caller_snapshot` 传递请求，尚未证明生产 owner 已绑定，
不能将测试 hook 地址或受控回包冒充原版钩子执行。
`proc_e1d3` 定向 Linux core 200/200；阶段快照 `proc_9311` Linux core／ASan core 各200/200、Linux app 206/206，尚非316最终门禁。
另以 case100 `0x0047B6F9` 和链节点 `0x0047F0E1` 的
真实调用续段，分别撤掉 hook owner；两处均在 `0x00488620`
IAT 首读前停下，已压实参和 actor／节点先前状态保留，
release port 调用数为零。定向 `proc_064b` core 200/200；
阶段快照 `proc_14ec` core／ASan 各200/200、app 206/206。
生产 caller 的 owner 初始化与深层 hook 仍未证实。

已持有的 `0x004A8360` 槽若明确含零，与缺 owner 不同：
`0x00488620` 先读出零目标、再写入 CALL 返回地址
`0x00488626`，ESP 相对父 CALL 前减76，此后只能在
guest EIP=0 的取指入口停止；原版具体异常/SEH 路径
尚未捕获，不能虚构 release port 返回。case2、case100
与链节点各有零目标独立测试，均不调用 release port，
同时检查已提交的返回槽、ESP 和访问数；
`proc_da29`、`proc_f062` 分别 core 200/200；阶段快照
`proc_8c12` core／ASan 各200/200、app 206/206，
尚非316最终门禁。

CRT 调试掩码0x4置位的分支从 `0x004885D3` 继续进入
`sub_488BB0`：在 `0x00488BB0` 压旧 EBP，`0x00488BB3`
以 `SUB ESP,0x18` 仅预留局部栈且重算 FLAGS，再于
`0x00488BB6/BB7/BB8` 压 EBX/ESI/EDI，`0x00488BB9`
向局部 `[EBP−4]` 写入1。此后停在第二次调试位全局读
`0x00488BC0 [0x004A82F4]` 之前：相对父 CALL 前
ESP 减88、EBP 减52、EAX 保持4，FLAGS 来自真实
ESP 的 SUB，末次局部写和先前 CALL 返回槽均保留；
`last_pushed_value` 仍为 BB8 保存的 EDI，不把 BB9 的 MOV 误记成 PUSH；
未执行 heap-check 的任何分支或深层 hook。case2
在五处栈写前分别核 token、ESP、EBP、FLAGS 和
release port 零调用；`proc_427f/proc_db76` core 各200/200；阶段快照
`proc_f8fe` core／ASan 各200/200、app 206/206；
修正 BB9 MOV 与末次 PUSH 的区别后，最终阶段快照
`proc_3444` core／ASan 各200/200、app 206/206。
其余两处共享前缀尚未单独覆盖此分支。

第二次 `0x00488BC0` 调试位重新读取 owner 并独立计数，
不复用第一次全局读取的缓存值。合成掩码4
经 `AND EAX,1` 和 `TEST EAX,EAX` 为零，`MOV EAX,1`
后按 `0x00488EEA` 起的 EDI/ESI/EBX/EBP 栈恢复与
`0x00488EF0 RET` 各自访问前障回到外层
`0x00488603` 参数首读前；此时 ESP 为父 CALL 前减44，
EBP 为父 CALL 前减28，外层 `XOR/TEST ECX,ECX`
留下 ZF=1，不伪造释放结果。合成掩码5 则在
`0x00488BD6 CALL sub_48B3E0` 返回槽写前可独立停；
成功压入 `0x00488BDB` 后停在深层校验入口，ESP
相对父 CALL 前减92，未执行下游校验。case2 对第二全局
读序号、两个掩码及 CALL 槽均核 no-release-port；
`proc_6cf5` core 200/200，初次阶段快照 `proc_d37c`
core／ASan 各200/200、app 206/206；改为真正二次解引用后
`proc_4279` core／ASan 各200/200、app 206/206；
补齐五处恢复前障与外层首读后 `proc_f06f` core 200/200；
阶段快照 `proc_c5db` core／ASan 各200/200、app 206/206。
其余调用、CRT 深层与原版异常仍 partial。

合成掩码4 恢复外层后，缺 `decoder_heap_alloc_owner` 的
case2 路径继续按原版参数 PUSH 顺序停在 `0x00488620`
释放钩子 IAT 读前：ESP 相对父 CALL 前减72、EAX 为
资源 token、EDX=1、末次 PUSH=3、release port 零调用；
相比非调试分支多12次已完成的内层物理访问，
`proc_2d3c` core 200/200，阶段快照 `proc_b6e6`
core／ASan 各200/200、app 206/206。生产绑定静态审查：
`include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp`
两项 owner 默认空；
`src/battle/legacy_battle_actor_frame_presentation.cpp`
只把 `caller_snapshot` 复制到 `child_request`，并仅为
`draw_source_token_owner`／`draw_height_third_owner` 补绑定。
四个父调用通过 `LegacyBattleActorFrameCallerBinding` 可选快照
进入该路径；当前 `src/` 内没有对这两项堆 owner 的赋值，
测试中的指针为合成输入。故本阶段只证明源码可消费注入值，
**未证明真实生产调用已初始化 owner**；不得把缺绑定的
IAT 读前停点当成原版正常释放。

合成掩码5 在 `0x00488BD6` 压入调用返回槽后，
`sub_48B3E0` 再顺序压 EBP、ECX，向 `[EBP−4]`
写 `0xFFFFFFFE`；末次局部 MOV 不改写 PUSH 记录。
在 `0x0048B3EB CALL sub_48C9B0` 先核独立栈写
前障，写入返回地址 `0x0048B3F0` 才停于
`0x0048C9B0`：相对父 CALL 前 ESP 减104、
EBP 减96，EAX=1，FLAGS 仍来自 `TEST EAX,EAX`，
release port 未调用。四处写前障均核已完成计数与
末次 PUSH，`proc_4bed` core 200/200，阶段快照 `proc_0560`
core／ASan 各200/200、app 206/206；`sub_48C9B0`
内部、后续堆验证 Win32 API 与原版异常尚未执行。

`sub_48C9B0` 入口再压 EBP、`SUB ESP,0x168` 预留
局部栈并更新 FLAGS；首次未绑定池索引全局读取
`0x0048C9B9 [0x0053E7B4]` 前停，ESP 相对最初
父 CALL 前减468、EBP 减108，末次 PUSH 为旧 EBP，
不虚构索引值。独立测试在入口 PUSH 前障停下；
即使合成 owner 已持有，也只以 opaque 子级状态停在
同一读前边界，不提前使用索引或调用 Win32 检查。
`proc_93d9` core 200/200，阶段快照 `proc_f1a8`
core／ASan 各200/200、app 206/206。

已持有的池索引在 `0x0048C9B9` 真正读取后，
`IMUL EAX,0x14` 以 signed32 输入计算低32位；仅 CF/OF
有效，其余算术 FLAGS 未定义，不能继承预留局部栈的
FLAGS。`0x0048C9C1` 单独压计算出的字节数，再在
`0x0048C9C2 [0x0053E7B8]` 池基址读前停；索引0
压0、−1 和 `0x7FFFFFFF` 均压 `0xFFFFFFEC`，
但后两者 CF/OF 分别为0/1。缺基址 owner 与已持有
基址的 opaque 停点不等价，且均未进入 `IsBadWritePtr`。
`proc_ab74` core 200/200，阶段快照 `proc_d818`
core／ASan 各200/200、app 206/206。

已持有的池基址 `0x0053E7B8` 经 `0x0048C9C2`
真实读取到 ECX，随后 `0x0048C9C8` 单独压入 ECX，
在 `0x0048C9C9 CALL ds:IsBadWritePtr` 的 IAT
`0x00499138` 目标读前停；相对原父 CALL 前 ESP 减476，
末次 PUSH 是池基址，EAX 保持有符号乘后的低32位，
未提交此间接 CALL 返回槽，未调用 Win32 后端。
三处读/写前障分别核 IP、访问序号、token 与此前栈状态；
`proc_a9ee` core 200/200，阶段快照 `proc_847d`
core／ASan 各200/200、app 206/206。
合成池基址不证明真实生产 owner 或原版内存池状态。

#### 已持有默认释放hook的真实叶函数前缀（WIP）

LST `.data:004A8360` 的静态初值是 `0x0048AA70`，不代表运行时
owner已经绑定；只有真正读取的目标等于该地址时才执行已核叶。
`0x0048AA70/71/73/78/79` 依次为 `PUSH EBP; MOV EBP,ESP;
MOV EAX,1; POP EBP; RET`，不按请求类别或token改变返回值。
此RET只回到 `0x00488626`，**不是**CRT或外层释放wrapper的返回。
其它非零目标仍保留实际入口，零目标仍先提交CALL槽再停EIP0；
缺owner仍在IAT首读前停，不用静态初值补造绑定。

三处共享release续段现追加默认叶的PUSH、POP、RET三个物理访问，
各有独立序号前障。设外层wrapper入口ESP=C，叶入站ESP=C−68、
内层CRT EBP=C−20；PUSH写C−72后才设置EBP=C−72、EAX=1。
POP前故障保留该新EBP；POP成功后RET前故障恢复EBP=C−20、
ESP=C−68。两次读不修改末次PUSH记录，它仍是保存的C−20。
叶返回后 `0x00488626 ADD ESP,1Ch` 清七个hook参数，ESP=C−36；
`0x00488629 TEST EAX,EAX` 留CF/OF/ZF/SF/PF=0、AF未定义，DF不变，
`0x0048862B JNZ` 到下一实参读 `0x00488658 [EBP+8]`，token=C−12。
该读的序号前障不调用release port。不透明整段CRT回包不返回时，
也在此尚未建模的读前保留已执行叶的GPR/ESP/FLAGS/DF；没有把叶的
EAX=1当作整个free成功。正常受控整段CRT回包仍只支持条件性父尾段
检验，不能证明后续 `sub_488F90`、堆元数据、Win32 free或SEH已完成。
独立台账为 `build/workpack316/release-default-hook-audit.tsv`。

case2以DF两方向和合成debug flags0/1/4核18个叶访问前障、6个后续
实参读前障及6个不透明尾段停点，另核异目标不套用默认叶；链节点和
case100消费相同前缀。两处旧链节点计数仅追加这三个真实访问。
`proc_30f1` 在旧源码下恰有两个新增断言失败；首次实现的
`proc_3b55` 新断言均通过，但旧链节点两个计数断言失败。修正计数和
重编译暴露的四处既有警告后，`proc_64a4` 定向1/1通过，diagnostic=0。
警告修正只给固定1/2/4字节写宽使用guest域u32、声明不使用的值参数，
并显式保留入口局部switch对未使用kind的旧缺省行为，不改变读写或分支。
受管 `wp316-default-release-hook-full-gates`（本轮ID `proc_f527`）
229秒退出0；`build/tmp/runtime/wp316-default-release-hook-{core,asan,app}-full.log`
分别核core200/200、ASan200/200和app206/206，diagnostic均0。
三份LastTest均实际执行17组动作数据与seq372受限差分，无资产缺失跳过。
此ID与历史早期入口验证同名，不以裸ID替代本轮进程名和日志路径。
上述是静态LST与UT证据，**没有**新原版释放差分，也未解决两项heap owner
的生产赋值。

门禁后继续独立审查发现，仅按EIP=`0x00488658`识别已返回叶不足：
held异目标也可直接等于该地址，却仍持有CALL返回槽和七参数。
新增同址反例的 `proc_d2b3` 恰有该一个断言失败，无编译诊断。
现同时核LST栈关系：已返回叶并清参数后 `ESP=EBP−0x10`，
而异目标入口 `ESP=EBP−0x30`；后者保留callee-call kind、token0、
EAX=原资源token和未弹CALL槽，不错误归类为下一栈读。
不新增推测owner或依赖静态全局初值。受管
`wp316-default-release-hook-collision-final-gates`（`proc_0e1c`）
264秒退出0；collision-directed-final及collision-{core,asan,app}-full
四份正式日志分别核1/1、200/200、200/200、206/206，diagnostic均0。
三份LastTest实际执行17组动作和seq372尾段局部差分，无条件跳过。
这是默认叶与同址分类修正快照的门禁，仍不是316整包最终验收；
后续校验前缀推进不能沿用它作为最新源码验证。
堆owner的静态初值、唯一命名setter的直接查询路径与C++赋值缺口另记
`build/workpack316/heap-owner-producer-navigation.md`；命名引用搜索不证明
已经排除间接别名或运行时修改。全部仍为316最终REVIEW的WIP，CRT深层
和inventory状态不升级。

#### 默认释放后的堆指针校验栈前缀（WIP）

继续从原始LST独立推导 `0x00488658` 的实参重读、PUSH及CALL，
进入 `sub_488F90`，再进入 `sub_488F40`。普通case以父压参前ESP=P计，
非零header路径新增18个物理栈访问，停在 `0x00488F52`
`CALL ds:IsBadReadPtr` 的 `0x004990BC` IAT首读前；ESP=P−92，
EBP=P−80，EAX=20h，ECX=token−20h，EDX=token。
该IAT未读、Win32 CALL返回槽未写，也没有Win32返回值。
末次PUSH是header；FLAGS来自 `CMP header,0`，不是先前SUB的CF/OF。
`SUB EAX,20h` 按32位回绕并在下一CMP前保留其借位和有符号溢出。

合成token=20h时header为零，`0x00488F48 JZ` 在任何Win32压参之前
跳 `0x00488F7D MOV [EBP−4],0`。上一快照只执行前14个栈访问，
在局部写前停，ESP=P−84，EBP=P−80，EAX=0。
本轮追加该局部写、结果读、两个POP与两个RET共六个物理访问。
MOV局部零不是PUSH，不覆盖末次PUSH；它仍为保存的ECX。
`488F87 MOV ESP,EBP` 先于488F89 POP读，恢复488F90的EBP；
488F8A RET只回488FAE，清12字节参数、TEST0、XOR0后跳489012，
第二次MOV ESP,EBP再先于489014 POP，489015 RET只回488661。
清最后4字节参数、TEST0后到488668断言首PUSH前；ESP=P−44、
EBP=P−28，EAX0，ECX保留或debug4下0，EDX20h，ZF/PF1、AF未定义。
没有断言PUSH、报告调用、INT3或完整free返回；不把两个内部RET当成释放成功。
真实序号前障阻止release port调用，opaque停止保留全部已执行现场。
显式held异目标等于488F52、488F7D或488668时，其ESP仍为EBP−30h；
不借同址EIP冒充已完成前缀。真实零结果返回后的488668为ESP=EBP−10h。

`tests/unit/battle/legacy_battle_actor_frame_free_validation_test.cpp`
使用独立LST地址、栈偏移和算术FLAGS推导408个访问前障，覆盖DF两方向、
debug masks0/1/4，以及普通、SUB溢出、SUB借位、header零四类token。
这些值全是测试输入，不声称原版动态值。台账为
`build/workpack316/free-validation-prefix-audit.tsv`。
`proc_5799` 在旧源码下恰有两个新增断言失败，无编译诊断。
首次推进的 `proc_e92e` 新向量全部通过；七个旧停点／计数断言失败，
另暴露TSW指针TEST的popcount有符号警告。旧断言按LST校验前缀更新，
两链节点计数再加18；parity仅改等价整数奇偶表达式，不改可观察行为。
最终快照门禁由 `proc_e1e6 / wp316-free-validation-prefix-final-gates`
342秒退出0；四份 `wp316-free-validation-prefix-{directed-final,core-full,
asan-full,app-full}.log` 分别核1/1、200/200、200/200、206/206，
耗时0.55s、20.93s、33.93s、81.93s，诊断计数均0。
三份LastTest实际打印17组动作与seq372局部差分，scoped计数各2、跳过0。
这是前一校验栈前缀源码快照门禁，不是316整包验收。

零header续段的 `proc_db6a / wp316-null-validation-return-red` 在旧源码下
恰有两个新增断言失败，编译诊断0。新增36个独立LST前障覆盖DF两方向、
debug0/1/4的六访问；原408个前缀向量仍保留，共444个访问前障。
另核六个断言首PUSH序号前障和六个opaque尾段现场，并增加488668异目标。
最新 `proc_b069 / wp316-null-validation-return-final-gates` 215秒退出0；
四份 `wp316-null-validation-return-{directed-final,core-full,asan-full,
app-full}.log` 分别核1/1、200/200、200/200、206/206，耗时0.61s、
23.46s、36.55s、84.94s，四日志诊断0；三LastTest各实际执行17动作与
seq372局部差分，scoped输出2、跳过0。无新原版释放差分。
六访问独立台账为 `build/workpack316/null-validation-return-audit.tsv`。

release port的接口注释同步明确：仍传原caller ABI，不传深层活跃GPR；
false回复没有未知续段的观察，不能回滚显式前缀；受控true也不证明CRT。
没有新原版释放差分、Win32/SEH实现或两项heap owner生产赋值证明。
三处释放消费者复用此前缀，不代表98 CALL/22 RET深层已经闭合。
本次将已验证的生产修正、对应测试和局部差分共同作为阶段发布边界；
316最终REVIEW仍未完成，inventory保持pending_audit。未验证的断言续段草稿不纳入提交。

case100 `0x0047B723` 的 reset CALL 另用已持有的独立 actor
配置 owner 将 `+0x2AA0` 设为1，沿完整 `sub_478850` 子函数
走过重复写入，再在 `0x00478A4D` 的随机子 CALL 停下；
该站的 EIP 是 `0x00439070`，父级三处尾写和 RET 尚未执行。
`random_callable=false` 时 CALL 已压实参和返回槽，但未真正调用
随机端口，故随机调用计数、bound 发布和端口调用数均为零；
不能把 PUSH 140 当作端口已收到 bound。首次断言误预期调用数1、
随后误预期 bound 已发布，`proc_5299/proc_2494/proc_7415`
分别失败；按该物理边界修正后 `proc_b106` core 200/200，
阶段快照 `proc_9131` core／ASan 各200/200、app 206/206。
这只覆盖 case100 的受控子路径，其余三个 reset CALL
的全路径与真实父 caller 仍待 REVIEW。

- case3/4 `0x00479CA6..0x0047A07E`：两支均在 signed phase `>32` 时跳公共重置，
  phase0 先播0x31；先读取 frame `+0x00` 发布 `dword_4CD730`，随后顺序调用矩形 `sub_416FF0`、绘制 `sub_4170E0`、第二个矩形、第二个绘制，
  最后把 phase 按 word 加2，**仅此时** `add esp,0x50` 一次性清掉四个调用累计压入的80字节，再以参数 `(0,0,640,480)` 调第三次矩形并返回0。

  case4 第二个绘制在 `0x0047A07E` 跳回 case3 的 `0x00479E7C`，不能另做独立收尾。设 `P=sign_extend(actor+0x2958)`、`X/Y=sign_extend(actor+0x0D66/+0x0D68)`、`W/H=zero_extend(frame+0x0C/+0x0E)`、`Ox=EBP`、`Oy=actor[+0x3E4]`，
  均用32位回绕算术：case3 矩形一 `(X-Ox,Y-2P-Oy,X+floor(W/2)-Ox,Y+H-2P-Oy)`；绘制一 `(X-Ox,Y-2P-Oy,W,H,actor[+0x2694]|4,0)`；
  矩形二 `(X+floor(W/2)-Ox,Y+2P-Oy,X+W+2P-Ox,Y+H+2P-Oy)`；绘制二 `(X-Ox,Y+2P-Oy,W,H,actor[+0x2694]|4,0)`。

  case4 矩形一 `(X-2P-Ox,Y-Oy,X+W-2P-Ox,Y+floor(H/2)-Oy)`；绘制一 `(X-2P-Ox,Y-Oy,W,H,actor[+0x2694]|4,0)`；
  矩形二 `(X+2P-Ox,Y+floor(H/2)-Oy,X+W+2P-Ox,Y+H-Oy)`；绘制二 `(X+2P-Ox,Y-Oy,W,H,actor[+0x2694]|4,0)`。

  每支仅在第一个矩形之后，按顺序将 `dword_4CD71C/4CD30C/4CD304` 均写为 `-P`；这些 OR4 仅作调用参数，
  不写回 actor `+0x2694`。重复访问帧宽高及资源 token 的真实位置不得前移。

- case3 首两 CALL 块 `0x00479CCD..0x00479DAE` 的故障/写前缀：从 `+0x2548` 的源首 dword 写全局 `4CD730`，
  读 phase、`+0x0D66`；重读源 token/高，再读 `+0x3E4`；**第三次**读源 token/宽，以半宽与 `+0x0D68` 拼出第一个 `sub_416FF0` 矩形。
  该矩形正常回复后才三次重读 signed phase，依次将其相反数写入 `4CD71C/4CD30C/4CD304`，然后重读源 token、高/宽、`+0x2694`、`+0x0D68/+0x0D66/+0x3E4` 构造 `sub_4170E0`：
  标志采用 **局部 OR4** 的 `+0x2694` dword，未在该点写回 actor；以 EDX 零扩展的高、ECX 零扩展的宽传参。

  因此首矩形 CALL 停止时缩放全局尚未刷新；绘制 CALL 停止时三项都已按前缀发布。上述四次 `+0x2548` 读取不能合并为入口 snapshot。
  首绘制正常返回后，第二个矩形又读 `+0x2548`、phase、资源 W/H、`+0x3E4`、坐标；第二次绘制前再三次重读 phase 依次发布三个缩放全局为 `-phase`，
  读当前 `+0x2694` 并只在调用参数副本 OR4、读 `+0x2548/+0x3E4` 和源 W/H（ECX/EDX 各先 XOR 清零再 `mov cx/dx`）。

  第二绘制返回后才将 phase word 加2；`add esp,0x50` 清掉两矩形两绘制的累计80 byte，再压 `(0,0,640,480)` 调第三矩形并单独 `add esp,0x10`。
  该第三次 `sub_416FF0` 可按当前 `4A0E78/7C` 裁剪，最终共享绘图窗口是它的结果，而非第二次绘制前发布的缩放参数。

- case4 四 CALL 段 `0x00479ED0..0x0047A07E`：首矩形前先读源 `+0x00` 发布 `4CD730`，
  随后两次重新读取 `actor+0x2548` 的高/宽，先把高零扩展后 `shr 1`、宽零扩展，再读 `+0x3E4`、phase、X/Y 形成 `(X-2P-EBP, Y-Oy, X+W-2P-EBP, Y+(H>>1)-Oy)` 并调 `sub_416FF0`。
  首矩形正常回复后，三次重读 phase，按序把 `-P` 写三个缩放全局；重读 `+0x2694` 并仅在参数副本 OR4，读最新源 token/高/宽（分别 zero-extend EDX/ECX）、`+0x0D68/+0x3E4/+0x0D66` 形成首 `sub_4170E0` 坐标 `(X-2P-EBP,Y-Oy)`。

  首绘制正常回复后，第二矩形两次重读源 token/高/宽，构造 `(X+2P-EBP,Y+(H>>1)-Oy,X+W+2P-EBP,Y+H-Oy)`；
  它的正常回复后，再三次重读 phase 发布 `-P` 缩放全局，重读 `+0x2694/+0x2548` 和源高/宽，原位压绘制参数，
  **不调用独立 case4 尾**而 `jmp 0x00479E7C`，与 case3 共用第二绘制 CALL 和 word 加2/累计栈清理/第三矩形。

  每个 CALL 之间物理 actor/frame/global 的重读不可跨 CALL 缓存；首矩形停止前只发布 `4CD730`，
  首次绘制停止前发布三项缩放，第二矩形/绘制后再分别裁剪或刷新。

- case5 `0x0047A083..0x0047A19F`：先读取 frame `+0x0E` 的 u16 高度并与 signed phase 比较；
  `phase>=高度` 则跳 `0x0047A935` 仅把 selector byte 改2、phase word 清零、返回0，
  不调 reset。否则 phase0 时播放0x31；把 frame `+0x00` 发布 `dword_4CD730`，先以原 `actor+0x2694` flags 调 `sub_4170E0(x=X-Ox,y=Y-Oy,w=W,h=H,flags,0)`，
  清其六参数栈；**此后**重读 frame 高度并在 signed phase 大于该 u16 高度时把 phase 低 word 写成 `(height-1)&0xFFFF`。

  把 `dword_4CD71C/4CD30C/4CD304` 依次写 -24，再用更新后的 phase 重读 flags/frame，
  第二次绘制为 `(X-Ox,Y-Oy,W,sign_extend(phase),flags|0x10,0)`；仅 `or al,0x10` 用于临时调用参数，
  不写回 actor。接着调用 `sub_416FF0(0,0,640,480)`，将 phase word 加14，`add esp,0x28` 一次清第二次绘制和矩形的40字节，
  返回0。frame 高度两次物理读之间夹着绘制 CALL，不得合并。

- case5 三绘制 CALL 与栈时序 `0x0047A083..0x0047A19F`：初始先读 `+0x2548` 及源 `+0x0E` 为 unsigned 高，
  与 `movsx(actor+0x2958)` 的 signed phase 作 `jge`；达界时只到 `0x0047A935` 改 selector2、清 phase，
  不能提前读源首 dword。未达界且 phase0 先播0x31；重读源 token/首 dword 发布 `4CD730`，重读 `+0x2694` 作为第一绘制未经 OR 的 flags，
  再重读源 token/高/宽，按 `(X-EBP,Y-Oy,W,H)` 调首 `sub_4170E0`。正常返回后**先**重读 `+0x2548`（六个参数仍压在栈），
  再 `add esp,0x18`，从该源 `+0x0E` 只换 EAX 低16位；把高度零扩展并与重读的 signed phase 比较，
  `phase>height` 时在 `0x0047A120` 对**完整 EAX**（高16位仍是首绘制回复）执行 `dec eax`，
  再仅写 `phase word=AX`，故正常存值为 `(height-1)&0xFFFF`，但该 DEC 的 FLAGS 与未改的高16位相关；
  若紧随其后的相邻访问停住，不得把 EAX/FLAGS 简化成16位高度减一（这不是每帧夹到0）。然后按 `4CD71C/4CD30C/4CD304` 顺序写三个 -24，
  再读原 flags、phase、源 token、Oy，对参数 flags 仅 `or al,0x10`，读 unsigned 宽 `+0x0C` 及 X/Y 并以 **phase 作为一个独立栈参** 调第二绘制。

  其回复后立即压 `(0,0,640,480)` 调矩形；矩形正常返回后才 `phase word+=0x0E`，随后统一 `add esp,0x28` 清第二绘制24 byte和矩形16 byte并返回0。
  首绘制后源 token 读故障发生于前六参尚未清栈时；第二绘制/末矩形停止时既不能加0x0E 也不能提前清40 byte。

- case3 后半 `0x00479DB3..0x00479EA9`：首绘制返回后的第二矩形准备只读**一次**当前 `+0x2548` token，
  从同一记录按 W、H 顺序零扩展；phase 读一次并在寄存器中倍增用于矩形右/上下坐标，`+0x3E4` 分别在矩形准备早段、后段读两次，
  构造并压第二矩形四参。第二矩形正常回复后，重读 phase 三次按 `4CD71C=-P` 写、**先压 EBX0 再**写 `4CD30C/4CD304`，
  读当前 flags/source/Oy，参数仅 OR4，第二绘制高/宽从同一 token 各读一次。该 CALL 成功后先 `0x00479E81 add phase word,2`、后 `0x00479E89 add esp,0x50` 清四 CALL 共80 byte；
  故 phase 加法写障仍保留全部80 byte，最终全屏矩形停止时 phase 已加且前80 byte已清，但新矩形16 byte仍在栈上。

  case4 从 `0x0047A07E` 跳入这同一 CALL/共尾，其尾栈与 phase 写点并非独立复制。
- case4 独立前缀 `0x00479ED0..0x0047A07E`：首矩形之前源 token 第1次读源首 dword并发布 `4CD730`、第2次读 unsigned H、第3次读 unsigned W（W 先于该矩形右边计算），
  两次读取 actor `+0x3E4`：第一次在 H 后/W 前，第二次在 bottom/right 已入栈后、top/left 入栈前；
  signed phase 在首次几何计算中重读并翻倍暂存局部栈槽。首矩形正常后**三次各自重读** phase 写 `4CD71C=-P`，
  压 EBX=0，然后写 `4CD30C/4CD304=-P`，当前 flags/source/phase/Y/Oy/X 再次重读，
  参数 flags 仅局部 OR4；首图像绘制 CALL 前积累16+24 byte参数。次矩形 `0x00479FAF` 又依次读 Y、token/H、Oy、phase、第二次 token/W、X、再读 Oy/Y，
  按 `(X+2P-Ox,Y+(H>>1)-Oy,X+W+2P-Ox,Y+H-Oy)` 压四参；次绘制前继续各自重读 phase 写三个 global，
  并在首 global 后压 EBX0，再读 flags/token/H/W、Y/Oy/X、又一次 phase，算 `(X+2P-Ox,Y-Oy,W,H)`。

  `0x0047A07E` 是**跳转**而不是第二个绘制 CALL，实际 CALL 指令只有共享 `0x00479E7C`，硬件 return address 固定 `0x00479E81`；
  两条路径都按各自实际经过的 CALL 次数计算请求 ordinal，不能新增第29个绘制 CALL 或错把 `0x0047A07E` 当作 call identity。

  只有此共享 CALL 正常返回才执行 phase word+2、清80 byte与全屏矩形。
- case10 `0x0047A815..0x0047A934` 与 case5 首次绘制前的结构相近，仍是独立的 LST 路径：
  首次 `sub_4170E0` 之后也在首绘制六参未清前重新读 `+0x2548`，接着 `add esp,0x18`、重新读源高并按 signed phase `>` 限幅；
  命中时 `0x0047A8B2 dec eax` 与 case5 一样对**保留首绘制回复高16位的完整 EAX** 运算，`0x0047A8B3` 只写 AX 到 phase word，
  故写障处 EAX/FLAGS 不可简化成16位高度；但从 `0x0047A8BA` 起**先**发布 `4CD71C=0/4CD30C=0/4CD304=16`，
  才读 `+0x2694`、phase、源 token/Oy，`or al,0x10` 后**再** `push ebx=0` 和 flags，
  并以 signed `movsx phase; inc ecx` 的完整32位值作第二绘制独立栈参（负1变0，32767变32768），
  而 case5 传原 phase 且在发布三项全局之前 `push ebx`。第二绘制及末矩形正常回复之后仅 `phase word+=4`，
  再清40 byte调用栈并返回0；任一 CALL 停止都不执行该加法。两支末矩形压入相同的原始 `(0,0,640,480)`，但裁剪上界仍由 `sub_416FF0` 实时读 global 决定。

  不能仅以不同缩放值和步长参数化两支而合并 ESP/写入前缀。
- case6 `0x0047A1A0..0x0047A265`：signed phase `<=-32` 跳 `0x0047A253` 清 `+0x2958/+0x2954`，
  再走 `0x0047B808` 清 `+0x2A12`/38 dword及 reset；否则零 phase 播0x31，将 frame `+0x00` 发布 `dword_4CD730`，
  按顺序把 i16 phase 的符号扩展原值写入三项 `dword_4CD71C/4CD30C/4CD304`，而非写负数；再把 EBP 覆盖为 `actor[+0x2B4]`，
  以 `sub_4170E0(x=X-sign_extend(actor+0x29B2),y=Y-actor[+0x2B4],w=W,h=H,actor[+0x2694]|4,0)` 绘制。

  六参数栈清除后 phase 按 word 减1，返回0。这里的两项坐标偏移不是 case5 的 EBP/`+0x3E4`。
- case6 顺序 `0x0047A1A0..0x0047A252`：`cmp ax,0xFFE0` 的 signed `jle` 在 phase<=-32 直接清主/辅助 phase，
  不读源或播放声音；其他值仅 phase==0 播0x31。随后读 `+0x2548` 源首 dword 发布 `4CD730`，三次各自重读 signed phase 并按 `4CD71C/4CD30C/4CD304` 次序发布其**原值**（非取负）；
  读 `+0x2694`、再次读 `+0x2548`，在参数副本 OR4，读取 `actor+0x2B4` **改写 EBP**，源高/宽分别零扩展到 EDX/ECX，
  读 i16 `actor+0x29B2`、`+0x0D68/+0x0D66`，以 `(X-29B2,Y-2B4,W,H)` 调绘制。

  正常返回后先 `add esp,0x18` 清6参，再对 phase word **减1**、xor EAX=0 并恢复初始 EBP 返回；
  CALL 停止时 EBP 已是 `actor+0x2B4`，ESP 尚含6参，不能只记录进入此 case 的 EBP 值。

- case7 `0x0047A266..0x0047A5F9`：signed phase `>=32` 直接走 `0x0047B801` 重置；
  零 phase 先播0x31。其余将 frame `+0x00` 发布 `dword_4CD730`，按 `sub_416FF0` 矩形、`sub_4170E0` 图像绘制交替调用**四组**；
  每组矩形后按序将三个缩放全局重写为 `-sign_extend(phase)`，每次绘制只将 `actor+0x2694 | 4` 作为调用参数，
  不写回 actor。第2次绘制后的 `0x0047A470` 和第4次绘制后的 `0x0047A5EF` 分别 `add esp,0x50` 清此前两组的调用实参；
  第4次绘制后 phase word 加1，再跳共享 `0x0047AF04`，绘制末尾全屏矩形 `(0,0,640,480)` 并返回0。

  以 `P/X/Y/W/H/Ox/Oy` 沿用上文定义，按原四组栈参数计算的矩形坐标依次是 `(X-P-Ox,Y-P-Oy,X+(W>>1)-P-Ox,Y+(H>>1)-P-Oy)`、`(X-P-Ox,Y+(H>>1)+P-Oy,X+(W>>1)-P-Ox,Y+H+P-Oy)`、`(X+(W>>1)+P-Ox,Y-P-Oy,X+W+P-Ox,Y+(H>>1)-P-Oy)`、`(X+(W>>1)+P-Ox,Y+(H>>1)+P-Oy,X+W+P-Ox,Y+H+P-Oy)`。
  对应四次绘制的 `(x,y,w,h)` 分别为 `(X-P-Ox,Y-P-Oy,W,H>>1)`、`(X-P-Ox,Y+P-Oy,W,H)`、`(X+P-Ox,Y-P-Oy,W,H)`、`(X+P-Ox,Y+P+16-Oy,W,H)`；
  最后一组 y 的原 `0x0047A5D9 lea edx,[eax+ecx+10h]` 多加16，不可对称化。四组都用临时 flags OR4 与零尾参。

  重复资源读的物理访问顺序、CALL 回复及异常前缀仍待逐条核验。
- case7 八次交替 CALL `0x0047A28D..0x0047A5F9` 逐原块的故障分界：首矩形前**三次**从 `actor+0x2548` 分别取源首 dword、高、宽，
  先发布 `4CD730`；首矩形目标 `(X-EBP-P,Y-Oy-P,X+(W>>1)-EBP-P,Y+(H>>1)-Oy-P)`。
  首矩形前的 `0x0047A2D1 xor ebx,ebx` 随后被 `mov bx,[frame+0x0C]` 与矩形右边运算覆盖，
  `sub_416FF0` 不触碰 EBX，故进入后续第一绘制时 EBX 是矩形右坐标而非入口0；首矩形正常回复后先写 `4CD71C=-P`、**`push 0` 立即数而非 `push ebx`**、再写 `4CD30C/4CD304=-P`，
  读当前 flags/source，参数 flags OR4，第一绘制 `(X-EBP-P,Y-Oy-P,W,H>>1)`。第二矩形重读源两次、高/宽、phase、坐标与 Oy，
  目标 `(X-EBP-P,Y+(H>>1)-Oy+P,X+(W>>1)-EBP-P,Y+H-Oy+P)`；第二绘制再次三读 phase、发布 `-P`，
  原位读新 flags/source 并绘制 `(X-EBP-P,Y-Oy+P,W,H)`。四 CALL 的 20 dword 参数此时**仍未清栈**：
  `0x0047A446` 起先读源 token/高/宽、Oy/phase/Y，直到 `0x0047A470` 才 `add esp,0x50`；
  故在此之前任何读障的 ESP 都含前20 dword。之后第三矩形 `(X+(W>>1)-EBP+P,Y-Oy-P,X+W-EBP+P,Y+(H>>1)-Oy-P)`，
  其绘制 `(X-EBP+P,Y-Oy-P,W,H)`，且第三绘制先把三项缩放全局都写完，`0x0047A4DB` **此后**才 `push 0`，
  不同于第一、二、四绘制在首项写后即压零；第四矩形 `(X+(W>>1)-EBP+P,Y+(H>>1)-Oy+P,X+W-EBP+P,Y+H-Oy+P)`，
  末绘制起点 `(X-EBP+P,Y-Oy+P+0x10)` 并用完整 W/H，不因几何看似四象限而去掉 `+0x10`。

第二组四 CALL 末 `0x0047A5EF` 再清 80 byte、`0x0047A5F2` 才将 phase **word 加1**；

  随后跳 `0x0047AF04` 压 `(0,0,640,480)` 调第九次全屏矩形并返回0。若末次矩形停住，phase 已加1；
  若末绘制停住，phase 未加且第二组栈未清。八次 CALL 之间的源/phase/flag 和绘图共享状态每次按原访问点重取，不作四组对象参数预计算。

- case8 `0x0047A5FE..0x0047A74D`：phase 与 CX=100 **仅等于**时转 case2 共用的双区清零和释放；
  否则 `+0x0E14` 非零就直接转粒子尾。为空时先清 `[+0x0E14,+0x0E6C)`、调用 `sub_4019A0` 并发布新 source。
  初始化区别于 case2：`+0x0E24` 先写有符号 -30，仅当 `+0x2B08` 完整 dword==1 时改写为670；
  `+0x0E2C=(actor+0x0E20)-10`、`+0x0E28=2`、`+0x0E30=zero_extend(frame.height)+10`，
  `+0x0E34/+0x0E36=100`、`+0x0E38=1`、`+0x0E3A=10`、`+0x0E3C` 低 byte OR0x16。

  再调用 `sub_47CE70`；仅当 EAX 恰为1时用 `or [actor+0x0E3C],bp` 写**低16位**（不是 case2 的 byte OR），
  EBP 此前被设为1。然后依次调用 `GetSystemMetrics(1)`、`GetSystemMetrics(0)`、`sub_433F30`、播放0x31，
  phase word 写1并进入粒子尾；这些步骤不能和 case2 共享一个混合参数列表或提前统一 owner 校验。

- case8 初始化 CALL 前后的补核 `0x0047A617..0x0047A74D`：在 22 dword 清零后，压三个输出栈地址，
  读首个 `+0x2548` 与其源首 dword 调 `sub_4019A0`；正常回复**先**把 token 写 `+0x0E14`、读第二次源 token，
  才 `add esp,0x10`，再读源宽写 `+0x0E18`，第三次读 token/源高写 `+0x0E1A`。随后按 `+0x0D66→+0x0E1C`、`+0x3E4/+0x0D68→+0x0E20`、`+0x0E24=-30` 的顺序写，
  **仅之后**读完整 dword `+0x2B08==1` 时改写 `+0x0E24=670`。由刚写的 `+0x0E20` 读回并写 `+0x0E2C=value-10`，
  把 EBP 改成1，再写 `+0x0E28=2`；第四次重读源 token 与 unsigned 高，先 OR `+0x0E3C` 低 byte 0x16，
  再写 `+0x0E30=H+10`、`+0x0E36=100`、`+0x0E34=100`、`+0x0E38=1`、`+0x0E3A=10`，
  调原三指令 `sub_47CE70`。仅回复 EAX==EBP==1 才 OR `+0x0E3C` 的 **word** 1；再按两次 `GetSystemMetrics(1,0)`、`sub_433F30`、`sub_485610(0x31)` 的独立调用顺序，
  声音正常后才写 phase word1并转粒子尾。case2 不改 EBP 为1，两个共享出口不能合并成一套停止时寄存器；源 `+0x0E` 读取在 OR 之前；
  若该读障发生，`+0x0E3C` 尚未 OR。若后续 `+0x0E30` 写障发生，则已 OR 的低 byte 必须保留，不能把整个初始化当作事务回滚。

- case9 `0x0047A752..0x0047A814`：signed phase `>=45` 跳 `0x0047A253` 清两项 phase word，
  再走重置尾；否则 phase0 播0x31。将 frame `+0x00` 发布到 `dword_4CD730`；以有符号乘法 `imul` 常量 `0x55555556` 与 signed phase，
  取高32位 EDX，以 `((u32)EDX >>31)` 加到 EDX 后再加1（所有运算低32位）为等级 `n`，依次发布 `dword_4CD724=n`、`dword_4CC2F0=15-n`。

  调用一次 `sub_4170E0(x=X-sign_extend(actor+0x29B2),y=Y-actor[+0x2B4],w=W,h=H,flags=actor[+0x2694]|0x14,0)` 后才清六参数栈，
  按顺序把 `dword_4CC2F0/4CD724` 清零，phase word 加1，直接落到 `0x0047A80B` default 返回0。

  负 phase 保留 signed 乘法与回绕，不提前夹到0..15。
- case9 `0x0047A752..0x0047A814` 的特殊全局前缀补核：signed phase `>=45` 转共享 reset，
  故此 case 后缀不再读源（共同入口已发生的访问除外）；phase0 的声音正常后首次读 `+0x2548`/源首 dword，
  `push ebx=0`，写 `4CD730`；`0x55555556` 作**有符号** `imul signed_phase`，
  取 EDX 高32位并以 `shr eax,31` 的符号补偿得 `n=EDX+signbit(EDX)+1`；原顺序写 `4CD724=n`、`4CC2F0=15-n`。

  接着才读当前 `+0x2694` 和第二次源 token，按参数副本 OR0x14，读源高/宽、i16 `+0x0D68/+0x29B2/+0x0D66` 与 dword `+0x2B4`，
  以 `(X-29B2,Y-2B4,W,H)` 调 `sub_4170E0`。仅在该 callee 正常返回后 `add esp,0x18`、依次清 `4CC2F0` 与 `4CD724`、`phase word++`，
  从修正后的 default `0x0047A80B` 退出 EAX0。即便绘制 callee 自己改写 `4CC2F0`，caller 两个清零仍是独立物理写；
  在绘制内层停点或两个清零之间故障时保留已发布 n 的可见前缀。

- case9 的等级与绘制 CALLEE 交叉门不能以 `n=15` 当成“没有调用”：LST 的单操作数 `imul ecx` 以 `EAX=0x55555556` 产生 signed EDX 高32位，
  phase=-32768/-3/0/2/3/44 分别得 `n=-10921/0/1/1/2/15`、先后发布 `4CD724=n` 和 `4CC2F0=15-n`（即10936/15/14/14/13/0）。
  若 actor flags 除低两位外为0，OR0x14使 `sub_4170E0` 入口 mode恰0x14；phase44 虽 opacity=0，
  外层依旧压六参**实际 CALL**，callee 首先 `0x004170E0/0x004170E8` 读并解引用当前 `4CD730`，
  然后 `0x0041711D jle 0x00417496` 在进入像素表前跳过，仍依次把八项绘图共享标量写零并返回；坏源必须停在早于 opacity 门的真实解引用点。

  phase=-32768 时 opacity10936>15，`0x00417128 and [arg_10],0x80000003` 降级为普通低位翻转模式而非夹到15，
  后续源/剪裁/像素访问仍按 CALLEE 分支发生。若原 flags 的 `&0xFFFC` 不是0x14，则这两条 opacity 分支不适用，
  不能单凭 phase 决定跳过真实绘制。此交叉核对仅锁定以上入口门及收尾，不替代其余 277 条 CALLEE 指令与异常审计。

- case10 `0x0047A815..0x0047A94C`：与 case5 一样在入口比较 signed phase 与帧 u16 高度，
  达标仅写 selector byte=2、phase word=0后返回0；不调用 reset。phase0 播0x31，然后首绘制 `(X-Ox,Y-Oy,W,H,actor[+0x2694],0)`、清实参，
  再重读 frame 高度并仅在 phase>新高度时按 word 改写为 `height-1`。不同于 case5，依次把三个全局 `dword_4CD71C/4CD30C/4CD304` 写成 `0/0/16`；
  第二次绘制 `(X-Ox,Y-Oy,W,sign_extend(phase)+1,actor[+0x2694]|0x10,0)`，
  接着全屏矩形，phase word 加4，一次 `add esp,0x28` 清第二绘制与矩形的40字节并返回0。高度两次读取之间仍可能有首绘制的修改。

- case11 `0x0047A94D..0x0047AA7A`：signed phase `<=-32` 跳 `0x0047A253` 先清 phase 和 `+0x2954` 两个 word，
  再 reset；phase0 先播0x31。其余将 frame `+0x00` 发布 `dword_4CD730`，先写 `dword_4CC2F0=15`，
  若 signed phase `<=-16` 则改为 `sign_extend(phase)+31`（活跃区间 -31..-16 得0..15）。

  首绘制 `(X-sign_extend(actor+0x29B2),Y-actor[+0x2B4],W,H,actor[+0x2694]|0x14,0)`；
  **绘制后**清 `dword_4CC2F0=0` 并按顺序把 `dword_4CD71C/4CD30C/4CD304` 写 signed phase 原值。

  第二绘制使用相同 x、`y=Y-actor[+0x2B4]+10*sign_extend(phase)`、帧宽高、flags临时 OR4及尾参0；
  调用后才 `add esp,0x30` 一次清两次绘制的48字节，phase word 减1，返回0。case6 与 case11 共用终态尾，
  却不能共用两次绘制与共享全局写入时序。

- case15 `0x0047B2E8..0x0047B408`：signed phase `<=-32` 也走 `0x0047A253` 双 phase word 清零加公共 reset；
  零 phase 先播0x31。将 frame `+0x00` 发布 `dword_4CD730`，**只在** signed phase `>=-15` 时把 `dword_4CC2F0` 写成 `phase+15`；
  区间 `-31..-16` 不写此全局，第一次绘制看到原先残留值，不能默认为0或15。首绘制为 `(X-sign_extend(actor+0x29B2),Y-actor[+0x2B4],W,H,actor[+0x2694]|0x14,0)`；
  其后清 `dword_4CC2F0=0`，依次把三个缩放全局写成 signed phase 原值，第二绘制 `(X-sign_extend(actor+0x29B2)+2*phase,Y-actor[+0x2B4],W,H,actor[+0x2694]|4,0)`；
  再一次 `add esp,0x30` 清两次绘制的48字节，phase word 减1，返回0。case11 则先写全局15并在 `<=-16` 时改写，
  不能合并前缀。

- case11 两绘制栈/全局具体序列 `0x0047A973..0x0047AA7A`：首次读 `+0x2548`/源首 dword 后，
  **先**写 `4CC2F0=15`、再写 `4CD730=源首值`；重读 signed phase，只有 phase<=-16 时把 `4CC2F0` 改成 `phase+31`，
  较大值保持15。读当前 `+0x2694/+0x2548`，只在绘制参数副本 OR0x14，**此时**读 `actor+0x2B4` 覆盖 EBP，
  源高/宽分别零扩展，首绘制 `(X-29B2,Y-2B4,W,H)`。首 CALL 正常回复后在原6参尚未清栈时写 `4CC2F0=0` 并再压第二组的0；
  三次各自重读 signed phase，依序写 `4CD71C/4CD30C/4CD304=phase`，读 flags/source/high/width/Oy/X/Y，
  参数副本 flags OR4，次绘制 `(X-29B2,Y-2B4+10*phase,W,H)`。第二 CALL 正常回复后才一次性 `add esp,0x30` 清两组共48 byte，
  然后 `phase word--`、EAX清0并返回；首 CALL 停止时 ESP 含24 byte，第二 CALL 停止时含48 byte，
  且 EBP 已是 actor `+0x2B4`。不可因两次 call 类型相同而分别清参或提前清第一次 `4CC2F0`。

- case15 与 case11 差异核对 `0x0047B30E..0x0047B408`：先读当前 `+0x2548` 源首 dword 并发布 `4CD730`，
  重读 signed phase；`cmp ax,0xFFF1; jl` 在 **phase<-15** 时直接跳过 `4CC2F0` 写入（保留前态），
  仅 phase>=-15 时写 `4CC2F0=phase+15`，没有 case11 的先写15。接着读 flags/source，
  参数副本 OR0x14，`push ebx=0`、以 `actor+0x2B4` 改写 EBP，再读源高宽及 `+0x29B2/+0x0D68/+0x0D66`，
  首绘制 `(X-29B2,Y-2B4,W,H)`。CALL 正常后、首6参**尚留栈上**就清 `4CC2F0=0`，压第二组0，
  再按三次各自读取 signed phase 写 `4CD71C/4CD30C/4CD304` 原值，读新 flags/source/宽高及 Oy、29B2、X/Y，
  以 `(X-29B2+2*phase,Y-2B4,W,H)` 与参数 flags OR4 作次绘制。仅次 CALL 正常后 `add esp,0x30` 清48 byte、phase word减1；
  首停点/次停点分别保留24/48 byte，EBP 为 `actor+0x2B4`。与 case11 的第二绘制 Y 加10倍 phase 不可合并成同一几何，
  且 phase<-15 时首绘制可见旧 `4CC2F0`。

- case12 `0x0047AA7B..0x0047ABAC`：signed phase `>=30` 先清 `+0x2958/+0x2954` 两个 word，
  跳 `0x0047B995` 的另一组重置尾；否则 phase0 播0x31，**在 frame 像素发布之前**依次写四个全局：
  `dword_4A0698=actor[+0x2B0]`、`dword_4A069C=actor[+0x2B4]`、`dword_4A06A0=0x400+sign_extend(i16 actor+0x2954)*sign_extend(i16 phase)`、`dword_4A06A4=0x400-(sign_extend(i16 actor+0x2954)+1)*sign_extend(i16 phase)`；
  乘加均取低32位。再将 frame `+0x00` 发布 `dword_4CD730`，读取 actor `+0x2694` **低 byte** bit0；
  置位时仅调用 `sub_423020`，未置位时仅调用 `sub_422C70`。两者同样接受 `(X-sign_extend(actor+0x29B2),Y-actor[+0x2B4],W,H)`，
  但每支都保留各自的物理资源宽高及 actor 读取点。入口先 `push ebx` 留下一份零栈槽；callee 返回后先把辅助 phase `+0x2954` word 加2，
  `add esp,0x14` 一次清四参数和零槽，再将主 phase `+0x2958` word 加1，返回0；不得把两个 phase 递增交换。

- case12 两分支物理访问/栈顺序 `0x0047AAA2..0x0047AB99`：先从 `actor+0x2B0` 读 dword，
  **立即压一个 EBX=0 栈槽**才写 `4A0698`，再读 `+0x2B4` 写 `4A069C`；两次各自重读 signed `+0x2954/+0x2958`，
  按32位低乘积先写 `4A06A0=0x400+aux*phase`，再写 `4A06A4=0x400-(aux+1)*phase`。

  至此才读 `+0x2548`/源首 dword 写 `4CD730`，以单 byte `actor+0x2694` bit0 判调用目标，
  **先**重读 `+0x2548` 再分支；两路各自读 `+0x2B4` 覆盖 EBP、零扩展源高/宽、读 `+0x0D68/+0x29B2/+0x0D66`，
  先后压 H/W/Y/X，调用 `sub_423020` 或 `sub_422C70`。两个 callee 都是 cdecl `retn`，
  且物理调用栈还保留四参之前的 EBX=0 槽；正常返回先 `aux word+=2`，才 `add esp,0x14` 清五个 dword、`phase word++`、EAX清0返回。

  若 callee 停止，五个 dword尚在栈；若 aux 写障，callee 已返回但栈仍未清。phase>=30 直接清 phase/aux 后走另一条38 dword reset 尾，
  不调用两绘制 callee，也不写上述四个 global。

- case12 两个 signed `imul` 的完整状态与故障矢量：若 aux=`i16(+0x2954)=32767`、phase=`i16(+0x2958)=-32768`（仍走 `<30`），
  `0x0047AAC8 imul ecx,edx` 得 `ECX=-1073709056`，`0x0047AACB add ecx,0x400` 得 `ECX=0xC0008400`；
  在 `0x0047AAD6` 写 `4A06A0` 时停下，已经压 EBX=0、已发布 `4A0698/4A069C`，但尚未发布 `4A06A0/4A06A4`，
  EAX仍为所读 `actor+0x2B4`、EDX已在 `0x0047AAD1` 变为0x400，FLAGS为这次完整32-bit ADD 的结果。

  若该写正常，则 `0x0047AAEA inc eax` 先使 aux+1=32768、`0x0047AAEB imul eax,ecx` 得 `EAX=-1073741824`，
  `0x0047AAEE sub edx,eax` 得 `EDX=0x40000400`、无符号 CF=1；在 `0x0047AAF0` 写 `4A06A4` 时停下，
  第三个 global 已发布而第四个尚未发布，FLAGS为此完整32-bit SUB 而非 IMUL。上述两个 IMUL 虽输入经16-bit符号扩展且真实乘积在32-bit有符号范围内，
  仍不可把整个四项 global 一次性事务提交或误把 `imul` 高32位（case1/9 另有单操作数型）套用到 case12。

- case13 `0x0047ABAD..0x0047AF23`：signed phase `>32` 跳 `0x0047B801` 清理，
  `==32` 仍进入绘制；零 phase 播0x31，发布 frame `+0x00`。其余交替调用**四组**矩形 `sub_416FF0` 与图像 `sub_4170E0`，
  第2组、第4组绘制后各一次 `add esp,0x50` 清累计80字节；每组矩形后按顺序重写三项共享缩放为 `-phase`，每次绘制仅把 actor flags临时 OR4。

  第4次绘制后主 phase word 加2，直接进入 `0x0047AF04` 全屏矩形并返回0。设 `Q=zero_extend(frame.height)>>2`（先整除4，
  再乘2/3，不能以 `height*3>>2` 替代）。以 `P/X/Y/W/H/Ox/Oy` 为上文 32 位回绕变量，四组矩形依次为 `(X-2P-Ox,Y-Oy,X+W-2P-Ox,Y+Q-Oy)`、`(X+2P-Ox,Y+Q-Oy,X+W+2P-Ox,Y+2Q-Oy)`、`(X-2P-Ox,Y+2Q-Oy,X+W-2P-Ox,Y+3Q-Oy)`、`(X+2P-Ox,Y+3Q-Oy,X+W+2P-Ox,Y+H-Oy)`。

  四次对应绘制 `(x,y,w,h)` 依次是 `(X-2P-Ox,Y-Oy,W,H)`、`(X+2P-Ox,Y-Oy,W,H)`、`(X-2P-Ox,Y-Oy,W,H)`、`(X+2P-Ox,Y-Oy,W,H)`，
  flags 均临时 OR4、尾参零。矩形高度分段与绘制区域不一致，也不等于 case7 的四象限模式；资源重读与 fault 前缀以如下物理访问为准，
  不能概称全部重读。

- case13 逐点交叉核对 `0x0047ABD4..0x0047AF23`：首矩形在写 `4CD730=源首值` 后**重读一次**源 token，
  取 unsigned 源高并 `shr edi,2` 得 Q，存活于 EDI 跨前两组四个 CALL（`sub_416FF0` 不写 EBX 且保存 EDI；
  `sub_4170E0` 保存 EBX/EDI）；四次矩形的 Q 不是每次重读新源高。第二矩形 `0x0047ACA8` 用首组 EDI 算 Q/2Q，
  仅读**当前源宽**，并重读 phase、X/Y/Oy；第二绘制重新读当前源高/宽。第三矩形 `0x0047AD61` 先读当前 Y/phase/Oy，
  **到 `0x0047AD75` 才清首组累计80 byte**，用 EDI 原 Q 生成 `EBX=3Q`，其后才重读源 token/宽并作矩形；
  所以 `0x0047AD61..0x0047AD6F` 的读障保留首组80 byte，而 `0x0047AD8A` 源读障已清栈。

  第三绘制重读源高/宽但不改 Q；第四矩形先重新读源 token、unsigned **新高度 H** 与当前 Oy/phase/Y，
  底边用新 H，顶边却用 `EBX=3Q` 这个首组旧高派生值，另读当前源宽，且 `+0x3E4` 顶/底两次物理读取不能强行复用；
  第四绘制另取新的 H/W。每次矩形后另三次读 phase 分别发布 `4CD71C/4CD30C/4CD304=-phase`，第一项写完才 `push 0`，
  再写剩余两项；图像 CALL 仍各自临时 OR flags 4。第四绘制完成后 `0x0047AEF9` 清次组80 byte、`0x0047AEFC` phase word加2，
  才从 `0x0047AF04` 压 `(0,0,640,480)` 作最终矩形；该最终矩形停止时 phase 已加2，两组均已清。

- case14 `0x0047AF24..0x0047B2E7`：signed phase `>32` 走 `0x0047A253` 清主/辅两个 phase word 并 reset，
  phase0 播0x31，发布 frame `+0x00`。phase `<9` 时调用两组矩形与绘制、在 `0x0047B0E5` 一次性清80字节；
  原值正好8时，先将辅助 word `+0x2954` 加1，**与仍在 AX 中的主 phase=8 作 word 比较**：相等就写主 phase=9，
  不等则保留主 phase=8；两路都跳共享 `0x0047B2CA` 全屏矩形并返回0。若早期主 phase不是8，就将 AX 的低 word 加4写回主 phase，
  再跳相同全屏尾。phase `>=9` 但 `<=32` 时在 `0x0047B11F` 走独立的两组矩形与绘制，清80字节后将主 phase word 加1，
  走同一全屏尾。两段在 frame/actor 固定且 callee 无干预时**几何公式相同**：先矩形 `(X-2P-Ox,Y-Oy,X+W-2P-Ox,Y+(H>>1)-Oy)` 与绘制 `(X-2P-Ox,Y-Oy,W,H,flags|4,0)`，
  再矩形 `(X+2P-Ox,Y+(H>>1)-Oy,X+W+2P-Ox,Y+H-Oy)` 与绘制 `(X+2P-Ox,Y-Oy,W,H,flags|4,0)`；
  均每次矩形后把三个缩放写为 `-P`。但早段先在局部栈槽保存 P，再翻倍为2P，在第二绘制前临时把 EBP 改成该值，晚段分别重读 actor phase；
  两路资源访问、寄存器和 callee 干预后的结果不能合并。两段的物理重读与停止前缀见下文指令对照；正式测试和最终双向 REVIEW 尚未进行。

- case50 `0x0047B747..0x0047B800`：signed phase `>=15` 走 `0x0047B801` 重置；
  phase0 时先读 `actor+0x428` 的 word、写 `actor+0x390` 的 word=0x31，再把读取的**原动作码**用于播放，
  不是固定播0x31。其余 frame `+0x00` 发布 `dword_4CD730`，`dword_4CC2F0=15-sign_extend(phase)`，
  以 `sub_4170E0(x=X-Ox,y=Y-actor[+0x3E4]-table[phase],w=W,h=H,actor[+0x2694]|0x16,0)` 绘制，
  六参数栈清后 phase word 加1，返回0。表访问 `0x0047B7CB` 用 signed phase 直接参与 `[0x004A7DB4+phase*4]` 低32位寻址；
  没有负数夹值。权威 LST `.data:004A7DB0..004A7DB3` 紧邻表首之前的四个字节是 `00 00 00 00`，
  故 phase=-1 在原版实际从表前一项读取 dword=0，不可将所有负索引一概判为越界。更早的负索引或地址回绕仍须按原内存读点与范围单独审计。

- case50 停止前缀补核 `0x0047B77D..0x0047B800`：首次读 `+0x2548` 后**先压 EBX=0**，
  才解引用源首 dword、写 `4CD730`、重读 signed phase 写 `4CC2F0=15-phase`；其后才重读当前 flags/source，
  参数副本 OR0x16，先后压 flags、零扩展源高；第三次重读 signed phase 并读 actor Y，**以 EAX 的 signed 32-bit 数值直接计算** `4A7DB4+EAX*4`，
  把表 dword 放 EDI 后才压源宽、读 Oy、Y/X 并调绘制。`0x0047B7CB` 表地址不可读时，ESP 已含最早 EBX=0、flags 和 H 三个 dword（12 byte），
  `4CD730/4CC2F0` 已发布而 phase未增加；不能在读表前校验成 `0<=phase<15` 并归零。callee 正常返回后 `add esp,0x18` 清六参、`phase word++` 再以 EAX0 返回。

  phase0 声音前的 `mov ax,[esi+428h]` 虽只替换低16位，但 selector 分派在 `0x004799BB` 已 `xor eax,eax`，
  故正常此路高16位为0；动作码若访问失败，`+0x390` 尚未写。

- case51 `0x0047B83E..0x0047B9CA`：signed phase `>=CX=100` 从 `0x0047B98E` 清 phase、`+0x2A12`，
  以入口 EDI 清38 dword，调用 `sub_478850`，依次写 `+0x2AAC=0/+0x2ABC=0/+0x2AB8=1`，
  返回1。phase==0 的非终态先将 `+0x0DD8/+0x0DDC` 都写0x400，`+0x0DC0=zero_extend(W)>>1`、`+0x0DC4=zero_extend(H)`、`+0x0DBC/+0x0DBE=W/H`、`+0x0DE0` word写0x16、`+0x0DC8=X-(W>>1)`、`+0x0DCC=Y`；
  调用 `sub_47CE70`，EAX恰为1才将 `+0x0DE0` **低 byte** OR 1。随后先把 `+0x0DF0` 写0x5A，
  再用帧像素和三个栈槽调用 `sub_4019A0`，先读 `actor+0x428` 的 word，**才**将回复 token 发布到 `+0x0DB8`、写 `actor+0x390` word=0x31，
  以先读的动作码覆盖 **ECX 低16位**，`0x0047B923 push ecx` 实际把完整32位交给音频；此处未清 ECX 高16位，
  它来自 decoder 正常返回后的 ECX，不能单凭 caller 假设声参被零扩展；例如合法 format8 的 `0x00401AC3` 内层分配正常返回后，
  首行 word0 沿 `0x00401ACB jz -> 0x00401B62` 正常返回，沿途未重写 ECX 高16位，该值取决于内层分配回复；
  声音调用后一次 `add esp,0x18` 清分配四参和声音两参。phase非零跳过此初始化/声音，直接与初始化成功后的公共路径汇合：
  先读 `+0x0DD8/+0x0DDC` dword，再将 phase word 加2，把 `+0x0DE4/+0x0DE8/+0x0DEC` 各写5，
  把两个读出的计数各加4后回写 `+0x0DD8/+0x0DDC`，再以 `dword_4CD76C` 和 `actor+0x0DB8` 为参数调用 `sub_4344E0`，
  忽略回复并返回0。

该尾临时把 ESI 从 actor 指针改成 `actor+0x0DB8`；不得用全局“先重置 actor”覆盖其原始调用栈和寄存器。

- case51 初始化读写/停止前缀 `0x0047B857..0x0047B929`：先取当前源 token，写 `+0x0DD8=0x400`、`+0x0DDC=0x400`，
  然后分别读源 W/H/W/H/W（宽三次、高两次），穿插写 `+0x0DC0=W第一次>>1`、`+0x0DC4=H第一次`、`+0x0DBC=W第二次`、`+0x0DBE=H第二次`、`+0x0DE0=0x16 word`，
  以**第三次 W>>1** 与当前 signed X 作 `+0x0DC8`，signed Y 写 `+0x0DCC`；前两项 dword 已写时源首宽故障不可回滚。

  `sub_47CE70` 回复 EAX==1 才 OR `+0x0DE0` 低 byte1；初始化成功后先压第一个输出指针，再读当前 `+0x2548`，
  写 `+0x0DF0=0x5A`，然后依次压第二输出指针、读源首 dword、压第三输出指针与像素 dword，调用 `sub_4019A0`。

  decoder 正常回复后 **`0x0047B907` 先读 `+0x428` word，再 `0x0047B90E` 写 token `+0x0DB8`**，
  故动作码读障时 decoder 的四个参数共16 byte 仍压着且 token 尚未发布；`+0x390` 写障时 token 已发布；
  音频停点还保留 decoder 16 byte +音频8 byte。音频正常后 `0x0047B929` 才 `add esp,0x18` 清共24 byte。

  非零 phase 绕过上述整段，而尾部 `0x0047B972` 把 ESI 改为 `actor+0x0DB8`、压该地址与当下 `4CD76C` 调 `sub_4344E0`，
  CALL 停止时 ESI 不等于原 actor 指针；+2 及三个5、两个 +4 均已写。

- case14 两入口不可混合的指令读序 `0x0047AF4A..0x0047B2E7`：共同前缀先读源首 dword 写 `4CD730`，
  **此后**重读主 phase word 与 signed 9 比，`<9` 早路将该 AX sign-extend 保存局部 `var_14=P`，
  首矩形在寄存器中临时算2P；第二矩形 `0x0047B026..0x0047B032` 才把保存的 P 翻倍回写 `var_14=2P`；
  首矩形的 unsigned H>>1、unsigned W 可分别从两次 `+0x2548` 取得。首矩形回复后从保存的原 P 构造 EDI=-P，
  **先 `push ebx=0` 再按 `4CD71C/4CD30C/4CD304` 写同一个 EDI 值**，首绘制参数另外读当前 flags/source/H/W/Y/Oy/X，
  以局部2P计算 X，未重新读 actor phase。第二矩形开始重读两次源 token（高与宽）、Y/Oy/X，但用局部2P；第二绘制在三项 global 再写**同一 EDI=-原 P 后**才 `push ebx=0`，
  重读 flags/source/H/W/Y/Oy/X，先用原 EBP 算 X-Ox，再把 EBP **改写成局部2P** 加到 X 坐标，
  因而该 CALL 的停止 EBP=2P。第二绘制正常后，`0x0047B0DE` **先读 actor 当前主 phase 到 AX，
  仍压着首组80 byte**，然后 `0x0047B0E5` 清栈；如果 AX==8，先 aux word++ 并再次读 aux 与 AX word 比，
  相等才将主 phase写9，不等就不改主 phase；若 AX!=8，则 `0x0047B110 add eax,4` 是**完整32-bit** 加法，
  再由 `0x0047B113 mov [esi+2958h],ax` 仅发布低 word：`0x0047B0DE mov ax,[esi+2958h]` 未清理第二绘制正常回复 EAX 的高16位，
  故写障时 EAX高16位与32-bit算术 FLAGS 均由该回复和低 word进位决定，不能用 `add ax,4` 或先零扩展 phase 代替。

  若 AX==8，则 `0x0047B0E8 cmp ax,8` 的 FLAGS 是 `0x0047B0EE inc word [esi+2954h]` 读写障时的最后算术标志；
  INC 成功后辅助 word 已提交、FLAGS 随之变化（CF 不被 INC 改），然后 `0x0047B0F5 cmp [esi+2954h],ax` **再次读取**辅助 word，
  若该次读障不能撤销先前增量；比较成功两个后续分支都保留 EAX高16位，只有相等时 `0x0047B102` 写主 phase9。

  全屏矩形停点须分开记录。即使首组 CALL 修改主 phase，上述几何用旧局部 P，后处理用新 phase。
- case14 `>=9` 晚路 `0x0047B11F..0x0047B2C3`：首矩形仍由 `0x0047AF58` 的旧 AX 倍增 P，
  且高/宽可从两次源 token 取得；但首矩形后每一项缩放都**各自重读 signed actor phase**，次绘制前的三个全局也是各自重读；
  第一次三项发布在首项之后才 `push ebx=0`，第二次亦然。首次绘制和第二次矩形/绘制分别在原物理访问点重读 phase/source，
  后组矩形以新的2P 算 X 偏移，第二绘制的 EBP 始终保留入口 Ox，不能沿用早路改成2P。四 CALL 的20 dword参数累计至 `0x0047B2C0` 才清，
  紧接主 phase word++；早/晚两路都转 `0x0047B2CA` 全屏矩形，若其 CALL 停止，phase 写已完成，
  且早路 EBP仍为局部2P、晚路 EBP仍为 Ox。两路全屏矩形的 raw 参数同 `(0,0,640,480)`，仍依赖 callee 实时共享裁剪状态；
  `0x0047B2D6` CALL **入站** EAX/FLAGS 不能取其正常返回后的 EAX1：晚路 `0x0047B2C3 inc word [+0x2958]` 不改 EAX且其结果标志保留 CF、覆盖其余算术 FLAGS，
  早路则在 `0x0047B110 add eax,4` 或 `0x0047B0F5 cmp [aux],ax` 之后没有再设置算术 FLAGS 的指令，
  EAX仍含最后图像 CALLEE 回复的高16位。若此全屏 CALL 停止，寄存器与 FLAGS 必须依真正前驱保留。

- case100 `0x0047B409..0x0047B42F` phase等于100、非零phase、phase0计数signed `>=6` 与副phase等于24的逐访问门已局部接入；`+0x2680`只经canonical `LegacyBattleTargetPhaseState::runtime_gate`读取，无owner则在该地址typed-stop。`0x0047B518..0x0047B537`三写按独立写障并物理四POP/RET接入，受管`proc_ff19`和`proc_882f`于2026-09-24分别core/CTest `199/199`；音频`0x0047B439..0x0047B44A`按全局handle、两个PUSH、真实CALL返回槽/子入口停、ADD ESP8，受管`proc_82c8` core/CTest `199/199`；`0x0047B44D..0x0047B47E`先PUSH零、匹配token资源首dword发布、三次独立signed aux读写motion全局，受管`proc_872c` core/CTest `199/199`。绘图`0x0047B483..0x0047B4C0`五参数加先前零尾参、真实CALL/子入口停/正常回复，于`proc_942e/proc_a786` core/CTest `199/199`；`0x0047B4C5`在六参仍在栈时重读+2680，再清栈并按signed余数选择aux ADD4/DEC、更新后signed范围门，于`proc_fbb1` core/CTest `199/199`；范围外先POP EDI后将旧计数+1写+2680，再专属物理RET，于`proc_3e98` core/CTest `199/199`。`0x0047B538`旧粒子owner分流及`0x0047B544..0x0047B555` 22-dword REP清理受管`proc_1cc9` core/CTest `199/199`；`0x0047B557..0x0047B56B`三处栈局部输出指针和帧源首dword按原序四PUSH，decoder真实CALL/子入口停与正常回复受管`proc_e8bf` core/CTest `199/199`。decoder回复在栈四参未清前写+0x0E14、两次独立源token/W/H访问、随后几何与+0x2B08门，`proc_4a4e/proc_c63f` core/CTest `199/199`；配置逐写、`sub_47CE70`三指令CALL/word OR1、先PUSH索引/IAT再写三项15、两次Metrics stdcall及host-surface CALL、0x31音频与phase=101写入，`proc_6ca4/proc_a9fd/proc_3f3f/proc_f4ab/proc_eae9/proc_d651`各core/CTest `199/199`。初次Metrics前缀测试因未显式设置IAT backing而失败，修正测试后复测通过，不把失败轮次计为验证。`0x0047B68D..0x0047B6E7`初次与已有owner均独立INC/重读phase，按signed IDIV3余数写+0x0E1C，共享真实粒子CALL；只有完整EAX==1写phase100并物理返回，其他EAX走default，`proc_41ca/proc_3aea` core/CTest `199/199`。先前case2适配端口错误将EAX>1当unbacked；按LST的CMP/JNZ修正该caller范围并更新测试。phase100的`0x0047B6E8..0x0047B746`非空释放窄CALL、清phase/delay、38+22 dword独立REP、reset真实CALL、三写及物理RET由`proc_70b0/proc_8597/proc_796a`分别core/CTest `199/199`验证；`proc_892f` 又独立核验phase100的38+22次DF=1双REP跨slot2/主记录及中间区/方向记录写、源header保持，core/CTest `199/199`；`proc_ddf0`因测试尝试复制含unique_ptr的state未编译，改为仅保存slot2与两个word后复测通过。case100当前**静态caller分支局部闭合**；释放包装层深层障点、其他selector/真实caller和316双向REVIEW仍未完成，不作整包验收。
- case100 `0x0047B409..0x0047B6B3`：phase 精确等于 CX=100 时先跳 `0x0047B6E8`，
  按与 case2/8 不同的顺序条件释放 `+0x0E14`、清 phase、清两个 dword 区域并 reset，返回1。phase==0 时先 signed 比较完整 `actor+0x2680` 与6；
  `>=6` 立即写 `+0x2956=0/+0x2958=1/+0x2680=0` 返回0；case100 此分支不额外读取帧尺寸或绘制，
  但共同前缀仍可能先访问 frame `+0x0C`。小于6时只有 `+0x2956` word 精确等于24才播放0xEB；然后发布 frame `+0x00`、把 signed 副 phase `+0x2956` 原值依次写三个缩放全局，
  绘制 `(X-sign_extend(actor+0x29B2),Y-actor[+0x2B4],W,H,actor[+0x2694]|4,0)`。

  绘制后重读完整 `+0x2680` 并按原 `and 0x80000001 / jns / dec-or-inc` 恢复 signed 除2余数；
  余数零时副 phase word 加4，非零时减1。以**更新后的** signed 副 phase 判 `>=24` 或 `<=-8`：
  命中才给旧 `+0x2680` 的 EAX 加1并写回 actor；`-7..23` 直接走 default 返回0，不增加计数。

  phase!=0 则按 `+0x0E14` 非零门选择重用粒子 owner 或初始化。
- case100 phase0 非终态 `0x0047B44D..0x0047B517` 故障前缀：首次读 `+0x2548` 后先 `push EBX=0` 才读源首 dword/写 `4CD730`；
  三次从 **signed i16 副 phase `+0x2956`** 分别读并写 `4CD71C/4CD30C/4CD304`，
  然后读新 flags/source，参数副本 OR4，以 `actor+0x2B4` 改写 EBP、读 u16 源高宽及 `+0x0D68/+0x29B2/+0x0D66`，
  绘制六参。该 CALL 正常回复时**先在六参仍在 ESP 上**重读完整 signed dword `actor+0x2680`（`0x0047B4C5`），
  才 `add esp,0x18`；若这次读障则绘制已正常执行但六参未清。后续 `and edx,0x80000001` 的负数补偿构造的是真 signed `%2`，
  基于本次重读计数判奇偶更新 aux word，旧的入口 `<6` 判值不得缓存；aux 更新后 signed 范围 `-7..23` 直接走 default EAX0，
  外围 `+0x2680` 不增；`>=24` 或 `<=-8` 才以重读的 EAX+1 写 `+0x2680` 并返回0。初始 phase==0 但 count>=6 的短路在这些源/绘制访问之前；
  不与非零 phase 的粒子门混合。

- case100 初始化 `0x0047B544..0x0047B684`：先清粒子区22 dword，再调用 `sub_4019A0` 存 `+0x0E14` 并发布 W/H、源坐标；
  `+0x0E24` 先写0、**随后** `+0x0E2C=1`，再读完整 `+0x2B08`，仅等于1时改写 `+0x0E24=0x154`；
  按序写 `+0x0E28=0x140/+0x0E36=24/+0x0E30=1/+0x0E34=100`，重读当前源 token/高后写 `+0x0E38=H>>2/+0x0E3A=40/+0x0E3C` word=0x56。

  调用 `sub_47CE70` 后仅 EAX精确1 时对 `+0x0E3C` **word** OR1；随后按序将 `+0x0E40/+0x0E44/+0x0E48` 各写15，
  调用 `GetSystemMetrics(1)`、`GetSystemMetrics(0)`、`sub_433F30` 与声音0x31，
  再把 phase word 写101。已有 owner 跳过以上全部初始化。从两路汇合的 `0x0047B68D` 起，先让 phase word 加1，
  做 signed `idiv 3`，用真实 signed 余数改写 `+0x0E1C=X+remainder-EBP`，然后调用公共粒子尾 `0x0047B6B4`。

  故首次初始化调用粒子前 phase=102、余数0；已有 owner 用当帧更新后的 phase 求余。尾部 `sub_434790` 仅当完整 EAX==1 才把 phase word 写100并返回0，
  其他回复直接落 default 返回0；此帧不立即执行 phase100 的释放/reset；仅当以后再次通过共同入口门和 selector100 时，
  才走该分支。粒子尾 `+0x0E14` 必须复用物理 actor 数据，与其他两个粒子 case 保持一份 owner。

- case100 粒子初始化原位停止 `0x0047B544..0x0047B684`：22-dword `rep stosd` 完成后，
  decoder 四参按局部输出指针、输出指针、输出指针、当前源首 dword 的压入顺序保留至 `0x0047B57C`；`0x0047B570` **先写 token `+0x0E14`、再读源 token，
  才清16 byte**，源宽/高与坐标随后逐点写。`+0x0E24=0`、`+0x0E2C=1` 均在 `+0x2B08` dword 精确1 门之前；
  写 `+0x0E28=0x140` 后再依 `+0x0E36=24/+0x0E30=1/+0x0E34=100` 写入，重读源高才写 `+0x0E38`，
  不允许把全粒子记录一次性物化而跳过部分前缀。`sub_47CE70` 回复恰为1才 OR word `+0x0E3C` bit0；
  `0x0047B649` **先压 nIndex=1 并装载 EDI=GetSystemMetrics 间接函数地址，再写三项 +0x0E40/+0x0E44/+0x0E48=15**，
  此处 fault 的 ESP/EDI 与写成功前缀须匹配。按 index1/index0 两个间接调用取得值后压相应结果调 `sub_433F30`（该 callee `retn 8` 清这两个入参）；
  随后声音0x31 正常返回才清声音8 byte、写 phase=101，下一条公共 `inc word` 使之102。已有 owner 完全跳过初始化，
  从其现有 phase 先增1，以 signed `idiv 3` 的 EDX 余数写 `+0x0E1C=X+rem-EBP`，`sub_434790` `retn 8` 且仅完整 EAX==1 写 phase100；

  内层停点不可提前转释放分支。进一步独立核对 callee `sub_434790` 的三条物理正常 `retn 8`：`0x00434898` 前明确 `mov eax,1`，
  `0x00434DA1` 和 `0x00434DC3` 前均 `xor eax,eax`；因此正常只回复1或0（不等于允许以任意 EAX mock 证明真实动态结果）。

  `0x0047B6CC cmp eax,1` 在正常0时直跳 default 返回0，**不**把 phase恢复100：本次初始化分支先写101再在 `0x0047B68D` 增为102，
  既有 owner 分支则从当前 word 增1；只有回复1才在 `0x0047B6D5` 写100。两条正常0出口的物理副作用不等价，
  需沿 callee 自身前缀观察其链表/计数字段，不能用同一个无副作用0回调代替。公共 `0x0047B68D..0x0047B6AE` 先对**word** phase 递增回绕，
  再 `movsx eax,word phase; cdq; idiv ecx`（ECX固定3），把向零截断的**有符号余数** EDX 与当前 signed X 相加并减 EBP，
  才写 `+0x0E1C`；例如原 phase=-2/-3/-4 时递增为-1/-2/-3，余数分别-1/-2/0，不能用非负模3或对递增前 phase求余。

  除法后的 EAX商又被 `movsx X` 覆盖，CALL `sub_434790` 入站 EAX 是本次 actor `+0x0E14` 的指针，
  而非商。

- case100 释放/reset `0x0047B6E8..0x0047B746`：读取 `+0x0E14` token 后立即令 EBP=`actor+0x0E14`，
  非零时压 token 调 `sub_4885A0` 再清4 byte；之后按 `phase word=0`、`+0x2A12 word=0`、38 dword `rep stosd`（沿入口 EDI）、EDI改为 EBP 后22 dword `rep stosd`、`sub_478850` 的次序，
  随后才置 EAX1 并依次写 `+0x2AAC=0/+0x2AB8=1/+0x2ABC=0` 返回1。两个 REP 之间不可清除原 EDI/EBP 所有权；
  本函数内没有 `cld/std`，正常 ABI 的 DF=0 才对应向高地址连续清 `+0x3D0..+0x467` 与 `+0x0E14..+0x0E6B`，
  若要求故障时独立观察 FLAGS/DF，必须保留 REP 的 DF 与逐 dword 前缀，而不能无条件假定正向。

此处 `phase` 是 actor `+0x2958` 的 signed 16 位词；所有 `+/-` 都按 word 回绕。与 32、100 的比较分别为 `jg/jge`、`jz/jge`，
不可合并；frame 高度以 u16 零扩展并与 signed phase 比较。case14 在 phase8 对 `+0x2954` 有独立比较。

case100 的 `+0x2680` 用 signed dword 与6比较，副 phase `+0x2956` 用 signed word 与-8/24比较，
`-7..23` 范围直接到 default，不递增计数。case50 的原 `.data:004A7DB4..004A7DEF` 十五项 signed dword 是 `0,4,12,24,32,24,16,8,4,0,-4,-12,-16,-20,-32`，
越界不得预先夹值。

共用目标 `0x0047A253`、`0x0047A935`、`0x0047B6B4`、`0x0047B6E8`、`0x0047B801/808/814`、`0x0047B98E/995` 从多个 case 汇入。
`0x0047A253` 比 `0x0047B801` 多先清 `+0x2954`；两路随后从 `0x0047B808` 清 `+0x2A12`、设 ECX=38，
以原 EDI=`actor+0x3D0` 在 `0x0047B814` REP 清 `+0x3D0..+0x467`，然后调 `sub_478850`、按 `+0x2AAC=0`、`+0x2AB8=1`、`+0x2ABC=0` 的顺序写入，
返回1。case2/8 从自己前缀先清 `+0x2A12` 和第一段38 dword，再按有无 token 释放、把 EDI 改成 `actor+0x0E14`、设 ECX=22，
才跳同一 `0x0047B814` 清粒子区域并重置。case100 的 `0x0047B6E8` 则**先**条件释放，再写 phase=0、清 `+0x2A12`、用旧 EDI 清38 dword、用 EDI=`actor+0x0E14` 再清22 dword、调用 `sub_478850`，
同样返回1；清零与释放顺序不能跨 case 统一。case12/51 从 `0x0047B98E/995` 进入另一条38 dword 尾，
phase reset 是否已发生由前驱决定；其 `sub_478850` 在 `0x0047B9A7`，三个 latch 的实际顺序为 `+0x2AAC=0`、`+0x2ABC=0`、`+0x2AB8=1`，
与 `0x0047B81A` 尾不同。`0x0047A935` 只切换 selector2 并清 phase，返回0，不调用 reset。

粒子尾 `0x0047B6B4` EAX==1 只置 phase100并返回0。任何尾部 EDI/EBP、ESP、FLAGS 和停止时前缀都必须按当前前驱独立核对；
以下 case 推导不能替代完整双向审计。

三个 reset 尾的寄存器/写障分界再次按独立指令核对。立即重置 `0x004798B3..0x0047991F` 暂时先写 phase word1000与 selector byte0，
按门可能 OR nested byte，然后清两 dword、再写 phase0、设置 ECX=38、`xor eax,eax`、写 `+0x2A12=0`、`0x004798F3 rep stosd`；
`sub_478850` 正常后写 `+0x2AAC=0/+0x2ABC=0`，**先压 EBP=1**、才写 `+0x2AB8=1`，
再调有 `retn 4` 的 `sub_47E950`，最后 `mov eax,ebp` 返回1，若 `+0x2AB8` 写障则栈上已有该入参。

通用尾 `0x0047B808` **在**设置 ECX=38 与 `xor eax,eax` **之前**写 `+0x2A12=0`，
而另一尾 `0x0047B995` **先**设置 ECX=38、`xor eax,eax` 才在 `0x0047B99C` 写 `+0x2A12=0`；
若该字段写障，两个尾的 ECX/EAX/FLAGS 不同。`0x0047B81F` 在第一项后缀写 `+0x2AAC=0` **之前**把 EAX设1，
依次再写 `+0x2AB8=1/+0x2ABC=0`；`0x0047B9AC` 则**先写 `+0x2AAC=0`、再把 EAX设1**，
随后 `+0x2ABC=0/+0x2AB8=1`，该首项写障时 EAX还保留 `sub_478850` 回复。case100 release 尾在调用 `sub_478850` 正常后先把 EAX设1，
再以 `+0x2AAC/+0x2AB8/+0x2ABC` 次序发布。正常 DF=0 时每段38 dword 从 `EDI=actor+0x3D0` 写半开 `[+0x3D0,+0x468)` 并使 EDI最终=`actor+0x468`；
22 dword从重新置入的 `EDI=actor+0x0E14` 写 `[+0x0E14,+0x0E6C)`、EDI最终=`actor+0x0E6C`。

本函数没有 `cld`，若保留非标准 DF=1 入站状态则 REP 逆向行走；REP 的内存故障应留 EIP=REP、ECX=尚未写的次数、EDI=失败地址、EAX=0，
以及此前逐项写前缀，不能整段事务回滚。正常每条 `rep stosd` 自身不改变算术 FLAGS，故其后首次 `sub_478850` CALL 入站保留最近一次 `xor eax,eax` 的标志（中间 MOV 不改 FLAGS）。

九个物理 REP 站点已独立枚举：`0x004798F3`、`0x00479B30`、`0x00479C81`、`0x0047A628`、`0x0047B555`、`0x0047B716`、`0x0047B71F`、`0x0047B816`、`0x0047B9A3`。
其中 `0x0047B816` 是**同一条指令的两种入口计数与 EDI 基址**：通常重置从 `0x0047B80F` 进入 ECX=38/EDI=`actor+0x3D0`，
case2/8 释放则从 `0x00479CA1`/其复用入口直接跳到 `0x0047B814`，携带 ECX=22/EDI=`actor+0x0E14`；
后者虽同 EIP，却不得误清38个 dword或重复清已在 case 前缀清过的第一段。其它八条 REP 的计数与首址从其各自前驱计算，
不能以“9个 REP 等于9种独立区域”作统计。

全部22个物理 `retn` 的**紧前一条**均为 `add esp,14h`（从原 LST 各自位置双向枚举22/22，而非依赖 case 入口猜测），
其紧前又均为 `pop ebx`；所以正常退出不沿用最后一次 phase CMP、绘制 CALL、`xor eax,eax` 或 `inc word` 的算术 FLAGS。

`add esp,14h` 更新 CF/OF/SF/ZF/AF/PF。以无非约定栈损坏、92个直接 callee 按 LST 物理 `retn` 清栈、六个 Win32 IAT `GetSystemMetrics` 按 stdcall ABI 正常清一参的路径为前提，
独立按 LST 的 CFG 传播 249 块、98个 CALL 到 22 个出口，CALL 临时栈深与清理匹配、全部汇合点深度一致；
设本函数入口 ESP=`S`，此 ADD 前是 `S−0x14`，后是 `S`（模2³²），故该出口 FLAGS 是 `ADD32(S−0x14,0x14)`，
并非不依赖 S 的常量。`retn` 只取返回地址、无算术 FLAGS 改写，DF 未因尾部栈清理改变；若返回地址读取故障，停止 EIP 是各自的 `retn`、ESP 仍为 `S`、FLAGS 已由 ADD 更新、parent 成功/失败后缀都未执行；
正常 RET 后 ESP 为 `S+4`。若测试需要 callee 正常返回 FLAGS，必须按真实返回栈指针计算；四个 parent 下一条 CMP 又会覆盖它，
但对手分派先执行的 `0x00456514 mov ebx,1` 不覆盖。错误把从任意 case 内中途算出的 flags 当作最终 RET 状态的向量应撤回。

22 个实际 `retn` 地址再次按原指令与修正 ledger 对照：正常 EAX=1 的 5 个出口为 `0x0047991F`（此处 `sub_47E950` 的两个正常 `retn 4` 在 `0x0047F0BC/0x0047F0F3` 前都 pop EBP，
故 caller `mov eax,ebp` 恢复值1）、`0x00479936`、`0x0047B746`、`0x0047B83D`、`0x0047B9CA`；
正常 EAX=0 的17个出口为 `0x00479B05`、`0x00479EA9`、`0x0047A19F`、`0x0047A252`、`0x0047A814`、`0x0047A934`、`0x0047A94C`、`0x0047AA7A`、`0x0047AB99`、`0x0047AF23`、`0x0047B2E7`、`0x0047B408`、`0x0047B517`、`0x0047B537`、`0x0047B6E7`、`0x0047B800`、`0x0047B98D`。

`build/workpack316/return-audit.tsv` 逐出口登记22个 EAX 来源指令、末次 ADD、正常 ESP、返回地址读障状态，
以及四个保存寄存器 POP 的各自 EIP/ESP 与其间指令；22组 POP 顺序已与 LST 对照。这是返回前故障站点**索引**，
不证明每条到达路径在 POP 失败时的 EAX/FLAGS、此前物理写或 nested CALL 已验收。特别是 `0x0047B508 pop edi` 成功后、`0x0047B50F pop esi` 前尚有 `0x0047B509 mov [esi+0x2680],eax`，
该写自身也可停止；不能把四次 POP 及 ADD/RET 抽成原子出口。

## 5. 物理 CALL 与状态下界

21 次 `sub_416FF0` 不可仅以接受四参的无副作用 mock 代替：callee 原 LST `0x00416FF0..0x00417047` 先从栈读 `(left,top,right,bottom)`；
把 **signed** left/top 小于零各裁剪为0，把 right/bottom 大于共享 `dword_4A0E78/dword_4A0E7C` 各裁剪到对应值（未把 right/bottom 额外裁到零或强制 width/height 非负）；
做32位 `bottom-top`、`right-left`。按 `4CD2F8=left`、`4CD720=height`、`4CD734=top`、`4CD310=width` 的指令顺序写共享绘图状态；
正常回复固定 EAX=1、ECX=width、EDX=top，EBX/EBP/ESI/EDI 保留，flags 最后由 `sub ecx,esi` 设置。

四参为 cdecl，callee `retn` **不清**调用参数；case3/4 及 case7/13/14 各对应四 CALL 绘制组跨两次矩形+两次绘制累计20 dword，
组尾才一次 `add esp,0x50`；case7/13 各有两组，不可对每次 CALL 提前清栈。Callee 的全局读取和每个共享字段写点还需纳入故障前缀；
`dword_4A0E78/7C` 并非本函数代码中的字面640/480。

28 次 `sub_4170E0` 同样不是只影响像素的空回调：callee LST `0x004170E0..0x004174CA` 先读共享 `dword_4CD730` 并解引用首 word，
读取共享矩形、源属性及诸多绘图标量；其中多处 `jle loc_417494/417496` 在不调用绘制函数的情况下也会进入共同收尾。

正常绘制分支在 `0x0041748A` 经 `dword_4CD318[ebx*4]` **间接 CALL**，此 CALL 若不返回，
不应执行清理。共尾 `0x00417494` 把 EBP 清零，`0x00417496` 虽是**跳过该指令的另一入口**，但对照所有三条跳向它的 `jle`（`0x0041711D/0x0041717E/0x0041729B`），
各自的 EBP 已由 `0x004170F5` 或 `0x00417105` 清零、且到该跳点前未改写；特别是第三条 `0x0041729B` 必经 `0x00417144 jnz loc_417239` 才能进入其前身 `0x00417239`，
而 `0x004171A0..0x004171AC` 虽可把 EBP 改为非零，其后 `0x00417234 jmp loc_417394` 隔绝通往 `0x00417239` 的错误线性落入，
因此两种正常收尾都依次把零写入 `4CD300/4CD304/4CD30C/4CD71C/4CDBE0/4CD75C/4CD718/4CC2F0`，
最后恢复寄存器并 `retn`。正常 FLAGS 还需分两类：内层像素 CALL 正常返回或六条跳到 `0x00417494` 的 `jle` 都执行 `xor ebp,ebp`，
故 ZF1/CF0/OF0/SF0/PF1；三条直接跳到 `0x00417496` 的 `jle` 跳过 XOR，FLAGS 分别保留 `0x0041711B cmp eax,ebp`、`0x00417176 cmp edi,ebp`、`0x00417293 cmp edi,ebp` 的结果（各处 EBP=0），
即使最终八项写零相同也不能统一为 XOR FLAGS。先前把 `0x00417496` 解释成“最终八项可能写非零”的猜测已撤回；

**两个入口的写前缀仍不相同**，即使正常返回最终八项相同，不能把它们在任意读障/内层 CALL 故障时提前清零。也不能假设所有分支必经像素绘制或 EAX 固定为1；
需分别保留实际内层绘制回复、EAX/ECX/EDX/FLAGS、逐项写与故障前缀。上述内层 CALL **不计入** `sub_479850` 主体的98个物理 CALL。

28处外层绘图调用又按 LST 精确区分四类**条件性**停点：`0x004170E0 mov eax,dword_4CD730` 首个全局读障发生在 callee 保存寄存器前；
成功读全局指针后，`0x004170E8 cmp word ptr [eax],0xFFFF` 可在已保存 EBX/EBP/ESI、尚未保存 EDI 时停住，
ESP 比该 callee 入口少12 byte，EAX为前一读出的指针，FLAGS仍为外层调用前值。若进入像素分支，`0x004173EF mov [esp+0x10+arg_10],eax` 的参数栈写障发生在 callee 已保存四个寄存器、未压像素五参时；
此时 ESP 比 callee 入口少16 byte，EAX仅保留原 EBX bit31，FLAGS来自 `0x004173EA and eax,0x80000000`，
此前绘图全局的真实路径写入不回滚。随后唯一间接 `0x0041748A call dword_4CD318[ebx*4]` 的目标表读障发生在四个保存寄存器和五个像素参数均已压栈后、间接调用的返回地址尚未入栈时；
ESP 比绘图 callee 入口少36 byte，EAX已被 `0x00417481` 改为 `arg_14`，FLAGS来自 `0x0041747F add ebx,eax`（此处加数为0或0x80）。

28次外层调用前尚有6/10/12/20个临时 dword，分别11/8/2/7处，故间接表读障的 ESP 相对本函数入口分别为减 `0x64/0x74/0x7C/0x9C`；
到表读之前，`0x00417128` 在 `(arg_10 & 0xFFFC)==0x14` 且 `dword_4CC2F0>15` 时先把**栈上** `arg_10` 与 `0x80000003` 相与；
进入绘制路径后 `0x004173A8` 读当下完整 `arg_10` 到 EBX，`0x004173E8..0x004173EF` 又把**同一栈槽** 改写为仅保留原高 bit31。

`0x00417468` 测试这个新栈值：非零则像素表基数0，零则基数0x80；`0x00417479` 只把保存于 EBX 的低16位保留下来并加基数，
形成 `table_index=(arg_10_at_0x004173A8 & 0xFFFF)+(bit31?0:0x80)`（按32位加法），
不是使用当前已被截为 bit31 的栈值作低位索引。原本栈参 `arg_10=0` 且 `0x004170FB` 没有 OR bit31 时索引0x80；
同样低16位为0而 bit31=1 时索引0。若 `0x004170E8` 首 word为0xFFFF、`dword_4CD764=0`，
`0x004170FB` 还会在两者之间先 OR bit31；若进入前述 `0x00417128` 分支，低16位只剩低2位。这些只给出**到达间接 CALL** 的索引推导，
不能证明每个输入均能到达该点；表读障时此前对 `arg_10` 栈槽的写与绘图全局写都已提交。`dword_4CD318` 并非 LST 固定像素函数地址：
初始化函数 `0x00416D90..0x00416F0E` 在多个分散的表项写入函数指针；这里的间接 CALL 实读运行时表项，
不能用任一静态默认函数取代，也不得把源索引截成当前表项数量。在同一页保护条件下早期全局/指针读障可能先于表读障，且像素 callee 的后续访问仍未审毕。

四处 `sub_47CE70`（`0x00479C1C/0x0047A70E/0x0047B634/0x0047B8D0`）的 callee LST 实际只有 `0x0047CE70 movsx eax,byte ptr [ecx+2694h]`、`0x0047CE77 and eax,1`、`0x0047CE7A retn`，
所以正常回复**只能0或1**，从调用时 actor render flags 的低 byte bit0读取，不得使用无依据的可任意 EAX mock 替代。

该单 byte 源读取发生于四个调用的各自时刻，CALL 后 `cmp eax,1` 决定目标低 byte/word OR1；原 callee 只改 EAX 与 AND 的 flags，
ECX/EDX/EBP/ESI/EDI/EBX 保留。本工作包可以按此三指令提供等价 typed 边界，但不会提前更改 inventory 中 `audit_order=329 / sub_47CE70` 的独立关闭状态；
原调用地址与 fault/RET 时序仍须保留。

六次间接 `GetSystemMetrics` 的三组 CALL/栈合同亦由各自主体 LST 区分：case2 `0x00479C2C` **先**载 EDI=IAT 再压1，
case8 `0x0047A71E` 先载 EDI=IAT 再压 EBP=1，case100 `0x0047B649` **先压 EDI=1**、`0x0047B64A` 才载 IAT 到 EDI，
且在第一 CALL 前先写三个 `+0x0E40/+0x0E44/+0x0E48=15`。按 Win32 `GetSystemMetrics` stdcall ABI（三处原 caller 无 `add esp,4`），
第一次 CALL 正常由 callee 清自己的 nIndex，再压其 EAX 回复，随后压 EBX=0 调第二次；第二次正常同样清自己的 nIndex，
再压新 EAX 调 `sub_433F30`，该已关闭 callee 在正常 `retn 8` 清两个返回值。因此第一次间接 CALL 入口停点的 caller 栈只增加1个 nIndex；
第二次入口停点另外仍有第一次回复值和第二个 nIndex 共2个 dword；`sub_433F30` 停点则保留两个回复值。该 host callee 正常先发布宿主 `+0x0B50/+0x0B54`，
再调用行表与 `sub_4342E0`；后者末尾 `0x00434333 add eax,edi`、`0x00434335 add edx,ebx`、写 `+0x0B60/+0x0B64`，
外层只弹寄存器后 `retn 8`，故正常返回 EAX 为传入 height（按此前已关闭的正常矩形证据），FLAGS 来自矩形最后的 `add edx,ebx`，
不能沿用行表/Win32 回复 FLAGS。

若 Win32 call 停止，不能伪造正常 stdcall 清参、不能将其回复推测为屏幕宽高；

case2 停点 EBP=已覆盖 Oy，case8 EBP=1，case100 EBP=入口 Ox，三个路径的 EDI 都是 IAT 函数地址。
任一点 CALLEE 可改变共用图形状态，下一物理 actor/source 读取须按原时序执行。三组各八处指令地址（一次 IAT dword 读取、两个间接 CALL、四次参数/回复 PUSH 和一次宿主 CALL）的先后记录在 `build/workpack316/metrics-pair-audit.tsv`，
仍为 partial。特定 IAT **读障**与外部函数内部故障不可混同：case2 `0x00479C2C` 和 case8 `0x0047A71E` 在压 nIndex=1 **之前**读取 `ds:GetSystemMetrics`，
而 case100 `0x0047B649 push edi` 已先将 nIndex=1 压栈、`0x0047B64A` 才读 IAT；
后一读障时 `+0x0E40/+0x0E44/+0x0E48` 三项仍未写。每组首次 Win32 正常回复后 `push eax` 独立写入高度槽，
随后才压 EBX=0；第二次正常回复后另一个 `push eax` 独立写入宽度槽，再调用 `sub_433F30`。故第一回复 PUSH 写障不能伪造第二次系统调用，
第二回复 PUSH 写障要保留第一次高度；第一次回复在第二次调用进行时仍是真实的栈字节，不是第二次调用的实参。CALL EDI 目标是已读取的 IAT 函数指针，
WIN32 内部访问及精确寄存器/FLAGS 出站不在 LST，现有证据只校验本函数可见指令和 stdcall 正常清一参的条件性栈合同。

所接 `sub_433F30` 本体21条指令：三次保存、读取宽/高外参、各压一份后在 `0x00433F3F` 写宿主 `+0x0B50=width`、`0x00433F45` 再写 `+0x0B54=height`，
然后调 `sub_433E90`（正常 `retn 8`）与 `sub_4342E0`（正常 `retn 10h`），三级 POP 后自身 `retn 8` 才清先前的两个指标回复。

19处可障栈/宿主/CALL站点见 `build/workpack316/host-callee-audit.tsv`，状态仍 partial：
第二字段写障时第一宽度已发布、高度仍旧，两个 nested 调用都未发生；不可因已关闭的 host helper 正常路径通过就省略此中途可见的分项前缀。

case2、case8、case100 三处 `CALL sub_433F30` 现先执行 CALL 返回槽，再按 LST 分别在 `0x00433F30/31/35/36/37/3D/3E` 保存 EBX、从原父栈读取 height、保存 ESI/EDI、读取 width、压内层 height/width，恰七项子级访问；在首次 `0x00433F3F mov [ESI+0B50h],EDI` 宿主字段写前停止时，相对父 CALL 前 ESP 减24，EBX=height、EDI=width、ESI=宿主 token，EAX/ECX/EDX/EBP、FLAGS/DF 保持原入站，末次 PUSH=width。请求显式标为不可写或序号恰在首写前均不调用宿主 port；受控 port 未返回时保留五项栈写和两次实参栈读，正常回复才按原 `retn 8` 清父双参。case2 逐序号 0–7、三处回包停点及 case2 显式不可写于 `proc_cca8` 定向 `battle.actor_frame_316` 1/1 通过；阶段源码快照 `proc_d513` 完整门禁通过 Linux core 200/200、ASan core 200/200、Linux app 206/206；测试说明修正后最终阶段快照 `proc_f35c` 重跑同三组门禁均通过（200/200、200/200、206/206）。**上述受控停点未执行**首宿主写、第二宿主写及两个深层子调用；正常受控回包也不证明原版宽高发布或 Win32/像素路径等价，仍为 partial。该阶段门禁不代替 Workpack 316 最终门禁。

四处 `sub_4019A0`（`0x00479B46/0x0047A63E/0x0047B56B/0x0047B902`）的正常返回不等于必须分配成功：
callee `0x004019A0..0x00401B66` 是四参 cdecl `retn`，入口 `mov cx,[pixel_header]` 与 `dword_4CDE74` 比较，
不符时 `0x004019BD` **EAX=0且三个输出指针尚未写**；通过后按物理顺序写三个输出局部 `[arg_4]`、`[arg_8]`、`[arg_C]` 的 zero-extended width/height 与 masked 格式 dword，
格式既非0x10也非8时 `0x004019FA` **EAX=0但三个输出已全部发布**。对合法 format，`0x00401A06/0x00401AC3` 的内层 `sub_487C10` 返回 EAX 后无 `test eax` 容量门；
它先保存在 ESI，而原 EAX 经后续循环保留直到正常 `0x00401AB9/0x00401B66`。若分配回复0且行流首 word 即0，
会跳过写入正常返回 EAX0（三个输出已写）；若含 literal/填充像素，则原内层指针的首次存储在 format16 `0x00401A4B` 等处、format8 `0x00401B05/0x00401B24/0x00401B42` 等处按读到的源命令发生，
指针0是否故障须以物理页映射判定，不能把所有分配零回复概括为“正常跳过”。四处 `arg_4/arg_8/arg_C` 指针**均依次指向当前函数的 `var_8/var_C/var_10`**，
栈入参顺序也均为 `push var_10 地址、push var_C 地址、push var_8 地址、push 源首 dword`；

前一草稿“物理 push 顺序不同”不成立，已撤回。差异是地址 LEA、源 token 读取与副作用穿插其间：case2/8 在 `rep stosd` 前先 LEA `var_C`，
REP 后再 LEA 另两个；case100 在 REP 前 LEA `var_8`，后按 `var_10/var_C` 计算；case51 的 `0x0047B8EF` 在已有第一指针压栈时写 `+0x0DF0=90`，
再压第二指针、解引用源首、**用已减8的 ESP** 的 `0x0047B8FC lea eax,[esp+0x2C+var_8]` 得第三指针。

四处 `+0x2548` token 首读均发生在第一个输出指针已入栈之后；case2/8/100 对源首 dword 的解引用发生在第三个输出指针也入栈之后，
case51 则在只压了两个输出指针且 `+0x0DF0` 已发布时解引用。若后者读障，第三指针 LEA/压栈尚未执行；四处无论何路径都不能把这些读障移到所有四参已准备完之后。

四个 caller 均不以 EAX0 当 typed-stop，而按自身次序写 token0 及随后各字段/音频/效果调用；若 callee 内部读/写故障，
须保留该分支已写的局部与入口参数栈，不能以“分配失败即跳过粒子后缀”的现代便利分支取代。case2/8/100 在 decoder 正常返回后先写 token、重读下一次 `+0x2548`，
**之后**才 `add esp,0x10`；case51 先读 `+0x428`、写 `+0x0DB8` token、写 `+0x390`，
再压两参调用 `sub_485610`，直到该声音 CALL 正常返回后 `0x0047B929 add esp,0x18` 才一次清掉声参8 byte和先前 decoder 16 byte。

因而声音 CALL 内停点仍承载该16-byte decoder 参数栈。

四处 decoder CALL 的16条独立 PUSH 原址及源首 dword 读取位置见 `build/workpack316/decoder-call-args.tsv`；
第4处 case51 的 `0x0047B8FA` 读源首时只完成两个输出地址 PUSH，其他三处源首读时已经完成三个。按本函数入口 ESP=S，
四个输出地址均落在物理栈槽 `var_10=S−0x10`、`var_C=S−0x0C`、`var_8=S−0x08`，但三个输出写的时间与栈位置不能混为一个原子动作。

`build/workpack316/decoder-prefix-audit.tsv` 按原 `sub_4019A0` 列出 header 匹配路径前16处内存访问：
`0x004019A0` 先读 arg0，`0x004019A4` 再读预期 header 全局；header 不符时三个输出不动，
匹配时四次保存 EBX/EBP/ESI/EDI 后按 `0x004019D0` 写 width 到 `S−0x08`、`0x004019D8` 写 height 到 `S−0x0C`、`0x004019E7` 写 masked format 到 `S−0x10` 的原顺序执行。

若第二处写障，width 已发布、height 尚未发布；若第三处写障，width/height 均已发布、format 尚未发布，
且 FLAGS 是较早 `cmp format,0x10` 的结果。三写成功后才区分 format 0x10/8/无效；无效 format 正常 EAX0 并保留三项输出，
不能当 typed-stop。以上前缀及两种正常早退不构成对163指令 decoder、两次内层分配、循环读写/深层故障的完整验收。

合法 format16/8 的 decoder 在输出三项均已发布后分别从 `0x00401A06/0x00401AC3` 调 `sub_487C10` 分配 wrapper。
其13条指令、11个栈/全局/CALL站点见 `build/workpack316/allocator-wrapper-audit.tsv`：
按 arg10=0、argC=0、arg8=1 入栈，`0x00487C19` **独立读取** `dword_53D1B4` 后压作 arg4，
`0x00487C1F` 再读 decoder 的 Size 栈 dword 并压为 arg0，`0x00487C23` 调32条指令的 `sub_487C80`；
仅后者正常返回才 `add esp,0x14`，重算 FLAGS、弹 EBP 并由普通 `retn` 回到 decoder。若 decoder 入站 ESP=`S−0x38`，
其四次保存和一次 Size PUSH/CALL 后 wrapper 入站 B=`S−0x50`；`0x00487C19` 读取全局故障时 ESP=`B−0x10=S−0x60`，
已压 wrapper EBP/三个立即数而尚未压全局值/Size，三项输出 dword 仍保持已提交。该全局读是条件性停点而非分配结果；
`sub_487C80` 可转 `sub_487CD0`（254条、11次嵌套 CALL）及缺内存回调 `sub_48A900`，
深层分配、retry、CRT 与各来源 FLAGS/寄存器仍为 partial。

case12 的 `sub_422C70` 与 `sub_423020` 亦不能当成仅返回的四参函数：
各 CALL 前有 **H/W/Y/X 四项被调实参，另有更早压入的 EBX=0**；callee 仅按 `arg_0/4/8/C` 读取四项，
调用点有五个 dword 留栈，`0x0047AB86 add esp,0x14` 在 aux phase 加2之后才清这20字节。

两 writer 各只有一条正常 RET：反向 `0x004233C8` 的紧前为 `0x004233C5 add esp,0x1C`、正向 `0x00423017` 的紧前为 `0x00423014 add esp,0x20`，
所以 callee 回复 FLAGS 来自各自栈指针 ADD，不是最后一个像素算术，且这两次 ADD 均**不**清 caller 留下的五参。

权威 callee LST 分别为 `0x00422C70..0x00423017`（279 条指令）和 `0x00423020..0x004233C8`（281 条指令），
均无内层 CALL、cdecl `retn`，但在循环中读取 `4A0698/4A069C/4A06A0/4A06A4` 的四个预发布变换全局，
以及 `4A0E74/4CD2F8/4CD310/4CD720/4CD730/4CD734/4CD76C` 的绘图共享状态；分支和内存访问可产生多个实际停止点。

现把 case12 两处生产 CALL 的受控停点移至首次未被局部模型读取的全局：设父 CALL 前 ESP=P，CALL 先压返回地址；正向 `0x00422C70 sub esp,20h` 后在 `0x00422C73` 读 `4A0698` 前 ESP=P−36，末次 PUSH 仍为父返回地址；反向 `0x00423020 sub esp,1Ch` 后 `0x00423023/24` 依次保存 EBX/EBP，再于 `0x00423025` 读 `4A06A0` 前 ESP=P−40、末次 PUSH 为旧 EBP。两路 FLAGS 由各自 SUB ESP 计算，DF 与其他寄存器仍为父入站值。首全局访问序号或缺可读 owner 不调用 raster port，反向两处保存写也能分别在提交前停止；窄 port 未返回时保留这些局部栈与 FLAGS，不回滚至 callee 入口。正常受控回复仅追加已知的反向两次栈写，不能据此认为像素循环或未建模全局已完成。两处父路由与直接停点的定向 `battle.actor_frame_316` 于 `proc_4d6a` 1/1 通过（首次 `proc_d7c8` 两处历史预期仍要求 callee 入口 IP，已修正）；阶段源码快照完整门禁 `proc_a410` 通过 Linux core 200/200、ASan core 200/200、Linux app 206/206。深层像素、裁剪、异常和原版差分仍为 partial；这不是 Workpack 316 最终门禁。

仓库已有独立 `assembly_exact` 证据 `analysis/04-reverse-engineering/evidence/legacy-scaled-rle-writers-00422c70-00423020.md`、`rendering::write_legacy_scaled_rle_forward/reverse` 与定向 UT；
callee 首部的四参、四项 transform 与共享画面访问已再对照 LST，case12 bit0=0 应转 forward，
bit0=1 转 reverse。需要在调用**时**物化当前四项 transform、clip/source/surface 并复用该 typed writer，
不能空 mock；现有 writer 将实际越出 owned framebuffer 的原裸指针故障隔离为 `destination_out_of_bounds`，
Workpack 316 须再审其状态如何映射到外层 typed-stop 的 EIP/ESP/前缀，不能因为先前 writer 已关闭就声称跨 callee 故障自然等价。

按各 callee 原 LST 对十五个直接目标的返回栈归属再分层（此处只证明入口、RET opcode 与是否含内层 CALL，
不代替逐路径副作用审计）：`sub_47CE70` 3 条、无嵌套、普通 `retn`；`sub_416FF0` 30 条、无嵌套、普通 `retn`；
`sub_422C70` 279 条与 `sub_423020` 281 条无嵌套、普通 `retn`。`sub_4019A0` 四个普通 `retn`（含两条 EAX0 早退）且有两处嵌套 `sub_487C10`；
`sub_4170E0` 普通 `retn` 且有一处**像素表间接** CALL；`sub_4885A0` 普通 `retn` 包含一次 `sub_4885C0`；
`sub_485610` 普通 `retn` 包含一次 `sub_485CE0`；`sub_478850` 普通 `retn` 包含一次 `sub_439070`。

`sub_433F30` 是 `retn 8`、两处内层 CALL；`sub_434790` 三个出口都是 `retn 8`、含内层 CALL；
`sub_4344E0` 亦 `retn 8`、含内层 CALL；`sub_47E950` 两出口皆 `retn 4`、含内层 CALL。

首两条初始化 callee `sub_4321E0` 七处普通 `retn` 且至少两处内层 CALL、`sub_4315D0` 普通 `retn` 且二十余处内层 CALL。
最后第十六类是六处 `call edi` 的 Win32 IAT `GetSystemMetrics`，其实现指令不在本 LST 内；
接口只能按真实调用 ABI/外部结果及可见前缀处理，不能凭本证据声称 WIN32 内部 fault 逐指令已验证。前述直接目标的 `retn 4/8` 不可误与 caller 自己的 `add esp,4/8/16/0x30/0x50` 混算；
原 callee 正常回复后重读 actor/全局的时刻依各调用点原指令确定。

音频 `sub_485610` 在本函数有19个物理 CALL，不能把 mock 任意 EAX 当作原版正常回复：该 wrapper `0x00485610..0x00485645` 先按两项入参算术转换并压**六个**内部实参，
调用 `sub_485CE0`，然后普通 `retn`，不清外层两参。内层 `sub_485CE0` 三个正常出口分别在 `0x00485DB2/0x00485E07/0x00485E87` 执行 `xor eax,eax`，
到 `0x00485DB5/0x00485E0A/0x00485E8A` 各 `retn 18h` 清六参；故只要该内部调用正常返回，
19处音频 CALL 均以完整 EAX=0 返回，算术 FLAGS 在内层出口由 `xor eax,eax` 给出（AF 不作已知断言），
EBX/EBP/ESI/EDI 由内层保存恢复；外层 ECX/EDX 已经历乘法、移位及内层/平台音频调用，不得复制入站值。任何内层或外部音频接口中途停止，
CALL 前已经压入的外层两参与内层参数/寄存器前缀须按实际停点保留，不能等同于上述正常零回复；case51 另在正常音频回复后才一次清掉其遗留 decoder16+音频8字节。

音频实物设备与外部 AIL 服务当前没有原版 oracle，这里只核固定 LST 正常出口。额外按原 LST 栈流核到条件性嵌套停点：
wrapper 六次 push 后以 `ECX=unk_4C8450` 进入 `sub_485CE0`，其四次保存寄存器后在 `0x00485CE6` 调无 prolog 的 `sub_485CC0`；
后者首个非栈读 `0x00485CC0 cmp dword ptr [ecx+0x54],1` 若故障，EIP即此处，ESP相对 `sub_479850` 入口在前18个音频点为 `−0x60`，
case51 的 `0x0047B924` 则为 `−0x70`（尚保留 decoder 四参），FLAGS 仍来自 wrapper `0x00485631 and ecx,0xFFFF`。

该故障之前 wrapper 的栈参数读与各保存/压参仍可能先停止；一处深层向量绝不意味着19个音频链路的完整异常已经核完。

19处音频 caller 的两参物理 PUSH 已逐点登记在 `build/workpack316/audio-call-args.tsv`（38条 PUSH）；
每处第一参先读**当下** `dword_4AB784` 并压寄存器作为 wrapper 的 arg4，不能当固定音量快照。arg0 的第二 PUSH 有16处立即数 `0x31`、case100 `0x0047B445` 立即数 `0xEB`、case50 `0x0047B775` 压 EAX、case51 `0x0047B924` 压 ECX。

后两处仅分别在 `0x0047B75D mov ax,[actor+0x428]` 与 `0x0047B907 mov cx,[actor+0x428]` 覆盖低16位；
PUSH 时完整高16位保留前驱回复残值，wrapper 随后才在 `0x00485631 and ecx,0xFFFF` 截断，
不能把故障时 caller 栈字节假造为零扩展。19指令 wrapper 的两次外参栈读、六次嵌套 PUSH、一次 CALL 和一次 RET 共10个物理栈访问站点、ESP/寄存器前缀见 `build/workpack316/audio-wrapper-audit.tsv`；
其 `0x0048561C imul ecx` 是带隐含输出 EDX:EAX 的单操作数乘法，arg4 左移7后乘常数 `0x2E8BA2E9`，
接着对高 dword 算术右移、符号校正并压成内层 arg8。外层入站 ESP 为 C 时，六参依次从 C−4 至 C−24 压为 arg14=0、arg10=1、argC=0、arg8=缩放值、arg4=低16音效 ID、arg0=0；
`sub_485CE0` 正常 `retn 18h` 后 wrapper 自己普通 `retn`，并不清父级两参。

内层 `sub_485CE0` 仍有未审 Win32 AIL、资源和 REP 分支，且先行链存在**两次修改 wrapper 自己压下的音效参数**：
在通过 `sub_485CC0/sub_485CD0` 并读到非零 arg4 后，`0x00485D14` 将 `[esp+10h+arg_4]` 写成来自内层 arg0 的 ESI=0；
随后条件调用 `sub_486490`，正常返回后 `0x00485D24` 再把其 EAX token 写入相同 dword。该槽从 wrapper 入站 C 看为 `C−0x14`（第五次 `0x00485638 push ecx`）；
后续 `0x00485D9C/0x00485DF1` 在分支上重读的是当时槽值，不是原音效 ID。若第二写故障，第一次写0不可回滚；
若条件性更早 nested 停住，不能伪造任一写入。此段仍是静态向量，不是19条音频链路的完整 typed-stop 或动态差分验收。

`sub_416FF0` 的 21 个矩形 CALL 在原 LST `0x00416FF0..0x00417047` 共30条指令、无嵌套，
15处物理可故障栈/全局访问已逐项列入忽略目录 `build/workpack316/rectangle-callee-audit.tsv`，
并关联至21条 `call-audit.tsv` 行；所有行仍为 partial。四个 cdecl 栈参的物理读序为 arg0、arg4、arg8、`dword_4A0E78` 上界、argC、`dword_4A0E7C` 上界；
原带符号 `JGE/JLE` 分别先将 left/top 负值归零、将 right/bottom 截到各自上界，然后作32位回绕 `height=bottom-top`、`width=right-left`，
无额外非负宽高校验。四次**各自可停**的全局写序为 `4CD2F8=left`、`4CD720=height`、（中间 `pop edi`）`4CD734=top`、`4CD310=width`，
最后设 EAX1、`pop esi`、`retn`；正常 EDX=top，ECX=width，ESI/EDI 恢复，FLAGS 来自 `0x00417027 sub ecx,esi`，
期间 MOV/POP 不改 FLAGS。第一个参数栈读 `0x00416FF1` 早于 EDI 保存；四处全局写及两个 POP、RET 各能留下不同前缀。

若某 caller 当前还有 N 个 4-byte 临时参（本组 N=4/10/14），矩形 callee 入站 ESP 为 `sub_479850` 入口 `−0x28−4N`，
其保存 ESI/EDI 后为 `−0x30−4N`；本模板只给 callee 通用指令与栈数量，不代替21处 caller 具体参数字节/FLAGS/重读及共享全局别名审计。

五处末尾全屏矩形的 LST 栈写顺序均为先压 bottom=480、right=640，随后四处 `0x00479E98/0x0047A186/0x0047A91B/0x0047B2D6` 是连续两条 `push ebx`，
而 `0x0047AF12` 是两条 `push 0`；本节此前将其几何简记 `(0,0,640,480)` 的四个 EBX 点，
以各前置正常 callee 都将 EBX 保持为0为条件，**不能在中途停止或未经逐层审计的回调上假定其原始实参是立即数0**。21行现均由 `build/workpack316/rectangle-call-args.tsv` 登记**四条原始压栈的独立指令地址及操作数**（bottom/right/top/left 的实际压栈顺序），
与 LST 共21×4条 PUSH 对齐；从首 PUSH 到相应 CALL 之间无别的 CALL，所解析的本函数直接局部跳转未进入该四压栈段的中间。

寄存器作实参时取的是各自 PUSH 时的值，不能统一替换为 CALL 时值：84条 PUSH 中72条压寄存器、12条压立即数；72条寄存器 PUSH 中有30条在它本身与 CALL 之间对同一寄存器（含 AX/BX/CX/DX 子寄存器）至少直接改写一次，
逐条写 IP 已登 `rectangle-call-args.tsv` 的末列。例如 case7 首矩形 `0x0047A2D0 push ebx` 冻结 bottom，
下一条 `0x0047A2D1 xor ebx,ebx` 后重新计算 right；CALL 处 EBX 已不是 bottom。此数量仅是直接目标寄存器改写的机械筛选，
不证明其余42条均可用 CALL 时值替代；逐点数值/FLAGS/嵌套回复与故障时已有栈字节仍为 partial。现有 canonical 绘图 owner 可复用 `rendering::LegacyRasterGeometryState`：
`surface.width/height` 对应 callee 实时上界，`clip_left/clip_height/clip_top/clip_width` 分别承接四项全局。

不过当前 `rendering::set_legacy_clip_rectangle()` 是不带逐访问 fault gate 的便利 API，
实际成员赋值序为 **left、top、width、height**，与原 callee **left、height、（`pop edi`）、top、width** 不同。

WP316 必须按四个 LST 写点逐项提交至 raster owner；例如第二写 `0x0041702F` 停止时只更新 left，
第三写 `0x00417035` 停止时 left 与 height 已改、EDI 已恢复，但 top 与 width 仍旧。不能用现有便利 API 的完整更新或无条件重排，
亦不能把当下 raster 的 surface 上界无理由固定为 640/480。

`sub_4170E0` 的 28 个物理绘制 CALL 有 **29 条**六参入站压栈链，而非28条：`0x00479E7C` 同一指令既有 case3 顺序入口 `0x00479E1B/48/62/63/7A/7B` 六次 PUSH，
又有 case4 在 `0x0047A07E jmp 0x00479E7C` 前的 `0x0047A01D/4A/57/65/7A/7D` 六次 PUSH；
必须按同一 CALL ordinal 合并两个前驱，不能统计第29个 CALL。其余27处各一条。完整29行在忽略目录 `build/workpack316/drawing-call-args.tsv`，
每行按实际 PUSH 顺序登记 arg14尾参、arg10 flags、argC高、arg8宽、arg4 y、arg0 x；合计29×6=174条物理 PUSH，
166条压寄存器、8条压立即数0。逐条静态筛选发现其中85条寄存器 PUSH 后到该 CALL（case4 到该 JMP）之前同寄存器又被直接改写，
故不能以 CALL 时寄存器快照回填旧栈字节。29段内部无其他 CALL/ESP 改动；所解析的本函数直接局部跳转只有上述 case4 分支切入 CALL 点、未从外部跳入六次 PUSH 的中间。

`call-audit.tsv` 的28条绘制行已链接原始压栈链，但入站具体字节、callee像素表及间接回调仍是 partial，
数字仅表示导航覆盖而非语义验收。入站 flags 不是不可变的按值抽象：callee 在保存 EBX/EBP/ESI/EDI 后，
`0x004170FB or [esp+10h+arg_10],0x80000000`、`0x00417128 and [esp+10h+arg_10],0x80000003` 与更深的 `0x004173EF mov [esp+10h+arg_10],eax` 均针对**同一个 caller 第二次 PUSH**形成的可写栈 dword；
从 callee 入站 ESP `C` 看地址 `C+0x14`，若 parent `sub_479850` 入站 ESP 为 `S`、当前累计 N 个 dword 参数，
则为 `S−0x14−4N`。前两处是有读与写的内存 RMW；随后 `0x004173A8` 先从**经过这两处可能改写的**栈槽重读 EBX，
再由 `0x004173EF` 将该 EBX 的 bit31 抽出覆写同一槽。三条写指令各是独立的可障停点，成功时须保留此前已提交的物理栈字节，
不能把访问合并为一次纯 flags 运算；`0x0041748A` 又经运行时像素表项间接调用。现有29链仅证明原始 PUSH 字节来源，
不覆盖这三次 callee 原位修改、别的内存访问、像素回调及 caller 之后的栈重读。

98 个 CALL 分布：`sub_4170E0:28`、`sub_416FF0:21`、`sub_485610:19`、间接 `GetSystemMetrics:6`、`sub_478850:4`、`sub_4019A0:4`、`sub_47CE70:4`、`sub_433F30:3`、`sub_4885A0:2`；
`sub_47E950/sub_4321E0/sub_4315D0/sub_423020/sub_422C70/sub_434790/sub_4344E0` 各1。

后续必须保留完整 98 次潜在调用点的物理地址、参数、回复、CALL ordinal 和停止时前缀，而非只计导航 TSV 的 92 个直接 CALL。
下列仅是 `calls.txt` 按 LST CALL 助记符机械分组的**全地址导航索引**（16类、合计98；`edi` 为六次 Win32 间接调用；
六位地址补 `0x00` 前缀），尚不是各点 ABI/异常已验收声明：

```text
sub_4019A0 (4)  479B46 47A63E 47B56B 47B902
sub_416FF0 (21) 479D39 479E07 479E98 479F37 47A009 47A186 47A2F0 47A3CB 47A49B 47A572 47A91B 47AC2F 47ACE6 47ADB8 47AE7E 47AF12 47AFBE 47B083 47B174 47B246 47B2D6
sub_4170E0 (28) 479AEC 479DAE 479E7C 479FAA 47A0FB 47A175 47A23A 47A368 47A441 47A511 47A5EA 47A7F0 47A88D 47A90A 47A9E3 47AA62 47ACA3 47AD5C 47AE2E 47AEF4 47B01B 47B0D9 47B1E7 47B2BB 47B374 47B3F0 47B4C0 47B7E8
sub_422C70 (1) 47AB79   sub_423020 (1) 47AB43
sub_4315D0 (1) 479940   sub_4321E0 (1) 479921
sub_433F30 (3) 479C40 47A731 47B66E
sub_4344E0 (1) 47B97F   sub_434790 (1) 47B6C7
sub_478850 (4) 4798F7 47B723 47B81A 47B9A7
sub_47CE70 (4) 479C1C 47A70E 47B634 47B8D0
sub_47E950 (1) 479911
sub_485610 (19) 4799FA 479C56 479CC5 479EC8 47A0AF 47A1BE 47A285 47A73E 47A771 47A841 47A96B 47AA9A 47ABCC 47AF42 47B306 47B445 47B67C 47B775 47B924
sub_4885A0 (2) 479C94 47B6F9
GetSystemMetrics via edi (6) 479C34 479C38 47A725 47A729 47B662 47B666
```

再次以 `swd3.exe.lst` 主体范围内的机器码 `call` 助记符独立枚举，得到 98 个不同地址；上表集合也为98个，双向差集均空，16类计数一致。这只证明 CALL 地址索引无遗漏，不证明每次 CALL 的实参、callee 内层副作用或故障现场正确。

`sub_478850` 既有证据登记的四次延期：`0x004798F7`、`0x0047B723`、`0x0047B81A`、`0x0047B9A7`。
最高 actor 访问为 `0x0047994B` 读取 `+0x2B20` 完整 dword，canonical byte image 至少需 `0x2B24` 字节，
高于 Workpack 315 的 `0x2B1C`。`+0x2548` 帧 token、`+0x2954/+0x2956/+0x2958` 独立 phase、`+0x0E14..+0x0E6B` 粒子 owner/参数/清零区、`+0x0DB8..+0x0DF0` 第二效果组均可能别名既有对象，
需由单一 canonical backing 负责。

## 6. 现有 owner 对齐风险（设计前必须解决）

- `LegacyBattleActorProgressState::frame_started` 已是 actor `+0x2B20` 的 canonical owner；
  当前 Workpack 316 未提交 WIP 已把 `kLegacyBattleActorImageSize` 从 `0x2B1C` 扩到 `0x2B24`，
  并使 `materialize_legacy_battle_actor_image()` 与逐次写回共用既有进度 owner 的 `+0x2B20 frame_started`、动作执行 owner 的 `+0x2B1C early_latch`；
  另从既有 base-initialization owner 物化并写回 `+0x2584 linked_action_head_token`。

  对应单测覆盖 Group-A/B 两组独立 owner 的值与三次分离提交；2026-09-23 UTC 执行 `./build.sh core --test`，
  构建和 CTest `199/199` 通过。这仅验证既有 actor image 借用接线，不是 Workpack 316 的独立分支测试、Linux app/sanitizer 或最终门禁。

  这只是现有 actor image 的必要借用接线，不等于 `sub_479850` 或子调用链已实现、review 已通过。
- `+0x2B20` 的上下游别名还必须贯通，而非只把 image 长度改大：LST 全 `.text` 静态共有7处直接 `[actor+2B20h]` 指令（`0x0046E57D/0x0046E599/0x0047561F/0x0047563F/0x0047994B/0x0047D629/0x0048400B`）。
  其中 `0x0047D629` 属于 LST `sub_47D350 (0x0047D350..0x0047D632)` 的重置写，
  不是单独的 `sub_47D580`（`0x0047D580` 仅为函数内部指令）；inventory 中这两个上游写者分别仍在 Workpack 337 / 418 待审，
  不得将其后续 closure 偷换为当前 WP316 已验收。现行 `LegacyBattleActorProgressState::frame_started` 是 Group-A `startup.party[index].progress` 与 Group-B `startup.enemies[index].progress` 的单一 typed owner；
  `src/battle/legacy_battle_actor_progress.cpp` 的两条完成路径读此值、等于1时清零，`legacy_battle_group_a_frame.cpp` 亦直接读/写它。

  但 LST 的 `sub_483FF0` 在 `0x00483FF0..0x00484017` 先读 `+0x2AA0`，仅当其==1才再读 `+0x2AB8`；
  两者都==1时直接 `retn 4` 不读 stack arg，否则读取参数并**先写 `+0x2B20`、后写 `+0x2B08`**。

  当前 `legacy_battle_group_a_frame.cpp` 和 `legacy_battle_group_b_frame.cpp` 仍以 opaque port CALL 调该地址，
  若回调只给回复而未同步同一 `progress.frame_started`，WP316 的 `0x0047994B` 会读到旧值；
  后续集成需给这两个 caller/port 明确写回责任，尤其在第二个写障时保留第一个字段已提交。此 leaf 的 `+0x2B08` 还有当前 C++ 重复 owner：
  `progress.post_action_value` 与 `action_execution.special_draw_mirror_mode` 都标作同一物理 dword，
  现有 image 仅从前者物化、`synchronize_actor_write` 将写入同步给两者；opaque port 写回若只改其中之一，
  其他已关闭消费路径将读旧值；当前 `legacy_battle_group_a_frame.cpp` 与 `legacy_battle_actor_progress.cpp` 还直接修改 `progress.post_action_value`，
  而现有 `synchronize_actor_write()` 只在 image 写回时同步另一 alias，因此再次物化时必须始终以 canonical progress owner 的最新值为准，
  不能反取可能陈旧的 action-execution 镜像。两次写必须依物理顺序传播同一值，而不能一次性同步到失败点之后。本次未提交 WIP 已将 `+0x2B1C/+0x2B20` 纳入 materialize/synchronize 双向映射；

  但 `sub_483FF0` 两处 opaque 调用的跨函数写回责任和第二写障前缀未接入，因此不能把 image 扩容误报为完整别名闭环或 Workpack 316 验收。
- 现有 `LegacyBattleActionDispatchState::group_a_target_phases[actor]` 中 `LegacyBattleTargetPhaseState::emitter` 表示同一 Group-A actor `+0x0E14`；
  可从这个 owner 借用，不另建粒子记录。Group-B 现有 `group_b_target_phases[source][target].emitter` 对应 `0x00484020` 的 **`source+0x0E6C+target*0x58`**，
  并非 `source+0x0E14`：原 LST `0x00484060..0x00484086` 先计算 `ebx+target*0x58` 再定位/清零/写入 `+0x0E6C`。

  不同 target 的记录因此确实不同，不能错误合并。Workpack 316 的当前函数读取 Group-B source actor **固定** `+0x0E14`，
  与二维 phase 无关；现于 `LegacyBattleActionDispatchState::group_b_fixed_particle_phases[source]` 建立每个Group-B actor唯一的固定 owner，独占堆数组以避免增大栈对象；resolver 分别映射 token 与 phase，而非借用二维target网格。

  已有两个 target 设置不同 flags 的测试验证的是 `+0x0E6C+target*0x58`，不能当作两份 `+0x0E14` 的依据。
- 当前 `include/openswd3/battle/legacy_battle_action_dispatch.hpp` 的 `LegacyBattleTargetPhaseState::emitter` 注释称 `actor+0x0E14 physical owner`，
  此注释仅对 Group-A 直接 actor phase 成立；Group-B 的同类型二维 `group_b_target_phases[source][target]` 有已验的 `source+0x0E6C+target*0x58` 布局，
  不能因为 C++ 类型和注释相同而复用为 Group-B 固定 `+0x0E14`。本包已新增Group-B source actor固定相位owner并回收resolver；`materialize_legacy_battle_actor_image()`和逐dword同步已覆盖该固定记录、相邻32字节中间段与方向记录的保留字节。Group-B真实caller接线及其它读取链仍待最终双向REVIEW，不以此局部别名测试宣称粒子路径完成。

- 物理记录边界核定：case51 的 `actor+0x0DB8..+0x0DF3` 恰为60 byte，其中 `0x0047B8EF` 对 `+0x0DF0` 写完整 dword 0x5A，
  已关闭的 `sub_4344E0` 在源记录相对 `+0x38` 读到该末 dword；随后 `actor+0x0DF4..+0x0E13` 恰是当前 Group-A `LegacyBattleTargetPhaseState::block_0df4[8]` 的32 byte，
  再后 `actor+0x0E14..+0x0E6B` 是固定粒子记录88 byte，Group-B 的二维 target0 记录才从 `source+0x0E6C` 开始。

  这四段**静态布局**相邻但无物理重叠；不能据此认定 REP 的写入也互不跨区。本函数未执行 `cld/std`：若 DF=1，从 `actor+0x0E14` 起始清22 dword 实际按 `+0x0E14,+0x0E10,...,+0x0DC0` 写，
  物理写集合是 `+0x0DC0..+0x0E17`，具体分割为 directional source 的后52 byte `+0x0DC0..+0x0DF3`、中间32 byte `+0x0DF4..+0x0E13` 与固定 emitter 的首 dword `+0x0E14..+0x0E17`；
  directional source 前8 byte `+0x0DB8..+0x0DBF` 不在写区，结束 EDI 为 `actor+0x0DBC`；
  DF=0 才是固定 emitter 的完整 `+0x0E14..+0x0E6B`。同理从 `actor+0x3D0` 起清38 dword，
  在 DF=1 时写 `[+0x33C,+0x3D4)` 而非 DF=0 的 `[+0x3D0,+0x468)`，最终 EDI=`actor+0x338`。

  此逆向38项的首 dword 落在 `reserved_action_record_02` 的 `+0x3D0`，其余37项落在 `primary_action_record` 的 `+0x33C..+0x3CF`，
  但不碰该主记录首 dword `+0x338`；该记录区还有当前 `LegacyBattleGroupAActionExecutionState`（Group-B lifecycle 亦复用此类型）
  分别保存的 `primary_value(+0x35C)`、`secondary_value(+0x360)`、`action_flags(+0x392)`、`record_mode_flags(+0x393)`、`secondary_auxiliary_word(+0x3AE)`、`auxiliary_word(+0x3B0)` 与七个
  `color_values(+0x3B2..+0x3BF)` 别名，
  故只通知原本 DF=0 对应的 slot2 owner 仍不够。当前 `legacy_battle_actor_runtime_reset.cpp` 的 `materialize_actor()` 以完整 `LegacyBattleActorActionRecordSlots` 作为该区 image 来源；
  `synchronize_actor_write()` 已新增对 `+0x35C/+0x360/+0x392/+0x393/+0x3AE/+0x3B0/+0x3B2..+0x3BF` 并存 scalar/array alias 的逐次字节重映射。

  Group-A/B 的逆向38项逐写模拟与第29/30写边界测试已加入，受管`./build.sh core --test`与CTest`199/199`于2026-09-23 UTC通过（`proc_fb38`，
  退出码0）；这不替代尚未实现的实际 REP、故障时 EIP/ECX/EDI/FLAGS 及其它直接 scalar 写者的前缀验收。

  此前既有代码闭合不等于这些并存镜像在本函数 DF=1 写后自动保持一致；必须以实际 LST 写者和后续消费者确定单一物理值，并在每个成功 dword 写之后使需要的 typed 视图同值，
  不能只在 REP 正常结束才批量同步。按 DF=1 自 `+0x3D0` 的零基索引，`+0x390` dword 是第17次、`+0x360` 是第29次、`+0x35C` 是第30次写；
  若第30次自身故障，`secondary_value` 已清而 `primary_value` 尚未清，ECX仍9、EDI指向 `+0x35C`。

  这些是 LST/物理布局推导的测试向量，不是运行验证结果。每个 REP 停在第 k 次访问（已有 k 次成功）时 ECX=`n−k`、EDI=`起点±4k`、EIP 仍为该 REP、已提交的 k 次物理写不得回滚；
  写入本身不改变 XOR 留下的 FLAGS。同一 REP 站点的 case2/8 终态与 case100 尾均须按 DF 将写入同步至真正物理 owner。

  `LegacyBattleDirectionalScanSource` 只是callee typed参数，并非 `actor+0x0DB8` 的长期owner；本包在两组 actor 的canonical `action_execution` 中映射case51源记录，并补齐 `+0x0DD0..+0x0DD7` 两dword和 `+0x0DE2` 高word，以容纳DF=1时不由case51正常路径读取的物理字节；
  不得把case51源记录塞进固定粒子 emitter 或Group-B的target0二维槽。Group-A固定emitter已由Workpack 204/205（`sub_4710D0/sub_471270`）维护，
  必须复用；但当前 `LegacyBattleTargetPhaseState` 将首 dword token 与 `LegacyBattleImageParticleEmitter` 拆开，
  后者含现代 `std::span<u16>` 等字段，并非可以直接按原版88 byte `memcpy` 的 packed actor 结构。

  WP316 的每次物理读写须按偏移投影到同一 phase owner，保留各 dword fault 前缀与后续 typed 消费者；
  不能用一次 `emitter={}` 隐藏DF=1第1次清 `+0x0E14`、其后清相邻中间/方向记录的独立访问。Group-B固定emitter与 `+0x0DF4..+0x0E13` 32字节中间段现共用其独立phase owner，绝不复用 `sub_484020` 的 source×target网格；
  `proc_4385/proc_94f8/proc_e6b4` Linux core/CTest均 `199/199`，分别验证堆owner与同步、逆向第1..9项前缀/第10项停点，以及完整22项跨三owner清零而源header前8字节不变；`proc_7b48` 还对case8独立 `0x0047A628` 站点验证DF=1完整22项跨owner写与源header保护；局部core/CTest同为`199/199`。`proc_bf80` 对上述新增owner执行局部ASan core/CTest `199/199`，`proc_76c4`局部Linux app/CTest `205/205`（均早于`proc_7b48`新增case8断言，非最终门禁）。此前 `proc_46d9/proc_662b/proc_733f/proc_9d32` 暴露旧测试无owner假设、栈尺寸、REP门范围与case8旧预期，修正并复测后不视为最终门禁。

- 四处 `sub_478850` 的父子 owner 合成不能只传递寄存器：现有 typed `reset_legacy_battle_actor_runtime()` 在入口从 canonical owners 通过 `materialize_actor()` 重建临时 image，
  并对每个成功写执行 `synchronize_actor_write()`。本函数的先行38/22项 REP、phase/`+0x2A12` 等写若仅落入一份尚未同步的父级局部 image，
  嵌套 reset 会读到旧 owner、丢失已经提交的父写；reset 正常或中途停止后，父级也不能继续读自己的旧局部 image 而覆盖 leaf 的写与故障前缀。

  typed 设计须在同一 actor 物理 owner 上按每次成功访问顺序同步父、子，尤其 DF=1 跨到相邻记录/中间块时，不得只在整个 REP 或 CALL 结束后批量提交。
  此条是接口前置不变量，尚非已实现的 caller 接入或测试结果。尤其到站 DF=1 时，父函数从 `+0x3D0` 清38项已写 `[+0x33C,+0x3D4)`；
  随后 `sub_478850` 自己的 `0x004789C5` 从 `+0x338` 清38项写 `[+0x2A4,+0x33C)`，
  紧接 `0x004789D2` 又从 `+0x3D0` 重复写 **与父级相同的** `[+0x33C,+0x3D4)`，再在 `0x004789DF` 从 `+0x468` 写 `[+0x3D4,+0x46C)`。

  即使数据均为零，父子两次同址物理访问可在不同 EIP/ESP/ECX/EDI 停止，不能合并或因父级已经清零跳过 leaf；现有 WP305 文档关于 leaf 七段 REP 的历史通过也不自动证明本次跨 CALL 别名组合正确。
- `sub_47CE70` 在四个调用点读取的同一 `+0x2694` 已归 `LegacyBattleGroupAActionExecutionState::presentation_render_flags`；
  应在 CALL 处读取，而不是复制入口 bit0 snapshot。已关闭 `sub_433F30` 的组合边界返回原宿主高度并保留宽高预发布/分配失败后矩形调用；
  已关闭 `sub_4344E0` 接收共享 surface 及 actor `+0x0DB8` 记录。这两处只能在保持当前调用前缀时直接复用 typed owner。

- 无显式 caller 快照的主分派 case7 在 child 前 `require_group_b()` 仍令 `index>=8` typed-stop；
  对手分派 case7 在无快照时的 `validate_group_a()` **允许 index<10，只拒绝 index>=10**（现行 `src/battle/legacy_battle_opponent_action_dispatch.cpp:105..112`）；
  两处有显式快照时已绕过该现代数组门，按低32位算 token、执行 CALL 并在无物理 owner 时停在 child 首读；
  此前把 A index8 当成现代前置 typed-stop 的结论错误，已撤回，A index8 本身是现代合法输入。Group-B 步进在算 token 前显式检查 `index==0xFFFFFFFF`；
  其余三处进入相应分支后不作 actor 容量检查，四处原 CALL 都**没有**现代这两个数组容量门。真正的边界例分别是 B index8：
  token=`0x0053AE48`、child 首读 dword=`0x0053D904..07`，LST `.data` 四条 `db ?` 均覆盖；
  以及 A index10：token=`0x005201D8`，恰为 `.data:005201D8 dword_5201D8[]` 首址、child 首读 dword=`0x00522C94..97` 也有四条 `.data db ?`。

  原 parent 都会先以低32位算 token 再发 CALL；若物理 dword 可读且为0，child 在 `0x0047985B` 读到零并正常走 default 返回 EAX0，
  主/对手 parent 才执行其各自正常非1后缀；无快照现代路径仍分别在 B8/A10 的 CALL 前 typed-stop，显式快照路径不再提前跳过 CALL。

  A index8 token=`0x0051A370` 与首读=`0x0051CE2C` 也落在 `.data:0050E6A0 char[72472]` 内，
  但仅证明物理映像交叉，**不**构成 `validate_group_a` 的索引8反例。LST 只证明这些静态映像地址，不证明字节当前值/页保护/宿主可读取或所有异常路径；
  仍须由 canonical 物理访问模型或原版 oracle 核定，不能把现代数组边界当作原版 child 入口故障。显式快照仅纠正 B8/A10 的前置停点顺序，
  无快照路径及越界内存的实际值仍不等价或未覆盖，不能声称四处 caller 完整等价；parent stop 前缀须保持原 child CALL 是否实际发出的区别。

- 最终角色步进的**child 正常 EAX1 之后**还有两个不同的现代容量/owner 早停边界，不能只回收 CALL 本身。
  组A `src/battle/legacy_battle_final_actor_step.cpp:160..167` 在 child回复1后先按 `group_a_completion_flags.size()==10` 拒绝 index>=10；
  原 LST `0x0045AA41 mov eax,dword_5054D0[esi]` 才读取组A actor `+0x2B00`（`0x005054D0=0x005029D0+0x2B00`），
  例如 index10 的真实读点=`0x00522CD8..DB` 在 LST `.data db ?` 内。因此若该地址映射可读，
  现代在原物理读前的 typed-stop 不可直接声明等价，且原 CALL 前 `0x0045AA2F` 已改父栈 arg_4。组B `legacy_battle_final_actor_step.cpp:356..375` 在 child回复1后，
  若 startup/lifecycle 未提供或 index不小于其 typed size，令 `coordinate_actor=nullptr` 并在 `read_legacy_battle_group_b_coordinate_offsets` 返回 typed-stop；
  原 LST 则先在 `0x0045ACCD..0x0045ACDF` 形成两个输出栈指针、压参、清原父栈两槽，再于 `0x0045ACE3` 发 `sub_475870` 才遇到它的物理记录读取。

  此前组B index8（或 owner缺失）若 child 正常返回1，现代早停会漏父栈写；现显式绑定在调用坐标子函数前依次处理两次指针 PUSH 和两个父栈 dword 清零，
  四处写障保留已提交的前缀，故**该特定前缀**不再提前跳过；但越界 child 返回条件、原页面状态与后续 CALLEE 内部故障仍须独立物理 backing 核定，
  不把 LST 静态数据声明当作原版动态差分。

- 四个真实 parent 分支现有**有条件 typed 接线 WIP**：主分派 case7 `0x004554F6`、对手分派 case7 `0x0045650F`，
  最终步进 A/B `0x0045AA33/0x0045ACBF`（由 Group-A/B frame context 转交绑定）。仅在调用者提供显式 parent 寄存器/ESP 快照时，
  `prepare_legacy_battle_actor_frame_caller()` 模拟 CALL 入栈并按原入站指令生成 child 寄存器/FLAGS，
  `advance_legacy_battle_actor_frame_caller()` 用 canonical owner 进入 typed child；仅 child 物理 RET 的 EIP/ESP 与对应父 CALL 后继/入站 ESP 同值才准许 parent
  消费**完整 EAX**。栈写障、缺 owner、缺端口及 child 故障统一阻止 parent 成功后缀；最终 B 的 `0xFFFFFFFF` sentinel 跳过 CALL。

  最终 A 显式绑定还要求父 ESP+0x18 对应的物理 `arg_4` dword owner：`0x0045AA2F` 写 actor token 在 CALL 入栈**之前**提交，
  后续 CALL 栈写障与 child 首读障不得回滚；缺 owner、token 失配或不可写时在 `0x0045AA2F` 停止，不进入 CALL。

  `proc_2c82/proc_d2fd` 局部 core/CTest 均 `199/199`，核对有效 owner 的正常与 CALL 栈写障提交，
  以及缺 owner、token 错位、写权限关闭时的前置停点且错误 owner 不被改写；
  其它父栈槽、真实页面映射及父子栈与 actor 的物理别名仍待验证。

  无快照时依然走旧 opaque `port.invoke(0x00479850)`：这是既有父函数 mock 兼容路径，**不是生产默认接线或关闭证据**；
  caller 物理栈/SEH backing、owner alias 与旧端口移除都未完成。显式绑定的正常 EAX0 路线、主/对手两处 parent 的 CALL 栈写障、
  最终 A/B parent 的栈写障、四处 adapter 物理返回及 B sentinel 已在 2026-09-25 的局部 Linux core/CTest `199/199`
  (`proc_9e6a`) 通过；再以 `+0x2AA0=1` 选择真实入口重置支，四处有效索引的 EAX1 物理 RET 分别接续主/对手目标移除、最终 A 清理与最终 B 坐标/结束消息，在 `proc_e566` 局部 core/CTest `199/199` 通过。

  对上述首版 caller 接线（早于新增最终A/B父栈前缀）另有局部 ASan core/CTest `199/199` (`proc_3244`) 与 Linux app/CTest `205/205` (`proc_d08b`)；
  阶段TG取得平台 API `ok=true/message_id=4282` 回执，无法证明用户客户端显示。

  Group-A/B frame 在显式绑定最终步进时向上传播真实 CALL 栈写障并禁止最终角色成功槽位后缀，
  `proc_8581` 局部 core/CTest `199/199`，随后 ASan core/CTest `199/199` (`proc_150f`) 与 Linux app/CTest `205/205` (`proc_4494`) 通过；
  缺绑定的既有 frame 测试依然走旧 opaque 端口。第二次阶段TG取得平台 API `ok=true/message_id=4285` 回执，
  仍不能证明用户客户端已显示。

  这些是四个特定正常返回向量；全部 EAX1 父后缀分支、深层 callee、物理栈别名与异常提交前缀**尚未验收**。
  旧 `LegacyBattleActionCallReply` 只有正常回复字段，缺 child stop；在未绑定分支只修改 mock 值或把 child fault 转成 EAX0 会使 parent 执行 `0x004554FB/0x00456514/0x0045AA38/0x0045ACC4`，与原异常不等价。

  最终B显式绑定在 EAX1 后已按 `0x0045ACD7/0x0045ACD8/0x0045ACDB/0x0045ACDF` 顺序处理两次指针PUSH、`arg_0` 清零和 `arg_4` 清零，
  各自写障公布 EIP/ESP/目标token；第二次清零故障保留第一次已提交的零，正常 `sub_475870` 回复才将两个输出低word写回对应父栈槽。
  原诊断输出指针已删除；这些父栈/坐标子调用停点现在由正式
  `LegacyBattleActionDispatchResult::final_group_b_physical_stop` 返回，
  caller 同时正式保留子 RET 的 ECX，并将其传入四处父栈写障快照。
  四处入站 ECX 原已从 actor token 得到（入口代码无需另设字段）；
  对不存在的 `entry_ecx` 字段尝试引发 `proc_edf4` 编译失败，
  撤回多余字段后 `proc_ce9d`、`proc_a72e` 定向 core
  1/1，`proc_a981` 定向 ASan 1/1 通过。
  这是显式测试快照的本地返回合同，不是四处真实生产入口完成，
  也不能替代整包完整门禁。

  `proc_5649` 局部 core/CTest `199/199` 覆盖四个故障停点与双零返回；
  `proc_151b` 同级通过非零坐标 5/6 的低word回写和父累加，`proc_8dbe` 同级核对资源读障保留两槽零、
  第一输出障仍为双零、第二输出障只将首槽改5而父累加不执行。

  三种子调用停点另由 `proc_7b3b` 局部 core/CTest `199/199` 核对：资源+0x62读障 `0x00475873`、
  首输出写障 `0x0047587B`、次输出写障 `0x0047588C` 的 EIP/ESP/token、EAX/ECX/EDX 与父 `XOR EBX,EBX` 遗留 FLAGS；
  当前父栈前缀及子函数停点版本的局部 ASan core/CTest `199/199` (`proc_4708`)、
  Linux app/CTest `205/205` (`proc_1b50`) 通过；这仍非整包最终门禁。

  callee首个actor+0x0C读障、资源+0x8A读障、RET读障、输出指针可能与actor/资源/父快照别名、
  源像素物理页与其它父栈写均未验，故不把该前缀当成完整最终B后缀收口。四处最早统一停点为 child `0x0047985B cmp [esi+0x2ABC],ebx` 的真实内存读：
  在该指令之前，child 已 `sub esp,0x14` 且先后 push EBX/EBP/ESI/EDI，`mov esi,ecx`、`xor ebx,ebx`；
  相对每个 parent 的 CALL **前** ESP，硬件 CALL return address 占4、child 本地占0x14、保存四寄存器占0x10，
  故故障 ESP=parent-call-ESP−0x28，ESI=actor token、EBX=0、EAX/EBP保留 parent CALL 入站值，
  算术 FLAGS 来自 XOR（DF 未改），而不是 parent 后续 CMP 的 FLAGS。四处显式快照缺 actor owner 的首读障已核对 EIP/ESP、四次完成的栈 PUSH、ESI/EBX 与 ZF，
  `proc_7dab` 局部 core/CTest `199/199`；主分派在缺 reset port 的 `0x004798F7` 还核对写回
  `+0x2AAC/+0x2AB8/+0x2A12` 后父后缀不运行，`proc_9c9a` 局部 core/CTest `199/199`。

  两条均不证明真实父栈内容、深层别名或后续 callee 故障。此数值为首次读障的专用场景，不可套到后续深层 callee fault。

- case51 `0x0047B97F` 后 `sub_4344E0` 的两参 `retn 8` 与父 `0x0047B984..0x0047B98D` 四次 POP/XOR/ADD20/RET 现有条件化接线：仅当 decoder 回复提供与 actor `+0x0DB8` token 匹配的真实源像素 span、共享surface token及方向向量/目标surface/共享状态/像素转换 owner 均明确时，直接调用已关闭的方向扫描 typed 实现；正常返回后才执行物理父尾，EAX置0。无 backing 仍停在真实子入口，不伪装正常返回；callee 发生内部 typed failure 时其已提交目标像素/共享写保持：对于水平/垂直除零依LST分别核到 `0x0043453C/0x00434566` 与相应入站ESP；对于源像素短读，依原 LST 镜像/非镜像路径分别核到 `0x00434651/0x004346B7` 并保留源token+字节偏移及该站 ESP；行表读障进一步按混色/直写分别核到 `0x004346E6/0x0043470E`，仅混色路径在读前已PUSH1、故ESP另减4；直写像素写障核到 `0x00434717`。这三站通过 `proc_914e` core/CTest `199/199` 覆盖case51子调用的EIP/ESP与独立扫描测试；混色目标像素读障另核到内层 `sub_4207E0+0x7A = 0x0042085A`，在前序PUSH三参/CALL及其四次保存寄存器后ESP为scan入站减`0x98`，同时三项mask低16位屏蔽已提交；`proc_15be` core/CTest `199/199`、`proc_c847` ASan core/CTest `199/199` 验证。行表及像素surface新增独立可选物理基址token：仅显式提供时按物理偏移公布读/写障token，未提供仍保持未知而非伪造；`proc_13b1` core/CTest `199/199`核对直写/混色的row与pixel地址。生产调用方的真实token resolver尚未接入；其它混色子callee失败的EIP与多项寄存器仍未知，不冒充子入口异常或正常返回。`proc_636d/proc_a645/proc_8f5a/proc_a01a` Linux core/CTest `199/199` 分别验证真实像素写/物理RET、独立POP/RET读障与源token失配、镜像/非镜像短源读点，以及水平/垂直IDIV零除的各自 EIP/ESP；`proc_6570`在方向扫描callee的独立单元验证同一两处读点，`proc_8d4e`对当前case51字节span接线执行本轮ASan core/CTest `199/199`，另`proc_045c` 本轮Linux app/CTest `205/205`；这些均非316最终发布门禁；之前 `proc_696e/proc_3584/proc_838a` 因测试以 synthetic `entry_esp` 推断返回ESP及两参 RET8 栈算术误计失败，改以CALL前 `case_fifty_one_tail_ready.esp + 48` 核对后通过。此为局部路径；最终 caller owner 接线、callee 深层停点和双向 REVIEW 尚未完成。

## 7. 独立向量与当前未决

独立向量详列于 `build/workpack316/branch-vectors.md`；覆盖入口/重置/更新双方、selector 0/1..15/16/49/50/51/52/99/100/101/255、各 case phase 零/临界值前后与负数/回绕、host 指标调用顺序、owner 分配/释放、粒子返回 0/1、十五项表、三种共享重置尾、98 个 CALL 和22个 RET故障，
以及四处 caller 精确 EAX 判断。上述是测试输入与期望推导，不是测试结果。当前工作材料 `build/workpack316/call-audit.tsv` 的98行已有正常 CALL 前临时栈深和 CALL 后继 EIP；
按原 LST 和六次 Win32 stdcall 正常清参假设独立运行 `verify-call-stack.mjs` 对249块、98次 CALL、22个 RET 均验证正常路径 ESP 数量守恒，
未见汇合冲突。98行现在都有入口栏（其中矩形21/绘制28/音频19只新增 LST 压栈链的原始 IP/操作数导航，而非完整时刻字节）、98行有不同程度的正常回复分类、98行有至少一个条件性故障栏（音频19行新增 wrapper 的10处通用栈访问模板，
但 AIL/更深嵌套访问尚未审毕）；**98行均为 partial，零行通过完整调用点语义/异常验收**，也不据此推定 Win32 IAT 内部指令。

`unanchored-basic-blocks.txt` 的20个块首地址仅表示源码未直接写出该地址文本，不等于不可达、未实现或已验收。
本轮从原LST逐块分类：selector分派2处 `0x004799BB/0x004799CD`；十处零phase音频比较门
`0x00479CB7/0x00479EBB/0x0047A0A1/0x0047A277/0x0047A833/0x0047AA8C/0x0047ABBE/0x0047AF35/0x0047B419/0x0047B84E`；
两处首绘后高度截断 `0x0047A120/0x0047A8B2`；两处signed phase绘图参数写
`0x0047A998/0x0047B329`；case14的加4写 `0x0047B110`；case100的负数余数修正与
signed范围门 `0x0047B4D8/0x0047B4DD/0x0047B4FD`。其中短分支可在相邻continuation找到对应运算，
但这组文本锚点检查不证明相邻CALL、故障前缀或249块双向REVIEW已完成。

已开始按块核对现有 C++：Block 011 更新 CALL 在记录 owner 缺失时原实现误把 callee 入口 `0x004321E0` 当成记录首读障；
首次修正为三次 callee PUSH 与 `XOR EBX,EBX` 后的 `0x004321EE`、`record+0x90`、ESP 再减12，
`proc_e231` 局部 core/CTest `199/199` 验证故障寄存器与不调用更新 port。

继续核对发现三次 PUSH 后、XOR 前还有 `0x004321E3 mov esi,[esp+0x10]` 栈读，
缺记录 owner 分支现分别计数并能停在三个 PUSH 写与该栈读，最后才到记录首读；
`proc_a3c3` 因测试缺枚举作用域编译失败，修正后 `proc_9a73` 局部 core/CTest `199/199`。

同版本 `proc_f193` ASan core/CTest `199/199` 与 `proc_7edb` Linux app/CTest `205/205` 亦通过；
补全四站寄存器/FLAGS断言后 `proc_9d2c` core/CTest `199/199`。

存在真实更新 owner 时，`sub_4321E0` 的
`0x004321F5/004321F9/00432203` 三处 CMP 先决定是否
直达 `0x00432A10`：若 `external_mode==1` 且非零
`command_cursor`，以16位 CMP FLAGS 返回；否则若
`action_id==0`，以32位 CMP FLAGS 返回。两条路径均
不修改记录、保留入口 ECX/EDX、将 EAX 设为1。
TSW 更新端口现直接执行此两条正常出口；
其余路径仍委托注入更新 port，未证实内部栈访问/异常回传，
不能据此关闭 Block 011。零动作、外部游标及非零动作委托向量经受管定向
`battle.actor_frame_316` 1/1（`proc_016e`）验证；
零动作父更新→真实 TSW 父查找的串联再经
同一定向目标 1/1（`proc_ea35`）验证。
父路由现只对声明快返 ABI 的端口，按 LST
先逐次压入 EBX/EBP/ESI、读取实参，读`+0x90`、
压 EDI，再按条件读`+0x42`或`+0x00`，
逐次 POP/RET 后才清父实参；每个零动作子调用
可见访问站的 EIP/ESP/访问序号与关键 GPR/FLAGS，
以及外部游标非零、零游标双读分支均经定向
1/1（`proc_ac64`）验证。这不是完整 core/ASan/app；
动作流其余六处物理返回、跨 owner 别名与原版
SEH 故障仍未闭合，Block 011 保持 partial。
2026-09-25 21:32:43+08 五段阶段 TG `proc_a19f` 获 API `ok=true/message_id=4305`；
只证明平台接受，不证明用户客户端显示。

Block 014 的 lookup CALL 回复所带显式资源头原先直到父 `0x00479945` 写成功才发布，导致该写故障时回滚已完成的callee副作用；
现先按 callee EAX token 发布资源头，再独立执行 `actor+0x2548` 写，正常写也不会因旧token判异而清除新头。

`proc_bd0f` 曾暴露测试计数和token归属级联失败，修正后 `proc_5284` 局部 core/CTest `199/199` 核对正常与父写障前缀；
同版本 ASan core/CTest `199/199` (`proc_a083`) 与 Linux app/CTest `205/205` (`proc_da02`) 通过。

Block 014 的查找callee `sub_4315D0` 首指令是 `0x004315D0 mov eax,[0x004CF840]`；
现 CALL 三次父栈写后先检查并计数该全局首读，读障停在真实 EIP、ESP=父 lookup 前缀减12，
且不调用窄port；正常路径后续访问序号相应顺延。`proc_cd97` Linux core/CTest `199/199`，
`proc_de0e` ASan core/CTest `199/199`，`proc_bb5e` Linux app/CTest `205/205`。

这只证明显式 reply 所述资源头和首读故障，不证明 sub_4315D0 内部所有缓存、分配和文件读写。
2026-09-25 15:16:37+08 阶段 TG `proc_a8dd` 获 API `ok=true/message_id=4288`；
仅证明 Telegram 平台接受，不能证明用户客户端显示。

Block 017–025 的首轮对照还确认 `0x00479965..0x004799D5` 的条件序列：
`+2B08==1` 才读 `+3E0`、在非零时读 `+2548` 与资源 `+0C`，随后读/写 `+2694`、
读actor资源指针与资源 `+20`、覆盖选择器、`DEC/CMP/JA/XOR` 和两级只读表；
当前 C++ 对这些显式条件、访问次序和直接 FLAGS 转换未见新差异，
但指针/表内存别名、完整 actor image owner 与各阻断前缀尚未逐项收敛，不能标记这些块完成。

另以 ledger 的 `MEMORY` 地址机械交叉查找 C++ 直接十六进制常量：23 个表面缺口中
17 个是并不读取内存的 `LEA`，另 6 个是 case10 共用 case5 路径按 `+0x792`
计算得出的真实读指令 `0x0047A85E/69/6D/71/79/82`；
六处 EIP 已在定向测试列出。这一静态排除不证明这条路径的共享别名或异常语义已经审核。

最终 B 父 `0x0045ACE8/0x0045ACED` 在 `sub_475870 retn 8` 后依次从两个已绑定父栈槽读取低word；
此前实现仅消费 callee 临时变量，漏两次独立读障和相同 canonical backing 被后写覆盖的可能。

现两处读障保留 callee 两个输出、父 ESP、EAX/ECX/EDX 和 XOR 的 FLAGS，
正常路径重读 owner 后才依序累加两个全局word；合成的共享宿主存储向量验证后写覆盖前写，不证明真实父栈有该别名。

`proc_9466` Linux core/CTest `199/199`、`proc_13cd` ASan core/CTest `199/199`、`proc_e673` Linux app/CTest `205/205`；
真实生产父栈/SEH backing 与更深全局别名仍未验收。

28 处绘图 CALL 共用六个 typed wrapper，`sub_4170E0` 首指令
`0x004170E0 mov eax,[0x004CD730]` 在此前只由窄port隐含，缺独立读障/序号；
现六个 wrapper 在 CALL 返回槽提交后分别检测/计数该全局首读，入口型 port 停止时不把未执行读取计入序号。

case1/100、9、11、6、5前后两次的局部故障向量均核对 EIP、ESP、token 和不调用窄port；
`proc_3b3a` 因四条原有故障序号前移失败，更新期望后
`proc_2185/proc_348b/proc_a3a7` 局部 Linux core/CTest 各 `199/199`；
同版本 `proc_3f9e` ASan core/CTest `199/199`、`proc_1c7e` Linux app/CTest `205/205`。

随后六个共享绘图 wrapper 已要求显式绑定 `0x004CD730` 指针 owner 与其所指向的源首字节 owner；
在首个全局读取后分别检测/计数 `0x004170E5/E6/E7` 三次寄存器保存、`0x004170E8`
首word读取和 `0x004170ED` 保存EDI，入口型 port 停止不计入未执行的六处访问。

case1完成源token缺失、token与字节owner错位、首word不可读和五站故障序号；
case9、3、5首/第二绘制、11 各有首word读障代表向量。

`proc_55fd` 因测试类型名编译失败、`proc_e866` 因旧访问序号断言失败，修正后
`proc_382d/proc_7573` Linux core/CTest `199/199`、`proc_a4db` ASan core/CTest
`199/199`、`proc_c250` Linux app/CTest `205/205`。源首word为 `0xFFFF` 的路径另加
`0x004170F0` 显式调色板指针 owner 首读；指针0时 `0x004170FB`
栈上 `arg_10` RMW因缺可写栈 owner 而停于该指令，未冒充正常绘制；
调色板读取障在 `0x004170F5 XOR EBP,EBP` 前，EBP必须保留入站值；
此前过早清零已修正并补故障向量。非 `0xFFFF` 的原始源路径跳过这次调色板读取。

两条路径及窄port入口型停止的可变访问序号先经 `proc_5060` core/CTest `199/199`；
本次修正及代表向量经 `proc_24ad` Linux core/CTest `199/199`、
`proc_4262` ASan core/CTest `199/199`、`proc_97fb` Linux app/CTest `205/205`。

调色板指针为零时，`0x004170FB` 的栈上 `arg_10` 仅在token匹配且有显式栈word owner时读、写；
写障不提交高位，成功则原位 OR bit31，更新FLAGS；`0x00417107` 从同一owner重新读，
经 `AND ECX,0xFFFC/CMP ECX,0x14`，分别停在未绑定的 `0x00417116` 或 `0x00417130` 全局读取，
不让窄绘图port代答。case1合成栈值0x21/0x14的读障、写障、写后重读障与两条后继全局读
按LST核对 EIP/token、先前写入和访问序号；`proc_6a3e` Linux core/CTest `199/199`、
`proc_5bc9` ASan core/CTest `199/199`、`proc_0b33` Linux app/CTest `205/205`。

非 `ECX==0x14` 分支在显式绑定源值时物理执行 `0x00417130 MOV EDI,dword_4CD75C`，
保留读取后的 EDI 与访问序号；因 `0x00417136 MOV dword_4CD744,EBP` 的目的owner尚未绑定，
停在该写入前。`ECX==0x14` 分支仍停在 `0x00417116`，不借用高度源值。

0x234合成高度及读取故障向量经 `proc_1c79` Linux core/CTest `199/199`、
`proc_e16e` ASan core/CTest `199/199`、`proc_2924` Linux app/CTest `205/205`。

测试 `request()` 的图像首字节、栈word及新增高度源值均为合成backing，不证明实际
`shared_action->turn_frame_source_token` 与绘图/像素页或生产父栈、全局高度值的别名。

`sub_4170E0` 后续全局/帧/像素页访问及故障前缀仍不闭合。
留底提交 `525a9822` 后五段阶段 TG `proc_21cd` 退出0；脚本仅在 API `ok=true` 时正常返回，
未输出 `message_id`，客户端显示未验证，留底不算316验收。

留底提交 `12380a67` 后五段阶段 TG `proc_fd11` 退出0；脚本仅在 API `ok=true` 时正常返回，
未输出 `message_id`，客户端显示未验证，留底不算316验收。

留底提交 `7b95f28b` 后五段阶段 TG `proc_142a` 退出0；所用脚本仅在 `sendMessage`
响应 `ok=true` 时正常返回，但未输出 `message_id`；客户端显示未验证，留底不算316验收。

2026-09-26 00:19:36+08 五段阶段 TG `proc_8cfb` 获 API `ok=true/message_id=4315`；
只证明平台接受，不证明用户客户端显示。

2026-09-25 22:18:00+08 五段阶段 TG `proc_546f` 获 API `ok=true/message_id=4308`；
只证明平台接受，不证明用户客户端显示。

共用 audio 子调用 `sub_485610` 入口 `0x00485610` 首先物理读取
`[ESP+8]` 第二参数，`0x0048561E` 再读 `[ESP+4]` 第一参数；
八个共享音频 wrapper 已在 CALL 返回槽提交后分别计数首读和第二读、保留
`SHL/IMUL` 的 ECX/EAX/EDX 及仅已定义的 CF/OF，入口型 port 停止不计入未执行的两次读取；
每个 wrapper 代表向量核对两处 EIP/token/访问序号及故障前不调用 port，case1 第二读另核对
ESP、乘积寄存器、CF/OF/DF。第一轮 `proc_11c7` 因 case1 旧序号断言失败，修正后
`proc_7040/proc_31c2` core/CTest `199/199`；两次读取的后续 `proc_f1d2/proc_bc89`
core/CTest `199/199`，`proc_af41` ASan core/CTest `199/199`、`proc_82f4`
Linux app/CTest `205/205`。

真实栈 backing、其余音频/CRT 内部副作用仍由窄port折叠；
八个 wrapper 的代表向量不等于十九处物理 CALL 的逐项双向REVIEW。

在两次读取之后又按 `0x00485622/28/2F/37/38/39` 六次 PUSH 顺序模拟音频callee的
`SAR/SHR/ADD/AND` 寄存器与 FLAGS；case1 六站栈写障、先前写入的 ESP/值/序号及窄port不调用通过
`proc_7645` core/CTest `199/199`。先前 `proc_b80b/proc_65d4` 因向量少算 CALL 返回槽写入而失败，
经逐站诊断修正；case50全宽声音ID到callee低16位、case100早期EB声音ID压栈另有向量。

当前 `proc_b964` core/CTest `199/199`、`proc_28b8` ASan core/CTest `199/199`、
`proc_ecb9` Linux app/CTest `205/205`。随后按 LST 把 `0x00485640` 的内部 CALL 返回槽、
`0x00485CE0..0x00485CE3` 四次寄存器保存和 `0x00485CE6` 子 CALL 返回槽纳入故障序号，
并在 `0x00485CC0` 对 `[0x004C8450+0x54]` 新增独立首读障；case1向量核对
深层 ESP、EBP、ECX 和此前提交的压栈前缀。`proc_9f58` 因测试变量重名编译失败，
修正后 `proc_32ff` core/CTest `199/199`、`proc_0d7f` ASan core/CTest `199/199`、
`proc_c0ae` Linux app/CTest `205/205`。

首读从请求中显式绑定的音频状态 owner 读取模式 dword；缺 owner 停在真实首读，
不能再凭默认可读布尔值计入成功。模式非1时按 `sub_485CC0 RET`、`sub_485CE0`
四次 POP、`retn 0x18`、`sub_485610 RET` 的物理次序直接 EAX0 早退，不调用窄port；
mode0六处栈读障和正常早退向量已通过 `proc_d969` core/CTest `199/199`、
`proc_268c` ASan core/CTest `199/199`、`proc_0b57` Linux app/CTest `205/205`。

首状态为1时，`0x00485CF5 CALL sub_485CD0` 先压返回槽，`0x00485CD0`
再从显式绑定的 `[0x004C8450+0x58]` owner 读取第二状态，`0x00485CD7` 独立RET；
第二状态非1则经 `0x00485CFA CMP AL,1` 和共享四POP/双RET早退，不调用窄port。

case1代表向量核对第二CALL栈写障、第二状态缺owner读障、内层RET读障与正常早退的
EIP/ESP/token/累计访问序号；八个共享wrapper在入口型停止时撤销本轮预读的19次访问。

初轮 `proc_0fb2` core/CTest `198/199`，新增向量少计父CALL返回槽，修正后
`proc_d18f` Linux core/CTest `199/199`、`proc_6d16` ASan core/CTest `199/199`、
`proc_3204` Linux app/CTest `205/205`。两次音频状态指针与真实全局之间的别名尚无生产证明。

两状态均为1时，`0x00485D02 MOV EDI,[ESP+0x18]` 读取 `sub_485610` 压入的低16位声音编号，
`0x00485D06 TEST EDI,EDI` 为零则按原四POP/双RET直接返回0，绝不调用音频port；
非零仍由窄port承接未展开的后续读取、缓存查找、AIL调用。八处共享wrapper入口型停止的
预读撤销由19改为20，case1补485D02栈读障；selector51源编号低16位为0时音频port调用数
由1更正为0，独立截断向量用高16位非零、低16位零验证物理早退及27次访问。

初轮 `proc_9ab5` core/CTest `198/199` 因两处selector51旧port期望失败，修正后
`proc_7e14` Linux core/CTest `199/199`、`proc_3504` ASan core/CTest `199/199`、
`proc_8d77` Linux app/CTest `205/205`。两次音频状态的生产别名、真实父栈可变别名及
非零声音编号后的更深callee/AIL仍未闭合；19处CALL仍为partial。

留底提交 `f6ffd84f` 后五段阶段 TG `proc_115f` 退出0；脚本仅在 API `ok=true` 时正常返回，
未输出 `message_id`，客户端显示未验证，留底不算316验收。

留底提交 `a0e3db5e` 后五段阶段 TG `proc_5023` 退出0；同一脚本仅在 API `ok=true`
时正常返回，未输出 `message_id`；客户端显示未验证，仍不是316验收。

2026-09-25 23:57:38+08 五段阶段 TG `proc_e4b5` 获 API `ok=true/message_id=4314`；
只证明平台接受，不证明用户客户端显示。

2026-09-25 23:23:54+08 五段阶段 TG `proc_b642` 获 API `ok=true/message_id=4312`；
只证明平台接受，不证明用户客户端显示。

2026-09-25 22:56:06+08 五段阶段 TG `proc_b63e` 获 API `ok=true/message_id=4311`；
只证明平台接受，不证明用户客户端显示。

2026-09-25 15:36:30+08 五段阶段 TG `proc_e469` 获 API `ok=true/message_id=4290`；
只证明平台接受，不证明用户客户端显示。

复查入口基本块 `0x00479850..0x00479861` 与默认出口首POP：`SUB ESP,0x14` 只改ESP，
`0x00479853/54/55/5A` 四次PUSH各应先检查栈可写；原共用push辅助遗漏权限。

`+0x2ABC==0` 时默认出口第一处 `0x0047A80B POP EDI` 应先检查栈可读，原共用pop辅助亦遗漏。
已修正，两条权限拒绝向量按LST核对 EIP/ESP/token/序号与默认CMP的ZF：
`proc_8bee` Linux core/CTest `199/199`、`proc_9e87` ASan core/CTest `199/199`、
`proc_4ec1` Linux app/CTest `205/205`。这仅闭合两处故障条件；入口各owner、REP反向别名、
`0x00479920`后两层callee与四处父caller仍待完整REVIEW，不能给001..020整体签字。

四处实际caller调用`advance_legacy_battle_actor_frame_caller`时，若显式快照未绑定绘图源token
`0x004CD730`与高度`0x004CD75C`，且resolved actor有共享action owner，则分别接入
`turn_frame_source_token`和`draw_height_third`；显式快照优先，不伪造缺失owner。

四处零门caller验证默认接线；Group-B case1激活绘图验证`0x004170E0`真实共享源token
读取后，因源字节仍未绑定，在`0x004170E8`故障前停、绘图port零调用。

本轮`proc_4821` Linux core/CTest `199/199`、`proc_6ad8` ASan core/CTest `199/199`、
`proc_0d3e` Linux app/CTest `205/205`；源字节、像素页、父栈别名的生产闭合尚待核对。

默认共享字段接线不是绘图全路径或四caller完整REVIEW。
两次留底后的五段阶段 TG `proc_3ca4`（入口栈权限）与 `proc_35eb`（共享绘图源）
均退出0；脚本未输出 `message_id`，仅证明 API 接受，不证明客户端显示或316验收。

四处`sub_4019A0`解码CALL（`0x00479B46/0x0047A63E/0x0047B902/0x0047B56B`）
在返回地址PUSH后，callee首指令`0x004019A0 MOV EAX,[ESP+4]`独立读取父栈首参数。

先前适配器直接调用解码port，未约束这次栈读；现不可读或访问序号截断时，在
`0x004019A0`以CALL返回槽已压、四实参未清的ESP/token停止且不调用port。

port仅支持入口型停止时撤销这次模型化读计数，不冒充已完成解码；后续`0x004019A4`
全局、输出参数的逐写及分配/循环故障仍未闭合。本轮`proc_c417` Linux core/CTest
`199/199`、`proc_a85a` ASan core/CTest `199/199`、`proc_045d` Linux app/CTest
`205/205`；四CALL仍为partial。该批留底后的阶段TG `proc_68a9` 退出0，
未输出 `message_id`，不能证明客户端显示。

继续对同四个解码CALL核对`0x004019A4 MOV EDX,[0x004CDE74]`：父栈首参数
`MOV EAX,[ESP+4]`已完成后，第二次物理读取若全局不可读或序号截断，停在
`0x004019A4`；EAX保留首参数、EDX与FLAGS未改、callee尚未保存EBX，解码port零调用。

入口型port停止仍回滚本阶段两次模型化读及EAX；本轮`proc_d756` Linux core/CTest
`199/199`、`proc_d36c` ASan core/CTest `199/199`、`proc_bba8` Linux app/CTest
`205/205`。全局`0x004CDE74`的真实owner和值、源header和随后三处输出栈槽写仍未接通；
“允许读”仅提供故障门，不是全局值或正常回复的独立证明。该批留底后的
五段阶段TG `proc_238e` 退出0，未输出 `message_id`，客户端显示未验证。

现在`0x004019A4`不仅检查全局可读，还要求快照显式提供`0x004CDE74`的独立owner；
即使通用全局可读，缺owner仍在该指令停，不借用绘图或actor字段伪造数据。

成功读取时模型将其完整dword写入EDX；入口型port停止时恢复原始EDX与前两次读计数。
测试快照给合成`0xFFFF`，与LST `0x004237F9`写入常量一致，但不能证明生产初始化、
可变别名或随后header比较。`proc_bbf2` Linux core/CTest `199/199`、
`proc_42db` ASan core/CTest `199/199`、`proc_4fcf` Linux app/CTest `205/205`。

该批留底后的五段阶段TG `proc_0a06` 退出0；无 `message_id`，客户端显示未验证。
继续核对解码callee：全局读后`0x004019AA XOR ECX,ECX`只改寄存器与FLAGS，
`0x004019AC PUSH EBX`是下一独立可障栈写。已在四处共享解码CALL中保留写前
EAX=首参数、EDX=格式全局、ECX=0、ZF=1，栈写失败时ESP仍指CALL返回槽，
未执行`0x004019AD`源header读取；当时port入口型停止可还原入站寄存器与栈计数；
当前显式输出栈槽写入后的port边界已改为保留前缀（见下文），不再执行这项回滚。

成功回复仅由窄port代表余下decoder与保存寄存器弹出，不宣称后续解码等价；
`proc_471b` Linux core/CTest `199/199`、`proc_6c9c` ASan core/CTest `199/199`、
`proc_1c9e` Linux app/CTest `205/205`。该批留底后的阶段TG `proc_8282`
退出0，未输出 `message_id`，客户端显示未验证。

解码callee下一站`0x004019AD MOV CX,[EAX]`要求图像源token与至少两个源字节同属
显式backing；首个EBX保存完成后，缺owner/字节不足/读障或序号截断均在该指令停止，
EIP与源token保留，ECX仍为0且后续EBP/ESI未压栈。四条测试token的合成
`0xFFFF`头只验证本次读取和正常窄port继续，不证明生产TSW缓存帧的物理token/寿命、
格式比较或真实输出；`proc_d1dc` Linux core/CTest `199/199`、
`proc_4eaa` ASan core/CTest `199/199`、`proc_d302` Linux app/CTest `205/205`；
该批已提交并推送 `dff5d250`，阶段TG `proc_c1f4` 退出0但未验证客户端显示。

解码头比较的下一段按 LST 的 `0x004019B0/B1` 保存 EBP/ESI、`0x004019B2 CMP ECX,EDX`、
`0x004019B4` 保存 EDI 后才分岔：不匹配时 `0x004019B7/B8/B9` 依次弹出
EDI/ESI/EBP，`0x004019BA XOR EAX,EAX` 清零并设置ZF，再从 `0x004019BC` 弹出 EBX、
`0x004019BD` 取返回地址，完全不调用深层解码port且三个输出指针尚未写。

八个新的独立栈停点均从本次CALL的真实ESP位置取序号；前两个保存写障保持读头后ZF，
第三保存写障及前三个POP读障保持比较FLAGS；XOR后的最后POP和RET读障则保留EAX0/ZF1。

测试用合成头 `0x1234` 对比合成全局 `0xFFFF`，只验证case2及共享callee这段前缀，
不能证明其余三caller的整个父路径或正常格式匹配后的宽高、格式、分配和循环。

`proc_8369` Linux core/CTest `199/199`、`proc_4eaf` ASan core/CTest `199/199`、
`proc_4de6` Linux app/CTest `205/205`；该批已提交并推送 `445052c4`，
阶段TG `proc_b56f` 退出0，客户端显示未验证。

匹配头后的 `0x004019BE/C2/C8` 依次从当前callee栈的
`CALL_ESP+8/+12/+16` 重读输出指针（`var_8/var_C/var_10`），先写 EDX/ESI，
`0x004019C6` 清ECX及置ZF，再写EDI，才在 `0x004019CC` 从首参数源指针
`+2` 读取宽度word。三次父栈读障和宽度读障分别保留已有寄存器/ESP/FLAGS，
源字节不足四个时停在真实宽度读取点，不调用深层port；port正常回复恢复保存的ESI/EDI。

测试四个源token的宽度值仍为合成2，父栈owner及资源字节别名未有生产证明；
`0x004019D0` 写首个输出栈槽以及后续高度、格式读取/写入仍未核对。

`proc_da5c` Linux core/CTest `199/199`、`proc_16da` ASan core/CTest `199/199`、
`proc_b09c` Linux app/CTest `205/205`；该批已提交并推送 `539e1543`，
阶段TG `proc_4ba5` 退出0，未验证客户端显示。

匹配格式头和源宽度读后，`0x004019D0 MOV [EDX],ECX` 首次写调用方
`var_8` dword，需输出token与显式可写word owner精确匹配；缺owner、错token、不可写或访问序号截断
均在该写点停止，保留宽度ECX、四项callee保存栈及先前读取顺序，不调用深层port。

测试中的 `S−8/S−0x0C/S−0x10` 栈槽是合成parent owner，不证明生产栈页、跨owner别名或输出缓冲。
写入正常完成后port只代表未审计后缀：非返回停点位于下一条 `0x004019D2`，必须保留已写父栈的宽度，
不能倒退到 `0x004019A0` 并撤销副作用；port接收该边界的EAX/ECX/EDX/FLAGS。

目前高度/格式源读与剩余两次输出写仍归未审计后缀，不能把本批解释成完整解码或生产资源接线。
本轮 `proc_e505` Linux core/CTest `199/199`、`proc_b057` ASan core/CTest `199/199`、
`proc_d71d` Linux app/CTest `205/205`；该批已提交并推送 `6d5ed1af`，
阶段TG `proc_d553` 退出0，客户端显示未验证。上一段仅记录提交当时的宽度后缀边界，
当前已继续向后审计：`0x004019D2` 清ECX、`0x004019D4` 从源`+4`读高度、
`0x004019D8` 写第二父栈槽`var_C`，`0x004019DA` 从源`+6`读格式、
`0x004019DE` 按`0x3FFF`掩码、`0x004019E4` 比较16，再在`0x004019E7` **先写**
第三父栈槽`var_10`；非16的格式另在`0x004019EB`比较8。

无效格式的`0x004019F4/F5/F6`弹栈、`0x004019F7 XOR EAX,EAX`、
`0x004019F9 POP EBX`、`0x004019FA RET`分项执行；这一路EAX0但三个输出已发布，
不能复用最早的格式头不匹配返回前缀。两个新的源读点、两次栈写和无效格式五个栈读停点
均有序号与已写owner值的局部向量。匹配16的深层port起点现为`0x004019FB`，
格式8为`0x00401ABA`；任何非返回回复均保留**三个**栈槽写入和相应ESP/FLAGS。

测试字节仍为合成头；allocator/循环/真实源页与父栈跨owner别名未证明，
不能以三个测试输出写入推断完整解码。`proc_d598` Linux core/CTest `199/199`、
`proc_13a1` ASan core/CTest `199/199`、`proc_db18` Linux app/CTest `205/205`；
该批已提交并推送 `89161ce0`，阶段TG `proc_262e` 退出0，客户端显示未验证。

下一步逐项审计分配前的输出重读：格式16先在`0x004019FB`从EDX指向的首输出槽读宽度，
`0x004019FD`把EDI换为源`+8`，`0x00401A00`再从ESI指向的第二槽读高度，
有符号`IMUL`的低32位之后在`0x00401A03`左移一位，至`0x00401A05 PUSH Size`待审边界。

格式8先在`0x00401ABA`设EDI=源`+8`，`0x00401ABD`从首输出槽重读到EAX，
`0x00401ABF`再乘第二槽高度，至`0x00401AC2 PUSH Size`待审边界；IMUL仅CF/OF有定义，
无后续SHL的8位路径不把其他算术FLAGS冒充已知。

源头写入成功不替代物理重读，两个输出槽均需原token可读owner；两种格式的四个读取停点
分别核对ESP/EAX/EDX/EDI和之前三项输出；合成宽2、高3的16位分配尺寸为12 byte，
8位分配尺寸为6 byte。测试不能证明真实栈页、分配器回包或首个图像命令读取。

`proc_82ee` Linux core/CTest `199/199`、`proc_adf7` ASan core/CTest `199/199`、
`proc_c4b9` Linux app/CTest `205/205`；该批已提交并推送 `6296c5ee`，
阶段TG `proc_c6fe` 退出0，客户端显示未验证。

分配请求前下两处可障父栈写已按 LST 分开：16位`0x00401A05 PUSH EDX(Size)` 与
`0x00401A06 CALL sub_487C10`，8位`0x00401AC2 PUSH EAX(Size)` 与
`0x00401AC3 CALL sub_487C10`。前者成功后ESP额外减4且保留尺寸，后者才
另写返回地址`0x00401A0B/0x00401AC8`并进入真实callee入口`0x00487C10`；
因此非返回窄port不得把已完成的两次栈写或父栈三项输出撤销。测试合成宽2高3对应
16位尺寸12、8位尺寸6，四个独立栈写故障序号、ESP、token、last-pushed前缀均有验证。

`sub_487C10` 自身`PUSH EBP`、保存栈基址、前三个常量PUSH、全局读取、
后两次参数PUSH、`sub_487C80`深层CALL、平栈/POP/RET以及分配后的首个图像命令读取
仍归未审后缀；
正常端口回包只代表该后缀具备条件性正常栈平衡，**不证明其分配/循环故障**。

`proc_53eb` Linux core/CTest `199/199`、`proc_36e8` ASan core/CTest `199/199`、
`proc_742c` Linux app/CTest `205/205`；该批已提交并推送 `98d6ef88`，
阶段TG `proc_7a2c` 退出0，客户端显示未验证。

分配wrapper `sub_487C10` 的九次独立可障访问继续按物理顺序接入：
`0x00487C10 PUSH EBP`、`0x00487C13/15/17 PUSH 0/0/1`、
`0x00487C19 MOV EAX,[0x0053D1B4]`、`0x00487C1E PUSH EAX`、
`0x00487C1F MOV ECX,[EBP+8]` 重读刚压的Size、`0x00487C22 PUSH ECX`、
`0x00487C23 CALL sub_487C80` 压返回地址`0x00487C28`。缺失显式全局owner时
在`0x00487C19`停下，先前PUSH 0/0/1与旧EAX保留；现有合成全局值`0x00790000`
只是测试backing，不证明生产堆状态。九个序号/ESP/EBP/token停点和先前三项父栈输出均有局部测试；
非返回深层port现停在`0x00487C80`入口，保留wrapper已压的七个dword及EIP。

`sub_487C80`后缀及后续分配、回包、decoder命令流仍未审计，不能据此声称成功分配等价。
`proc_a1cb` Linux core/CTest `199/199`、`proc_51e0` ASan core/CTest `199/199`、
`proc_922d` Linux app/CTest `205/205`。该wrapper前缀已提交推送`111d3eb7`，阶段TG
`proc_08b9`退出0，客户端显示未验证。

`sub_487C80` 的首轮再向内展开到 `0x00487C94 CALL sub_487CD0`：先
`0x00487C80 PUSH EBP`、`0x00487C83 PUSH ECX`建立帧/局部槽，再依次从
`[EBP+18h/14h/10h/8]`重读此前wrapper所压的0/0/1/Size，分别PUSH后压入
`0x00487C99`返回地址。11个栈读写站点各自保留故障前ESP、EBP与输出栈槽；
原 `sub_487CD0` 后缀尚未验证，port非返回暂在其入口停下。使用此前压入参数的
合成缓存重放内层栈读，不证明真实栈别名和原版分配语义。

`proc_6a6b` Linux core/CTest `199/199`、`proc_d084` ASan core/CTest `199/199`、
`proc_96f3` Linux app/CTest `205/205`；该批提交推送为`c7db2b8d`，阶段TG
`proc_e26a`退出0，客户端显示未验证。

`sub_487CD0` 普通分配路径继续逐项追到 `0x00487D49` 间接CALL：入站先PUSH EBP，
`SUB ESP,10h`更新FLAGS，再PUSH EBX/ESI/EDI，零写`[EBP-0Ch]`；
`0x00487CE0` 从显式owner读取`0x004A82F4`并TEST bit2。bit2为1时
`0x00487CEC`压返回地址`0x00487CF1`转调试堆检查，不冒充普通分配。

普通路径依次读取`0x004A82F8`、写读`[EBP-8]`、比较`0x004A82FC`；
相等时`0x00487D30 INT3`另设停点，不传递申请参数。LST 在成功分配的
`0x00487E85/8B/8E`重读、加一并写回`0x004A82F8`，因此此值是本路径
请求序号而非已经证明的堆句柄；`0x004A82FC`是触发INT3的比较值。接口和
测试相应改称请求计数、断点比较值，不臆测宿主句柄。不等时重读
`[EBP+14h/10h/-8/0Ch/8]`，按原顺序压入五个值，再PUSH 0/1；
`0x00487D49`先读函数指针`[0x004A8360]`，再压返回地址`0x00487D4F`。

普通路径的24个可障栈/全局站点各有独立ESP、EBP、token序号测试；
合成堆全局与间接目标只是测试backing，真实生产owner和内存别名未证。

间接CALL及调试检查之后的回包、retry、CRT、像素解码仍归opaque后缀；
Port正常回包只用于检查条件性栈平衡，**不证明**分配或真实图像数据等价。

首次core `proc_7dad`有1项战斗测试失败：opaque端口返回后遗漏decoder自身CALL返回槽的
四字节弹栈；已按原物理RET修正，不把失败轮次计作通过。

`proc_e5c3` Linux core/CTest `199/199`、`proc_4c7a` ASan core/CTest `199/199`、
`proc_760d` Linux app/CTest `205/205`。该批提交推送为`078d1eba`，阶段TG
`proc_e243`退出0，客户端显示未验证。

继续独立检查 allocator CALL **回包而非仅入口**：LST `0x00487D4F` 在间接目标返回后
先`ADD ESP,1Ch`，`0x00487D52 TEST EAX,EAX`判回包是否为0；
此处**不写**`[EBP-4]`，该槽直到第二次申请回包的`0x00487E75`才写入。

零回包按`[EBP+0Ch]`决定是否调`sub_48A900`及重试，非零回包从`0x00487DB4`
检查请求类别、`0x004A82F4` bit0和Size上界，`0x00487E6D`再调用
`sub_48AA10`申请包含36字节调试头的内存，然后写多个堆全局/头字段。

本调用点的内层参数按LST固定为 `Size/arg_4=1/arg_8=0/arg_C=0`；
若 decoder CALL 前快照ESP为 `P`，`sub_487CD0` 的EBP为 `P−88`，
`[EBP−4]`回包槽为`P−92`，四个参数分别在`P−80/−76/−72/−68`。

间接CALL正常退回后先清七参恢复`ESP=P−116`；若回包EAX为0，
`0x00487D56`读`[EBP+10h]=0`走`0x00487D87`诊断，而非直接将零当作
`sub_487C10`的完成结果。若非零，`0x00487DC2`重读调试标志bit0，
本次 `arg_4=1` 的普通路径经Size上界后在`0x00487E6D`调用额外内存申请；
成功时更新`0x004A82F8`和多处`0x0053D1xx`堆状态，写调试头、填充字节，
最终`0x00487FD6..D9`从原始块指针加`0x20`返回用户指针。

这些写入、故障前缀和`0x00487C80/10`物理回包尚未接入，
不能直接将本批间接目标的opaque正常回复等同于返回像素指针。

独立核对 B4 `rendering::decode_legacy_image_command_stream` 的已有实现：
它将输出长度与`width*height*(depth==16?2:1)`严格比较，不符时返回
`pixel_count_mismatch`；而LST的`0x00401A0E/0x00401ACB`在内存申请后首先
`CMP word [EDI],0`，若首行命令字为0，直接经`0x00401B62..66`的
四次POP与RET返回分配指针，**不检查像素数是否匹配**。

故不可把该B4安全解码器当作本调用点全部输入的原版等价后缀；
仍需分离合成目标可验证性、真实命令字背书及像素页故障语义。

`0x004A82F8` 后续确实在`0x00487E85/8B/8E`按32位加一写回，
本批把之前误称`heap_handle`的接口改为请求计数、把`0x004A82FC`改称
INT3比较值；值本身未变。`proc_42b9` Linux core/CTest `199/199`、
`proc_5b69` ASan core/CTest `199/199`、`proc_6fec` Linux app/CTest `205/205`。

LST `.data:004A82F4/F8/FC` 的**静态初值**分别为1/1/`0xFFFFFFFF`，
`off_4A8360` 的**静态初值**为 `sub_48AA70`；该函数 `0x0048AA70..79`
只保存/恢复EBP并令EAX=1。初始化值不能取代当前运行时的owner或别名证明，
但后续可按已绑定函数指针的实际值审计首层回包，不应默认它就是分配像素的函数。

为验证**明确绑定到**`0x0048AA70`、且Size使`Size+0x24<=0xFFFFFFE0`
的这一个物理路径，已新增子函数PUSH EBP/POP EBP/RET，再按LST由
`0x00487D4F ADD ESP,1Ch`清七参、`0x00487D52 TEST EAX,EAX`确认回包为1后走成功分支；
重读`[EBP+arg_4]=1`、再次读取`0x004A82F4`的bit0、必要时将`[EBP-0Ch]`写1，
进行Size阈值与类别比较，`0x00487E60..6D`重读Size、加`0x24`写读本地槽，
最后独立压Size与返回地址`0x00487E72`，opaque后缀移动到`sub_48AA10`入口。

此次绑定采用LST静态指针初值作为**显式合成值**，不证明所有运行时调用永远指向该叶；
间接目标变动或越过已审Size上界时仍保留原opaque边界。

叶子栈读写/RET和其后总计15次栈/全局访问分别有故障序号、ESP/EBP/token向量；
审阅暂存diff时发现原先误在`0x00487C9C/9F`增设本地槽写读，
LST对应实际为`0x00487D52 TEST`，该两处未提交且已删除并重跑门禁；
`0x004A82F4` bit0为1时不写本地槽，目标不等于已绑定叶时停在原间接CALL入口，
均有独立反例。默认合成Size为12/6，对应第二次申请的`Size+0x24`为48/42；
含错误指令地址时的`proc_7d81/18a7/9345/7195`虽各门禁通过，
**不计入修正后验收**。修正后的`proc_ec48` Linux core/CTest `199/199`、
`proc_6657` ASan core/CTest `199/199`、`proc_f38f` Linux app/CTest `205/205`。

该阶段提交推送`2be3e27e`；TG `proc_cdb2`退出0，客户端显示未验证。
`sub_48AA10`此前尚未审计。当前沿显式叶目标继续核对其入口：
`0x0048AA10`保存EBP、`0x0048AA13`保存ECX，`0x0048AA14`重读Size+0x24；
`0x0048AA17`要求显式`0x004A8390`当前值owner。LST `.data`初值为`0x3F8`，
但不能代替实时owner。Size不大于阈值时，`0x0048AA1F..23`重读Size并分别压参、
返回地址`0x0048AA28`，在`sub_48BB80`入口停住；测试中的48/42字节走此支。

显式合成小阈值16时，原版`0x0048AA39`改走系统堆分支：重读Size、按16字节对齐、
写回参数槽，压Size和Flags；从显式`0x0053E7BC`堆句柄owner及
IAT `0x00499198`函数指针owner读取，再压句柄和返回地址`0x0048AA65`，
在显式目标入口停住。小块分支7处、系统堆分支10处独立栈/全局故障向量覆盖
EIP/ESP/EBP/token；无阈值owner时停在`0x0048AA17`，并不假造阈值；
显式低阈值下正常执行至合成HeapAlloc入口，但不把其端口回包当真实内存证明。

`proc_8fb2` Linux core/CTest `199/199`、`proc_cbbd` ASan core/CTest `199/199`、
`proc_6d31` Linux app/CTest `205/205`。小块/系统堆callee真实回包、
第二次申请后的调试头、像素流及生产owner别名仍未验证。

再核对小块子函数`sub_48BB80`：`0x0053E7B4`池索引、`0x0053E7B8`池基址
和后续`0x0053E7AC`扫描游标在LST `.data`均为`dd ?`，没有可替代
运行时owner的静态初值。仅当显式绑定索引时，从`0x0048BB80`物理保存EBP、
局部开栈、保存ESI，再以索引乘`0x14`、池基址相加，按Size+0x17对齐成尺寸类；
两种尺寸类分支分别构造位掩码，尚未为扫描游标绑定owner，故都在
`0x0048BBE1 MOV ECX,[0x0053E7AC]`读取前停止。

未绑定池索引仍停在原`sub_48BB80` opaque入口，不把合成池首址当生产堆内存；
子函数回包、页位图、池链表和随后的调试头依旧未审。

显式池索引0、池基址`0x00800000`为**合成测试值**，低尺寸类源产生Size12、
`[EBP-20h]=3`和低位掩码`0x1FFFFFFF`；另用16×16、16位的真实格式头
产生Size512、类号34，覆盖高位掩码`0x3FFFFFFF`，仍在扫描游标全局前停止。

低类路径14处独立栈/全局故障向量及缺池基址故障均验证ESP/EBP/token，
IMUL和多位SHR的未定义算术FLAGS不冒充已知。

首次`proc_d851`及补高类后`proc_1ddb` Linux core/CTest `199/199`、
`proc_a25d` ASan core/CTest `199/199`、`proc_a218` Linux app/CTest `205/205`。

该批提交推送为`5fe7be5d`，TG `proc_64fa`退出0，客户端显示未验证。
新阶段将**显式注入的非零小块分配回包**与既有“整个解码后缀的正常回包”分开：
只在未进入池内审计前缀、且专门绑定回包EAX非零时，视`sub_48BB80`已完成并弹
其CALL返回槽，按`0x0048AA28`清Size实参、`0x0048AA2B/2E/34`写读本地槽，
`0x0048AA65/67/68`恢复ESP、EBP并物理RET，`0x00487E72`清第二次Size实参，
`0x00487E75/78`写读`[EBP-4]`并比较回包，`0x00487E85`读取请求计数、
`0x00487E8B`加一，在尚无可写owner的`0x00487E8E`**写入前停住**。

EAX=0的池回包不冒充成功；其真实fallback与池内副作用仍由opaque边界承担。
合成非零token不证明真实内存可写，也不替代分配器完成或父栈别名证明。

九处新增栈/全局读写分别测试故障前ESP/EBP/token；合成EAX=`0x00804000`
使计数从合成`0x00760000`加至`0x00760001`，但由于计数写入owner未绑定，
未宣称写回。零回包明确保留`sub_48BB80`原opaque入口。

`proc_3366` Linux core/CTest `199/199`、`proc_6cfd` ASan core/CTest `199/199`、
`proc_5ad5` Linux app/CTest `205/205`。该批提交推送`7e8b7993`，TG
`proc_0044`退出0，客户端显示未验证。

在显式同址可写owner下，`0x00487E8E`把本次请求计数+1写回；缺owner、
与读取owner不别名或预写故障均停在本指令，不能只凭两个值相同推断同址。

随后`0x00487E94`按早先`0x00487CD9`初始化、`0x00487DCF`可选写入的
`[EBP-0Ch]`判路：调试bit0为0时物理读`[EBP-4]`、在原始块首dword
`0x00487E9D`写入前因无块backing而停止；bit0为1时直接在
`0x00487EE3`读取未绑定统计全局前停止。这些停点分别保留已提交的计数增量，
不假造块头、链表或后续像素。

新增`allocator_block_write`枚举项放在原有访问种类末尾，不移动原枚举数值；
调整前core `proc_53b8`、ASan `proc_883e`各`199/199`及app `proc_e9b9`
`205/205`不替代复验。追加枚举后`proc_6dd6` Linux core/CTest `199/199`、
`proc_25d5` ASan core/CTest `199/199`、`proc_ea7b` Linux app/CTest `205/205`。

该批提交推送`292cc21d`，TG `proc_b08d`退出0，客户端显示未验证。
针对显式绑定且可写的合成原始块，未链接调试头路径按LST
`0x00487E9D/EA6/EB0/EBA/EC7/ECD/ED7`七次独立写入偏移
`0/4/8/0xC/0x10/0x14/0x18`，值分别为0/0/0/`0xFEDCBABC`/
原始Size/3/0；七次写之间必须在各自原IP重读`[EBP-4]`或Size槽。

owner必须与分配回包的原始块token同址并具有至少四字节当前写窗；
短窗或停止时保留已经写入的字节。头写完先在`0x00487F83`压入首个
填充调用的Size=4，然后`0x00487F85`清EDX并置ZF，在尚无owner的
`0x00487F87`全局byte读取前停住，不伪造填充或像素返回。

合成块不是原版分配器真实回包证明；linked统计分支不受这组写入影响。
16处独立停点验证EIP/ESP/EBP/地址及写入前缀；头部小端字节检查
`00 00 00 00`、`BC BA DC FE`、Size=12、类别3，未覆盖的偏移`0x1C`
仍保持初始`0xA5`。错误token与20字节短窗均先停止且保留已写字节。

`proc_0e03` Linux core/CTest `199/199`、`proc_7688` ASan core/CTest `199/199`、
`proc_b3a8` Linux app/CTest `205/205`。该阶段提交推送`bb7cfe9f`，TG
`proc_5ab2`退出0；客户端显示未验证。

首个填充子调用的前缀：只有显式绑定`0x004A8300`字节owner时，
`0x00487F87`读取运行时byte至DL；依次压入Val、重读原始块指针、
加`0x1C`后压入目标地址，在`0x00487F95`压入返回地址
`0x00487F9A`，停在`sub_48A930`入口。没有实际填充或正常回包证明；
填充目标、块可写性及生产owner仍待核验。

五处逐项停点核对`0x00487F87/F8D/F8E/F94/F95`及ESP和写入前缀；
合成`0xFD`只是在显式owner下回包，不冒充生产global。

`proc_65cf` Linux core/CTest `199/199`、`proc_36a2` ASan core/CTest `199/199`、
`proc_bd51` Linux app/CTest `205/205`。该阶段提交推送`03594f71`，
TG `proc_ac23`退出0，客户端显示未验证。

显式绑定首个填充子调用的合成栈时，按`sub_48A930`
`0x0048A930/A934/A93E/A942`分别读取Size、目标指针、Val与压EDI；
对齐目标在`0x0048A971 REP STOSD`实际内存写入前停住。

汇编的`SHR ECX,2`计数为2，OF不定义，不能沿用原先known OF；
当前仅覆盖Size=4的首个调用前缀；对齐分支停在实际dword写入前，
未对齐分支停在`0x0048A951`字节对齐循环前，不声称三次填充或父函数返回。

补充未对齐测试前`proc_0e84` Linux core/CTest `199/199`、
`proc_69d8` ASan core/CTest `199/199`、`proc_f618` Linux app/CTest
`205/205`；加入未对齐测试后重跑`proc_30f1` core `199/199`、
`proc_c8d7` ASan `199/199`、`proc_7ec1` app `205/205`。该阶段提交推送
`601e71d8`，TG `proc_3b0e`退出0，客户端显示未验证。

只有合成原始块同址可写且首个填充子调用的栈显式绑定时，
`0x0048A971 REP STOSD`按一个dword实际写`[raw+0x1C..+0x1F]`为
当前全局字节的四次重复；然后分别按`0x0048A97D/A981/A982`读取返回
参数、已保存EDI与返回地址；父层`0x00487F9A`回收12字节参数，再按
`0x00487F9D..FB3`读取第二次运行时byte、Size和raw块，压入第二次
填充调用参数与返回地址，在第二个`sub_48A930`入口停住。

DF=1时第一轮dword写入内容相同，但REP写后暂存EDI倒退4，POP后
还原；无合成内存权限时仍停在实际写入前，尾端保护字节保持未写。

第二次填充、像素填充及真实分配器回包尚未验证。
十处逐项停点核对第一个RET/回收和第二个CALL前的访存与ESP；
默认DF与反向DF的首轮写入均保留正确字节，反向DF时写后EDI暂为
`raw+0x18`。`proc_1127` Linux core/CTest `199/199`、`proc_ad1d`
ASan core/CTest `199/199`、`proc_046c` Linux app/CTest `205/205`。

该阶段提交推送`4fb6a5b2`，TG `proc_b45c`退出0，客户端显示未验证。
linked分配分支在显式同址可写owner下按
`0x00487EE3`读取总尺寸`0x0053D124`、`0x00487EE9`读取原始Size、
`0x00487EEC`按32位回绕写回总尺寸；随后停在尚无owner的
`0x00487EF2`第二个统计全局`0x0053D12C`读取前。错误读写别名或
故障都不更新总尺寸；该合成统计owner不证明生产链表与统计全局。

初版测试错误地沿用未链接分支多一个栈读取的序号，`proc_2705`和
临时诊断`proc_2394`各有1个失败测试；诊断显示linked分支首个统计读取
早一位。修正四个停点序号、移除临时诊断后，`proc_4e7e` Linux core/CTest
`199/199`、`proc_fae7` ASan core/CTest `199/199`、`proc_993d`
Linux app/CTest `205/205`。失败轮次不算通过证据。该阶段提交推送
`4422d5f9`，TG `proc_7388`退出0，客户端显示未验证。

对显式绑定的第二统计量`0x0053D12C`执行`0x00487EF2/EF7/EFA`
读、加Size、同址写；`0x00487EFF`实际重读后在`0x00487F05`
用无符号比较峰值`0x0053D130`。若当前值大于峰值，必须在
`0x00487F0D`重读第二统计量后才在`0x00487F13`写峰值；相等或更小
不写峰值。两条路径均停在未绑定的链尾`0x0053D128`读取前；合成
统计量不证明原版运行时owner或节点别名。较大/较小两侧、八处故障
停点及峰值写入别名测试均使用显式合成统计owner。

`proc_cccb` Linux core/CTest `199/199`、`proc_ec4b` ASan core/CTest
`199/199`、`proc_8e40` Linux app/CTest `205/205`。该阶段提交推送
`f556db99`，TG `proc_d87b`退出0，客户端显示未验证。

显式绑定链尾全局`0x0053D128`读取后，原版`0x00487F19`比较零值。
零链尾在`0x00487F2F`重读raw块，并停在尚无owner的
`0x00487F32`链头`0x0053D120`写入前；非零链尾在
`0x00487F22`再次读取链尾，再于`0x00487F27`重读raw，停在
旧尾节点`[tail+4]`写入前。未提供旧尾节点可写backing，不冒充链接成功。

零链尾三处、非零链尾四处独立故障向量核对写前统计量持久化；
`proc_b819` Linux core/CTest `199/199`、`proc_ae81` ASan core/CTest
`199/199`、`proc_c2ec` Linux app/CTest `205/205`。该阶段提交推送
`aa32cbbd`，TG `proc_efb5`退出0，客户端显示未验证。

链尾为零且链头全局有显式可写owner时在`0x00487F32`写raw块指针；
非零链尾且旧尾块有独立同址可写backing时在`0x00487F2A`写
`[tail+4]=raw`，不能把raw块backing冒充旧尾块。之后两条路径统一在
`0x00487F38`重读raw块、`0x00487F3B`重读链尾，停在当前raw块
`0x00487F41 [raw]`反向链接写入前。旧尾块与raw块同token但不同物理
backing时拒绝虚构别名；上述合成绑定均不证明生产链表所有权。

空链头四处与旧尾四处故障前缀均验证旧统计量持续可见；初次
`proc_d631`因测试向量一处括号拼接编译失败，不算门禁通过。修复后
`proc_a98b` Linux core/CTest `199/199`、`proc_fa2a` ASan core/CTest
`199/199`、`proc_6a09` Linux app/CTest `205/205`。该阶段提交推送
`840f296f`，TG `proc_37f3`退出0，客户端显示未验证。

两条linked分支写完链头或旧尾后，仅在raw块有同址可写字节backing
时于`0x00487F41`写当前链尾token至`[raw]`，在`0x00487F43`
重读raw后于`0x00487F46`写`[raw+4]=0`；然后`0x00487F4D`
再读raw，停在尚未审计的`0x00487F50 [EBP+arg_8]`读取前。

零链尾和非零链尾各五处故障顺序与前缀字节分别验证，合成字节不
证明实际堆块来源或生产节点别名。

`proc_7561` Linux core/CTest `199/199`、`proc_5498` ASan core/CTest
`199/199`、`proc_7a67` Linux app/CTest `205/205`。该阶段提交推送
`e160ffd3`，TG `proc_9911`退出0，客户端显示未验证。

linked块在当前raw前驱/后继写入后，按LST`0x00487F50..F77`五次
从物理父栈重读`arg_8=1`、`arg_C=0`、Size、来自全局`0x0053D1B4`
的`arg_4`运行时值与本次请求前计数`var_8`，依次写到raw偏移`8/0xC/0x10/
0x14/0x18`；后者是写回计数递增**前**的快照。`0x00487F7A`
重读raw后，仅在尾指针全局读写同址且可写时`0x00487F7D`写
`0x0053D128=raw`；随后停在`0x00487F83`首次填充压栈前。

空链与非空链各17处故障检查写入前缀，仍不证明生产owner。
初版`proc_cb1c`因请求前计数快照局部作用域编译失败，不算通过；
改为同一条件路径保存快照后，`proc_8533` Linux core/CTest
`199/199`、`proc_2598` ASan core/CTest `199/199`、`proc_bf08`
Linux app/CTest `205/205`。该阶段提交推送`71d8f67d`，TG
`proc_be9c`退出0，客户端显示未验证。

linked块在完成尾指针写回后与unlinked块同到`0x00487F83`：压入
Size=4，`0x00487F85 XOR EDX,EDX`确定ZF=1，停在尚无owner的
`0x00487F87`保护字节读取前；没有偷用静态初始化字节代替运行时值。

空链、非空链两条均在该读取前检查ESP、EBP、EDX、FLAGS、尾指针和
已写元数据。`proc_7cf2` Linux core/CTest `199/199`、`proc_a507`
ASan core/CTest `199/199`、`proc_9767` Linux app/CTest `205/205`。

其后的linked填充子调用及两条路径的正式合流仍未审计。
linked块显式绑定首填充byte后，按`0x00487F87/F8D/F8E/F94/F95`
读全局byte、压填充值、重读raw、压raw+0x1C与返回地址；空链
与非空链各五处故障验证旧元数据/尾写已提交，未绑定子栈仍停在
`sub_48A930`入口，不臆测填充字节。`proc_b37d` Linux
core/CTest `199/199`、`proc_91bc` ASan core/CTest `199/199`、
`proc_1115` Linux app/CTest `205/205`。该阶段提交推送
`91ac1996`，TG `proc_452f`退出0，客户端显示未验证。

显式绑定linked子栈后，`sub_48A930`三项实参读取、旧EDI入栈与
目标对齐分支依物理顺序执行：对齐目标停在`0x0048A971` REP写前，
未对齐目标停在`0x0048A951`首个byte写前；两种填充均未虚构为
已写入。空链/非空链各四处子栈故障前缀与原raw元数据已在合成
owner向量核对；`proc_f3de` Linux core/CTest `199/199`、`proc_156f`
ASan core/CTest `199/199`、`proc_e59d` Linux app/CTest `205/205`。

该阶段提交推送`413fad2c`，TG `proc_76c3`退出0，客户端显示未验证。
显式同址可写的linked raw块在`0x0048A971`首填充写四个保护字节，
保留DF对EDI的移动方向，再逐项恢复子返回地址、清理父实参、
按`0x00487F9D`压第二填充长度，停在`0x00487FA1`保护byte重读前。

空链/非空链各六处故障前缀与合成字节向量不证明生产别名。
`proc_6679` Linux core/CTest `199/199`、`proc_595a` ASan core/CTest
`199/199`、`proc_72e3` Linux app/CTest `205/205`。该阶段提交
推送`defedee9`，TG `proc_11cf`退出0，客户端显示未验证。

linked分支显式读取第二次`0x004A8300` byte，按`0x00487FA7`
压填充值、`0x00487FA8/FAB`重读Size与raw、`0x00487FB2`
压raw+Size+0x20、`0x00487FB3`压返回地址，停在第二次
`sub_48A930`入口；空链/非空链各六处故障核对首填充已提交。

第二次子函数的内存写入仍未审计。初版`proc_f57f`的旧无stop
向量预期仍停FA1而新版显式owner已继续到CALL，导致一项测试失败；
改为在FA1实际访问前注入停止后，`proc_8d8a` Linux core/CTest
`199/199`、`proc_c70b` ASan core/CTest `199/199`、`proc_b919`
Linux app/CTest `205/205`。失败轮次不计通过。该阶段提交推送
`b4c4925b`，TG `proc_b9b7`退出0，客户端显示未验证。

linked块显式绑定第二次填充的合成子栈后，按`sub_48A930`
三参读取、EDI保存、低byte展开与对齐条件到`0x0048A971`
REP写前；同址尾部guard依旧未写。空链/非空链各四处子栈
故障以独立绑定核对；未对齐第二填充仍待单独向量。

`proc_9203` Linux core/CTest `199/199`、`proc_f4dc` ASan
core/CTest `199/199`、`proc_4550` Linux app/CTest `205/205`。

该阶段提交推送`515d9b83`，TG `proc_7709`退出0，客户端显示未验证。
仅显式同址可写linked块在第二次`0x0048A971`向raw+Size+0x20
写四byte保护值，再依次执行子栈读取、RET及父栈清参；
`0x00487FBB`重读Size、`0x00487FBE`压参，停在第三次填充的
`0x00487FC1` byte全局读取前。空链/非空链各七处故障保持首、
尾两处保护值写入先后和各自故障前副作用；第三次填充未开始。

`proc_d23f` Linux core/CTest `199/199`、`proc_a001` ASan core/CTest
`199/199`、`proc_f244` Linux app/CTest `205/205`。该阶段提交
推送`7176c19c`，TG `proc_dd2f`退出0，客户端显示未验证。

linked块在显式绑定独立`0x004A8302` byte后，从`0x00487FC1`
读取像素填充值，按`0x00487FC6/C7/FCD/FCE`压值、重读raw、
压raw+0x20及返回地址，停在第三次`sub_48A930`入口。空链
/非空链各五处故障向量不得以已有保护字节冒充像素写入。

`proc_0695` Linux core/CTest `199/199`、`proc_7bc9` ASan
core/CTest `199/199`、`proc_5526` Linux app/CTest `205/205`。该阶段提交
推送`bf2b7145`，TG `proc_3004`退出0，客户端显示未验证。

第三次填充显式绑定子栈后，`sub_48A930`读Size=12、目标raw+0x20、
独立像素byte，按LST计算对齐及`SHR ECX,2`，对齐目标保留
ECX=3并停在首个`0x0048A971 REP STOSD`写前；不得把三处
连续dword写合并成一次安全边界。空链/非空链各四处子栈故障
验证，未对齐像素目标仍待单独向量。`proc_a12a` Linux core/CTest
`199/199`、`proc_0958` ASan core/CTest `199/199`、`proc_7dc7`
Linux app/CTest `205/205`。该阶段提交推送`28379709`，TG
`proc_744c`退出0，客户端显示未验证。

显式可写linked raw块以`REP STOSD`的三次物理dword写执行
Size=12像素填充：DF=0写raw+0x20/+0x24/+0x28，DF=1写
raw+0x20/+0x1C/+0x18。后一条件会依次覆盖原首填充guard及
原请求计数元数据，不能以“前序已写”视为之后仍不变。两种DF
各在三次写前注入独立停止并检查已提交字节、EDI/ECX；完整
三写后停在`0x0048A97D`栈读前，未虚构子RET与分配回包。

`proc_7c09` Linux core/CTest `199/199`、`proc_b5dd` ASan
core/CTest `199/199`、`proc_dcbe` Linux app/CTest `205/205`。

暂存差异审阅发现三写循环仅验证Size=12，已将显式写入门收紧到
Size恰12；其他Size仍停在写入前，不能套用固定循环。

收紧后`proc_68a1` Linux core/CTest `199/199`、`proc_08b7`
ASan core/CTest `199/199`、`proc_e39f` Linux app/CTest `205/205`。

该阶段提交推送`935f674b`，TG `proc_4567`退出0，客户端显示未验证。
显式绑定第三次填充的合成返回栈后，逐项执行`0x0048A97D`
读取原目标、`0x0048A981`恢复EDI、`0x0048A982`读取返回地址
`0x00487FD3`，清理三项父实参，停在`0x00487FD6`
读取allocator局部raw前。DF正、逆写入已提交的块字节不得回滚；
仅验证条件化Size=12路径，不证明生产块别名或完整分配回包。

初版`proc_b962`的`battle.legacy_battle_setup`失败：跨DF向量未重置
“返回栈已绑定”条件，导致旧`0x0048A97D`停点向量继续运行；
新返回向量亦有合取断言失败。重置绑定后，`proc_8c22`仍失败；
拆分断言的`proc_3919`将唯一差异定位到测试误把诊断字段
`last_pushed_value`当作CALL返回地址，实际`0x0048A942 PUSH EDI`
是更晚的物理压栈。已改为校验旧EDI压栈值与恢复值，
上述失败轮次均不计通过；修正后`proc_a324` Linux core/CTest
`199/199`、`proc_d96f` ASan core/CTest `199/199`、
`proc_7bc7` Linux app/CTest `205/205`；`git diff --check`和
更改行`clang-format --dry-run --Werror`均通过。

该阶段仅覆盖
显式合成返回栈的Size=12路径，不能视作316完整验收。

该阶段提交推送`31ac299a`，远端SHA一致；TG `proc_6113`退出0，
客户端显示未验证。其后对同一显式路径，LST `0x00487E75`
将`sub_48AA10`的raw回包写入`[ebp-4]`；第三次填充返回后
`0x00487FD6`重新读取该槽，再由`0x00487FD9`将其加`0x20`，
停在`0x00487FDC POP EDI`真实读取前。仅显式绑定raw本地槽且
先前确认回包等于合成块token时执行；DF双向、空链/非空链、
停在读取前与成功读取后的访问计数、ESP/EIP、EAX、FLAGS、
像素块字节均有独立向量。初版`proc_f3b2`的
`battle.legacy_battle_setup`失败：DF下一轮错误沿用本地槽绑定；

修正后`proc_a666` Linux core/CTest `199/199`、`proc_7de4`
ASan core/CTest `199/199`、`proc_3a26` Linux app/CTest `205/205`，
失败轮次不计通过。此条件路径不证明生产分配器回包、真实本地槽或跨owner别名。

该阶段提交推送`ce7c0ee6`，远端SHA一致；TG `proc_f5fc`退出0，
客户端显示未验证。显式绑定allocator的三个保存寄存器栈槽后，
按LST `0x00487FDC/0x00487FDD/0x00487FDE`逐次POP
EDI、ESI、EBX，`0x00487FDF`令ESP回到EBP，停在
`0x00487FE1 POP EBP`读取前；成功POP不修改EAX/FLAGS/DF。

四个故障前缀和DF双向、空链/非空链终点向量均读取
`0x00487CD6/0x00487CD7/0x00487CD8`的物理保存值；
不能用外层decoder快照的EDI/ESI冒充该子函数保存值。

`proc_e117`合取断言失败；`proc_4670`将其定位为测试的
错误对照值，修正后`proc_dd9b` Linux core/CTest `199/199`、
`proc_4035` ASan core/CTest `199/199`、`proc_8209` Linux
app/CTest `205/205`，失败轮次不计通过。此合成栈不证明真实父栈来源。

该阶段提交推送`f01a4e2d`，远端SHA一致；TG `proc_d2a9`退出0，
客户端显示未验证。在显式绑定合成父栈时，LST
`0x00487CD0 PUSH EBP`保存的上层EBP被`0x00487FE1 POP EBP`
读取，`0x00487FE2 RET`取回`0x00487C99`，父函数随后
`ADD ESP,0x10`，停在`0x00487C9C`写本地像素指针前。

分别覆盖POP EBP、RET及父本地写的故障前缀、DF双向和两种链尾。
`proc_c096` Linux core/CTest `199/199`、`proc_a41f`
ASan core/CTest `199/199`、`proc_bd99` Linux app/CTest `205/205`。

合成返回地址和父栈不证明生产调用链。
该阶段提交推送`33e6258c`，远端SHA一致；TG `proc_db41`退出0，
客户端显示未验证。父函数在`0x00487C9C`写本地返回指针，
`0x00487C9F`重读并与零比较；合成块的非零指针走
`0x00487CAB`再次读取原本地值，再由`0x00487CC6`恢复ESP，
停在`0x00487CC8 POP EBP`前。已按DF双向和空链/非空链
覆盖写/比较/重读/POP四个逐次访问故障与最终寄存器/FLAGS。

若指针为零，仅停在`0x00487CA5`读取重试条件前，
不能据此声称零回包重试已验收。`proc_6224` Linux core/CTest
`199/199`、`proc_c85f` ASan core/CTest `199/199`、
`proc_9a69` Linux app/CTest `205/205`。生产父栈别名仍未证明。

该阶段提交推送`bb87b791`，远端SHA一致；TG `proc_8072`退出0，
客户端显示未验证。显式绑定`sub_487C80`父返回栈时，
LST `0x00487CC8 POP EBP`读取`0x00487C80 PUSH EBP`
的保存值，`0x00487CC9 RET`取回`0x00487C28`，随后
`ADD ESP,0x14`清五项实参，停在`0x00487C2B POP EBP`前。

初版`proc_dc0d`测试失败：预期父EBP错写为入口ESP减36，
实际六个物理压栈槽对应入口ESP减32；修正后`proc_fb7d`
Linux core/CTest `199/199`、`proc_d5f7` ASan core/CTest
`199/199`、`proc_6d3a` Linux app/CTest `205/205`。

失败轮次不计通过。
合成返回地址不证明生产父调用栈或其他caller。

该阶段提交推送`0673b160`，远端SHA一致；TG `proc_7e3a`退出0，
客户端显示未验证。显式绑定最外层分配返回栈时，按LST
`0x00487C2B POP EBP`读`0x00487C10 PUSH EBP`保存值，
`0x00487C2C RET`取回格式16的`0x00401A0B`或格式8的
`0x00401AC8`，清一项实参，再停在格式对应的首次源字读取
`0x00401A0E`/`0x00401ACB`前。当前故障向量仅覆盖格式16
显式合成块的两次栈读、DF双向和空链/非空链；格式8未由本轮向量证明。

`proc_b155` Linux core/CTest `199/199`、`proc_7e5b` ASan
core/CTest `199/199`、`proc_99cc` Linux app/CTest `205/205`。

真实像素数据及生产块来源仍待核对。
该阶段提交推送`2e66f6f4`，远端SHA一致；TG `proc_eba4`退出0，
客户端显示未验证。显式绑定扩展源span且保留同一token时，
按LST `0x00401A0E`/`0x00401ACB`读取EDI所指首个16位命令，
按16位CMP更新FLAGS并将分配回包移到ESI：零命令停在
`0x00401B62 POP EDI`前，非零命令停在格式16的
`0x00401A1A`或格式8的`0x00401AD7`读取下一源字前。

本轮独立向量只覆盖格式16的`0x8001`与`0x0000`，
验证符号位、零值、访问计数及故障前已写像素；不据此宣称
格式8或后续命令流已完成。`proc_574f` Linux core/CTest
`199/199`。初版`proc_2ac4` ASan/CTest失败，报告
`stack-use-after-scope`：测试把局部源数组的span写入持久夹具引用，
后续向量越过作用域继续读取。已改为只在局部请求副本中绑定扩展源；
失败轮次不计通过；修正后`proc_4f0d` Linux core/CTest
`199/199`、`proc_f98a` ASan core/CTest `199/199`、
`proc_f3f4` Linux app/CTest `205/205`。

生产源数据和块别名仍未证明。

该批提交推送`40b08a1c`，远端同SHA；阶段TG `proc_1086`退出0，
客户端显示未验证。格式8另用显式合成宽3、高4、格式8、Size=12的
源span和空链堆块（DF=0）独立验证首个命令字`0x8001/0x0000`：
按LST `0x00401ACB`在源`+8`先读16位、置CMP FLAGS和ESI，
非零停在`0x00401AD7`源`+10`读取前，零停在`0x00401B62`
首个POP前；源故障前缀和像素填充、三个输出槽保持。

`proc_fa7d` Linux core/CTest `199/199`、`proc_3790` ASan
core/CTest `199/199`、`proc_226f` Linux app/CTest `205/205`。

仅验证该Size=12的显式空链合成路径，不证明其他尺寸、真实源页、
分配器块来源或后续格式8命令流。该批提交推送`b47e45e2`，
远端同SHA；阶段TG `proc_98b4`退出0，客户端显示未验证。

继续在显式源span且首词非零、第二词为普通字面类
`(word & 0xC000)==0`时独立读源`+10`的16位词：格式16在
`0x00401A1A MOV DX,[EDI+2]`、格式8在
`0x00401AD7 MOV CX,[EDI+2]`，保留对应寄存器高16位；
两个`ADD EDI`使源游标到`+12`。第二词0时分别在
`0x00401AAB`/`0x00401B58`下一行标记读前停下，EBP=2，
TEST低16位FLAGS为零；字面计数2时分别在
`0x00401A45`下一个像素字、`0x00401B02`下一个像素字节读前
停下，EBP=4、16位路径EDX=2/ECX=0、8位路径ECX=2/EDX=0、
AND FLAGS保持非零且奇偶位为0。

高位编码、短源、未绑定源继续
停在第二词读前，不将窥视的合成数据冒充可观察访问；输出像素流
尚未执行。格式16向量含空/非空链与双向DF，格式8仅空链DF=0；

均为Size=12显式合成块和源。`proc_335e` Linux core/CTest
`199/199`、`proc_f022` ASan core/CTest `199/199`、
`proc_71c6` Linux app/CTest `205/205`；这不证明真实生产别名。

该批提交推送`cc8e5f7c`，远端同SHA；阶段TG `proc_6097`退出0，
客户端显示未验证。普通字面计数非零的首个像素源读另按显式合成源
`+12`审计：格式16 `0x00401A45 MOV BX,[EDI]`读`0x1234`、
`0x00401A48 ADD EDI,2`后停在`0x00401A4B`写目标前；
格式8 `0x00401B02 MOV BL,[EDI]`读`0x5A`、
`0x00401B04 INC EDI`保留先前CF并停在`0x00401B05`写前。

两处目标都是合成像素块`+0x20`，停点时先前填充字节`0x7E`
尚未被新像素覆盖；源读故障序号、EDI/EBX/FLAGS/ESP和目标token
均独立检验。格式16仍仅Size=12的空/非空链及双DF，格式8仅
Size=12空链DF=0；不把读成功当作像素写入、生产来源或完整解码。

`proc_3d50` Linux core/CTest `199/199`、`proc_c635` ASan
core/CTest `199/199`、`proc_927a` Linux app/CTest `205/205`。

该批提交推送`b122ab83`，远端同SHA；阶段TG `proc_dcf9`退出0，
客户端显示未验证。仅对显式可写Size=12块、普通字面计数2的
合成路径，按LST `0x00401A4B MOV [ESI],BX`提交低字节`0x34`、
高字节`0x12`，`0x00401B05 MOV [ESI],BL`仅提交字节`0x5A`；
原有调试头与填充dword仍按4字节写，不把像素写扩成dword。

故障序号截断在各目标写前时保持填充`0x7E`；写后16位路径
ESI加2、ECX=1、EBP=6，8位路径ESI加1、EDX=1、EBP=5，
两者均比较`1<2`并在下一像素源读`0x00401A45`或
`0x00401B02`前停下，ESP与已写像素保持。`proc_9355`
Linux core/CTest `199/199`、`proc_2d87` ASan core/CTest
`199/199`、`proc_3566` Linux app/CTest `205/205`。

其他计数、源/目标别名和真实生产块未由此证明。
该批提交推送`37ae19ac`，远端同SHA；阶段TG `proc_bca9`退出0，
客户端显示未验证。相同显式Size=12、普通计数2合成路径的第二像素
逐访问接入：格式16在`0x00401A45`读源`+14`的`0xABCD`，
`0x00401A4B`以word写像素区`+2`；格式8在`0x00401B02`
读源`+13`的`0xA6`，`0x00401B05`只写像素区`+1`。

源读前、目标写前、写后各有独立停点；写前首像素保留、
第二像素仍为`0x7E`，写后16位像素区字节为`34 12 CD AB`，
8位为`5A A6`。格式8第二次INC EDI继承前次`CMP 1,2`
的CF=1，写后`CMP 2,2`使ZF=1/CF=0；16位对应计数
ECX=2/EDX=2、EBP=8，8位为EDX=2/ECX=2、EBP=6；
分别停在`0x00401A9E`或`0x00401B4B`行标记读前。

`proc_6c88` Linux core/CTest `199/199`、`proc_7801` ASan
core/CTest `199/199`、`proc_eacf` Linux app/CTest `205/205`。

尚未核对行标记、其他命令、真实源/目标别名或完整caller。
该批提交推送`95ec5c27`，远端同SHA；阶段TG `proc_5ce0`退出0，
客户端显示未验证。四处真实 caller 中主动作分派及对手动作分派的
现代前置容量门在**显式 caller 快照**时改为先按 LST 发物理 CALL：
B index8 的组B token=`0x0053AE48`、首读地址=`0x0053D904`，
A index10 的组A token=`0x005201D8`、首读地址=`0x00522C94`；
无 owner 时均在 `0x0047985B` 保留子入站 ESP=`P−0x28`，
不执行 parent EAX 后缀或 opaque port。无快照仍保留现代数组门；

原版 `.data` 的运行时值、是否可读、两个最终步进后缀和生产别名
仍未核对。`proc_9c92` Linux core/CTest `199/199`、
`proc_7a85` ASan core/CTest `199/199`、`proc_da82`
Linux app/CTest `205/205`。该批提交推送`51cf8a76`，
远端同SHA；阶段TG `proc_b77a`退出0，客户端显示未验证。

最终角色步进组A index10与组B index8也按 LST 用显式 caller
快照走到子首读 `0x0047985B`，无当前物理 owner 时以
`P−0x28`的ESP停止，分别公布`0x00522C94/0x0053D904`
故障token；A 的`0x0045AA2F`父栈`arg_4`已经先写
`0x005201D8`，B 不伪造这个前置写。四处 caller 的**合成无owner
首读边界**已分别有测试，但真实内存页/字段值、正常EAX1后的
完整父后缀、生产alias和跨owner栈并未验收。

`proc_887e` Linux core/CTest `199/199`、`proc_9bea` ASan
core/CTest `199/199`、`proc_b262` Linux app/CTest `205/205`。

仅在显式 Size=12、普通字面计数 2、源字节可读且目标块可写的条件下，
第二像素提交后再按 LST `0x00401A9E/0x00401B4B` 读取行命令字：
16 位源偏移 `+16`，8 位源偏移 `+14`，两者都只读 16 位。

读取前停点不改变先前像素；已绑定零字时分别清 ECX／EDX，
保留上一像素寄存器，停于 `0x00401AAB/0x00401B58` 的下一标记比较前；
已绑定普通非零字时进入对应计数、EBP 加 2 且 EBX 清零，
停于 `0x00401A45/0x00401B02` 的下一像素读前。两路的 EDI、ESP、
源读计数、FLAGS 与写后像素按独立向量核对。高两位非零的命令
即使有合成字节也保持前次未审计行命令读障，不将其读后解释为普通命令；
其他尺寸、更多像素和生产源／目标别名仍未核对。

`proc_5e98` Linux core/CTest `199/199`、`proc_25e9` ASan
core/CTest `199/199`、`proc_1170` Linux app/CTest `205/205`。

零行命令后的终止比较也仅在上述显式合成路径接入：格式 16 从
源 `+18`、格式 8 从 `+16` 以 `CMP word [EDI],0` 读取下一字。

读取障碍先于该字且保留原始两像素；字为 0 时保留比较 FLAGS、
ESP 和保存寄存器，停在 `0x00401AB5/0x00401B62` 的首次 POP
栈读取前，不冒充完成 RET；字非零（含 `0x8000`）则停在
`0x00401A1A/0x00401AD7` 的下一行命令字读取前，地址为 EDI+2，
不提前清理栈或改变 EDI。零／正／符号位样本在读障与读后停点
分别核对源读次数、FLAGS、寄存器及已写像素。

`proc_6a11` Linux core/CTest `199/199`、`proc_a56e` ASan
core/CTest `199/199`、`proc_1b54` Linux app/CTest `205/205`。

仅在零行命令、零终止字且显式合成栈快照下，继续逐个读取
`0x00401AB5..0x00401AB8` 或 `0x00401B62..0x00401B65` 的
EDI、ESI、EBP、EBX 保存字：各停点在本次 POP 前保持前次恢复结果，
成功 POP 后 ESP 分别加 4，FLAGS 仍为终止字比较结果；四次恢复后
停在 `0x00401AB9/0x00401B66` 返回地址读取前，EAX 不被伪置零，
也不提前发布父函数回包。空链、非空链格式16和空链格式8的
五个停点（四次 POP 读障与 RET 读前）分别按原快照寄存器及先前像素字节核对。

`proc_515a` Linux core/CTest `199/199`、`proc_6773` ASan
core/CTest `199/199`、`proc_c4b8` Linux app/CTest `205/205`。

显式合成栈快照且 `return_address_readable` 为真时，继续核对
格式16 `0x00401AB9`、格式8 `0x00401B66` 的 RET 物理读：
访问序号挡在 RET 前时，ESP 停于 decoder CALL 返回字，
EAX 仍为合成分配回包像素区 token，父回包未发布；允许本次读后
ESP 加 4，EIP 到 case2 父 `0x00479B4B`，EAX 保留
`0x00804020`，保存寄存器已恢复且终止比较 FLAGS 未变。

父目标 token 尚未写入，其余三个 decoder caller 和生产同址栈仍待核对。
首次 `proc_c692` core 仅 198/199：测试将全局
`return_address_readable` 置假，实际先停在嵌套分配器较早的 RET；
改用精确访问序号断在 decoder RET 后，`proc_4ba0` Linux
core/CTest `199/199`、`proc_1b09` ASan core/CTest `199/199`、
`proc_1beb` Linux app/CTest `205/205`。失败轮次不计通过。

在原本仅支持普通字面的显式 Size=12 解码前缀中，额外只允许
第二命令字 `0x8001` 进入高位填充的首个全局颜色读障。

格式16 按 LST `0x00401A5C..0x00401A69` 令 EBX=0x8000、
EDX=1、ECX=0、EBP=4，停在 `0x00401A69` 读取
`0x004CDE20` 低16位之前；格式8 按 `0x00401B11..0x00401B1E`
令 EBX=0x8000、ECX=1、EDX=0、EBP=4，停在 `0x00401B1E`
读取 `0x004CD780` 低字节之前。两路均保留 EDI=源+12、
ESI=像素区起点、原有填充字节及访问序号；其它高位编码仍停在
命令字读取前。尚未核对真实全局颜色值或任何高位填充像素写。

`proc_0379` Linux core/CTest `199/199`、`proc_c627` ASan
core/CTest `199/199`、`proc_e77d` Linux app/CTest `205/205`。

同一 `0x8001` 路径的全局颜色在显式只读 owner 下依次接入：
格式16 从 `0x004CDE20` 读低字 `0x1234`，LST `0x00401A70`
仅写目标像素区 `+0x20` 的两个字节 `34 12`；格式8 从
`0x004CD780` 读字节 `0xA5`，只写像素区 `+0x20` 的单字节。

格式8 的 `MOV BL` 保留高字节，故写前 EBX=`0x80A5`；
全局读前、目标写前、写后各独立停点，写前原填充 `0x7E`
仍保留。写后格式16 ESI 加2、ECX=1、EDX=1，格式8
ESI 加1、EDX=1、ECX=1；EBP 都仍为4，比较计数相等后分别
停在 `0x00401A9E/0x00401B4B` 的下一行命令字读取前。

`proc_b7d0` Linux core/CTest `199/199`、`proc_8a9c` ASan
core/CTest `199/199`、`proc_e86d` Linux app/CTest `205/205`。

`0xC001` 在同一显式 Size=12、单次填充条件下另有独立全局：
格式16 按 `0x00401A7E..0x00401A92` 读
`0x004CDE78` 的低字 `0xBEEF`，只写一个 word `EF BE`；
格式8 按 `0x00401B2F..0x00401B42` 读
`0x004CD7B4` 的低字节 `0xB6`，`MOV BL` 保留 BH=0xC0，
写前 EBX=`0xC0B6`，只写一个 byte。两种格式分别在自己的
颜色读前、目标写前与下一个命令字读取前停住，写前原填充
`0x7E` 不变；此前 `0x8001` 的全局地址和测试保持独立。

`proc_8147` Linux core/CTest `199/199`、`proc_fd31` ASan
core/CTest `199/199`、`proc_828c` Linux app/CTest `205/205`。

`0x8002/0xC002` 仍限 Size=12 合成 owner：`AND EDX/ECX,3FFFh`
留下计数2，格式16 用 ECX、格式8 用 EDX 从0增至2；第一次
颜色读／写后比较不等，分别回跳 `0x00401A69/0x00401A8B`
或 `0x00401B1E/0x00401B3C` 重新读取全局，而不是复用
首读颜色。第二次读前目标首像素已写、下一像素仍为 `0x7E`；
第二次目标写前的 ESI 为原地址加2／加1；写完后 ESI 为原地址
加4／加2、比较 ZF=1，才停在 `0x00401A9E/0x00401B4B` 的
后续命令读取前。四种格式／颜色命令组合各有五个访问停点。

初测 `proc_5428/proc_6770` core `198/199`：格式8 合成堆尾
未复位，先在 `0x00487F2A` 停住，故不计通过；修正测试后
`proc_7e99` core、`proc_707c` ASan 各 `199/199`，
`proc_02d9` app `205/205`。

填充低14位为0也不走空操作：LST 中
`0x00401A63/0x00401A85/0x00401B18/0x00401B36` 的 AND
令计数零，但紧接的颜色读仍无条件执行。只对 `0x8000/C000`
显式 Size=12 合成输入验到首次颜色读前、首像素写前及第二次
颜色读前：格式16 EDX=0、ECX 由0变1、ESI 加2；格式8
ECX=0、EDX 由0变1、ESI 加1；第一次比较低16位的1与0
不等，ZF由1变0，首像素已写而后续字节仍保持 `0x7E`。

静态指令允许推导需要计数回绕后才会相等，但 `0x10000` 次
迭代完整结果及跨物理分配边界尚无测试或生产证明。

`proc_b537` core、`proc_2d40` ASan 各 `199/199`，
`proc_e84b` app `205/205`。

高两位为 `01` 的合成命令 `0x4000/0x4001/0x7FFF`
按 LST `TEST BX,BX`、`CMP BX,8000h`、
`CMP BX,0C000h` 均不命中像素写分支，直接停在
格式16 `0x00401A9E` 或格式8 `0x00401B4B` 的下一个
行命令字读取前。EBP=4、EDI=源+12、ESI 仍为像素区起点；
EBX=0x4000，格式16 EDX／格式8 ECX 保留完整命令低16位，
FLAGS 是最后一次 `CMP 4000h,0C000h` 的 CF=1/PF=1/
ZF=0/SF=1/OF=1，目标首像素仍为 `0x7E`。

该阶段只证明 Size=12 合成源读后的三种代表值及读障，不证明
下一行命令或生产源内容。`proc_1b39` core、
`proc_8a0c` ASan 各 `199/199`、`proc_734e` app `205/205`。

两组填充命令改按高两位 `10/11` 识别；计数3 在显式
Size=12 合成源及目标下覆盖格式16/8、两组颜色的七个
访问停点：三次颜色读、三次相邻 word／byte 像素写，最后
才到下一命令读取；ESI 累计加6／加3，ECX／EDX 计数3，
末次 CMP ZF=1。格式16 的 DF=1 初始 dword 填充向低地址，
第三像素在写前仍为旧 `0xA5` 而非 `0x7E`；DF=0 才是
`0x7E`。`0xBFFF/0xFFFF` 则只核对 AND 后计数
`0x3FFF`、两组全局颜色的**首读前**，不验证其长循环。

初测 `proc_db7c/proc_4246/proc_39c2` core 为
`198/199`：测试把反向 DF 的第三像素旧值误认作 `0x7E`；
修正后 `proc_d8ab` core、`proc_56a4` ASan 各
`199/199`、`proc_cd42` app `205/205`。失败轮次不计通过。

解码器分配 Size=16（宽2、高4、格式16）且显式 64-byte
合成块时，`0x0048A96C SHR ECX,2` 得四个 dword；
`0x0048A971 REP STOSD` 对 DF=0 依次写原始块
`+0x20/+0x24/+0x28/+0x2C`，DF=1 依次写
`+0x20/+0x1C/+0x18/+0x14`（第四次触及较低地址，
不视作越界前可忽略的写）。每一次写前有独立读障，最后
ECX=EDX=0、EAX=`0x7E7E7E7E`，停于
`0x0048A97D` 的合成返回栈首次读取前，不冒充成功返回
或后续图像解码；空链／非空链、双 DF 均覆盖。

`proc_511b` core、`proc_691c` ASan 各 `199/199`，
`proc_4750` app `205/205`。加强为逐次 dword 字节检查与
固定 ESP=`S−136`／首次栈 token=`S−128` 后，
`proc_7013` core、`proc_ecf9` ASan 各 `199/199`、
`proc_5bfa` app `205/205`；Size 非 12/16 与真实堆块仍待审。

从 LST `0x0048A930..0x0048A987` 独立推导 Size=6（宽1、高3、格式16）
合成块的相邻两次填充：原始块 token 为 `0x00804000` 时，第二护栏目标
`raw+0x26` 不按四字节对齐；Size=4 在 `0x0048A951 SUB EDX,ECX`
后先于 `0x0048A953` 顺序写 `raw+0x26/+0x27` 两字节，再因
`0x0048A96F JZ` 跳过 `REP STOSD`，于 `0x0048A977` 顺序写
`raw+0x28/+0x29` 两字节。此短路没有 `0x0048A973 TEST EDX,EDX`；
`SHR ECX,2` 从2得0且 CF=1，尾循环的 `DEC EDX` 不改 CF。

`0x0048A93C XOR EAX,EAX; 0x0048A93E MOV AL,[Val]` 使对齐前两次
写障的 EAX 为 `0xFD`；直到 `0x0048A959..0x0048A965` 才扩展为
`0xFDFDFDFD`，不能把后值提前用于前两次停点。护栏的逐字节
`INC EDI` 不受 DF 影响。若合成请求未显式绑定第二次护栏写入，
应在 `0x0048A951` 已减 EDX 后、`0x0048A953` 首字节写前
停止；原始块 `raw+0x26` 不应被可写 span 本身暗中改动。

第三次填充目标 `raw+0x20` 已对齐；Size=6 使 `SHR ECX,2`
得一个 dword、剩余 EDX=2，写 `raw+0x20..23` 后的 EDI
在 DF=0 时为 `raw+0x24`、DF=1 时为 `raw+0x1C`；
`0x0048A973 TEST EDX,EDX` 清 CF 后，尾循环分别向
`raw+0x24/+0x25` 或 `raw+0x1C/+0x1D` 正向逐字节写入。

反向尾写覆盖先前护栏的低端两字节，不可合并为六字节连续正向填充。
两组顺序访问对应 `0x0048A953/0x0048A977`
及 `0x0048A971/0x0048A977`；合成返回栈首读仍位于
`0x0048A97D`。合成统计初值 `live=0xFFFFFFFD,peak=8` 时，
Size=6 算术回绕后 live=3，不满足 LST `0x00487F05/0B`
的严格大于门，因此跳过 `0x00487F0D` 重读与 `0x00487F13`
峰值写；Size=12 则 live=9>8，会多两次物理访问。

相对 `decoder_pending` 的 Size=6 合成访问偏移因此护栏为
154–157，像素为172–174，返回栈首读为175；原 Size=12
测试使用不同偏移，不能直接移用。空／非空链与双 DF 的
逐次合成写障、未绑定首写停止与第一次返回栈读障已逐项检查；
首轮错误的期望偏移和对齐前 EAX 已纠正，新增未绑定护栏不得
越过首写点的回归向量。该版本的受管仓库门禁：
Linux core `199/199`（`proc_6d8b`）、ASan core `199/199`
（`proc_2162`）、Linux app `205/205`（`proc_8c46`）。

又按 LST `0x0048A97D MOV EAX,[ESP+8]`、`0x0048A981 POP EDI`、
`0x0048A982 RET`、父 `0x00487FD3 ADD ESP,0x0C`、
`0x00487FD6 MOV EAX,[EBP−4]` 增补 Size=6 的三项返回读取与父局部首读
合成停点：相对 `decoder_pending` 的访问偏移为175–178；
第176项前 EDI 为 DF=0 的 `raw+0x26`／DF=1 的 `raw+0x1E`，
POP 后恢复 `0x004019FD LEA EDI,[source+8]` 所保存的
`source+8=0x7766554C`，EAX 已为 `raw+0x20`。

第178项仅在未绑定父 `raw-local` 时停在真实首读 `0x00487FD6`；
保留绑定会继续执行后续解码命令，不能当作该读取的故障。

首次测试误写 `source+12`，后又遗漏解除父局部绑定，两次失败均不计通过；
修正后的返回向量随 Linux core `199/199`（`proc_84c1`）、
ASan core `199/199`（`proc_1a78`）及 Linux app `205/205`
（`proc_ae04`）通过；这三个门禁均覆盖新增合成返回栈向量。

继续用宽1、高3、格式16的12-byte合成源：`source+8/+10`
各为 `0x8001`。据 LST `0x00401A0E/1A/63/69/70/73/76/77/9E`，
分配成功后取第二个 word 的高两位 `10`、低14位计数1，
从 `0x004CDE20` 读合成颜色 `0x1234`，在 `0x00401A70`
只将 `raw+0x20/+0x21` 写成 `0x34/0x12`；随后因合成源
不含 `source+12` 的下一 word，在 `0x00401A9E` 读取前停止。

DF=0/1 及空／非空链的四组向量均核对 EAX=`raw+0x20`、
ESI=`raw+0x22`、EDI=`source+12`、EBX=`0x1234`、
CX/DX=1、FLAGS.ZF=1 与已写像素；含新增首命令向量的
Linux core `199/199`（`proc_7cff`）、ASan core `199/199`
（`proc_3164`）、Linux app `205/205`（`proc_129d`）均通过。

另用16-byte合成源在 `source+12/+14` 各补零 word；
LST `0x00401A9E/AA1/AA3/AA6/AAB/AB5..AB9` 依次读零命令、
清计数、读独立零行哨兵、POP EDI/ESI/EBP/EBX 和 RET 至
`0x00479B4B`，期间没有第二个像素写。将原 literal 两像素专用的
行尾共用为填充单像素路径，四组 DF×链向量核对
`decoder_pending+202(+旧尾写1)` 次访问、ESP 归位、EAX=`raw+0x20`、
ZF=1 与已写像素；本次新增路径随 Linux core `199/199`
（`proc_7881`）、ASan core `199/199`（`proc_bad7`）及
Linux app `205/205`（`proc_1a1f`）通过。

再把首命令改为 `0x4001`：LST `0x00401A5C/61` 与
`0x00401A7E/83` 的两次不相等比较直达 `0x00401A9E`；
低14位不决定像素写入，但下一读前 DX 仍保存 `0x4001`。

不读颜色也不写像素；随后的双零 word 仍经共用尾返回。
四组 DF×链合成向量核对访问数为
`decoder_pending+200(+旧尾写1)`，`raw+0x20/+0x21` 均维持
填充值 `0x7E`；本次 skip 分支随 Linux core `199/199`
（`proc_e811`）、ASan core `199/199`（`proc_c479`）及
Linux app `205/205`（`proc_b412`）通过。

再在 `0x00401A9E` 读前停住同一 `0x4001` 向量：
相对 `decoder_pending` 为193次访问（旧尾加1），DX=`0x4001`、
BX=`0x4000`、CF/SF/OF=1、ZF=0，且像素仍为 `0x7E`；
证明 skip 只剥离 EBX 高两位，没有提前清除 EDX 的低14位。

将另一初始命令改为 `0xC001`，据 LST `0x00401A85/8B/92`
从第二颜色全局取合成 `0xBEEF` 并只写 `raw+0x20/+0x21`
为 `0xEF/0xBE`，双零后物理返回；四组双 DF×链测试随
Linux core `199/199`（`proc_cc93`）、ASan core `199/199`
（`proc_2e81`）及 Linux app `205/205`（`proc_45c3`）均通过。

又按 LST `0x00401A3F/45/48/4B/4E/51/52/55/58/5A`
补首条 literal 计数1：从 `source+12` 读一个 `0xBEEF` word、
写至 `raw+0x20`，CX=DX=1 后不再读第二像素而跳
`0x00401A9E`；`source+14/+16` 双零结束整行并物理返回。

四组 DF×链向量核对访问数 `decoder_pending+202(+旧尾写1)`、
输出 `0xEF/0xBE`、ESP 与返回站点，Linux core `199/199`
（`proc_941c`）、ASan core `199/199`（`proc_e91a`）及
Linux app `205/205`（`proc_3bd4`）全部通过。

又把同一 Size=6 改为宽2、高3、格式8，保留 `0x8001`
首命令：LST `0x00401ACB/D7..0x00401B2B` 从 `0x004CD780`
读合成颜色 byte `0xA5`，只写 `raw+0x20`，`raw+0x21`
仍为填充 `0x7E`，双零行尾走 `0x00401B4B/58/62..66`
返回；四组 DF×链向量随 Linux core `199/199`（`proc_8d38`）、
ASan core `199/199`（`proc_2b02`）及 Linux app `205/205`
（`proc_557d`）通过。

格式8首命令再改为 literal 计数1：LST `0x00401B02/04/05/07..0F`
从 `source+12` 读 `0x5A` 并写 `raw+0x20`，DX=CX=1 后
在 `0x00401B4B` 从奇数地址 `source+13` 读零命令，
`0x00401B58` 从 `source+15` 读独立零行哨兵；四组 DF×链
在物理下一读前核对 token=`0x77665551`、访问数
`decoder_pending+195(+旧尾写1)`，并在完整返回时核对
`decoder_pending+202(+旧尾写1)` 和像素后缀。Linux core
`199/199`（`proc_2c45`）、ASan core `199/199`（`proc_cac8`）
及 Linux app `205/205`（`proc_305b`）均通过。

首源 word=0 时，LST 格式16 `0x00401A0E/14` 与格式8
`0x00401ACB/D1` 均直接跳共同 `0x00401B62` 出栈尾；
不读命令或颜色，也不写像素。把已核对的 POP/RET 尾共用给
此提前返回，宽1×高3×16和宽2×高3×8、空／非空链及双DF
八组向量分别在首 POP 前核对访问数
`decoder_pending+192(+旧尾写1)`，正常返回后核对
`decoder_pending+197(+旧尾写1)`、ESP和原填充值；
Linux core `199/199`（`proc_26ac`）、ASan core `199/199`
（`proc_40e7`）及 Linux app `205/205`（`proc_1da7`）通过。

从 LST 反查生产来源：case2 `0x00479B3B` 读取
`actor+0x2548` 的描述符 token，`0x00479B43` 再读取
`[descriptor+0]` 的命令流 token，在 `0x00479B46`
作为首实参调用 `sub_4019A0`。现代 `lookup_frame()` 回复只发布 descriptor 的
`resource_value_00/04/0c/0e` 数值到 actor resource owner；
`LegacyTswRuntime` 虽能返回转换后的 `primary_stream` 字节 span，
但四处 caller 尚未把 runtime 接到 lookup port。现让可选 lookup 回复携带
借用的 `(source_token, primary_stream)`，仅在已返回、descriptor 首 dword
与字节流 token 都等于当前 decoder 实参时由解码器接收；显式
`decoder_sources` 仍优先。

未匹配的合成 owner 在 `0x004019AD` 停，
匹配 owner 的格式16/8 空首字双 DF×空／非空链正常 POP/RET；

Linux core `199/199`（`proc_a25d`）、ASan core `199/199`
（`proc_eabb`）及 Linux app `205/205`（`proc_6d0a`）通过。

这只使 port 回传的字节有绑定入口，不构成实际 TSW owner 接线；
四处 caller 仍需真实 snapshot/port 注入与缓存生命周期核对，
不能用已有 TSW 模块的资产验证冒充 316 的生产路径。

同一行后续非零 `0x4001`：格式16 在 `0x00401A9E/AA9`
读取后回 `0x00401A2F` 分派、由 `0x00401A7E` 跳过写并再读；
格式8 在 `0x00401B4B/B56` 回 `0x00401AEC`，由
`0x00401B2F` 再读。现仅对后续 `0x4000` 类加入逐次
读取与跳过循环，双格式的首个填充后连续两条 skip、双零结束，
双 DF×空／非空链在第二次 skip 首读前检查 EDI/EBP/EBX/flags
及 `decoder_pending+196(+旧尾写1)`，返回后检查
`decoder_pending+204(+旧尾写1)` 和像素尾未改；

Linux core `199/199`（`proc_39a1`）、ASan core `199/199`
（`proc_2d51`）及 Linux app `205/205`（`proc_8dad`）通过。

最初 8-bit 向量缺颜色 owner 且断言直接解引用空指针，
`proc_d032`、`proc_2364` 失败；补充独立颜色 owner 后复验通过。

非零行哨兵后的空第二行：`0x00401AAB/0x00401B58`
比较非零后分别跳 `0x00401A1A/0x00401AD7`，从 `[EDI+2]`
读取下一行零命令，EDI 加4、EBP 置2，再在
`0x00401AAB/0x00401B58` 比较最终零哨兵返回。

仅在显式声明下一行命令已绑定后前进；双格式×双 DF×
空／非空链在最终哨兵首读前核对 EIP、EDI、EBP、flags、
`decoder_pending+198(+旧尾写1)`，完整返回核对
`decoder_pending+204(+旧尾写1)`；Linux core `199/199`
（`proc_c699`）、ASan core `199/199`（`proc_7474`）及
Linux app `205/205`（`proc_f2d5`）通过。

首行非零 header 后立即零命令时，`0x00401A1A/0x00401AD7`
直接跳 `0x00401AAB/0x00401B58` 行哨兵比较，不应停于零命令；
已合并首行与后续空行的共同行尾循环，可重复处理多个非零行哨兵
与零命令。格式16/8×双 DF×空／非空链的三空行向量在首个
行哨兵前停于 `decoder_pending+193(+旧尾写1)`，完成三行及
POP/RET 后到 `decoder_pending+203(+旧尾写1)`，不写像素；
Linux core `199/199`（`proc_49f8`）、ASan core `199/199`
（`proc_8cce`）及 Linux app `205/205`（`proc_f799`）通过。

同一行后续 `0x8001/0xC001`：抽取首命令填充的逐像素颜色首读、
输出写及16-bit计数比较为共用步骤，并从 `0x00401A9E/0x00401B4B`
真实读取后续高位命令后重复执行；原先把高位命令停在读取前的
旧测试期望已改为到 `0x00401A69/0x00401B3C` 的真实未绑定颜色首读。

首命令 `0x4001`、后续两种填充及双零结束的双格式×双 DF×
空／非空链在第二颜色读前核对 EIP/token/寄存器及
`decoder_pending+197(+旧尾写1)`，返回后核对
`decoder_pending+206(+旧尾写1)`、两个像素与本次写入前后相同的尾字节；
16-bit 尾字节在此前两次诊断中分别观测到 `0xA5/0x7E`，
固定值没有同路径基线证明，故仅声明第二填充未改写该字节。

Linux core `199/199`（`proc_40a0`）、ASan core `199/199`
（`proc_2dac`）及 Linux app `205/205`（`proc_8975`）通过。

后续非零 literal1：首 skip `0x4001` 后从 `0x00401A9E/0x00401B4B`
读取 `0x0001`，共用已验收的首/第二像素读取与写入步骤，
在 `0x00401A4B/0x00401B05` 写前停点核对
`decoder_pending+195(+旧尾写1)`；双格式×双 DF×空／非空链
完成下一命令（8-bit 源+15奇地址）和双零返回时核对
`decoder_pending+203(+旧尾写1)`、像素字节与栈。

Linux core `199/199`（`proc_e4e2`）、ASan core `199/199`
（`proc_9182`）及 Linux app `205/205`（`proc_8433`）通过。

literal 长度3：按 LST 16-bit `0x00401A45..0x00401A58` 与
8-bit `0x00401B02..0x00401B0D` 每像素独立读取/写入/比较；
计数不限于旧合成向量的1或2。双格式×双 DF×空／非空链
在第三像素写前核对 EIP、输出 token、EDI、BX/BL、
8-bit `INC EDI` 保留前次比较 CF 与 16-bit `ADD EDI,2` 重算 CF，
访问数 `decoder_pending+198(+旧尾写1)`；三像素和双零返回
核对全部6/3像素字节及 `decoder_pending+206(+旧尾写1)`。

Linux core `199/199`（`proc_95a0`）、ASan core `199/199`
（`proc_88f6`）及 Linux app `205/205`（`proc_373a`）通过。

非空第二行：把行哨兵非零后的 `[EDI+2]` 命令首读
（`0x00401A1A/0x00401AD7`）交回同一非递归命令循环，
避免为每一行复制填充/literal 分支；空行路径仍逐次按
`0x00401AAB/0x00401B58` 读取行哨兵。合成首行单像素填充、
第二行单像素 literal、双零结束的双格式×双 DF×空／非空链
分别在第二行命令读前 `decoder_pending+197(+旧尾写1)`、
第二行像素写前 `+199(+旧尾写1)` 和正常返回
`+207(+旧尾写1)` 核对 EIP/token/输出/栈；

Linux core `199/199`（`proc_85b1`）、ASan core `199/199`
（`proc_4a01`）及 Linux app `205/205`（`proc_8bb4`）通过。

读取真实库存 `tsw-frame-descriptors.tsv` 的 width×height：六份 TSW
各自最小为 all_char 384、all_item 26400、all_magic 4800、
all_sys 49、all_map1 3072、all_map2 33920；无 6 像素物理帧。

因此 Size=6 全为故障和指令顺序合成向量，不能冒充生产资产。
`sub_48A930` 的 guard 对齐字节与 payload `REP STOSD` 无
Size=6/12/16 特判；解除原现代实现中的尺寸护栏，显式宽
10×高1×16（Size=20）与宽3×高7×8（Size=21）及
128 字节 owner 向量完成合成返回，核对首像素与 live/peak。

Size=21 经过非零对齐前缀，Size=20 经过五次 payload dword；
两者累计 live/peak 分别为17/18。尚未逐访问审计这两种尺寸。

该阶段 Linux core `199/199`（`proc_2c95`）、ASan core `199/199`
（`proc_a01c`）及 Linux app `205/205`（`proc_601e`）通过。

真实资源读取：六份 TSW 均存在时，测试经
`LegacyTswRuntime::query_cached(347, 2)` 和 `(9040, 3)` 取得
`all_char.tsw` 24×16 与 `all_sys.tsw` 7×7 帧；各自转换后
16-bit 指令流由缓存持有，借入匹配的 lookup reply token 与
source span。合成 allocator raw owner 分别容纳 768 与 98
像素字节，逐字节比较现有 `decode_legacy_image_command_stream`
的结果。第一帧 Linux core `199/199`（`proc_b859`）、ASan
core `199/199`（`proc_369c`）、Linux app `205/205`（`proc_a6e9`）
通过；增加第二帧后 Linux core `199/199`（`proc_7b58`）、
ASan core `199/199`（`proc_9c8d`）、Linux app `205/205`
（`proc_b764`）通过。

测试宏已改为仅在全部六份 TSW 文件存在时
定义的 `OPENSWD3_REAL_TSW_ROOT`；宏切换后 Linux core `199/199`
（`proc_545b`）、ASan core `199/199`（`proc_e5e9`）、Linux app
`205/205`（`proc_7af7`）通过；core compile command 亦核对该宏。

第三张原生 16-bit 小帧来自 `all_sys.tsw` `(9128, 0)`，
16×16 / 512 像素字节也完成相同的缓存借用与逐字节比对；
增加该帧及大帧停点后 Linux core `199/199`（`proc_fa76`）、
ASan core `199/199`（`proc_5a32`）和 Linux app `205/205`
（`proc_0d6b`）通过。这只证明缓存源字节可在受控 port 回复内解码，
尚非真实 actor lookup port 的生产接线或原版差分。

另取真实 `all_char.tsw` 资源 `(1, 0)`，物理尺寸47×95、
原存储8-bit；其转换后画素要求8,930字节，大于合成小池阈值
`0x3F8`。不提供外部 decoder 的伪造成功回复时，现有实现
明确停在 Win32 `HeapAlloc` 合成端口 `0x0077A200`，
ESP 为父入口栈顶减148，EAX/EDX 均为按16字节向上取整的
8,976 字节、ECX 为 heap token `0x00780000`，CALL return
`0x0048AA65` 已压栈，无像素写入；强化参数断言的 Linux core
`199/199`（`proc_a40f`）、ASan core `199/199`（`proc_6148`）、
Linux app `205/205`（`proc_f1e4`）通过。

此前单列阶段 Linux core
`199/199`（`proc_23dc`）、ASan core
`199/199`（`proc_0836`）通过；最新三帧版的三门结果见上段。

对于该同一真实大帧，显式注入 Win32 `HeapAlloc` 非零返回 token
并提供独立 raw block owner 后，按 stdcall 移去一条 CALL 返回槽与
三项实参，借同一 `+0x00487E75` raw-local 后缀继续走分配头、
统计、guard、像素指令与返回栈，8,930 像素字节逐字节匹配
renderer 既有解码器，外部整体 decoder port 调用数为零。

零返回独立走 `0x00487E7E XOR EAX,EAX`。未绑定保存栈时
停在 `0x00487FDC POP EDI`；绑定后依次读三处保存寄存器、
父 EBP 和返回地址，写零到 `0x00487C9C` 的局部槽，
在 `0x00487CA5` 重试实参首读前停住。三个 POP 分别设置
访问前停点；零路径未增加 heap request counter。
这段版本 Linux core `199/199`（`proc_2b9d`）、ASan core
`199/199`（`proc_6a26`）、Linux app `205/205`（`proc_2dc5`）通过。

按原 LST，本合成向量的重试实参非零：绑定原 wrapper 来源后，
从 `0x00487CA5` 首读进入 `0x00487CB0` Size 读取、两次压栈、
`sub_48A900` 的保存 EBP/ECX，然后在回调全局 `0x0053D1B8`
首读前停住。七个独立访问前停点均已加入；显式绑定零回调值
只能推进到 `0x0048A909` 局部写之前，不伪造后续返回。
该版本 Linux core `199/199`（`proc_389e`）、ASan core
`199/199`（`proc_de5c`）、Linux app `205/205`（`proc_61f4`）通过。

显式零回调值绑定 `sub_48A900` 局部槽后，按顺序写/读
`0x0048A909/0x0048A90C`、XOR 零返回并读保存 EBP/RET，
经 `0x00487CB9` 清一参、TEST 零及 XOR 零，停在父 wrapper
`0x00487CC8 POP EBP` 首读前；五个局部和返回访问分别可停。
非零回调 token 则停在 `0x0048A912` Size 首读前，不调用未绑定函数。
这一版本 Linux core `199/199`（`proc_271f`）、ASan core
`199/199`（`proc_f421`）、Linux app `205/205`（`proc_adbe`）通过。

零回调的父 wrapper 进一步按 `0x00487CC8/0x00487CC9`
恢复 EBP/RET，`0x00487C28` 清五参，再按
`0x00487C2B/0x00487C2C` 恢复外层 EBP/RET；decoder
`0x00401A0B` 清 Size 参后停于 `0x00401A0E` 首次
资源命令字读取。四处 POP/RET 及源首读分别可停；当时较宽松的断言下，
Linux core `199/199`（`proc_b37b`）、ASan core `199/199`
（`proc_3310`）、Linux app `205/205`（`proc_3824`）通过。

进一步逐点核对 status/token 时，`proc_c35c` 和 `proc_f029`
均为 `198/199`：同一测试中的父 wrapper 前两处 POP/RET token 错位。
对照 LST，`0x00487CC6 MOV ESP,EBP` 必须在
`0x00487CC8 POP EBP` 之前恢复 ESP；补回该指令的可观察
寄存器效果，并断言首 POP 的 `ESP=EBP`、外层 POP 的
`ESP=EBP+0x1C`。最新 Linux core `199/199`（`proc_1916`）、
ASan core `199/199`（`proc_9092`）、Linux app `205/205`
（`proc_4e58`）通过。零输出指针后的命令执行、
非零回调调用及重试成功路径仍未验收。

另一条合成 Win32 零回包向量把 wrapper 的 allocator 实参
`0x0053D1B4` 设为零：`0x00487CA5` 比较后不调用
`sub_48A900`，`0x00487CAB` 从此前写过的局部槽读回零，
和零回调路径共用 `0x00487CC6` 起的两层返回后缀。
`0x00487CAB/CC8/CC9/C2B/C2C/00401A0E` 六处分别设置
访问前停点并核查物理 token，heap request counter 不增加。
Linux core `199/199`（`proc_b6ec`）、ASan core `199/199`
（`proc_985f`）、Linux app `205/205`（`proc_ba64`）通过。
原版生产环境中该 allocator 实参是否为零尚无动态值证据。

两条零回包共用返回后缀到 `0x00401A0E`；仅显式绑定的
16 位命令流才从匹配的 source owner 读首 word，按 LST 的
16 位 CMP 更新 FLAGS、将零 EAX 移给 ESI。首字非零则
在 `0x00401A1A` 第二 word 首读前停；零则在
`0x00401B62 POP EDI` 首读前停。真实大帧选择条件按
流字节计算；合成 `0x8000` 和零首字分别验证 sign/zero
及 ESI/ESP，没有对空输出块写像素。
Linux core `199/199`（`proc_7347`）、ASan core `199/199`
（`proc_fb4f`）、Linux app `205/205`（`proc_8fc1`）通过。
第二 word 以后各读写、空输出指针的异常前缀仍未验收。

显式绑定第二个资源 word 后，非零首字从
`0x00401A1A` 读取行命令，按 `0x00401A1E–3D` 更新
EDI/EBP/ECX、DX 和 16 位 TEST FLAGS。合成的零、字面、
`0x8000` 填充、`0xC000` 填充、`0x4000` 跳过五类行标记
分别停在 `0x00401AAB/45/69/8B/9E`；两种填充
停在未绑定的全局颜色读，未伪造输出。第二 word 读取前
可独立停点，寄存器保持原值。Linux core `199/199`
（`proc_bb9e`）、ASan core `199/199`（`proc_3473`）、
Linux app `205/205`（`proc_02f2`）通过。

字面行再显式绑定 `0x00401A45 MOV BX,[EDI]` 的合成
资源 span：先按其独立访问序号停在源 word 之前，再读出
合成 `0x1234`、执行 `0x00401A48 ADD EDI,2`，
于 `0x00401A4B MOV [ESI],BX` 输出写之前以新的
`decoder_output_write` 类型停住。此时 ESI 和输出 token
均为零；原始程序的异常结果未动态捕获，不能据此宣称
已经完成异常等价。Linux core `199/199`（`proc_604a`）、
ASan core `199/199`（`proc_bf85`）、Linux app `205/205`
（`proc_4a35`）通过。

非零重试回调 token 不作函数指针调用。显式绑定
`sub_48A900` 的 Size 栈参后，可按 LST 读取
`0x0048A912 [EBP+8]` 并由 `0x0048A915` 压入该 Size，
在 `0x0048A916 CALL [EBP-4]` 读间接目标前停；
三处访问均有独立序号、状态和物理 token 断言。
此向量 Size 为 8,930，heap request counter 保持原值。
Linux core `199/199`（`proc_e74c`）、ASan core `199/199`
（`proc_4f01`）、Linux app `205/205`（`proc_6d7f`）通过。
回调体、回调返回及可能重试的再次分配仍未绑定。

进一步显式绑定同一局部槽的间接目标首读；在
`0x0048A916` 分别可停于 `[EBP-4]` 指针读取和
`ESP-4` 返回地址写入。写入返回地址 `0x0048A919`
后令 EIP 指向合成回调 token，以 `heap_retry_callback_typed_stop`
标记未绑定回调体，不执行该地址的代码。三种访问前/入口
状态均有物理 token、ESP 与 Size 断言。
Linux core `199/199`（`proc_619f`）、ASan core `199/199`
（`proc_3f24`）、Linux app `205/205`（`proc_90e2`）通过。

为非零回调增加显式的 opaque 端口：只有回复
`returned=true` 才按 cdecl RET、`0x0048A919` 清一参、
`0x0048A91C` 测试 EAX 并压缩为 0/1，随后恢复
`sub_48A900` 的 EBP/RET 和父 wrapper 实参。
合成端口的零回复停在 decoder `0x00401A0E` 首读前，
非零回复停在 `0x00487C84` 下一轮分配参数首读前；
`returned=false` 保留回调目标入口停点。
三条向量各记录端口调用次数与 Size=8,930，不宣称
生产回调、再次分配或动态异常已闭环。Linux core
`199/199`（`proc_f6ec`）、ASan core `199/199`（`proc_2ab8`）、
Linux app `205/205`（`proc_002c`）通过。

另将合成源格式改为 8 位并保持 Win32 零回包：
`0x00401ACB` 首次读取命令 word 与 16 位路径
使用相同的源租约/尺寸边界，但非零首 word 必须停在
`0x00401AD7` 第二 word 首读前，不能误用 `0x00401A1A`；
零首 word 则到 `0x00401B62 POP EDI` 首读前。
合成 `0x8001` 与零分别断言 CMP16 的 SF/ZF，ESI/EAX
仍为零。Linux core `199/199`（`proc_4c5d`）、ASan core
`199/199`（`proc_e10b`）、Linux app `205/205`
（`proc_f022`）通过。8 位行命令与零输出首写待续。

8 位零回包继续按 `0x00401AD7–0x00401B05`
读取行 word，零、字面、`0x8000`、`0xC000`、
`0x4000` 五类标记分别停在
`0x00401B58/02/1E/3C/4B`，两处全局字节
颜色读不伪造。字面资源的合成首字节 `0x7D`
在 `0x00401B02` 可单独读障；读出后经
`0x00401B04 INC EDI`（保留 CF），以 ESI=0
停在 `0x00401B05 MOV [ESI],BL` 写之前，
没有解引用零指针。Linux core `199/199`（`proc_42c0`）、
ASan core `199/199`（`proc_f44e`）、Linux app
`205/205`（`proc_9d3d`）通过。

两种格式在零回包且合成首命令 word=0 时，LST
`0x00401B62–66` 依次 POP EDI/ESI/EBP/EBX、RET。
五处独立栈读停点核查 EIP/ESP token；完整 RET 后
EAX=0、`decoder_child.returned=true`，停在父级
`0x00479B4B` actor token 写之前。该路径只保留前一
CMP16 的 FLAGS，不错误地套用其他早退路径的 XOR
FLAGS。最初两个 core 诊断 `proc_9706/proc_1d1a`
因测试的访问序号多加一而失败；按首 POP 前的
`accesses_completed` 修正后，Linux core `199/199`
（`proc_042e`）、ASan core `199/199`（`proc_bfed`）、
Linux app `205/205`（`proc_0d81`）通过。父级 token
零写与下游仍待单独验收。

用两种格式的零回复再触发父级 `0x00479B4B`：
访问前停点保留原 actor `+0x0E14` owner 与像素 span；
显式放行后该 token 写为零、emitter 像素 span 变空，
然后停在 `0x00479B51` 下一处 actor 读之前。
这证明不会在 decoder 零回复时跳过父级写；未证明
原版动态输出或该 actor 读以后的渲染行为。
Linux core `199/199`（`proc_2e6d`）、ASan core
`199/199`（`proc_52f1`）、Linux app `205/205`
（`proc_5009`）通过。

零 token 写完后，两种格式再显式调用父级尺寸阶段，
分别在 `0x00479B51 MOV EAX,[actor+0x2548]` 的
实际 actor 读前停住；EAX/ESP 不变，已写的 `+0x0E14`
仍为零。Linux core `199/199`（`proc_f108`）、ASan
core `199/199`（`proc_439a`）、Linux app `205/205`
（`proc_aadb`）通过。资源指针进一步解引用与后续
图像呈现尚未由此向量验收。

两种格式各取 `0x8001/0xC001` 填充行，显式绑定
`0x004CDE20/0x004CDE78` 16 位颜色或
`0x004CD780/0x004CD7B4` 8 位颜色。四处全局颜色读均可独立停点；读出后分别
在 `0x00401A70/A92` 或 `0x00401B24/B42` 的
`[ESI]` 写前以 ESI=0 停住，低 16/8 位颜色值与
LST 的 MOV BX/BL 一致，heap 计数仍未增加。
Linux core `199/199`（`proc_93e1`）、ASan core
`199/199`（`proc_adde`）、Linux app `205/205`
（`proc_cbb2`）通过。未执行对空输出块的原版写入。

修改 allocator token 不得误用该 Win32 回包。大帧正常返回时
heap 总量为8,928，实时与峰值均为8,927，request counter 加一；
guard 尾首末字节保持 `0xFD`。上一版本 Linux core `199/199`
（`proc_5a64`）、ASan core `199/199`（`proc_ec54`）及 Linux app
`205/205`（`proc_dbb8`）通过。该回包仍为受控注入，
并非生产 Win32 分配 port 的真实所有权接线。

受控 Win32 分配端口核对：对真实 TSW 47×95 帧，
`0x0048AA5F` 的间接目标 `0x0077A200` 收到
heap token `0x00780000`、标志0和对齐大小8976，
CALL 入站 EAX/EDX=8976、ECX 为 heap token。
`returned=false` 保留间接 callee 入口停点；零回复
在 `0x00487FDC` 的保存栈读前停住，未写堆块或增加
请求计数；非零但短于大小的 raw owner 也保守停住。

非零回复 `0x00804000` 携带 9000 字节租约时，
8930 字节实际写在同一个 raw owner `+0x20`，与
renderer 字节相等，原合成块全为哨兵且未改写。
端口释放自己的 owner 后，结果租约仍保留这些像素。
此处的 heap、callee 和 raw token 仍是受控合成值，
没有生产 Win32 HeapAlloc 后端或资源记录 token 注册，
也不将 owner 长度不符时的宿主停点称为原版故障。
Linux core `199/199`（`proc_220d`）、ASan core
`199/199`（`proc_6804`）、Linux app `205/205`
（`proc_e863`）通过。此边界只是分配调用的局部接线。
内部 decoder 的 16-bit 返回后，在父级
`0x00479B4B` token 写完成才将同一 raw owner 的
`+0x20` 可写 `u16` 像素租给 emitter；token
改写会释放旧租约。受控真实帧核对像素字节及
双向写别名；同 token 重写保留租约，异 token
写释放租约。当前代码 Linux core `199/199`
（`proc_f86a`）、ASan core `199/199`
（`proc_b945`）、Linux app `205/205`
（`proc_e5cc`）通过。8-bit 和外部无租约 span
的生产别名尚未统一收口。

后续新增 `LegacyBattleActorFrameHostHeapPort` 平台适配后端：
仅当注入的 IAT 目标、heap 身份、零标志和 `0x0048AA39–52`
对齐后的字节数一致，才在 `0x0048AA5F` 交付独立 raw 租约；
guest token 与 TSW 缓存记录及命令流共用单调地址预留入口，
当前动态池为 `[0x50000000,0x70000000)`，避开已知世界物品
sentinel。`proc_b231` 首次因八次复用的假租约 token 夹具失败，
修正夹具后定向 core CTest `battle.actor_frame_316` 1/1 通过
（`proc_1976`）；但拆分目标遗漏 `OPENSWD3_REAL_TSW_ROOT`，
该轮并未编译、执行真实 TSW `(1,0)`／宿主堆 47×95 断言，
不得据该轮宣称资产字节验证完成。拆分目标补宏后首次
`proc_eb0b` 因测试夹具混用合成/真实 token 失败；第二次
`proc_93f1` 定位到 decoder 第四栈参数仍是旧合成 token，
实际在 `0x004019AD` 读该 token 停下。同步记录、主流及
第四栈参数后，真实 TSW 三个小帧与 `(1,0)` 47×95 帧
共 8930 像素字节通过定向 `battle.actor_frame_316` 1/1
（`proc_2c26`）；独立外部 raw 像素 span 与 guest token
同主租约、冲突 token 不误借 owner 也在同次通过。
随后把显式冲突租约残留的无主 emitter span 清空，
`proc_b2d3` 定向 `battle.actor_frame_316` 1/1 复验通过。
外部 raw 块现仅在 EAX、span 与宿主 owner 同址且合法时
登记其完整 guest 字节区间；同一 owner 可重复借用，
不同活跃 owner 的交叠范围、与既有动态地址交叠均拒绝，
外部 owner 销毁后可重用，动态池能跳过未来已登记区间。
这些范围和借用断言经定向 `battle.actor_frame_316`
core 1/1（`proc_6b22`）和 ASan 1/1（`proc_7f21`）
通过；后续发现把越过外部区间的动态高水位整体当作占用会
误拒绝空洞重登，现仅以实际动态分配区间判断碰撞；
外部 owner 释放后的池内空洞复用经定向 core 1/1
（`proc_661e`）、ASan 1/1（`proc_4a28`）通过。
原版 PE 头与既存 `p4-oracle-runtime-baseline.tsv` 均确认
ImageBase `0x00400000`、SizeOfImage `0x001A7000`、
无重定位目录：已拒绝外部 raw 与固定镜像
`[0x00400000,0x005A7000)` 的全/部分交叠，
包括镜像尾部跨界；定向 core 1/1（`proc_b10c`）和
ASan 1/1（`proc_6e40`）通过。以上均非完整门禁；
仍未登记其它模块/固定 guest 地址，不能宣称全进程地址无碰撞。
未交付显式租约的外部合成 span 仍仅限注入测试。
宿主分配端口先保留 guest 地址再尝试宿主分配；无法容纳的
`0xFFFFFFF0` 字节请求只给入口停点、不尝试巨量宿主分配；
宿主 OOM 时 guest token 不回收且不伪造 Win32 失败回复。
该边界的定向 core 1/1（`proc_ba42`）、ASan 1/1
（`proc_bbca`）通过；未覆盖原版 Win32 失败语义。
这是宿主堆适配和真实资产字节验证，不是原版 Win32 寄存器/异常差分，
也未接通四处 caller 的真实栈与 owner；外部别名、8-bit、
异常栈及完整双向 REVIEW 仍未收口，不能据此验收 316。

未出现为 C++ 字面量的 20 个基本块起点已逐项对照：
其中十个为 CMP AX,BX 合流分岔，五个为
阈值／低16位算术，三个为 case100 signed 余数，
两个为 selector 表入口。无字面量不证明缺块，
但这也未覆盖尚未审查的块；
`build/workpack316/bidirectional-review.md` 是工作记录。

逐块核对 `0x00479978–84` 时发现旧实现只把
`[EDX+0x0C]` 解析到 actor `+0x0C` 资源字节；
此时 EDX 是刚重读的 actor `+0x2548` 帧记录 token，
原 LST 并未要求它等于 actor `+0x0C` token。
现优先从已发布且 token 匹配的帧记录读取其低16
宽度；只有帧记录未绑定而 token 与 actor `+0x0C`
确实相等时才读后者的原始资源字节。分离 token 的
受控向量读出帧宽0x0150并计算 EBP=0x012D，
维持旧 actor 资源字节0x0145；将此物理读设为
不可访问则停在 `0x00479980`，尚未写 `+0x2694`。
两宿主 owner 同 token 但宽度矛盾的处理仍需审计。
Linux core `199/199`（`proc_9c8b`）、ASan core
`199/199`（`proc_7070`）、Linux app `205/205`
（`proc_232b`）通过；原版差分尚未执行。

同 token 的两个宿主 owner 若给出冲突的宽度，
不任选一方制造不存在的原版同地址异值，而在
`0x00479980` 保守停住；两个宽度一致时正常前进。
访问序号提前停点不会预读 profile 字节。
Linux core `199/199`（`proc_9d6c`）、ASan core
`199/199`（`proc_2cf7`）、Linux app `205/205`
（`proc_f0bb`）通过。这是宿主一致性约束，
不是已观测到原版异常或生产 token 注册。

生产 lookup 边界核对：`0x00479940` 的父 CALL 经
`LegacyBattleActorFrameUpdatePort::lookup_frame` 接到
`sub_4315D0`；该阶段 `src/battle/` 尚无该抽象端口的生产实现。
`LegacyTswRuntime::query_cached` 的 `LegacyTswFrameView` 是 span；
每次查询前可淘汰节点，`clear_cache` 也会清除缓存所有权。
查询结果现带 `frame_owner` 租约，战斗 lookup 回复可携带该
owner 到 decoder；decoder 核验租约主流与回复 span 的首址和
长度一致，不一致停在 `0x004019AD`，不借错流。

后续主工作树增加了尚未发布的
`LegacyBattleActorFrameTswUpdatePort`：对无辅助流/调色板的
正常 TSW 查询分别交付 32-bit 节点记录 token 与主流
地址 token，并把帧 owner 放进 decoder source。
LST `sub_431DF0` 表头命中时 ECX 是压缩键；非表头命中
在 `0x00431E7C` 读旧桶首节点、移链后保留该节点地址
于 ECX，EDX 留桶地址；正常缺失装载经 `sub_431C50`
于 `0x00431DCC` 读节点 `+0x18` 主流长度留于 ECX，
EDX 为累计缓存字节数。真实 TSW `(1,0)` 和 `(11,0)`
同桶查询实测：缺失装载后节点／主流 token 不复用，
重新查 `(1,0)` 为非表头命中，ECX 为 `(11,0)`
记录 token 减 8，EDX 为 `0x004CF86C`，
FLAGS 来自父级嵌套回复的 TEST。
当前主树 Linux core `199/199`（`proc_31ef`）、
ASan core `199/199`（`proc_5913`）、Linux app
`205/205`（`proc_08e2`）通过。仅正常 lookup 路径
有独立局部验证：`sub_4315D0` 初始化／异常前缀、
特殊记录、主循环注入、共享 guest 地址注册仍未完成；
不能将此 CALL 标作完整 reviewed 或 316 已验收。

父级 lookup 正常返回时，资源头与 source token、租约主流
首址和长度均一致才把租约交给 actor 资源记录；即使
`0x00479945` 的独立 actor 写随后故障，callee 已发布的
资源和 owner 仍保留。伪造子 span 不会被保存为记录租约。
当 `+0x2548` 被其他物理写改成不同 token 时，已有记录
租约随资源头失效；上一动作的资源发布若没有对应帧租约，
也清除旧 owner，避免旧源错配新 token。这些是宿主借用
生命周期行为，不声称原版悬空指针仍可读取。
受控帧在 `0x00479945` 写障后保有记录租约。
同次路由若 caller 未提供显式绘制源，则从这个
记录租约向 `0x004170E8` 绘制首读自动借出
primary_stream；记录 token 改变就撤销借用。
受控 lookup／draw 回复证明无重复流缓冲和
首读故障仍可停于原 IP。Linux core
`199/199`（`proc_fb72`）、ASan core
`199/199`（`proc_f8da`）、Linux app
`205/205`（`proc_4173`）通过。显式传入 token
但缺字节的源不自动改借宿主 owner，仍于
`0x004170E8` 停住；当前代码 Linux core
`199/199`（`proc_a0de`）、ASan core
`199/199`（`proc_bda3`）、Linux app
`205/205`（`proc_1eeb`）通过。此处未实现
深层 `sub_4170E0` 绘制或真实 TSW 查询端口。
独立读取真实 TSW `(347,2)` 的 24×16 已转换帧，
通过受控 lookup 回包把它的 primary_stream
租给同次绘制路由；`0xFFFF` 首 word 走真实
格式首读，frame_owner 在 actor 记录中一致。
恢复后续测试夹具后 Linux core `199/199`
（`proc_ba6c`）、ASan core `199/199`
（`proc_ee13`）、Linux app `205/205`
（`proc_ab59`）通过。此向量不是原版动态差分，
也不是生产 lookup 后端或完整绘图像素验证。

端口与返回结果释放租约后，记录是唯一保活者，
仍能读取原字节；替换记录后旧弱引用失效。
该加强向量 Linux core `199/199`（`proc_8af3`）、
ASan core `199/199`（`proc_8048`）、Linux app
`205/205`（`proc_8e67`）通过。
故意注入只指向主流子 span 的回复时，记录不保存
该租约；组 A/B 相同 token 写保留租约，不同 token 写
释放租约；前一动作重发资源亦释放无源租约。
这些记录存活测试使用受控回复，不构成生产查询。
Linux core `199/199`（`proc_9ee4`）、ASan core
`199/199`（`proc_ed84`）、Linux app `205/205`
（`proc_2764`）通过。

同 token 的显式 decoder source 也不得绕过不匹配的缓存租约，
租约匹配时必须优先使用其主流。

合成所有权失配/借用测试通过；真实 `all_char.tsw` 大帧
`(1,0)` 查询后把原查询的租约释放、清缓存，保留在 lookup
回复中的租约仍支撑受控解码。Linux core `199/199`
（`proc_7297`）、ASan core `199/199`（`proc_9603`）、Linux app
`205/205`（`proc_4e19`）通过。再加入同 token 的冲突输入
和失配停点，Linux core `199/199`（`proc_32c6`）、ASan core
`199/199`（`proc_d5b1`）、Linux app `205/205`（`proc_34f0`）
通过。TSW 首查询不是生产 `lookup_frame` 端口；注入的
资源 token 仍是合成值。

LST 的资源返回记录 `+0x00` 指主流，`+0x04` 由当前加载
路径置零，`+0x0C/+0x0E` 是宽高。生产接线仍需验证资源
record token、字节流与分配输出别名指向稳定 owner，
不可用合成固定 token 冒充原版地址。

case100 `0x0047B6A1 IDIV ECX` 到
`0x0047B6A3` actor X 读取之间没有定义
算术 FLAGS 的指令。原 C++ 延续了此前
INC 的 FLAGS 可知状态，现改为 IDIV 后
`flags_known=false`；`0x0047B6AA/AC` ADD/SUB
后恢复为 true。故障向量在读 X 前保留
已递增的 phase、商/余数和未知 FLAGS，
在 `0x0047B6AE` 写 X 前确认 FLAGS
重新已知、仍未写新 X。Linux core
`199/199`（`proc_e058`）、ASan core
`199/199`（`proc_89c4`）、Linux app
`205/205`（`proc_aeef`）通过。

四处 caller 后缀的局部 FLAGS 审计：主动作
case7 在 `0x004554FB CMP EAX,EBX` 成功
（EBX=1）的结果进入 `sub_478710`
首参数读前，AF 有定义，原上级代码却用逻辑零
结果将其标为未定义；已改为减法零 FLAGS。
对手动作 case7 的 `0x00456521 XOR EBP,EBP`
位于 `0x00456519 CMP` 后，首参数读前 AF
反而未定义，原上级代码错误标为有定义；已按
XOR 零结果修正。各自用未绑定 `sub_478710`
首读停点核对后继 FLAGS。最终组 B 的
`0x0045ACCD/D1` 两枚 LEA 后，`0x0045ACD5`
XOR 清 EBX；四个物理 PUSH／父栈写停点
保留 EAX=`P+0x14`、EDX=`P+0x18`、
ZF/PF=1、AF 未定义，单测逐站核对；
ECX 及更深后缀仍未收敛。当前代码 Linux core
`199/199`（`proc_f7e9`）、ASan core
`199/199`（`proc_3b10`）、Linux app
`205/205`（`proc_5892`）通过。此为局部
caller 故障前缀，不代表四处生产 caller 完整
入站／后缀通过，也非原版动态差分。

`0x0047B6D5` 的 phase100 写入目标物理地址为
`[ESI+0x2958]`。原宿主继续函数在 request.actor_token
与入口 ESI 冲突时仍改写旧 actor 的 phase；
现于该写点保守停止，故障 token 用 ESI 推得，
不提前提交 phase 或四次 POP／`0x0047B6E7` RET。
冲突 token 与原正常 EAX1 路径测试已通过；
Linux core `199/199`（`proc_ee34`）、ASan core
`199/199`（`proc_7ce3`）、Linux app
`205/205`（`proc_339f`）通过。此冲突测试
是注入的宿主归属边界，不是原版单地址运行差分；
其余 RET 及 ESI 归属继续保持审计未完成。

相邻音频后写点 `0x00479C5E` 的操作数是
`word [ESI+0x2958] <- 1`，而
`0x0047A746` 是 `[ESI+0x2958] <- BP`。
原共用宿主路径无条件向 request.actor_token 写 1；
现对两个物理点分别保留 ESI 冲突写障，
正常匹配时 case2 写 1、case8 写 BP 低 word。
冲突 ESI 与受控非 1 BP 的测试均通过；
Linux core `199/199`（`proc_0c8c`）、ASan core
`199/199`（`proc_b3f9`）、Linux app
`205/205`（`proc_6b0e`）通过。
这仅证明两处局部前缀和输入寄存器的受控结果；
原程序 caller 没有被执行，不能推论任意 BP
值在实战中可达，其他写点仍需审计。

case8 首个 `0x0047A5FE CMP [ESI+0x2958],CX`
之前，宿主仅持有 request.actor_token 对应 actor；
ESI 冲突时不得借用其 phase 计算 CMP。
已在该读点返回实际 ESI 派生的故障 token，
不读取旧 owner，也不进入第二个
`0x0047A60B` 源 token 门；受控测试通过。
Linux core `199/199`（`proc_f2de`）、ASan core
`199/199`（`proc_b83c`）、Linux app
`205/205`（`proc_1b8a`）通过。
本测试只证明冲突输入的保守停点，
不表示原版能同时为同一物理地址提供
矛盾宿主 owner。

case9 首个 `0x0047A752 MOV AX,[ESI+0x2958]`
先于有符号 45 门及后续 `0x0047A253/5A`
清零。原宿主在冲突 ESI 下仍读取旧 actor
并可能错误清零其 phase/motion；现于首读前
保守停住，报告 ESI 派生 token。冲突输入
保持 phase=45、motion=0x1234，访问序号为零；
Linux core `199/199`（`proc_0317`）、ASan core
`199/199`（`proc_39ca`）、Linux app
`205/205`（`proc_753f`）通过。仍是
宿主归属故障前缀，不是原版运行差分。

`0x00487CC4 JMP 0x00487C84` 已按原回边
重新执行 wrapper 栈读、四次压参和第二轮分配，
不复用首轮注入的零回包。受控回调非零时，
在回跳后的 `0x00487C84` 首读前可独立停住，
ESP=`EBP-4`、EAX=1、TEST 的 ZF=0；
无新 Win32 回复停在第二次 HeapAlloc CALL，
ESP 与首轮 CALL 相同，堆计数未写。
有注入式新端口时，第二次返回失败仅停在
第二次 CALL；非零回复须持有足量 raw owner，
其租约进入解码结果。另测两次非零回调
导致第二／第三次各自调用分配端口，
及全程只由端口提供零／非零回复的路径。
真实 TSW `(1,0)` 47×95 帧还验证：首次零回包、
一次非零回调、第二次分配用不同的 32 位 token
和独立 raw owner 后，8930 个像素字节恰好写入
新块 `+0x20`，旧测试块仍全为原填充值，
堆请求计数只因非零分配增加一次，原生 decoder
正常抵达父级 `0x00479B4B` token 写前。
Linux core `199/199`（`proc_be39`）、ASan core
`199/199`（`proc_2def`）、Linux app
`205/205`（`proc_dc11`）通过，`git diff --check` 无误。
这些只证明受控本地前缀、像素和租约，既不是
生产 Win32 HeapAlloc 实现，也不是原版动态差分。

四处 `sub_47CE70` 调用
`0x00479C1C/0x0047A70E/0x0047B634/0x0047B8D0`
已单独复核 LST 三指令叶子及各父级 CMP/OR：
case2/case51 为 byte OR，case8/case100 为 word OR，
对应低 bit0 置位与未置位。四组受控向量逐点
检查 CALL 压栈、`[ECX+0x2694]` byte 读、
RET 栈读、父级 OR 字段读与写的地址、访问序号、
ESP、EAX/ECX/EDX、CALL 返回地址、
CALL 前 FLAGS/DF 保留和 AND/CMP 条件 FLAGS；
case8 word OR 和 case51 byte OR 均保留高 byte。
`call-audit.tsv` 的四行仅标记上述局部
leaf/父级 OR 前缀；未把后继 Metrics/decoder、
其余 CALL 或整包列为 REVIEW 通过。
当前 Linux core `199/199`（`proc_5274`）、
ASan core `199/199`（`proc_e30f`）、Linux app
`205/205`（`proc_0356`）通过。

case3 首次矩形 `0x00479D39` 的四参
在受控资源宽5、高7、位置(43,27)下按
bottom/right/top/left 顺序压栈；`sub_416FF0`
以固定 640×480 surface 对 left/top/right/bottom
进行 signed 钳位，按 clip left/height/top/width
四次独立写全局，EAX=1、ECX=2、EDX=24
并恢复 ESI/EDI，RET 回 `0x00479D3E`。
该物理 caller 的 15 个内存/栈故障序号
各自核对 EIP、token、ESP、访问序号、FLAGS/DF、
四个全局写提交前缀，后八处另核对寄存器；
`0x00417041 MOV EAX,1` 先于 `0x00417046 POP ESI`，
最初测试误把第13号故障 EAX 当7，定位后将
期望改为1，生产实现未改动。通用叶子的
signed 钳位向量另覆盖负值；本 caller 后续绘制
和其他20个矩形 CALL 仍待完整 REVIEW。
最新 Linux core `199/199`（`proc_239a`）、
ASan core `199/199`（`proc_12a8`）、Linux app
`205/205`（`proc_c83b`）通过。

case100 指标对 `0x0047B662/0x0047B666`：
先 `0x0047B649 PUSH EDI=1`，再读 IAT，
三次写 actor `+0x0E40/+0x0E44/+0x0E48=15`
才进入第一次 `GetSystemMetrics`；受控回包
720/1280 各由 stdcall 清除各自 nIndex 和返回槽，
高度先压栈、第二次 nIndex=0 后才压宽度。
两个外部 CALL 邻近的五个 PUSH 故障点逐一核对
EIP、物理栈 token、ESP、调用次数、EAX/ECX/EDX、
受控 FLAGS/DF 与三个已写字段；另测无回包不
清栈、第二次调用前保留高度。原版 Windows
callee 内部不可见，生产指标后端和后续矩形
仍为 partial。Linux core `199/199`（`proc_d43a`）、
ASan core `199/199`（`proc_1b98`）、Linux app
`205/205`（`proc_4992`）通过。

首组三处 parent RET `0x0047991F/0x00479936/
0x00479B05` 已把 LST 的 EAX 写序、四 POP、
`ADD ESP,0x14`、RET 同实现和测试逐点互核。
在入口 ESP=`0x0012FF00` 下，每处 RET 读障
报告 token=`ESP`、未提交弹栈，EAX 分别为
1/1/0，正常返回 ESP=S+4；首两处第一 POP
之后才有 `MOV EAX,EBP`。核对每次 POP 的
现场寄存器、FLAGS/DF、独立访问计数与故障
提交前缀。曾把测试 `forward` 的 DF=false
误当原始 input 的 true；仅纠正断言，生产代码
未改。此为三处尾部局部 REVIEW，前置 reset、
lookup、音频/绘制 child 与其余19处 RET 尚未关闭。
Linux core `199/199`（`proc_0809`）、ASan core
`199/199`（`proc_5b50`）、Linux app `205/205`
（`proc_d455`）通过。

共享矩形返回尾的 `0x00479EA9/0x0047AF23/
0x0047B2E7` 也逐处与 LST 核对：callee 返回后
`ADD ESP,0x10; XOR EAX,EAX; POP EDI/ESI/EBP/EBX;
ADD ESP,0x14; RET`。三个物理 RETURN 各覆盖四
POP 与 RET 的独立 token、ESP、GPR、FLAGS/DF、
访问序号和正常 EAX0。首次将三个合成前缀
误套用固定入口 S=`0x0012FF00` 的 PF/AF，
只在 RET 第4号故障失败；改由各自
`prefix.esp+0x34` 按 ADD 14h 推旗后全部通过。
这证明共享尾局部逻辑，不证明合成前缀的
parent 栈归属或三个图像链已完结。
Linux core `199/199`（`proc_b231`）、ASan core
`199/199`（`proc_26dc`）、Linux app `205/205`
（`proc_3f55`）通过。

case5/10 `0x0047A19F/0x0047A934` 的共享
phase word 读写、ADD ESP28h、四 POP 与 RET；
case6 `0x0047A252` 的 word DEC、XOR EAX0
及尾部；selector default `0x0047A814` 的
前三 POP、XOR EAX0、第四 POP 与尾部，
均已局部比对 LST 和 C++。case5/10 各七
站点与 default 五站点核状态、token、ESP、
EAX/ECX/EDX、保存寄存器、FLAGS/DF；
case6 补齐 RET 读障及正常回包旗值。
十处 RET 仅为各自尾部局部 REVIEW，
其余12处和各自上游 child／parent 所有
故障仍未验收。Linux core `199/199`
（`proc_840b`）、ASan core `199/199`
（`proc_dff3`）、Linux app `205/205`
（`proc_a276`）通过。

另四处尾部 `0x0047A94C/0x0047AA7A/
0x0047B408/0x0047B800` 分别对应
case5/10 达高度、case11、case15、case50。
前者测试两条路径各五处 POP/RET 栈读障，
包括第三 POP 后 XOR EAX0；后三者均在
word DEC/INC 的读写完成后 XOR、四 POP、
ADD ESP14h 才 RET。受控测试核正常 EAX0、
保存寄存器、ESP/FLAGS/DF 与末次 RET 故障的
token、访问序号和不提前返回。只算四处局部
尾部 REVIEW，其余8处 RET 与 child/caller
仍未完成。Linux core `199/199`（`proc_ac55`）、
ASan core `199/199`（`proc_413a`）、Linux app
`205/205`（`proc_0e63`）通过。

续核 `0x0047AB99` case12 主绘图完成尾、
`0x0047B517/0x0047B537` case100 计数／短路径尾，
以及 `0x0047B6E7` case2 粒子成功尾。
case12 两种方向各五个 POP/RET 栈障核
EAX0、逐步恢复 GPR、ESP、栈 token、访问
序号、XOR 和 ADD flags/DF；case100 计数尾的
`0x0047B508 POP EDI` 位于 `0x0047B509`
actor 写之前，后三 POP 位于写后，INC 保留
先前 CF；短尾独立覆盖四 POP/RET。
case2 粒子成功先写 phase100，再三 POP、
`0x0047B6E1 XOR EAX,EAX`、第四 POP、
ADD ESP14h 与 RET；发现 C++ 在 XOR 后
未把未知 callee FLAGS 改为确定值，已补
`flags_known=true` 并测未知入站 flags、
五个 POP/RET 停点和正常出口。
这些仍是四处尾部局部 REVIEW；上游
child、绘图、资源来源和 caller 异常栈仍未收敛。
最新 Linux core `199/199`（`proc_7202`）、
ASan core `199/199`（`proc_2755`）、Linux app
`205/205`（`proc_e1e9`）通过；当前仅余四处
RET 尚未获得局部尾部结论，整个函数仍
`pending_audit`。

此前 case51 扫描 `0x0047B98D` 只核正常返回
及首 POP `0x0047B984`、末 RET 的故障 token、
ESP、FLAGS 和寄存器；当时不能逐个注入中间三
POP：有限 `stop_before_access` 在尚无父级
序号映射的 `sub_4344E0` 子调用入口停住。
首次尝试误把完整扫描返回的总访问数直接
作为父级 POP 注入序号，定向测试失败；撤除
该错误断言后才取得上列通过结果，此 RET 行
当时保持 partial，不能伪称扫描子调用已关闭。

三处 reset 后缀 `0x0047B746`（case100）、
`0x0047B83D`（共享）和 `0x0047B9CA`
（case51）也已逐物理地址核 LST 的四 POP、
ADD ESP14h、RET。各自让 reset child
先正常返回，再用五个独立停止序号验证
EIP、读 token、ESP、EAX=1、访问数、
GPR 恢复、末次 ADD FLAGS/DF、reset_calls=1
及无提前返回；三条正常返回仍为 EAX1。
这是三处局部尾部 REVIEW，不表示 reset
child 的内存/异常序列及其上游图像和释放
链已通过。彼时 22 处 RET 中 21 处仅获
局部审查，扫描尾一处仍为 partial；98 CALL
和 249 块并未据此完成语义收敛。
Linux core `199/199`（`proc_91b4`）、ASan
core `199/199`（`proc_5337`）、Linux app
`205/205`（`proc_c12b`）通过。

再核四处 reset CALL `0x004798F7/0x0047B723/
0x0047B81A/0x0047B9A7`：各先写实际返回槽，
随后 `sub_478850` 先 PUSH EBX、把 ECX
变 EBX、XOR EAX0、PUSH EBP，在
`0x00478856` 首次 actor word 写前停止时
ESP=parent CALL 前 ESP-12，actor 尚未被此
callee 写入。四处受控测试逐一核 CALL 栈写
故障与 child 首写故障的 return_ip、actor
写 token、EAX/ECX/EDX、保存寄存器、
ESP、FLAGS/DF、child 两次 PUSH 的访问数
及 parent 后缀不提前执行；#1/#93/#98
另测 child 首访问停点。此前正常回复与
reset 后缀测试仍在；本次仅审 CALL 入口和
首写合同，不能把 callee 的全部112条指令
或其 parent 剩余路径列为本包已审。
Linux core `199/199`（`proc_f7b1`）、
ASan core `199/199`（`proc_09e0`）、
Linux app `205/205`（`proc_bfab`）通过。
`call-audit.tsv` 的 98 处现仅 9 处局部审查，
余89处 partial，不代表 316 整包 REVIEW。

TSW 正常查询补充独立 LST 核对：`sub_431C50`
在 `0x00431C81` 分配 32 字节缓存节点、
`0x00431DCF` 得节点 `+8` 的记录地址；
`0x00431DF0` 命中时 `0x00431E88` 同样返回
节点 `+8`，非首命中先把节点移到桶首。
正常 miss 返回前 ECX=主流长度、EDX=新增
缓存总字节；hit 桶首 ECX=低16位组合键，
非首 ECX=旧桶首节点地址，EDX 为桶头地址。
外层 `0x0043173C/4A TEST EAX,EAX`
将成功回包的 ZF/CF/OF/PF/SF 定义为
测试命中记录地址或装载返回1的结果；AF
未定义，之后 `0x00431753` MOV EAX 记录
不改 FLAGS。战斗父级将记录地址写
actor `+0x2548`，命令流从记录 `+0x00`
取得，两个地址不可相等。

新 `LegacyBattleActorFrameTswUpdatePort` 可调用
真实六包缓存查询；32 位节点/主流虚拟地址由
TSW 运行时跨实例唯一递增分配，不窄化宿主
指针。缓存 owner 同时保有记录和流身份；
重新装载不复用已借出的 token。真实
`all_char.tsw (1,0)` 测试核缓存 miss/hit
寄存器、帧宽高47×95、父级记录写入与
清缓存后的字节租约；合成特殊帧另核同桶
非首命中旧桶首 ECX。Linux core 199/199
（`proc_a05a`）、ASan core 199/199
（`proc_25ab`）、Linux app 205/205
（`proc_cf23`）通过。
这些是有租约的 TSW 正常主流路径，
仍缺战斗主循环的真实端口注入、
外部资源/堆块统一地址注册、初始化和失败
路径的逐写副作用、特殊帧附加字段，
也非原版动态差分或 316 全局 REVIEW。

父级 `0x00479B4B` 写 actor `+0x0E14` 后，
如果 decoder 显式交付的 `source_pixels` 正好从
同一个 raw block `+0x20` 开始，且 EAX 对应
该块的 guest 输出 token、格式为 direct16、
span 未超出块尾，emitter 现在也保有 raw 块
租约；这与未显式给 source_pixels 时的
自动借用指向同一批像素字节。真实 TSW
`(1,0)` 47×95 的 4465 个 16-bit 像素
测试验证源 span 与租约同址，异主显式 span
不会错误借到 raw 块。Linux core 199/199
（`proc_3d5d`）、ASan core 199/199
（`proc_942a`）、Linux app 205/205
（`proc_511a`）通过。外部无租约像素源、
完整 8-bit 输出与生产 Win32 分配仍未接通；
此测试不是原版像素差分或整包验收。

内部 byte 输出到粒子 u16 读取的安全前缀新增同 raw owner
借用：只暴露 `floor(min(pixel_count, committed_bytes)/2)` 对
已经顺序写入的相邻字节；LST 允许解码在尺寸未填满前返回，
返回前以输出 ESI 与起点之差记录实际提交量。
真实命令流受控回放写2字节即早停，合成5-byte/3-byte
向量再验证两个/一个 little-endian word 原址别名、
回写相同 raw 字节；奇数尾 byte、未初始化堆填充不借用，
超过 owner byte 长度的虚构声明清空跨度与租约。
定向 core `proc_9716`、ASan `proc_2b03` 各1/1。
真实 8-bit 帧、后续粒子越界索引及原版动态差分仍未闭合；
不得从该安全前缀推断整条 8-bit 路径已验收。

不能以四张帧的比对宣称图像 decoder 已闭环。
更长 literal 的边界及异常停点、其他图像分支、
生产调用链的 TSW owner、allocator 输出块真实别名及四处 caller 尚缺；
不能据此验收完整 Size=6 或 316。

颜色 owner 和原始目标块仍为合成输入；大计数各迭代的
独立故障前缀、生产值与跨 owner 别名均未由此验收。

里程碑 (3) 尚需其余解码命令和尺寸，仍为 `2/8 = 25%`。
其余块还未完成双向追溯，也未完成共享内存可变时的几何重读、所有逐条可观察访问顺序、字段别名、EAX/ECX/EDX、FLAGS、DF、ESP/EIP 和每个异常停点的校验；
`platform_adapted` / `assembly_exact` 尚未判定。原版动态 oracle 缺失时只能在实现和静态门全部完成后登记 `blocked_runtime_oracle`，
不能事先宣称差分通过。production/parent 仅有部分条件化接线与局部测试；inventory、PLAN 和模块文档未因这些阶段性证据预先关闭。

逐块双向工作记录更新：001–249 共249/249块完成**单块**
LST→C++ 与 C++→LST 的局部核对；98 CALL/22 RET 深层、生产 caller 与
跨块状态尚未整体收敛，不能由单块测试推出整包 REVIEW。
新增块030：case1 A02 物理 ESI+2548 源、A0D 压 EBX、源首字发布4CD730，
三次独立 signed phase 高乘写4CD71C/4CD30C/4CD304；A77 起重读源与 phase、
源+0E 写4CD75C，ESI+2694 读/掩码/置位/写，再读源/Oy/H/W/Y/首全局/phase/X，
压 flags/H/W/Y−Oy−4*motion−phase/X−EBP，AEC CALL sub4170E0 压 AF1。
修正生产源、motion、height、flags RMW、draw actor 访存为物理 ESI/backing；
7+6+13站、CALL槽及外来ESI核 token/kind/ESP/FLAGS/DF/已提交前缀；
子级深层与原版差分 partial，定向 core/ASan `proc_4979` 各1/1。
跨块复查纠差：LST A973/A98B、AA15/AA1B/AA32/AA39/AA47/AA4E/AA58
及 A242/AA6A/B3F8/B7F0 均以物理 ESI 为访存基址，不能用请求 actor token
假定另一 actor 的 backing。已修 case11 源/第二绘图和 case6/11/15/50 共享相位
RMW 的 token/背书；外来 ESI 分别在 case11 源首读、第二绘图首读、
DEC 首读停下，不提交后续写入；其他正常读写与分支沿用原址。
`proc_087d` 定向 core/ASan 各1/1。`proc_14ef` 对此前块030提交快照
core/ASan 各200/200、app 206/206；**不覆盖本次跨块修正**。
`proc_c961` 机械静态栈审计249块/98 CALL/22 RET 入口深度及
汇合深度无错误，但84个 CALL 行仍标 partial，22个 RET 的深层前驱
未整体审完；真实生产 caller 的缺省 port 与条件化 binding 未收敛。
继续纠差块031–033：LST AF1 的 word ADD、B06 的 phase CMP、B13 的粒子源 CMP
均取物理 ESI，而非请求 token；C++ 已修访存 token 和背书，外来 ESI 在
AF1/B06 首读停下，原 word 与前缀保持；`proc_3725` core/ASan 各1/1。
继续纠差块017–024、026：LST 资源门 9965–999C、选择器 A5/AB/B5/BD
及 99DC 均以实际 ESI 访存；C++ actor image 的 read/write 从
request actor token 改为 ESI+offset，且 backing 仅在 ESI 相等时放行。
外来 ESI 在资源门首读、选择器资源+20已读而角色首写前、case1
phase 首读前停止，保持先前物理写；`proc_e5d4` core/ASan 各1/1。
共用 selector 入口 A1A0/A94D/B2E8/B747/B83E 各物理 ESI+2958 首读与
A253/A25A terminal word 写改为 ESI+offset/backing；五个入口外来 ESI
均未借用 actor 的相位字，`proc_a940` core/ASan 各1/1。
case51 B8DA 属性调用返回后物理 ESI+0DE0 的 byte OR 分离读写，
外来 ESI 在已完成 47CE70 子调用后、OR 首读前停止；
`proc_16f9` core/ASan 各1/1。case2/8/100 属性 OR 的
C26/A717/B63D 均物理 ESI+0E3C，case9 共享 terminal 写与 case8
A60B 粒子读的停点亦已统一；case2 子级 RET 后外来 ESI
不再借请求 backing 读 emitter，`proc_6978` core/ASan 各1/1。
CALL #2 子级 sub_47E950 在 E95D `MOV ESI,ECX` 后 F0BF/C5/D0/D6
均以真实 ESI 访问 actor；修正 list-release image 借用及停点，
外来 ECX 在四次寄存器保存后停于 F0BF，已提交 caller 前缀不回滚，
`proc_884d` core/ASan 各1/1。子级链表节点和 CRT 深层仍 partial。
以上均为局部验证，未关闭跨块 caller。
对上述同一源码／测试快照，`proc_1061` 完整 Linux core
200/200、ASan 200/200、Linux app 206/206 均通过；
不代替98 CALL/22 RET 深层和生产 caller 的最终 REVIEW。
矩形子级交叉验证：独立按 LST `0x00416FF0–0x00417047` 的 15 次物理
栈／全局访问列出指令地址与种类，逐处对照本函数 21 个 `E8 rel32`
矩形 CALL（地址集合与 LST 双向相等）。新增独立测试文件对这 21 个
CALL 各自验证 CALL 返回槽写前停点、五字节返回地址、进入同一
`sub_416FF0` 后 15 个访问前障的地址／token／ESP／已提交四处
clip 写、后七站的 GPR／FLAGS／DF 和正常返回的四个 clip 值及
父级下一 EIP。测试用同一受控四参数 `(left=43, top=24,
right=640, bottom=480)` 驱动子级，只证明各 site 的调用边界和
共享子级访问模板，不证明 21 条真实父级路径各自压入相同参数、
前后缀收敛或原版差分。`proc_ad38` Linux core／ASan 定向
各 1/1；随后测试集快照的完整 Linux core／ASan 各 200/200、
app 206/206（`proc_aea2`）通过；21 行审计状态仍按各自上游事实
保持 partial／局部 reviewed。
音频 CALL #5 `0x004799FA` 先压返回槽；`sub_485610`
两次外层实参读、六次压参，`sub_485CE0` 四次保存与两次
`sub_485CC0/485CD0` 的调用、模式查询及 `0x00485D02`
实参读取，在模式和子模式均为1且低16位音效编号非零时，
已完成 20 次子级访问。此前 case1 适配器回包未返回时
把计数回滚至 CALL 入口且把 ESP/EIP 复位到 `0x00485610`，
不符合 LST 已提交的栈与寄存器状态；先修正 case1
（`proc_f224` core／ASan 定向各 1/1）。现复核其余18处，
统一保留这20次访问并停于下一处未建模的 `0x00485D0E`
`MOV ESI,[ESP+0x14]` 前：19处受控音频 CALL 各自核
外层返回槽加20次访问、实际 ESP（相对 CALL 前减48）、
上一 PUSH 的内层返回地址 `0x00485CFA`、物理 stack_read
及 `ESP+0x14` token、EAX/ECX/EBX/EBP/ESI/EDI、
`TEST EDI,EDI` 后已定义 FLAGS 与 DF。原先另外18处
回包失败错误回滚20次并复位到 wrapper 入口的实现已修；
case2、case51 和 case100 两条音频分支另验证序号恰停
在深层读前时不调用声音 port。前期定向 core 1/1
（`proc_4f67`）；最终源码／测试快照通过串行完整门禁
`proc_5c9c`：Linux core 200/200、ASan core 200/200、
app 206/206。
随后复核 LST `0x00485639 PUSH 0` 与 `0x00485D0E MOV ESI,[ESP+14h]`：
内层 `arg_0` 是此调用自己压入的零值，物理地址为内层 ESP+14h，
而非外部未知 owner。模式／子模式均为1且音效编号低16位非零时，
现于原20次子级访问之后先核 D0E 栈读前障，成功后完成第21次子级访问、
令 ESI=0，再由 `0x00485D12 TEST ESI,ESI` 得 ZF/PF=1、CF/OF=0、
AF 未定义；不进入 `JNZ`，在 `0x00485D14 MOV [ESP+18h],ESI`
覆写内层 `arg_4`（先前压入的音效编号）之前停住，ESP 不变，
写 token=ESP+18h。每次新音频 CALL 进入 wrapper 即清空旧
`sample_child` 回包标记，避免本次 D0E 读障错误继承上一次已返回状态；
case1 两个序号分别停于读前／写前且不调用声音 port，19处共享前缀的
受控回包停点保留21次子级访问；Linux core 阶段 `proc_1bd8` 200/200；
追加旧回包标记断言后的最终阶段快照 `proc_090c` 经 Linux core／
ASan core 各200/200、Linux app 206/206。该阶段 D14 栈槽覆写、
`sub_486490`／Miles／CRT 与真实生产父路径仍 partial。
随后再按 LST 建模 `0x00485D14` 内层栈 `arg_4` 写零：写前可停，
写后把原音效编号被覆盖的事实保留在专属结果字段，不冒充已持有真实
guest 栈字节。按顺序执行 `0x00485D1A PUSH EDI`、D1D CALL 返回槽、
`sub_486490` 的 `SUB ESP,8` 与三次保存 EBX／EBP／ESI、
`0x00486496` 从此前压入的 arg_0 读音效编号并 `SHL ESI,4`；
于首次无已持有 backing 的音频表字段读 `0x0048649F [EBP+67Ch]`
前停止，ESP 相对父 CALL 前减76、ESI=低16位音效编号左移4、
EBP=0x004C8450、token=0x004C8ACC，SHL 四位的 OF／AF 未定义。
子级共28次物理访问，另有父 CALL 返回槽一次；case1 覆盖 D14 之后
七个逐访问序号及显式 `global_readable=false`，均不先调用声音 port；
19处音频 CALL 共享受控前缀。阶段 `proc_6987` Linux core 200/200；
最终阶段快照 `proc_676a` Linux core／ASan core 各200/200、
Linux app 206/206。音频表、Miles／CRT 深层与生产父栈仍 partial，
不将受控回包当作原版差分。
这只是受控回包与栈前缀，不是19处真实父路径实参逐字节收敛，
更不证明后续 Miles/CRT 子级、物理父栈、生产 caller、
全部98 CALL／22 RET 或原版差分。

绘图的28处物理 `CALL sub_4170E0` 反向复核发现同型错误：
窄 draw port 不返回时，旧 C++ 在已读取源和保存寄存器后
将访问计数、ESP、EIP 与最后 PUSH 值回滚到 CALL 入口。
LST `0x004170E0` 先读 `4CD730`，E5/E6/E7 依次保存
EBX/EBP/ESI，E8 读源首 word，ED 保存 EDI；原始头
`word!=FFFFh` 于 `0x00417105 XOR EBP,EBP`，共完成
6 次子级访问；标记头加 `0x004170F0` 调色板 token 读取、
`0x004170F5 XOR EBP,EBP` 和 F7 比较，调色板非零时
共完成7次。下一个未建模物理访问均为 `0x00417107`
从四个保存寄存器下的 `[ESP+24h]` 读取 arg_10；
不透明回包与恰好到该读前的访问序号停止现在保留父 CALL
返回槽、四个已压寄存器、真实 `ESP=父 CALL 前−20`、
`stopped_token=ESP+24h`、EAX 中源或调色板 token、
分别由 XOR 或 CMP 导出的 FLAGS、DF 和已提交的父级写入。
28个物理位点的受控回包及两种资源头的序号前障由
`battle.actor_frame_316` 定向 `proc_d9fe` 1/1 核对；
若序号停在下一读前，不调用 draw port。
正常回包之后的父级 actor 故障测试改用显式不可读 owner，
不能再把 `+8` 的序号误认为父级第一次访问：它首先是
`0x00417107` 的未建模子级读。
这只收敛绘图子级到首次 arg_10 读之前；arg_10 字节、
随后全局写、间接像素 CALL、异常/Win32、真实生产 caller
和原版动态差分均为 partial，28行审计状态不升级。
上述 case1／矩形阶段源码的完整 Linux 门禁 `proc_4476`：core
200/200、ASan 200/200、app 206/206；不覆盖后续19处音频
改动，也不升级整包状态。
新增块094：case7 第四绘图 A577 三次 signed phase NEG 写三全局，A585 首写后 PUSH 字面零，压 flags|4/H/W/Y−Oy+P+16（LEA 不改 FLAGS）/X−Ox+P，A5EA CALL sub4170E0 压 A5EF，第二组20参仍在调用方栈。7全局+13绘图物理站及 CALL 核 token/kind/ESP/FLAGS/DF/已提交前缀；像素/原版 partial，core/ASan `proc_f19e` 各1/1。
新增块093：case7 第四矩形 A516 一次 source 供 H/W，Oy/Y 各独立重读，压 Y+H−Oy+P、X+W−Ox+P、Y+(H>>1)−Oy+P、X+(W>>1)−Ox+P；A572 CALL sub416FF0 压 A577。13物理站及 CALL 核 token/kind/ESP/FLAGS/DF/已压前缀；clip 深层/原版 partial，core/ASan `proc_200b` 各1/1。
新增块092：case7 第三绘图 A4A0 后三次 signed phase NEG 全部写全局后才在 A4DB PUSH 字面零（不同于前两轮首写后压零）；压 flags|4/H/W/Y−P−Oy/X+P−Ox，A511 CALL sub4170E0 压 A516。6全局+14绘图物理站与 CALL 区分辅助 PUSH/五参并核 token/kind/ESP/FLAGS/DF/已提交前缀；像素/原版 partial，core/ASan `proc_25ab` 各1/1。
新增块091：case7 第三矩形 A446 读 H/W/phase/Oy/Y 时前20参仍在栈上；A470 ADD ESP,50h 清理后再压 Y+(H>>1)−Oy−P、X+W−Ox+P、Y−Oy−P、X+(W>>1)−Ox+P；A49B CALL sub416FF0 压 A4A0。12站与 CALL 核清理前后 token/kind/ESP/FLAGS/DF/已提交前缀；clip 深层/原版 partial，core/ASan `proc_f0df` 各1/1。
新增块090：case7 次绘图 A3D0 三次 phase NEG 写三全局，A3DE 压字面零；重读 flags/source H/W/Y/Oy/phase/X 压 flags|4/H/W/Y−Oy+P/X−Ox−P；A441 CALL sub4170E0 压 A446，前两组20个 dword 实参仍待清。7全局+13绘图物理站及 CALL 核 token/kind/ESP/FLAGS/DF/已提交前缀；像素/原版 partial，core/ASan `proc_4e47` 各1/1。
新增块089：case7 A36D 从首绘图返回重读 Y/H、phase、source W、Oy/Y，EBX 重用于 right；压 Y+H−Oy+P、X+(W>>1)−Ox−P、Y+(H>>1)−Oy+P、X−Ox−P，A3CB CALL sub416FF0 压 A3D0，前10参数仍在调用方栈。14物理站及 CALL 核 token/kind/ESP/FLAGS/DF/已提交前缀；clip 深层/原版 partial，core/ASan `proc_7665` 各1/1。
新增块088：case7 A2F5 首绘图前独立 signed phase NEG 写三全局；A304 PUSH 字面零，非 EBX（EBX 保存前矩形 right），压 flags|4/H>>1/W/Y−P−Oy/X−P−Ox，A368 CALL sub4170E0 压 A36D。7全局+13绘图及 CALL 核 token/kind/ESP/FLAGS/DF/已提交写和参数；像素/原版 partial，core/ASan `proc_4f3a` 各1/1。
新增块087：case7 A28D 源 first dword 发布4CD730，不压辅助零；A29D 和 A2BF 独立读 H/W；EBX 清零后复用 H/2、W/2，压 Y+(H>>1)−Oy−P、X+(W>>1)−Ox−P、Y−Oy−P、X−Ox−P，A2F0 CALL sub416FF0 压 A2F5 并保留 EBX=right。源3+矩形13物理站及 CALL 核 token/kind/ESP/FLAGS/DF/已提交前缀；clip 深层/原版 partial，core/ASan `proc_4de1` 各1/1。
新增块113：case9 A779 源读/压 EBX/首字写4CD730，signed IMUL 55555556*phase 计算 n=highEDX+signbit+1，依序写 4CD724=n、4CC2F0=15−n；flags OR14 仅参数，压 flags/H/W/Y−[actor+2B4]/X−signed([actor+29B2])；A7F0 CALL sub4170E0 压 A7F5。源、phase、绘图 actor 读物理 ESI 替代请求 token，并校验 backing；7源+13绘图及 CALL、外来 ESI 核 token/kind/ESP/FLAGS/DF/已提交前缀；像素/原版 partial，core/ASan `proc_a5d0` 各1/1。
新增块120：case10 A849 source、A84F PUSH EBX 而非合成零，源首字发布4CD730，flags 原值/H/W/Y−Oy/X−Ox 五参；A88D CALL sub4170E0 压 A892；共用 case5/10 helper 但独立4源+12绘图地址及 CALL 核 token/kind/ESP/FLAGS/DF/已提交前缀；像素/原版 partial，core/ASan `proc_d4b6` 各1/1。
新增块080：case6 A1C6 读物理 ESI+2548，PUSH EBX 后源首字写4CD730；三次独立 signed ESI+2958 原值写 4CD71C/4CD30C/4CD304，后续压 flags|4/H/W/Y−[actor+2B4]/X−signed([actor+29B2])，A23A CALL sub4170E0 压 A23F。修正生产源及相位访存从请求 token 派生地址为实际 ESI、背书不符访问前停止；10源+13绘图站与 CALL、外来 ESI 核 token/kind/ESP/FLAGS/DF/已提交前缀；像素/原版 partial，core/ASan `proc_3836` 各1/1。
新增块070：case5 A0B7 actor+2548，A0BD PUSH EBX（非合成零），源首字写4CD730；独立读 flags/H/W/Y/Oy/X 压原 flags/H/W/Y−Oy/X−Ox，A0FB CALL sub4170E0 压 A100。源4站、绘图12站及 CALL 物理 token/kind/ESP/FLAGS/DF/已提交前缀检验；像素/原版 partial，core/ASan `proc_3e97` 各1/1。
新增块054/065：次矩形返回后各三次独立 signed phase NEG 写 4CD71C/4CD30C/4CD304，首次写后 PUSH EBX 而不是常数0；case3 压 flags|4/H/W/Y+2P−Oy/X−Ox，case4 压 flags|4/H/W/Y−Oy/X+2P−Ox，分别自然顺流/JMP 至共享块055 E7C image CALL。各7全局+13绘图物理站核 token/kind/ESP/FLAGS/DF/已提交前缀；像素/原版 partial，core/ASan `proc_1652` 各1/1。
新增块053/064：首次绘图返回后 case3 次矩形 DB3–E07 独立 source H/W 与 phase、坐标/Oy 重读形成 Y+H+2P−Oy、X+W−Ox、Y+2P−Oy、X+(W>>1)−Ox；case4 次矩形 FAF–A009 形成 Y+H−Oy、X+W+2P−Ox、Y+(H>>1)−Oy、X+2P−Ox。E07/A009 CALL sub416FF0 分别压 E0C/A00E，13/14物理站及 CALL 槽检验 token/kind/ESP/FLAGS/DF/已压前缀；clip 深层/原版 partial，core/ASan `proc_623e` 各1/1。
新增块052/063：case3/4 首次绘图，三次独立 signed phase NEG 依次写 4CD71C/4CD30C/4CD304，第一次写后 PUSH EBX 而不是常数0；各自重读 source/flags/H/W/phase/X/Y/Oy，依序压 flags|4/H/W/y/x，DAE/FAA CALL sub4170E0 并压 DB3/FAF 返回槽。各7处全局读写和13处绘图参数访问按物理 token/kind/ESP/FLAGS/DF/已提交前缀及 CALL 检验；绘图子级像素/原版 partial，core/ASan `proc_5f35` 各1/1。
新增块062：case4 ED0 源像素写4CD730，EE0 起新 source 独立读 H/W，signed phase×2 在先压 bottom 后入 var_14；bottom=Y+(H>>1)−Oy、right=X+W−2P−Ox、top=Y−Oy、left=X−2P−Ox，四压参 F37 CALL sub416FF0。15站及返回槽核 token/kind/ESP/FLAGS/DF/已压前缀；clip 深层/原版 partial，core/ASan `proc_7b65` 各1/1。
新增块051：case3 CCD 起取 source 首字写4CD730，signed phase×2 入栈 var_14；按物理 source/H/W/Oy/Ox 读及本地栈读写计算 bottom=Y+H−2P−Oy、right=X+(W>>1)−Ox、top=Y−2P−Oy、left=X−Ox，压四值 D39 CALL sub416FF0。16站及返回槽核 token/kind/ESP/FLAGS/DF/已压前缀；clip 深层/原版 partial，core/ASan `proc_d6aa` 各1/1。
新增块055：case3/4 共用 E7C CALL sub4170E0 压返回 E81，两个第二绘图前缀各自五压参及辅助0，正常回复保留共20个栈字供块056清；分别核 token/kind/ESP/FLAGS/DF/序号及参数数组，深层像素/原版 partial。core/ASan `proc_71b0` 各1/1。
新增块114：case9 A7F5 正常绘图 ADD ESP18h，A7F8→4CC2F0=0、A7FE→4CD724=0，A804 INC word 物理 ESI+2958（CF 保持，FFFF→0）；修复 C++ INC 读 token 错从 request.actor_token 导出且可能写外来 owner。四站核 kind/token/ESP/FLAGS/DF/前缀，外来 ESI 在两个全局已清后停。绘图子级/原版 partial；core/ASan `proc_5bb5` 各1/1。
新增块099、101：case8 A643 发布解码 EAX 至 ESI+E14（含0），A649 重读来源后 ADD ESP10h，宽高两字读写；A66E–A69C signed X/Y 减 Oy 写 E1C/E20，E24=-30，再按 +2B08 dword 恰1 改670。A6AF–A70E 读 E20 减10 写 E2C、E28=2，重读来源高、OR byte E3C16h 后写 E30=H+10、E36/E34=100、E38=1、E3A=10，CALL 属性叶；修复 publish/dimensions/geometry/fields actor 物理 ESI token/backing，1+6+7+1+12 故障站核 token/kind/ESP/FLAGS/DF/前缀。decoder 深层/原版 partial；core/ASan `proc_ec26` 各1/1。
新增块035：case2 B4B 写解码 EAX 到 ESI+E14（含0），B51 重读 source 再 ADD ESP10h，分别读宽高/写 E18/E1A；B76–BC7 四次 source/坐标/半宽计算写 E1C/E20/E24，BCB OR byte16h 后 BD2–C13 计算半高与 Oy 并写 E30/E2C/E28/E36/E34/E38/E3A，C1C CALL 属性叶。修复 publish/dimensions/geometry/fields 物理 ESI token 与 backing，1+6+12+10 故障站核 token/kind/ESP/FLAGS/DF/已提交前缀；解码深层/原版 partial，core/ASan `proc_dbf0` 各1/1。
新增块034、098：case2 B1F–B46 与 case8 A617–A63E 分别 ECX16h、XOR EAX0、EDI=ESI+E14、EDX=ESP+18，按 DF REP STOSD22，随后两个 LEA、四压参，之间物理读 ESI+2548 和 source 首字，CALL sub4019A0。修正共用 C++ +2548 读错用 request.actor_token／失配背板；核 REP 首中末/DF1，六处 decoder 准备与 CALL 返回槽 token/kind/ESP/FLAGS/DF/栈前缀。decoder 深层/原版 partial，core/ASan `proc_9070` 各1/1。
新增块102：case8 A713 CMP EAX,EBP1、A715 JNZ A71E；sub47CE70 签扩展 +2694 字节后只取 bit0，0/2/80h 为 EAX0 跳过 A717，1/FFh 为 EAX1 对 +E3C 作 word OR（AB16h→AB17h）；测试 FLAGS/ESP/DF/高字节及访问计数，子级深层/原版 partial。core/ASan `proc_ef30` 各1/1。
新增块107：case8 A736 读全局4AB784→EAX，A73B/A73C 压句柄/31h，A73E CALL sub485610；三站和返回槽核 kind/token/ESP/EAX/FLAGS/DF/前缀，actor+428 保留 AA（不同 case2）。深层音效/原版 partial；core/ASan `proc_4358` 各1/1。
新增块104–106：case8 A71E/A724/A725 读 IAT499214、压 EBP1、间接 CALL EDI；A727/A728/A729 压首个 height720、EBX0、再 CALL；A72B/A72C/A731 压 width1280、ECX=53B0B8、调用矩形。独立于 case2 测七处物理访问及 A731 返回槽 kind/token/ESP/EAX/EDI/FLAGS/DF/已提交参数；Win32 受控回复/矩形深层/原版差分 partial。core/ASan `proc_be44` 各1/1。
新增块043：case2 C6C/C7A 清两个 actor 字，C78 XOR EAX0、C73 ECX26h、C81 随 DF REP STOSD 38 次，C83 物理 ESI+E14 读，C89 EDI 改为同地址、C8F/C91 比 EBX 跳零分支；修复两字写/C83 读故障 token 错依赖 request.actor_token、无 ESI 背板验证。ordinal 0/1/2/3/39/40 与正逆 DF 分核 token/kind/ESP/ECX/EAX/EDI/FLAGS/DF/已清前缀，子级/原版 partial；core/ASan `proc_774a` 各1/1。
新增块044–046：LST C93 PUSH 非零固定 token，C94 CALL sub4885A0 压 C99 返回地址，正常回复 C99 ADD ESP4 清 caller 实参，C9C ECX16h、CA1 JMP B814；零 token 从前置 C91 JZ 直进 C9C。两栈写故障和零/非零出口核 kind/token/ESP/FLAGS/DF/前缀；深层 wrapper/CRT、共享粒子 REP 和原版差分 partial。core/ASan `proc_ba1f` 各1/1。
新增块041：case2 C45 写物理 ESI+428 word31h，C4E 读全局4AB784 到 EAX，C53/C54 压句柄/31h，C56 CALL；修正 C++ C45 错用 request.actor_token 推 fault token／ESI 不匹配时仍写 owner。四站及返回槽核 token/kind/ESP/EAX/ESI/FLAGS/DF/前缀，失配在首写停；子级深层/原版差分 partial，core/ASan `proc_cb49` 各1/1。
新增块038–040：LST C2C 读 IAT 499214→EDI、C32 压 index1、C34 间接 CALL；stdcall4 返回后 C36 压 height720、C37 压 EBX0、C38 再 CALL；C3A 压 width1280、C3B ECX=53B0B8、C40 CALL sub433F30。七处 metrics 故障及 C40 返回槽核 token/kind/ESP/EAX/EDI/FLAGS/DF、已提交值与调用计数，矩形子级两值分开/RET8 正常清栈。Win32 回复是受控 port，原版/矩形深层 partial；core/ASan `proc_7132` 各1/1。
新增块028：case1 799ED CMP AX,BX0 新 FLAGS，799F2 读4AB784 到 EAX 句柄22220000，799F7/799F8 压句柄/31h，799FA CALL；三站及返回槽核 token/kind/ESP/EAX/FLAGS/DF/前缀，缺 owner 首读停。深层/原版差分 partial，core/ASan `proc_c0e5` 各1/1。
新增块111：case9 A763 先 CMP AX,BX0 新 FLAGS（ZF/PF），A768 读4AB784 到 EDX 保留 EAX 高 ABCD，A76E/A76F PUSH 句柄和31h 后 A771 CALL。三站及返回槽核 token/kind/ESP/EAX/EDX/FLAGS/DF/前缀；首次错误假设继承 header FLAGS 导致定向失败，改按 CMP 后 core/ASan `proc_1f55` 各1/1。深层/原版差分 partial。
新增块068、118：case5 A0A6 与 case10 A838 读4AB784 到 EDX 保留 EAX 高 CDEF，分别压句柄/31h 再 CALL sub485610；各三站和返回槽核 token/kind/ESP/EAX/EDX/FLAGS/DF/前缀。首次新增测试变量撞名编译失败，更正后 core/ASan `proc_b28f` 各1/1；音频深层/原版差分 partial。
新增块060：case4 LST 79EC0 全局4AB784 读到完整 EAX（非 case3 EDX），EC5/EC6 PUSH 句柄/31h、EC8 CALL；三站及返回槽核 token/kind/ESP/EAX/FLAGS/DF/前缀，子级首参读/正常清栈已有测试，深层/原版差分 partial；core/ASan `proc_b618` 各1/1。
新增块049：case3 零相位 LST 79CBC 读全局4AB784 到 EDX，BCC2 压样本句柄、BCC3 压31h，BCC5 CALL sub485610；三站和返回槽核 token/kind/ESP/EAX/EDX/FLAGS/DF/先压实参，子级双栈参读及正常清栈已有测试。音频深层/原版差分 partial，core/ASan `proc_095c` 各1/1。
新增块014–015：B937 物理 ESI+41A MOV CX 保留 ECX 高16位 CAFE，PUSH EBX0/完整 CAFE1357 并 B940 CALL，四站核 token/kind/ESP/GPR/FLAGS/DF/前缀；返回 B945 写 ESI+2548 token，再读 +2B20/+3E0，ADD ESP8/CMP，三站核旧值和子级已发布资源。错误 backing 实际先在 B937 停，不能虚称独立到 B945；首次误设断言后纠正，core/ASan `proc_489b` 各1/1。深层/original diff partial。
新增块011：B920 物理 EDI=actor+3D0 压参、B921 CALL sub4321E0 压 B926 返回地址；两站核 token/kind/ESP/GPR/FLAGS/DF/访问序号，callee 缺失动作子记录路径三 PUSH 后 B4321EE 首读停。B926 属块012，深层动作流/caller/原版差分 partial；定向 core/ASan `proc_7aa4` 各1/1。
新增块009：B8FC/B902 正常 reset 子级返回后写物理 ESI+2AAC/2ABC=EBX0，B908 压 EBP1，B90B 写 +2AB8=1，ECX=ESI 后 B911 CALL release；四站核 token/kind/ESP/EAX/ESI/FLAGS/DF/子级 latch 与旧值及错 backing 的物理 ESI 早停。B911 返回槽/首参测试原有；release 深层/原版差分 partial，定向 core/ASan `proc_30f4` 各1/1。
新增块008：LST 四处清除 ESI+2C4/+2C8 dword 和 +2958/+2A12 word 后 ECX38/EAX0，B8F3 从物理 EDI=actor+3D0 按 DF 做38次 REP，B8F5 ECX=ESI，B8F7 CALL reset。四个字/双字写前站和 DF0、DF1 各38次 REP 逐站核 token/kind/ESP=entry-24h/ECX/EDI/FLAGS/DF/重叠前缀，CALL 返回槽与子级首站已有测试；首次三次失败源于新增断言的 DF/ESP 错误预期，纠正后定向 core/ASan `proc_5630` 各1/1。子级深层/原版差分 partial。
新增块245：case51 B92C–B97F 物理 ESI+DD8/+DDC 比例读、+2958 word加2、DE4/DE8/DEC 依次写5、EBX/EDI 各加4并回写比例；读4CD76C、ADD ESI,DB8 并压新 ESI/全局，ECX=53B0B8 CALL。12站核 token/kind/ESP/各 ADD FLAGS/DF/旧值、错 ESI 首读停，CALL 返回槽/子级入口 typed stop 覆盖。粒子深层/原版差分 partial，定向 core/ASan `proc_801d` 各1/1。
新增块243：case51 decoder 返回后从物理 ESI+428 写 CX 而保留 ECX 高16位 FEDC，写 ESI+DB8 token、+390 word31h，读全局句柄并 PUSH 句柄/完整 ECX；6站核 token/kind/ESP/FLAGS/DF/已提交旧值和错 ESI 首读停，B924 CALL 返回槽障碍保留 decoder 四参。深层/原版差分 partial，定向 core/ASan `proc_dc08` 各1/1。
新增块242：case51 B8E0–B902 从原 ESP LEA var10/varC，先 PUSH var10 才按物理 ESI+2548 读源，写 ESI+DF0=90，PUSH varC 后读取源首 dword，再 LEA/PUSH var8 和像素；7站核 token/kind/ESP/FLAGS/DF/已压前缀与 DF0 旧值，错 ESI 在首压后停。B902 CALL 返回槽写前障保持四参；解码深层/原版差分 partial，定向 core/ASan `proc_d8e2` 各1/1。
新增块239：case51 B857–B8D0 四个初始和13个几何访问站验证物理 ESI/资源地址、kind/ESP/SHR-SUB FLAGS/DF/旧值，错 ESI 首读/首写停；1024 双比例、W5 H7、半宽2、flags16h、X41/Y27 与 LST 对应。B8D0 属性 CALL 返回槽/首读已有测试；子级深层/原版差分 partial。定向 core/ASan `proc_aa76` 各1/1。
新增块248：LST B995 ECX38、B99A XOR EAX0（AF undefined）、B99C 写物理 ESI+2A12 word BX、B9A3 从入栈 EDI/DF 作38次 REP，B9A5 ECX=ESI、B9A7 CALL。新鲜 case12 39个正向写前站核 token/kind/ESP/EAX/ECX/EDI/FLAGS/DF/旧值，错 ESI 首写早停和 DF1 次写站，case51 正常路径及 CALL 障碍已有测试；深层/caller/原版差分 partial，定向 core/ASan `proc_590a` 各1/1。
新增块226：case100 子级 reset 返回后 EAX1、三处物理 ESI+2AAC/+2AB8/+2ABC 写站逐点核 token/kind/ESP/FLAGS/DF/旧值及错 backing 诊断；原有四 POP、ADD ESP14h flags、RET 读站和返回值测试。深层/caller/原版差分 partial，定向 core/ASan `proc_5960` 各1/1。
新增块236：B81F EAX=1，三处物理 ESI+2AAC/+2AB8/+2ABC 写站依次核 token/kind/ESP/FLAGS/DF/已提交旧值及 wrong backing 诊断；原有四 POP、ADD ESP14h FLAGS、RET 物理读和返回 EAX1 测试继续有效。caller/原版差分 partial，定向 core/ASan `proc_f48c` 各1/1。
新增块235：LST B814 XOR EAX0（AF undefined）、B816 REP STOSD 保持入栈 DF；case14 38个 DF0 写站和 case8 E14 起22个 DF0 写站逐站核 token/kind/ESP/EAX/ECX/EDI/FLAGS/DF/前缀，case1/Group-B 的 DF1 完整及中途停止已有覆盖。B818 ECX=ESI、B81A CALL reset 返回槽障碍与子级首站停已核；深层/caller/原版差分仍 partial。定向 core/ASan `proc_c429` 与 `proc_9aaa` 各1/1。
新增块234：LST B808 写物理 ESI+2A12 word BX，B80F ECX=38；case14 fresh actor 首写核 token/kind/ESP/ECX/FLAGS/DF/旧值和错 ESI 早停；后续38次 DF0 REP 单独采样，块235 的 case2 22次分支尚待完整核对。定向 core/ASan `proc_c429` 各1/1，非整包门禁。
新增块231：case50 源侧按物理 ESI+2548 读源、先 PUSH EBX0 后解引用源像素写4CD730，signed 相位写 4CC2F0=15-P；6站核 token/kind/ESP/FLAGS/DF/全局旧值，错 ESI 首读停。绘图重读 flags/source/H/W/+2958/Y、带符号查 `[4A7DB4+P*4]`（-1/-3 已知，-6 未知停）、Oy/X；14站核 token/kind/ESP/OR-XOR-SUB FLAGS/DF/EDI/栈前缀，P14 表值-32 得 `(F6h,7,5,56,43-EBP)`；CALL #92 B7E8 写前障保持六参。callee/site 深层、原版差分 partial，定向 core/ASan `proc_3291` 各1/1。
新增块229：case50 零相位 CMP AX,BX 等于0后从物理 ESI+428 读 actor 字入 AX 保留 EAX 高16位，写+390 word31，读全局4AB784 并压 handle/完整 EAX=CDEF1234h；5站核 token/kind/ESP/CMP FLAGS/DF/访问序号/GPR/旧音效及栈前缀，错误 ESI 首读前停，CALL #91 B775 回址写障保留两参。样本 ID 低16位仅在 wrapper 内截取，真实音频/原版差分 partial；定向 core/ASan `proc_d25e` 各1/1。
新增块225：case100 结束先写相位+2958 word0，ECX38/XOR EAX0 后写+2A12 word0；两段 REP 从沿用 EDI=actor+3D0 写38 dword，再 EDI=EBP=actor+E14 写22 dword。DF0 全62写前站核 token/kind/ESP/ECX/EDI/EAX/FLAGS/DF、访问序号与旧相位/旧延迟/固定 token/宽度，DF1 两段逆序完整路径已有测试；CALL #90 B723 深层 reset 与原版差分 partial，定向 core/ASan `proc_7466` 各1/1。
新增块223：case100 相位100且 E14 固定 token 非零时，PUSH 先前读取的完整 token 后 CALL #89 B6F9；两处栈写前障核物理 token/kind/ESP/FLAGS/DF、访问序号/EAX/旧粒子状态，ReleasePort 不调用；受控释放结果不等于真实分配器、callee/site 与原版差分 partial。定向 core/ASan `proc_f85a` 各1/1。
新增块219：case100 共用固定粒子尾先读全局4CD76C，LEA 物理 ESI+E14 而非请求 token，顺压指针与全局值，ECX=53B0B8；三站核 token/kind/ESP/FLAGS/DF/访问序号/GPR/旧栈，错误 ESI 不读 actor 仍计算真实物理指针；CALL #88 B6C7 回址写前障保留两参、粒子端口未调用。callee/site/原版差分 partial，定向 core/ASan `proc_50c7` 各1/1。
新增块218：case100 共用主相位 word INC 保留 CF，signed 重读后 CDQ/IDIV3 使算术 FLAGS 未定义，读 X 并以 X+余数-EBP 写固定粒子 E1C 前通过 ADD/SUB 恢复 FLAGS；相位 -5→-4 得商 -1、余数 -1。五站核物理 token/kind/ESP/DF、访问序号/GPR/旧相位与旧 X；后续粒子 callee/原版差分 partial，定向 core/ASan `proc_a07a` 各1/1。
新增块216：case100 窗口矩形调用返回后再读音频全局 4AB784 入 ECX，先压 handle 后压 31h；三站核 token/kind/ESP/FLAGS/DF、访问序号、GPR/已压前缀，CALL #87 B67C 回址写障不进 SoundPort。受控音频回包不证明原版声音，wrapper/site 深层与差分 partial；定向 core/ASan `proc_fcae` 各1/1。
新增块214–215：case100 首次受控 GetSystemMetrics(1) 回复 EAX720 后 B664 压全宽高度、B665 压 EBX0、CALL #85 B666 GetSystemMetrics(0) 写前障，三栈站核 token/kind/ESP/FLAGS/DF/寄存器/高度前缀；第二次受控回复 EAX1280 后 B668 压全宽宽度、ECX=53B0B8、CALL #86 B66E 回址写前障保留高宽，矩形端口未调用。两次 stdcall4 各消费其 index 与回址；720/1280 不是原版观测，真实 Win32/IAT/矩形 callee/site 与差分仍 partial。定向 core/ASan `proc_ac32` 各1/1。
新增块213：case100 先 EAX=15、PUSH EDI1，后读 GetSystemMetrics IAT 499214 入 EDI，再顺写 actor+E40/E44/E48=15；五站核物理 token/kind/ESP/EAX/EDI/FLAGS/DF、访问序号及旧字段提交前缀。错 ESI 首个 actor 写前停，PUSH 与 IAT 已提交；CALL #84 B662 的首个 index1/stdcall4 写障另有父层5站，真实 IAT 失败/原版差分 partial。定向 core/ASan `proc_2da6` 各1/1。
新增块210：case100 可选 E24 覆盖后写 E28=320、重读源 token、EDI1、依次 E36=24/E30=1/E34=100，读取资源 H 执行 SHR AX,2（保留 EAX 高16位、CF=H bit1、AF/OF 未定义），写 E38=H/4/E3A=40/E3C=56h；9站核物理 token/kind/ESP/DF/FLAGS/访问序号/字段旧值前缀。CALL #83 B634 回址 B639 写障与 leaf 已另审，原版差分 partial；定向 core/ASan `proc_0fc6` 各1/1。
新增块208：case100 解码 EAX 无论零否先写物理 ESI+E14，错误 ESI 同站写前停；源 token 在 ADD ESP10h 前读，W/H 写 E18/E1A 六站核 token/kind/ESP/ADD FLAGS/DF/旧值。X-EBP 写 E1C、Y-actor+3E4 写 E20，E24=0、E2C=1，按全32位 actor+2B08 CMP1 分 B5DA/B5E4；几何8站核 token/kind/ESP/SUB FLAGS/DF/已提交字段前缀。初次把 EBP 错设为7 导致测试失败，修正为根据实际 EBP 推导后定向 core/ASan `proc_15ff` 各1/1；callee/site 深层和原版差分仍 partial。
新增块207：case100 空固定 token 下 ECX22/EAX0，以物理 ESI+E14 起 `REP STOSD`；DF0 22个写前站、DF1第二站 E10（全22站由共享 case8 已测）、错误 ESI 第一站均核 token/kind/ESP/ECX/EDI/EDX/FLAGS/DF 和已提交宽度。三输出指针及源像素参数6站与 CALL #82 B56B 回址 B570 写障保留四参、不调用解码器；深层/site/原版差分 partial，定向 core/ASan `proc_d1c1` 各1/1。
新增块196：case100 读 actor 源 token 后先 PUSH EBX、从源+00取指针写4CD730，三次独立 signed 辅助相位重读写 4CD71C/30C/304；源侧10站核 token/kind/ESP/FLAGS/DF、访问序号及已提交全局旧值。绘图侧读 flags/source/+2B4/H/W/Y/+29B2/X，五参 `(24h,13,11,13,97)`，13站核 token/kind/ESP/FLAGS/DF/栈前缀；CALL #81 B4C0 回址 B4C5 写前障保留六参。callee/site 深层和原版差分仍 partial；定向 core/ASan `proc_59e9` 各1/1。
新增块194：case100 actor 辅助相位比较等于24才从 4AB784 读采样 handle 到 ECX，再顺压 handle 与 EBh；3处读/栈写站核 token/kind/ESP/FLAGS/DF、访问序号、GPR 与已压前缀，CALL #80 B445 回址 B44A 写障不进 SoundPort。受控 wrapper 回包仅父级，深层 wrapper/site 与原版差分 partial；定向 core/ASan `proc_3a33` 各1/1。
新增块188：case15 首图六参仍活跃时先将 4CC2F0 清0、压 EBX 辅助值，再三次重读 signed phase 并以原值发布 4CD71C/30C/304；8站核 token/kind/ESP/DF、访问序号/寄存器/全局旧值，受控绘图回包 FLAGS 未知不伪造。伪造 ESI 时清模式和 PUSH 已提交，首次 phase 读 B380 前停。第二图再读 flags/source/H/W/Y/P/+2B4/+29B2/X，14站核 `(F4h,7,5,24,9)` 的参数及物理前缀；第二图生产 actor 读改用真实 ESI，错误 ESI 于 B3A6 新 PUSH 前停。CALL #79 B3F0 回址 B3F5 写障保持两组六参，callee/site 深层、生产父栈与原版差分仍 partial；定向 core/ASan `proc_d1d6` 各1/1。
新增块187：case15 首图从 actor 读 flags/source，OR14h AF 未定义，压 EBX 辅助值后从 actor+2B4 取 EBP，依 flags、资源 H/W、有符号+29B2/Y/X 读取，按 `(flags|14h,H,W,Y-EBP,X-i16(+29B2))` 压五参；14站核 token/kind/ESP/FLAGS/DF、访问序号及辅助/已压前缀，受控五参 `(F4h,7,5,24,39)`。共用生产 actor 读已绑定真实 ESI 与请求角色 token，伪造 ESI 在 PUSH 前停；CALL #78 B374 回址 B379 写障不进 DrawPort。定向 core/ASan `proc_8be4` 各1/1；绘图 callee/site 深层、生产父栈及原版差分仍 partial。
新增块185–186：case15 B30E/B314 物理读取 ESI+2548 源 token 与 `[ECX]` dword，先写 4CD730 后才重读 ESI+2958 phase；5站核 token/kind/ESP/FLAGS/DF、序号、寄存器与未写全局旧值。生产已将两处 actor token 诊断及别名判定绑定真实 ESI，错误 ESI 在首读前停止而不代读另一 actor。signed -16 保留旧 4CC2F0/高位 EAX 及 CMP16 FLAGS；-15 的 B32F 写前停核 EAX0/ADD FLAGS/旧模式，-14 正常写模式1/EAX1 与 ADD FLAGS。定向 core/ASan `proc_a4c3` 各1/1；生产父栈及原版差分仍 partial。
新增块183：case15 零相位在 B2FE 读全局 4AB784 采样 handle，B303/B304 顺压 handle、31h；三站核 token/kind/ESP/FLAGS/DF、访问序号及未压参旧前缀，FLAGS 属于此前 CMP AX,BX 而非 CMP AX,FFE0h。CALL #77 B306 回址 B30B 写障保留两参且不进 SoundPort，受控回包后 ADD ESP8 已另核；wrapper 深层/site 与原版差分 partial。初次错误预期及临时编译失败已修复，最终定向 core/ASan `proc_cb27` 各1/1。
新增块177：case14 晚期最后图对 actor phase word 三次物理重读并依次写 `4CD71C/30C/304`，首写后压 EBX 且第三读前不清 EDX；P10 七站核 token/kind/ESP/FLAGS/DF、访问序号与全局旧值/寄存器。随后受控 P11 的绘图13站按 flags/source/H/W/Y/Oy/X/P 核五参 `(flags|4,7,5,16,65-EBP)` 及 OR 后 AF 未定义；CALL #75 B2BB 回址 B2C0 写障保留六参，DrawPort 未调用；绘图 callee/site 深层、生产父栈、原版差分仍 partial，定向 core/ASan `proc_3722` 各1/1。
新增块176：case14 晚期绘图返回后重读 Y/源/Oy/H/P，压底边后再读源/W/X 及 Oy/Y 并压右/顶/左；14站独立核 actor/resource/stack 的 token/kind/ESP/FLAGS/DF、访问序号、已压参数及 SHL 的 AF 未定义，P10/H7 得 `(23,68-EBP,19,63-EBP)`；CALL #74 B246 回址 B24B 写前障保留四参，绘图留下的父栈前缀未清，深层 callee/site 与原版差分 partial；定向 core/ASan `proc_7224` 各1/1。
新增块175：case14 晚期首矩形后对 actor+2958 的 word 分三次物理重读，各自取负并写 4CD71C/30C/304；首写后才压 EBX 辅助值。7 个站核 token/kind/ESP/FLAGS/DF、访问序号与旧全局值，第三次读前 EDX 清零；全局 P9 完成后受控改 P10，首图续段独立重读 flags/source/H/W/P/Y/Oy/X，13个站核五参 `(flags|4,7,5,16,23-EBP)` 的物理前缀及 OR 后 AF 未定义。CALL #73 B1E7 回址 B1EC 写前障不进 DrawPort；定向 core/ASan `proc_7210` 各1/1，绘图 callee/site 深层、生产父栈及原版差分 partial。
新增块174：case14 晚期首矩形先重读源/Y/H/Oy、MOVSX AX 到 signed P 并翻倍，先压底边 Y+H/2-Oy，再于 B150 将2P写本地 ESP+10h、B154 读回，续读 W/X 并压右/顶/左 `(X+W-2P-EBP,Y-Oy,X-2P-EBP)`；14站核 token/kind/ESP/FLAGS/DF、访问序号、本地写前后与已压四参，P9/H7 得 `(19,30-EBP,16,25-EBP)`。CALL #72 B174 回址 B179 写障仅父级参数，callee/site 深层、生产父栈、原版差分仍 partial；定向 core/ASan `proc_c2a3` 各1/1。
新增块173：非8相位在 B110 全宽 ADD EAX,4，B113 仅将 AX 低16位写 actor+2958，B11A 跳共享尾；受控 phase7 与图像回包 EAX12348000h 在写前障保留旧相位7、EAX1234000Bh/ADD FLAGS/ESP+80/DF/token/kind/instruction，正常落相位11且不截断 EAX 高位。受控 FFFF8000h 与 phaseFFFEh 全宽累加回绕到 EAX2/CF1；定向 core/ASan `proc_54cd` 各1/1，生产父栈及原版差分仍 partial。
新增块172：仅当辅助 INC 后重读等于 AX8，B102 写 actor+2958 为9，B10B 跳共享尾；写前停点保留主相位8与辅助8，核 token/kind/ESP+80/EAX12340008h/CMP16 零标志/DF/访问序号；正常返回主相位9且 FLAGS/EAX 保持，辅助不等时跳过。定向 core/ASan `proc_5919` 各1/1，原版差分未核。
新增块171：B0EE 对 actor+2954 的同一 INC word 分读、写两处物理停点，写前值7保持，B0F5 重读见8；三站核 token/kind/ESP/FLAGS/DF/EAX 高16位/访问序号，7→8 的 INC 不改来自 CMP AX8 的 CF，辅助0→1 与 AX8 不等时保留旧主相位8及 CMP16(1,8) FLAGS。定向 core/ASan `proc_efee` 各1/1；父层与原版差分仍 partial。
新增块170：第二图 CALL 返回后20参仍在栈，B0DE 读 actor+2958 word 覆写 AX 低16位、高16位取受控回包，ADD ESP50h 回收80字节再 CMP AX,8。读前故障保持旧 EAX/ESP/FLAGS/DF，下一站 B0EE 见 EAX=12340008h 与 CMP16 相等标志、辅助标记取消；受控非8与高位回包向量分别在 B113 写前见完整 EAX=1234000Bh、FFFF8000h/phaseFFFEh 加4回绕至2/CF1，不将关联块 B110 或辅助块171–172提前标 reviewed。定向 core/ASan `proc_a185` 各1/1；生产父栈及原版差分仍 partial。
新增块169：case14 第二矩形回复后沿用 EDI=-原始 P，按 4CD71C/30C/304 写三全局；3个站核旧值前缀、token/kind/ESP/FLAGS/DF。第二图独立读源/flags/H/W/Y/Oy/X，依次压辅助 EBX 与 `(flags|4,H,W,Y-Oy,X-旧EBP+本地2P)`；14站核访问序号与已提交参数，B0D1 重读本地2P 后切 EBP，OR 后 AF 未定义。第一幅图像的辅助已压标记继承至第二幅，不能在新辅助 PUSH 前误判为 false；已修正测试前缀。CALL #71 B0D9 回址 B0DE 写障不进 DrawPort，定向 core/ASan `proc_319c` 各1/1；callee/site 深层、真实父栈及原版差分仍 partial。
新增块168：case14 第二矩形先取本地 signed P 并 ADD 翻倍存回原槽，随后交错读源、H/W、Y/Oy/X，并依底/右/顶/左压 `(Y+H-Oy,X+W+2P-EBP,Y+H/2-Oy,X+2P-EBP)`。17 个停点核 token/kind/ESP/FLAGS/DF、访问序号、存本地前后和已压参数；H7 半高3。受控续帧 P=-1 时 B032 写前本地 FFFFFFFFh、EDX=FFFFFFFEh，B036 写后本地 FFFFFFFEh、右46-EBP/左41-EBP；此人为向量不证明原版差分。CALL #70 B083 回址 B088 写障只核父级前缀，callee/site 深层和真实父栈 partial；定向 core/ASan `proc_a8b9`、`proc_2fde` 各1/1。
新增块167：case14 首矩形四参仍存栈时，先读本地 signed phase、压 EBX 辅助值，再写 4CD71C/30C/304 三全局为同一负相位；5 个站的旧值、token/kind/ESP/FLAGS/DF、访问序号已核。随后读 flags/source/H/W/Y/Oy/X 并按 `(flags|4,H,W,Y-Oy,X-2P-EBP)` 压五参，12 站核访问与已压参，OR 后 AF 未定义；B012 写前只剩前三绘图参已压，逐站诊断纠正原断言提前一站。CALL #69 B01B 回址 B020 写障保留活参且不进 DrawPort；定向 core/ASan `proc_9839` 各1/1，callee/site 深层、生产父栈及原版差分仍 partial。
新增块166：case14 早期首矩形重读源、Y、资源高度、Oy，再存 signed phase 到本地栈槽；随后读 X/W 与本地 phase 并压底/右/顶/左四参。14 个物理站逐点核 token/kind/ESP/FLAGS/DF、访问序号、本地写前后与已压参数；H7>>1=3，phase8 四参 `(19,32-EBP,16,27-EBP)`，FFFFh 符号扩展 P=-1 时右/左为 `50-EBP`/`45-EBP`。CALL #68 AFBE 回址 AFC3 的写前障补完整活参和寄存器前缀，定向 core/ASan `proc_21a2`、活参全量断言复跑 `proc_2a25` 各1/1。矩形 callee/site 深层、生产父栈及原版差分仍 partial。
新增块165：case14 依序读 actor+2548 源 token、`[ECX]` 首 dword，先写全局 4CD730 再读 actor+2958 的 phase word；四物理站核 token/kind/ESP/FLAGS/DF、访问序号与先前已提交全局值。MOV AX 只替换 EAX 低16位，CMP AX,9 按有符号 JGE 将0/8/FFFFh/8000h引向 AF69，9/32 引向 B11F，独立核 PF/AF/ZF/SF/OF/CF 与原高16位；定向 core/ASan `proc_7d6f` 各1/1。声音深层、真实父栈及原版差分仍 partial。
新增块163：case14 phase0 由 CMP AX,BX 零值进入，先读全局 4AB784 到 EAX，再压 handle、31h，CALL #67 AF42 正常回址 AF47；三个父级站核 token/kind/ESP/FLAGS/DF、访问序号及已提交 handle，CALL 回址写障及 wrapper 前两次栈读仅核受控前缀，不外推深层。受控 wrapper `+85610/+8561E` 的两个读停点分别核栈位置与 ECX=handle<<7，core/ASan `proc_be59` 各1/1。深层 AIL/REP、真实父栈与原版差分仍 partial，模拟回包不证明原版 EAX0。
新增块157：case13 第四矩形后独立三次 signed P 重读、NEG 及 4CD71C/30C/304 全局写；首写后 PUSH 辅助零。7 个全局站逐处核 token/kind/ESP/FLAGS/DF 与旧写前缀，P=-1 三处值均1；随后重读 flags/source/H/W/Y/P/Oy/X，OR flags|4 的 AF 未定义、后续 XOR ECX/EDX 再覆盖 FLAGS，五参按 `(flags|4,H,W,Y-Oy,X+2P-EBP)` 压。13 个绘图站核访问序号、token/kind/ESP/FLAGS/DF 与已压参，P=-1 的 X 参数减2；CALL #65 AEF4 回址 AEF9 写前障不进 callee。core/ASan `proc_ed99` 各1/1；绘图 callee/site 深层、生产父栈及原版差分仍 partial。
新增块156：case13 第四矩形独立重读资源 token、Oy、H、signed P/Y，首 PUSH 底边 Y+H-Oy；随后重新清 EDI 并取资源 W、signed X，SHL P 后按右边 X+W+2P-EBP、旧 EBX=3Q 加新 Oy/Y 的顶边 Y+3Q-Oy、左边 X+2P-EBP 先后压栈。12 个物理访问站核 token/kind/ESP/FLAGS/DF、序号及已压栈前缀，P=-1 时右/左各减2。CALL #64 AE7E 返回槽写前障保持四参且不进入 callee，正常压 AE83 回址；core/ASan `proc_4ae7` 各1/1。矩形 callee/site 深层、真实父栈与原版差分仍 partial，整包状态不变。
新增块155：case13 第三矩形后仍按三次 signed P 重读、NEG 与三处 4CD71C/30C/304 全局写，首写后压辅助零；7 站的旧全局前缀及 P=-1 各结果1已核。第三图13站依 flags/source/H/W/Y/Oy/P/X 读取，OR bit2 后再 XOR EDX0，首 PUSH 前 AF 未定义；五参为 `(flags|4,H,W,Y-Oy,X-2P-EBP)`，P=-1 的 X 参数+2。逐站核 token/kind/ESP/FLAGS/DF/访问序号和压栈前缀，CALL #63 AE33 回址写障经 core `proc_b07c`、ASan `proc_b9a6` 各1/1；callee/site、生产父栈与原版差分仍 partial。
新增块154：case13 第三矩形先读 Y/P/Oy 再 ADD ESP50h 回收首两对矩形/绘图20参；清理后本地 Y-Oy 与 signed 2P 分写 ESP+20h/+10h，之后按局部时序重读；缓存 EDI=Q 保留，四参按 `(Y-Oy+3Q,X+W-2P-EBP,Y-Oy+2Q,X-2P-EBP)` 压。14 处读写站核 token/kind/ESP/FLAGS/DF、已写局部与栈前缀；受控 ESP FFFFFFB0h+50h→0 时首本地写 token20h，P=-1 的 2P=FFFFFFFEh 与左右边界+2已核。CALL #62 ADBD 回址写前障经 core `proc_8b4d`、ASan `proc_84e9` 各1/1；callee/site、生产父栈及原版差分仍 partial。
新增块153：case13 第二矩形回复后再次三次读取 signed P，NEG 后按 4CD71C/30C/304 写入，首写后压辅助零；7 个故障站及 P=-1 各值1、旧全局前缀已核。第二图13站依源/flags/H/Y/W/Oy/P/X 原序读取，按 `(flags|4,H,W,Y-Oy,X+2P-EBP)` 压五参；受控 P=-1 的 X 参数-2、OR ECX4 AF 未定义（生产共用实现已修正）、CALL #61 AD61 回址写前障与前20个活参经 core `proc_2a44`、ASan `proc_f7c2` 各1/1。绘图 callee/site、真实父栈及原版差分仍 partial。
新增块152：case13 第一组矩形4参和绘图6参仍活跃时重读 signed Y/Oy/P/source W/X，不重读 H 而复用 EDI=Q=1。由 `Y-Oy` 与 `2P` 依次压 `(Y-Oy+2Q,X+W+2P-EBP,Y-Oy+Q,X+2P-EBP)`；10 个物理读写站核 token/kind/ESP/FLAGS/DF、访问序号及栈前缀，受控 P=-1 的左右边界各减2。CALL #60 ACEB 回址写前障经 core `proc_6820`、ASan `proc_83ae` 各1/1；矩形 callee/site、生产父栈与原版差分仍 partial。
新增块151：case13 首矩形回复后重读 signed phase 三次并 NEG，依序写 4CD71C/30C/304，首写后先 PUSH 辅助零。7 处站的旧全局前缀与 `P=-1` 三处结果1已核。后续13处 flags/source/H/W/P/Y/Oy/X 读写站依原顺序压 `(flags|4,H,W,Y-Oy,X-2P-Ox)`，核 token/kind/ESP/FLAGS/DF、访问序号、已提交栈；OR flags|4 的 AF 应未定义，共用生产绘图参数实现已修正。`P=-1` 的 X 参数+2及 CALL #59 回址 ACA8 写前障经 core `proc_6dbf`、ASan `proc_7d77` 各1/1；绘图 callee/site、生产父栈及原版差分仍 partial。
新增块150：case13 首矩形的源 token／首 dword／全局 4CD730 与 Y/H/Oy/P/W/X 读取，H 无符号右移两位缓存 EDI；按 bottom、right、top、left 次序压四参，signed 2P 写本地栈槽后重读。3+13 个物理站逐个核 token/kind/ESP/FLAGS/DF、访问序号、已提交压栈与首全局写障旧值；受控 P=-1 的本地槽 FFFFFFFEh 和边界偏移+2 已核。CALL #58 返回槽 AC34 写障与正常压栈经 core `proc_cf8e`、ASan `proc_b14c` 各1/1；矩形 callee/site 合同、生产父栈及原版差分仍 partial。
新增块144–145：case12 两路 writer 正常回包后先 ADD word actor+2954,2，再 ADD ESP14h 清五参、INC word actor+2958，随后 XOR EAX0/四次 POP/ADD ESP14h/RET；四个 word 读写停点及返回五栈读站核 token/kind/ESP/FLAGS/DF。受控 motion FFFFh→1、phase FFFFh→0、ESP FFFFFFF0h+14h→4 证明 INC 保留 CF1；块145 signed phase>=30 则按主 phase、aux 次序写零，失败前缀 `(30,2)→(0,2)`，JMP 特有共享 reset +7B995。core `proc_1724`、ASan `proc_02ee` 各1/1，writer 深层、共享 reset 尾、生产父栈与原版差分 pending。
新增块141–143：case12 flags bit0=1/0 分别落到独立的 +7AB14/CALL #55 (`sub_423020`)、+7AB4A/CALL #56 (`sub_422C70`)；两路各自 XOR 清高位后读资源 H/W，MOVSX Y、源偏移与 X，再接已压辅助零构成五参 `(36,-45,5,7,0)`。每路10处物理停点、CALL 写障核 token/kind/ESP/FLAGS/DF；源偏移=-1 时 X44/CF1/AF1。反向 writer 正常返回后块142 JMP +7AB7E 保留回复寄存器、FLAGS、DF、累计五参和访问序号。core `proc_be7d`、ASan `proc_9ca0` 各1/1，两个 writer 的深层像素/原版差分及真实父栈 pending。
新增块140：case12 先读 actor+2B0、压 EBX0 后写 4A0698，再读 +2B4 写 4A069C；独立的两轮 MOVSX aux+2954/phase+2958 计算 `aux*phase+400h` 与 `400h-(aux+1)*phase` 写 4A06A0/+6A4。之后依次读源 token、源首 dword 写 4CD730、只读 flags 低字节 TEST bit0、再读源 token 才分流。11 个缩放站与5个资源/分流站核对 token/kind/ESP/FLAGS/DF/全局写前缀；aux32767、phase=-32768 的低32位及 PF0/PF1 分流已测。core `proc_c649`、ASan `proc_4dde` 各1/1；两路 writer 深层、生产父栈和原版差分 pending。
新增块085/148：case7/13 各在 `CMP AX,BX0` 相等后读全宽 4AB784 到 EDX，各自压 handle、31h 并在 CALL #30/#57 压回址 A28A/ABD1。每路三处父读写、CALL 栈写及 wrapper 前两次实参读核对 token/kind/ESP/FLAGS/DF、受控 `0x70123456<<7` 的 ECX；常规 ADD ESP8 在已审块086/149。定向 core `proc_cf5f`、ASan `proc_b311` 各1/1，AIL/REP 深层、生产父栈和原版差分仍 pending。
新增块138：case12 零相位由前块 `CMP AX,BX0` 留 ZF/PF1；`+7AA91` 读 4AB784 的完整 dword 入 ECX，按序压 handle/31h 并在 CALL #54 压入返回地址 AA9F。三处父停点、CALL 写障、wrapper 前两次栈读均验证物理 token/kind/ESP/FLAGS/DF 及 ECX=(handle<<7)；正常 ADD ESP8 已局部核对。core `proc_daf7`、ASan `proc_0959` 各1/1，AIL/REP 深层、原版差分和生产父栈仍 pending。
新增块134：case11 首图回复但第一组六参未清时先写 4CC2F0=0，再压第二组辅助零；对相位 word 重读三次，依次写 4CD71C/4CD30C/4CD304。第二图重新读 actor flags/source、H/W、signed phase、dword Y offset、signed Y/X/source offset；LEA×5 与 SHL×2 计算 `10*P`，受控 P=-15 得六参 `(39,-126,5,7,F4,0)`，8+14 个停点核对物理 token/kind/ESP/FLAGS/DF 和全局写前缀；CALL #53 返回槽单独核对。定向 core `proc_4ad5`、ASan `proc_5fbf` 各1/1；绘图深层/真实父栈/原版差分 pending。
新增块133：case11 首图先读 actor flags/source 后 OR ECX,14h；EBX0 辅助 PUSH 在 actor+2B4 读取及 flags PUSH 之前。源 H/W 均经 XOR 零扩展，+29B2、Y/X 三个 word 经 MOVSX，再按 Y-dword(+2B4)、X-i16(+29B2) 顺序 SUB；受控六参 `(39,24,5,7,F4,0)`、14 故障站与 CALL #52 返回槽 A9E8 已核。`+29B2=FFFFh` 时 EAX=FFFFFFFFh，X=43 得44/CF1/AF1；首次把39的 PF 错判，已按 0x27 四置位修正。core `proc_cdf7`、ASan `proc_3aa0` 各1/1，深层绘图/原版差分仍 pending。
新增块131–132：case11 首图前先读源 token/首 dword，按次序写 4CC2F0=15、4CD730=源 dword，再读相位 AX 且保留 EAX 高16位，signed `CMP AX,FFF0h` 的 JG 对 -15 跳过第二写，-16/-17/-31 均先 MOVSX+ADD31 后重写 4CC2F0。-31 的写障保留旧 mode15、源值和全宽 EAX0/CF1/ZF1；-17/-16 的 14/15 还分别验证 PF/AF。定向 core `proc_ef4e`、ASan `proc_b55a` 各1/1；真实 owner、原版差分 pending。
新增块078/129：case6/11 零相位先由 `CMP AX,BX` 生成 ZF/PF1，再读 4AB784 完整 sample handle，压 handle 与 31h 后 CALL #28/#51；三处父读写故障及 CALL 返回槽、两处 wrapper 栈实参读站核对 token/ESP/FLAGS/DF 与已推参数。首次定向失败由误把 header 的 FLAGS 用作本块比较结果造成，已按 LST 改正；定向 core `proc_f4b2`、ASan `proc_c2a9` 各1/1。深层 AIL/REP、原版差分及真实父栈仍 pending。
新增块073/123：case5/10 第二图各自依原指令顺序先写三处全局量，再读 actor flags/phase/resource/坐标并推出五个图像实参；case5 先 PUSH 辅助零再写 -24/-24/-24，case10 先写 EBX0/EBX0/16 再 PUSH 零/flags。12/13 个读写站的 token、ESP、kind、FLAGS/DF 和受控六参已核；定向 core `proc_537b`、ASan `proc_f917` 各1/1；CALL #26/#49 深层绘制 pending。
新增块074/124、CALL #27/#50：case5/10 第二图六参未清时按 LST PUSH 480、640、EBX0、EBX0；分别在子入口压返回槽 A18B/A920。两条受控站点各自验证 4 个 PUSH、CALL 写障、子调用 15 个物理故障站和正常 640×480 回包 EAX1/ECX640/EDX0；定向 core `proc_afa9`、ASan `proc_ba59` 各1/1。第二图深层语义、原版差分与真实父栈仍 pending。
新增块071–072、121–122：case5/10 第一图正常回复后先在六参未清时读 `actor+2548`，ADD ESP18h 产生 FLAGS，再读资源 `+0E` 的 height 到 AX、符号扩展相位、截低 ECX 后作 signed CMP/JLE；取 DEC 路径时更新完整 EAX 而仅写相位低 word。case10 受控 `0→0xFFFFFFFF` / `0x80000000→0x7FFFFFFF` 的 OF/CF，以及两路写障、源/高度/相位读站与清栈前后 FLAGS 已核。定向 core `proc_4315`、ASan `proc_b116` 各1/1；前置绘图内部、原版差分与真实父栈仍 pending。
新增块179 与 CALL #76：case14 早/晚分支共用全屏 PUSH 480、640、EBX、EBX 和 `sub_416FF0`，两条受控入站的 EBX0、不同 FLAGS/DF 前缀、640×480 正常回复及各15处子访问故障站已核。定向 core `proc_cb54`、ASan `proc_047e` 各1/1；原版差分和真实父栈仍 pending。
新增块178：case14 晚分支第四图正常回复后先清80字节栈参数，再 INC 相位 word；已存在的受控 ESP/相位回绕向量验证 `0xFFFFFFE0+0x50→0x30` 后 CF1，`0xFFFF→0` 时仍保留，读/写停点不撤销清栈。最近定向 core `proc_9716`、ASan `proc_2b03` 各1/1；第四图子调用和下游矩形尚未据此验收。
新增块159 与 CALL #66：case7/13 各自更新相位后共用四个全屏 PUSH 和 `sub_416FF0`；两个来源的故障前缀前两站保留各自 FLAGS/DF，受控 640×480 的正常返回与每路径15个子访问站经 core `proc_c597`、ASan `proc_6382` 各1/1；原版差分、生产父栈仍 pending。
新增块095/158：case7 第四图后的 INC word 相位保留前置 ADD ESP50h 的 CF；受控 ESP `0xFFFFFFB0+0x50→0` 配合相位 `0xFFFF→0` 仍 CF1。case13 相应 ADD word2 对 `0xFFFF→1` 产生 CF1；两条父路径的独立读/写停点、清栈和共享后续四 PUSH 站已核对。定向 core `proc_21b9`、ASan `proc_acf3` 各1/1；第四图子调用与真实父栈仍 pending。
新增块056 与 CALL #19：case3/4 第二次绘图正常回复后，相位 word 加2、清80字节实参并产生 FLAGS，依次压480、640、当前 EBX 两次再 CALL `sub_416FF0`；受控 EBX0、640×480 全屏子调用的15个读写故障站、四处顺序写与 EAX1/ECX640/EDX0 经 core `proc_f678`、ASan `proc_fb9e` 各1/1；原版差分、上游绘图和真实生产 caller 仍 pending。
新增块126 `+7A935..+7A94C`：case5/10 共用终止尾先写选择 byte2 和相位 word BX0，再 POP 三次、XOR EAX0、POP EBX、ADD ESP14h 和 RET；两次写及五次栈读的受控故障和正常返回已经核对。定向 core `proc_7080`、ASan `proc_b0ee` 各1/1；未代替生产父栈绑定。
新增块081/135/189：case6 一组六参、case11/15 两组六参在绘图正常返回后分别清24/48字节；三条 DEC word 相位尾均保持 `0→0xFFFF`，并有七个可停读站与最终 ADD ESP14h/RET 本地验证。定向 core `proc_7080`、ASan `proc_b0ee` 各1/1；前置绘图子调用和真实入站仍 partial。
新增块075/125：case5/10 在裁剪子调用正常回复后，按 word 给相位分别加14/4；`0xFFF8→0x0006` 与 `0xFFFE→0x0002` 保留回绕，随后清40字节栈槽、XOR EAX、四 POP、ADD ESP14h 与 RET。各七个读写停点和受控返回经定向 core `proc_f0b3`、ASan `proc_01b8` 各1/1；裁剪子调用和真实父栈仍 pending。
新增块057/160/180：矩形子调用正常返回后，三段八指令父尾各清四栈槽、XOR EAX0、四 POP、ADD ESP14h 和独立 RET；受控正常返回与分支停点已由定向 core `proc_a34e`、ASan `proc_c57c` 各1/1 覆盖。矩形子调用仍 partial。
新增单块031 `+79AF1..+79B05` 绘制正常回复后相位 word RMW、六实参清栈、四 POP 和 RET 顺序双向核对：phase `0xFFF0` 加 `0x1F` 回绕 `0x000F`，CF/PF 置位，SF/OF 不置位；定向 core `proc_a34e`、ASan `proc_c57c` 各1/1。绘图子调用自身仍 partial。
新增单块010 `+79916..+7991F` 的四 POP/MOV/ADD14h/RET 顺序与五个可停读站已双向核对；仅在前置 `sub_47E950` 正常返回后成立，非空链与深层 release 仍未闭合。
本轮补查 `+799BD` selector byte 读障、100-byte
选择表全部值与19个 dword 跳转目标；case1/2 的
16-bit 相位边界和 case3/4/5/10 的有符号入口，以及 case6/11/15
的有符号 `−32` 门、零音频比较和共享两次
word 清零；case8 的 phase100/固定 emitter
分流、case9 的 signed45 门与零音频比较；
case2 属性叶正常回复后仅 EAX1 触发 byte OR
（独立 RMW 故障及 EAX0 跳过），
case7 `>=32` 与 case14 `>32` 互异门、
case12 `>=30` 和 case51 `>=100` 的 signed word
与独立零 phase 比较、99/100/`8000h` 等边界，
两处首读障保持 selector GPR/FLAGS/DF/ESP；
case13 `>32` 与 `==32` 两侧、负数门及独立
`+7ABAD` 首读障亦已核对，四组绘图仍未审；
case1/3/4/5/6/7/9/10/11/12/13/14/15 的
十三处声音 CALL 正常回复后 `ADD ESP,8`
按地址逐条核机器码，并验证清两参的 ESP
与 ADD FLAGS；深层声音 callee 仍 partial，
`+A776` 为 case9 而非 case8；case100
首次音频 `+B44A ADD ESP,8` 单独核
ESP/FLAGS/DF；第二次音频后 `+B681`
先清两参，`+B684` phase101 写前障保留
旧0x55与 ADD FLAGS，正常写101；
case2 音频后的 `+9C5B ADD ESP8`、
`+9C5E` phase1 写前障保留旧 phase4、
FLAGS/DF/ESP，+9C67 跳共享粒子尾；
case8 音频后的 `+A743 ADD ESP8`、
`+A746` BP 低字写 phase 与 +A74D
跳共享粒子尾逐指令核对；写前障保留旧
phase99、ESP/ADD FLAGS，BP=0x1234ABCD
正常仅写 ABCD，声音/粒子深层仍 partial；
case8 `+A717` 恰 EAX1 时对 `+E3C`
做 word OR，读/写障保持旧 `0xAB16`，
正常保留高 byte 并得 `0xAB17`/16-bit SF；
`sub_47CE70` 内部仍 partial；
case8/100 均验证 `+2B08` 完整 dword
恰1才覆写 emitter `+E24` 为 670/340，
值 0、2、`0x00010001` 跳过；case8
写前障保留旧 −30，case100 写前障保留
旧0且不触及随后的目标宽度，其他后继
配置与深层调用仍 partial；
case100 的 phase100/零 phase 比较和首读障，
以及零 phase 计数 signed >=6 与 aux word=24
比较顺序：计数5时独立在 `+7B42F` aux 读前
停住，保留 5−6 CF/AF/SF/PF/ZF/OF、EAX/ECX/EDX、
ESP 和 DF，不提前播放音频；
case100 绘图返回的负数奇偶余数经 DEC/OR/INC
修正，偶数 aux ADD4、奇数 DEC 各自的读写障及
`+7B4F0` 重读障保留已提交字节，signed −8
进入计数尾、−7 进入默认尾；绘图 CALL 后先读计数
再清六参的 `+7B4C5` 首读障核未提交栈和
GPR/FLAGS/DF；计数尾先 POP EDI 后写计数、
短尾三项 phase/counter 写及各自物理 POP/RET
停点已局部核对；`+E14` 固定 emitter token
首读障及 CMP 0 的复用／清空分支、属性回复
EAX 恰1才 OR word `+E3C` 的读写障也已局部
核对；共享粒子调用后仅 EAX 恰1写 phase100、
相同物理尾的逐 POP/RET 停点已局部核对；
主 phase100 释放门先读 `+E14` 再取 EBP 和
CMP 0，读障保留完整前缀，0 跳过释放而非
跳过后续重置；绘图、属性、粒子与释放 CALL
本身仍待审；case51 的 EAX 恰1才 OR
`+DE0` 低 byte，读写障保留上位 byte，
属性 CALL 本身仍 partial；case51 signed >=100
先写主 phase 零再进入与 case12 共享 reset，
首写故障保留旧 phase，后继 REP/CALL 待审；case50 signed 14/15、
FFFF/8000 门与零音频分流、首读障的
原 FLAGS/DF/ESP 已核，绘图返回后的六参清栈、
word 相位 INC 与逐 POP/RET 局部核对，绘图
CALL 本身仍待审；
case100 释放后清一参、case50 音频后清两参、
case51 音频后清含 decoder 六参的各自 ADD
ESP FLAGS/DF/ESP 已核，深层 CALL 未升级；
case51 扫描子调用正常返回后的四 POP/XOR/
ADD/RET 现拆成原生产链共用的局部续段：
以真实子返回首 POP 前障前缀，仅重置测试
元状态后逐点核四 POP/RET 的 GPR/FLAGS/
DF/ESP 和正常出口一致；未映射的子调用内部
访问不得当作父级序号，扫描 callee 仍 partial；
共享 reset 的 `+B801` 首写前障已验证旧 phase481、FLAGS/DF/ESP，后继 `+B808` 前障核 phase0 已提交；共享 REP 仍未闭合；
case51 reset child 正常回复后，父级先在
`+B9AC` 写 `+2AAC=0`（首写前 EAX 保持真实
child 回复值），再 MOV EAX1、`+2ABC=0`、
`+2AB8=1`；三处父级写前障与五 POP/RET
已核，reset child 内部仍 partial；
updater 正常 EAX0/1 分流及零回复尾四 POP、
ADD/RET 的独立读障（不升级其深层 CALL）；
默认尾四 POP/XOR/ADD/RET 的五个独立
物理停点；资源 `+2548` 帧 token、`[EDX+0C]` 低16位宽度、
32-bit EBP 减法回绕及 `+2694` mask 的四处独立
故障与先前已提交写。当前受测版本 Linux core
199/199（`proc_8f40`）、ASan core 199/199
（`proc_3693`）、Linux app 205/205（`proc_dd6d`）
通过。精确块状态及反向记录见
`build/workpack316/block-audit.tsv` 和
`bidirectional-review.md`；CALL/RET 的局部计数
仍分别是9/98、22/22 局部尾部，不代表其上游闭合。

## 原版诊断采集工具准备（不升级联合差分）

`analysis/tools/battle-actor-frame-oracle/` 新增独立 spawn 工具，
便携产物在 `build/vm/battle-actor-frame-oracle/`。
四处父 CALL 的五字节锚点及 `0x00479850` 入口按 LST 校验；
本地 `swd32.exe` SHA-256 为
`4c4c226876fd2f3169bfe62c58ede86bba59e0036b7cef4cfaf7d49475c03f2a`。
两份 EXE 的整体 `.text` 不同；四处 CALL、
`0x00479850..0x0047B9CA` 本体以及当前释放链所比片段逐段相同，
不能外推整份二进制等同。

采集按真实返回地址区分四处父调用，保存 actor 0x2B00 字节、
入口/正常返回通用寄存器、所列全局、栈128字节、帧头16字节及
链头首dword。块事件启用 Frida `annotate:true`；逐批落盘，
不因 leave 先发生而丢失随后排空的事件。异常回调返回 false，
保存异常地址和当时 actor/栈快照，但不宣称完整 SEH 链可用。
缺快照、重复文件、大小不符、空块或截断均不得计为有效完整采样。

FLAGS 只有 context 实际暴露 `eflags` 才保存；缺失显式记录，
绝不以零替代。全部嵌套记录、完整异常栈、SEH 分支及块队列
完整排空仍未经原版验证，`summary.json` 固定不宣称
`original_diff_verified` 或完整轨迹。本工具不写业务状态，
但 Frida 插桩会改变代码与时序，不能称无侵入观测。

最终快照离线证据：`proc_5cef` 的模拟 Frida 回调测试与8项
记录器测试通过，Windows打包成功，打包EXE仅执行 `--self-test`，
Frida16.5.1/device/agent 自检通过。打包 agent 与源码 SHA-256 同为
`4a64886f19c0e614e7045d054c1cd782b1e0c12ddce34e3391843ea28ec9b8c3`。
这些测试均未 spawn 或 attach 原版，不证明实际挂钩成功。
构建脚本禁用 UPX，TEMP、Python缓存与 PyInstaller缓存均指向
仓库内 `build/tmp/runtime/`。此前失败构建留下的61份用户目录
缓存已迁到 `build/tmp/migrated-system-tmp/pyinstaller-20260930-wp316/`，
清单哈希61/61核验，原位置不存在；失败便携目录保存在
`build/tmp/runtime/oracle-partial-upx/`，未覆盖用户资产。
`proc_5cef` 最终快照阶段门禁 core200/200、ASan200/200、app206/206
全部通过，定向回调与8项记录器测试同样通过；这些不是原版运行证据。
316 深层合同与真实生产绑定仍未收敛，inventory 保持 `pending_audit`。

### 首次原版回传与v2修正

用户交回
`build/vm/battle-actor-frame-oracle/battle-actor-frame-oracle-output/run-20260930-094832-10840/`，
操作为进入战斗、按A自动战斗、战斗结束、Alt+F4退出。
`proc_8beb` 只读核验512对入口/返回和3072份快照的大小及SHA-256均一致，
无记录到的异常、采集错误或丢块报告。仅覆盖最终组A476次、最终组B36次，
行动组B及对手组A未采到；全部512样本缺FLAGS且达到总上限。
组A476对角色字节未变，组B36对有变化；不能由此证明完整战斗覆盖。
原始采集保留，不补造缺失数据，也未升级原版差分。

LST重新确认目标读取 `+2B00/+2B04/+2B08/+2B20`，
v1的0x2B00快照不够。v2扩为组B完整步长0x2B28的公共前缀，
不冒充组A完整0x2F34或全部嵌套输入；相邻可读映射可连续读取，
不可读缺口依旧明确标记。v2逐调用台账保留识别到的四处调用/返回，
完整快照预算改为每处4096；捕获片段变化和每64次重复采样，
预算耗尽、重复筛选不冒充完整场景采集。

原生FLAGS桥严格限定Frida16.5.1 ia32，以该版
`guminterceptor-x86.c` 的
`GUM_FRAME_OFFSET_CPU_FLAGS=GUM_FRAME_OFFSET_CPU_CONTEXT+sizeof(GumCpuContext)`
读取保存的PUSHFD字，即cpu指针后36字节，九dword上下文由编译期尺寸校验。
`proc_5bc1` 独立自有32位PE的32对入口/返回验证
CF/PF/AF/ZF/SF/DF/OF，不读取或启动原版。
完整联测先暴露跨可读映射误判，再暴露Stalker默认可信内联缓存路径
不能保持OF。修正映射扫描并设置 `Stalker.trustThreshold=-1` 后，
`proc_0114` 实测32次调用都返回、四处各8次，故意设置每处预算2，
得到8组完整快照、24次明确预算跳过及七项FLAGS精确匹配。
同步块callout不依赖延迟队列，结束标记由接收块计数和截断状态核验。
异常FLAGS只在可读、身份吻合的Win32 ia32 CONTEXT上观察，
复制至独立对象，不能写入原始CpuContext；完整SEH链仍未捕获。

v2输出独立 `build/vm/battle-actor-frame-oracle-v2/`，
不重打包或清理含用户采集的v1目录。最终v2快照 `proc_b60a`
通过模拟回调、13项记录器测试、原生桥32对测试、完整采集器32次调用联测、
Windows打包与便携自检，以及core200/200、ASan200/200、app206/206。
打包与源码agent SHA-256均为
`f50932e1ab4b820500efeabc924737fc4ac5db7c81d743cf72aba5a7480bf01a`，
桥源码与打包文件均为
`cb07e93b98a76c355c25a8bd53930392e2e88afa82bad70a9ba4cbe6b3dabef3`。
补强联测 `proc_e221` 还验证七个非EAX通用寄存器入口/返回一致、
全部32次返回EAX=1，八组角色变化严格只含自有测试代码预设的单字节增量。
同进程只读重核原始3072份快照哈希/大小及旧agent哈希均未变，
系统用户目录的违规缓存仍不存在；v2共68份打包文件的哈希清单保存在
`build/tmp/runtime/wp316-oracle-v2-package.sha256`。
这些工具测试不等于原版v2运行，316完整CALL/RET、生产绑定和联合差分仍未完成。

### v2原版回传核验与默认返回复放

新原版材料位于
`build/vm/battle-actor-frame-oracle-output/run-20260930-123416-10172/`。
`proc_392e` 只读核验实际注入来源与发布v2哈希一致，
2424份角色、栈和帧头快照的大小、SHA-256、索引及事件引用全部一致。
416次调用均有返回：最终组A398次、最终组B18次；
其中404组完整样本为A386组、B18组，另12次明确按已捕获片段重复筛选，
没有预算耗尽、未返回、异常、warning或采集错误。
808次入口/返回状态均具有原生PUSHFD FLAGS；404条同步块序列
合计1128块，接收计数、结束标记及无截断条件吻合。
链头首字的808次 `null_pointer` 是观察到的空指针，不改写为节点内容。
行动组B及对手组A仍未观测；这不是四处调用全覆盖或最终联合差分。

按LST独立分离386组 `actor+2ABC==0` 输入：
`0x00479850..0x00479861` 完成四个保存寄存器压栈与门比较，
随后直接到 `0x0047A80B..0x0047A814` 四POP、XOR EAX、ADD ESP14h、RET。
该路径不调用子函数，不写角色。386组实际角色0x2B28字节逐字节未变，
实际Stalker序列均为 `[479855,479867)`、`[47A80B,47A815)`；
入口最初五字节被Interceptor搬入跳板，不伪称已经动态追踪到这些原址。
最小观察标量和逐样本来源哈希保存于
`artifacts/battle-actor-frame-default-00479850/`，未提交原始游戏内存包。

复放之前先核Frida16.5.1 `guminterceptor-x86.c` 和 `guminterceptor.c`：
ia32 `gum_emit_leave_thunk` 的 `stack_displacement=-sizeof(gpointer)`
使CpuContext.esp仍指向next-hop字，离开跳板的RET才消费它；
所以原始leave ESP保留在夹具，实际父级返回ESP单独按 `raw+4` 换算。
`gum_function_context_fixup_cpu_context` 在begin/end两处均将EIP设为函数地址，
故采集中的leave EIP=479850不是游戏RET目标；返回地址只能由已核验的
最终组A CALL锚点及真实调用方分类确定为45AA38，不称EIP动态零差异。
这些观测归一化不写原始采集、不改寄存器、不改变游戏业务。

`legacy_battle_actor_frame_oracle_default_test.cpp` 将全部386组真实入口标量
送入生产 `advance_legacy_battle_actor_frame_entry_route`，不提供受控子回包。
EAX/ECX/EDX/EBX/EBP/ESI/EDI、归一化后的ESP、CF/PF/AF/ZF/SF/OF及DF
均与实际leave一致；RET状态、父级返回地址、角色门字段和零子调用同时核验。
仅入口门字段被该路径读取，未捕获的完整组A尾部或嵌套输入不作为零值使用。
`proc_3186` core200/200，`proc_dab2` ASan200/200与app206/206通过，
其中 `battle.actor_frame_316` 实际编译并运行新增复放测试。
该结论仅为这386组默认返回的标量/寄存器差分；
原始summary的 `original_diff_verified=false` 不改写，
18组非默认路径、另两处调用、完整CALL/RET与真实生产绑定仍待收敛。
因此316仍为 `pending_audit`，不得将本测试单独作为最终REVIEW或发布完成。

### 18组非默认路径与17组动作数据差分

`proc_7ba4` 核18组最终组B非默认样本均进入selector=3，
由LST的 signed phase 与20h比较分为三条实际轨迹：
sequence337为phase0→2，21块、8次父级CALL，包含一次音频；
另16组phase2..32每次加2，20块、7次父级CALL；
sequence372为phase34→0，15块、3次父级CALL并进入共享重置。
合计356块、123次父级CALL、9个独特CALL站，与386组默认路径的
772块相加恰为回传的1128块。17条绘制轨迹各两次绘图和三次矩形；
这只是动态路径事实，不能把这些调用的深层合同自动升级。

LST `0x00479887/0x00479889` 在更新前仅对slot2动作记录
写captured profile和base variant24h。绘图/矩形不以slot2为参数，
父级后缀也不写此记录；相应17组返回记录可用于动作数据局部比较。
sequence372在 `0x0047B816 REP STOSD` 清空整个0x98字节slot2，
其返回记录不是 `sub_4321E0` 的输出，因此明确排除，不能拿零记录
伪作更新器期望结果。`proc_f11e` 核各组原始角色大小/哈希并提取
两份0x98字节记录，最小夹具、来源及本地六份ACT哈希保存于
`artifacts/battle-actor-frame-action-00479850/`；夹具SHA-256为
`1c96aa915b281bfc0e216ddd3df68b7eece84581f594cf5863523acda8e77713`。
原版VM的ACT文件哈希及 `[0x004FB308]` 缓存设置未捕获，不推断与
本地文件身份相同，也不将两个设置中的任何一个称为原版实际设置。

新增 `legacy_battle_actor_frame_oracle_action_test.cpp` 使用生产
`LegacyActRuntime / LegacyActActionStreamProvider / LegacyActionUpdater`，
从原始before记录应用上述两条父写，再分别走cached/direct真实ACT路径。
17组各比较148字节及stream-pointer可用性，全部零差异；
`+0x54` 按既有64位指针适配只比较零/非零，不比较原版guest地址身份。
没有注入受控子回包、命令流、缺失的通用寄存器或FLAGS。
该测试不执行完整父函数，也不比较更新器的寄存器、FLAGS、栈、SEH或
内部控制流，不能称18条非默认联合复放通过。

`proc_a876` 初次定向1/1通过，但重编译暴露旧测试中五处类型/优先级警告。
相同计数域改用size_t、parity测试保持popcount的整数域、条件加括号后，
`proc_ef22` 对最新代码快照重新执行core200/200、ASan200/200、app206/206，
均通过且没有编译警告、测试失败或sanitizer错误；三组LastTest日志均明确
打印17条动作记录、两种ACT缓存设置的实际复放，不是条件编译跳过。
该测试随当前已验证的生产修正共同进行阶段审查与发布，不独立作为316关闭提交。

`proc_9fa5` 对18组输入再核LST及实际事件，保存 `nondefault-paths.json`：
36份栈和36份帧头均可读且哈希一致；36次链头为空是原始观测。
未捕获的直接依赖包括矩形宽/高限制 `[0x004A0E78/0x004A0E7C]`
（`0x0041700A/0x00417019`）、首次音频参数 `[0x004AB784]`
（`0x00479CBC`）、ACT缓存设置 `[0x004FB308]`（`0x0043240D`）。
角色资源 `+0x0C` 指向的字节、ACT/TSW缓存节点、源图像流、目标像素及
行存储、音频表/后端、子调用通用寄存器/FLAGS、深层栈和SEH也未捕获。
`0x004799A3→0x004799AB` 只能约束资源字节的bit5，不能生成整个字节；
宽高限制不默认为640×480，缺失数据不以零或合成回包替代。
当前材料不足以证明完整非默认联合差分，但这些缺口不替代仍待进行的
静态深层合同和生产绑定审查。316继续 `pending_audit`。

### 效果3地址核验与原版重置尾段受限差分

重新从LST核效果3 `0x00479CA6 MOV AX,[ESI+2958h]` 至共享重置返回，发现该首读
没有核物理ESI与提供的actor backing是否相同。旧C++会借用错误角色的相位字段。
现与其它相位入口一致，在没有对应backing时于真实首读前typed-stop；token仍是
物理ESI+2958h，不消费访问、不覆盖AX、FLAGS、DF或ESP。这是既有非法地址平台边界，
不是新增游戏逻辑nil guard。异角色ESI独立向量在 `proc_743c` 中先失败，
随后 `proc_ecce` 定向1/1通过。相同审查另发现重置子函数两处寄存器赋值延后；
304个REP写障和8个文本写障的两个DF方向寄存器检查、修正及旧证据更正见
`battle-actor-runtime-reset-00478850.md` 第10节。正常写序与RNG消费不变。

sequence372 的相位34经 signed `CMP AX,20h / JG` 到 `0x0047B801`；该段先写
相位与进度、清slot2的38个dword、在 `0x0047B81A` 执行真实typed重置子函数，再按
三次父写、四POP、ADD ESP14h和RET返回。尾段六个实际角色输入均已捕获：
相位word34，`+0D9C/+0D94` byte0，`+2B00/+2B04/+2AA0` dword0。
因此本尾段不复制坐标，也不调用RNG，不需要缺失的绘图、声音或ACT/TSW输出作为读入。
本路径不读的旧角色字节不能被称为真实输入。

`proc_678c` 先只读核这些字段；最终 `proc_bd7e` 从原始frame／stack文件核大小、哈希、
真实父返回地址45ACC4，以及同步目标块，再按LST的35个固定写、7个独立STOSD、
文本／哨兵写和父级后缀提取写掩码。两次304-dword REP在站点清单中分别保留，
仅最终数据比较的地址集合去重，共2010个字节、34条期望字节游程。
来源、六个读字段、56个写站点及原始标量保存于
`artifacts/battle-actor-frame-reset-00479850/`；夹具SHA-256为
`57164b2f20d25a87af68ffb5b644fd0338f809a6180eed80dbf1a8e88069a090`。
首次提取对Stalker块边界的假定错误，随后又让LST续字节行覆盖指令行，两次断言失败；
均按原始块边界及实际带opcode的指令行修正，未修改原始材料或降低真实字段断言。

`legacy_battle_actor_frame_oracle_reset_test.cpp` 只从效果3相位入口执行上述生产尾段，
不执行其前置ACT/TSW调用，不伪造子回包。ESI、EDI、EBX、ESP及EAX高16位由已核
LST前缀、平衡实参栈与callee保存合同派生；这些不是捕获的479CA6寄存器现场。
未观测且在尾段被覆盖的EAX低16位、ECX、EDX、EBP和FLAGS不冒充原版值。
三个明确标记为测试数据的不同哨兵扰动这些死GPR及所有即将覆写的旧字节，
初始算术FLAGS保持unknown，不把默认成员值称为观测；随后只导入六个真实读字段。
RNG不可调用，若被实际调用则终止测试，不返回合成值。

三种扰动下，2010个写字节、七个父级返回GPR、归一化ESP、已定义算术FLAGS与DF
都与原始leave零差异。原始leave ESP仍保留，guest ESP按已核Frida16.5.1跳板消费
next-hop字单独+4；EIP不以Frida修正值比较，父返回地址还实际核了原始入口栈首dword。
`proc_bd7e` 定向1/1实际运行该尾段及既有17组动作数据差分，但编译有四处size_t到
32-bit字段的警告；将夹具元数据按guest偏移／长度域定义为u32后，`proc_7c70` 对
最新代码重跑core200/200、ASan200/200、app206/206全部通过，三份完整日志没有
warning、测试错误或sanitizer报告；三份LastTest均实际打印17组动作数据及本尾段比较。
源manifest／events和夹具哈希再次吻合，未改原始采集。
这里不比较前置callee输出、内部逐指令trace、异常页或SEH，更不是18组完整非默认
联合复放。这些修正、测试与夹具随已验证阶段共同审查与发布，不构成316最终REVIEW验收，inventory不升级。

### 资源释放默认回调、内部校验与报告栈前缀

共享生产入口 `read_release_callee_prefix()` 由case2的 `0x00479C94`、
case100的 `0x0047B6F9` 和链节点的 `0x0047F0E1` 释放路径使用。
这里只推进明确持有的 `0x0048AA70` 默认回调；缺回调、持有零目标和
其它目标继续保留各自真实前缀，不以静态初值补生产绑定。
`0x0048AA79 RET` 只回到 `0x00488626`，不能当作完整CRT释放返回。

前一已发布快照完成默认叶的保存／返回、七个实参出栈，以及
`0x00488658..0x00488F44` 两层内部校验。非零header继续停在
`0x00488F52` 的 `IsBadReadPtr` IAT `0x004990BC` 首读前。
header零时，`0x00488F7D MOV [EBP-4],0` 不改变末次PUSH值；
`0x00488F84` 读回零，两个独立POP／RET只返回到 `0x00488FAE`
及 `0x00488661`，随后 `TEST EAX,EAX` 进入报告分支。
这些校验保留408个独立访问前障及36个零header局部／POP／RET前障，
DF不变；逻辑运算AF未定义，MOV／POP／RET不改FLAGS。
前一源码快照的 `proc_b069` 定向1/1、core／ASan各200/200、app206/206
通过，不据此宣称完整CRT、堆生命周期或316验收完成。

本轮从上述断点独立核LST机器码，不用未验证草稿作为行为真值。
以父级压释放实参之前的ESP为P，真实可达报告前缀为：

```text
访问  指令地址   动作及写值             ESP写前   写地址
0     00488668   PUSH 0049B6EC          P-44      P-48
1     0048866D   PUSH 0                 P-48      P-52
2     0048866F   PUSH 000003F3          P-52      P-56
3     00488674   PUSH 0049B5DC          P-56      P-60
4     00488679   PUSH 2                 P-60      P-64
5     0048867B   CALL；返回槽00488680   P-64      P-68
6     0048AA90   PUSH CRT EBP=P-28      P-68      P-72
7     0048AA98   CALL；返回槽0048AA9D   P-72      P-76
8     00491FD0   PUSH ECX               P-76      P-80
```

`0x0048AA91 MOV EBP,ESP` 先设EBP=P-72；`0x0048AA93 MOV EAX,302Ch`
先于第二次CALL。九次栈写之前的FLAGS均来自外层TEST零：ZF／PF为1，
CF／SF／OF为0，AF未定义；每个前障保留先前已提交栈写与末次PUSH。
写入两个地址常量并不读取字符串内容，也不表示已经输出报告。

`0x00491FD1 CMP EAX,1000h` 的输入由前述MOV固定为302Ch；
`0x00491FD6 LEA ECX,[ESP+8]` 得P-72，`0x00491FDA JB` 不跳。
随后 `0x00491FDC SUB ECX,1000h` 得P-1048h，
`0x00491FE2 SUB EAX,1000h` 得202Ch，均保持32位回绕。
首个未建模读取是 `0x00491FE7 TEST dword [ECX],EAX`：
ESP=P-80、EBP=P-72、EAX=202Ch、ECX=P-1048h、EDX=20h，
末次PUSH仍为进入探测前保存的ECX；FLAGS来自末次SUB，
CF／PF／AF／ZF／SF／OF均0、AF有定义、DF不变。
未捕获栈页值，不能执行TEST、生成其FLAGS或宣称探测／报告返回。

独立测试使用三种合成ESP `001AFE00/00000020/80000044`、两种DF和
调试设置0／1／4，并分别走case2、case100和链节点三个现有生产continuation，
形成486个栈写前障；这些不是原版观测值，不证明父级真实栈／堆绑定。
链节点先按 `0x0047F0DE MOV ESI,[EAX]` 读取合成节点的canonical next，
再压节点参数；所以访问序号另加1，ESI保留这次读取值，不借旧ESI。
case2的38个清零写在释放之前，case100的后续清零在释放之后，
本轮不重排或回滚这些已经由父路径验证的写序。
同时核首读取的序号停止与不透明后缀停止，以及明确持有回调地址恰为
`0x00491FE7` 的反例。后者仍有七个回调实参和返回槽，ESP=CRT EBP-30h，
不能借用已经进入报告的ESP=报告EBP-8现场；EIP相等不足以分类。
原版报告首栈页、报告返回、INT3和SEH未观测，不补造。

`proc_680a / wp316-free-assertion-prefix-red` 13秒退出8：
旧生产实现恰两项新增断言失败，分别是162写障现场和首栈页读取现场；
同址异目标反例及既有测试未失败，编译无诊断。
补齐生产前缀后，原408个内部校验前障、36个局部／POP／RET前障仍保留，
零header的无序号不透明停止期望随实际前缀前移到 `0x00491FE7`；
原六个断言首PUSH前障不跳过，新486写障及三入口同址异目标反例共同验证。
独立机器码解析另拼接续字节、解码两次相对CALL和实际立即数，核三种ESP
的32位回绕及202Ch的六项算术FLAGS；首轮解析未处理CRLF而未找到指令，
修正只读解析器后吻合，不改LST、C++或测试，也不补读页值。

`proc_6c50 / wp316-free-assertion-prefix-final-gates` 282秒退出0：
定向1/1（0.56秒）、core200/200（21.24秒）、ASan200/200（34.49秒）、
app206/206（81.32秒）全部通过，四份正式日志编译／测试／内存诊断0。
三份LastTest均实际执行17组动作数据与sequence372重置尾段局部差分，没有跳过；
本轮没有执行原版、Windows门禁或完整非默认联合差分。
三个源码／测试文件的冻结SHA在门禁前后及完成后均匹配，文档状态另按实际结果同步。
正式日志与源码冻结清单：

```text
build/tmp/runtime/wp316-free-assertion-prefix-directed-final.log
build/tmp/runtime/wp316-free-assertion-prefix-core-full.log
build/tmp/runtime/wp316-free-assertion-prefix-asan-full.log
build/tmp/runtime/wp316-free-assertion-prefix-app-full.log
build/tmp/runtime/wp316-free-assertion-prefix-source-freeze.sha256
```
本轮只对明确范围的生产修正、三处消费者测试和证据共同进行阶段审查与发布，
不升级inventory，不把本阶段当作完整工作包REVIEW。
该局部前缀不替代全部98 CALL／22 RET、真实caller与CRT创建／销毁绑定、
完整非默认联合差分和最终双向REVIEW；316仍为 `pending_audit`。

### 查帧缓存容量的初值与有符号淘汰条件

从 `0x00479940` 的真实查帧依赖继续核LST。`0x004A6020` 的数据字节为
`00 00 60 00`，默认限额是6 MiB，不是零。`0x004315C0..0x004315C9`
只取四字节参数并原样写入限额，不清高位或钳制负值。
`0x00431721 CMP ECX,EAX / 0x00431723 JL` 在查找之前决定是否淘汰；
`0x00431ED8 CMP EDX,EAX / 0x00431EDA JL` 和
`0x00431F74 CMP EDX,EAX / 0x00431F76 JGE` 也按signed32比较。
`LegacyTswRuntime` 的原默认值零及unsigned条件与上述指令不同。
最小修正仅恢复默认值并以 `bit_cast<i32>` 比较原u32存储；
setter、u32长度加减、一次选桶、尾部删除及查找先后不改。

独立合成向量先用正限额建立桶0的 `[10,0]` 和桶1的 `[1]`，每帧4字节，
总计12字节。十五种限额为0、1、4、7、8、9、11、12、13、
`7FFFFFFE/7FFFFFFF/80000000/80000001/FFFFFFFE/FFFFFFFF`。
0..8及四个负限额删除桶0两项，9..12只删旧尾，13及两个最大正值不删。
请求旧尾0后分别验证8／12字节总量、各桶节点数、加载次数、
命中／重载身份和ECX／EDX。输入桶非空且计数小，不覆盖空桶错误释放、
16位计数回绕或大于2 GiB的实际主流分配。

真实TSW消费者仍用现有 `LegacyBattleActorFrameTswUpdatePort`，先在正限额下
加载资源1和11，再用 `80000000/FFFFFFFF` 核淘汰先于查找。
唯一非空桶排空后，请求1必须重载为新记录和主流token；验证ECX主流长度、
EDX字节总量、miss的TEST1定义FLAGS、47×95尺寸及真实主流字节一致。
旧host租约只证明宿主借用生命周期，不表示原版已释放指针仍有效。
高位向量之后恢复正限额，避免把既有关闭重载测试变成空桶错误释放场景。

`proc_98ed / wp316-tsw-signed-capacity-red` 16秒退出8，两次定向各退出8。
旧生产源码两项SHA前后相同，恰三项新增断言失败：默认容量、十五阈值、
真实战斗查询端口的高位限额；没有其它断言或编译诊断。
本red是恢复测试容量之前的初版；新增断言本身未改，不把后续修订冒充旧快照。
动作17组及sequence372局部原版差分均实际执行，不是查帧原版差分。

首份冻结快照 `proc_0664` 389秒退出0：两个定向各1/1、core／ASan各200/200、
app206/206，日志诊断0。停止请求到达时实际已经成功结束，输出和五项冻结SHA
另核吻合，不称其被中途终止。该结果属于尚未恢复测试容量的旧测试快照。
随后只修正测试隔离及格式，生产文件不再修改；
`proc_0b5e / wp316-tsw-signed-capacity-isolated-final-gates` 198秒退出0。
新快照的两个定向各1/1（0.40／0.55秒）、core200/200（22.64秒）、
ASan200/200（33.30秒）、app206/206（81.05秒）通过，五份日志诊断0。
五项源码／测试冻结SHA在门禁前后及完成后匹配；三个LastTest各实际执行
一次17组动作与sequence372局部差分，skip=0。本轮未运行原版或Windows门禁。
正式日志及新冻结清单：

```text
build/tmp/runtime/wp316-tsw-signed-capacity-runtime-directed-v2.log
build/tmp/runtime/wp316-tsw-signed-capacity-battle-directed-v2.log
build/tmp/runtime/wp316-tsw-signed-capacity-core-full-v2.log
build/tmp/runtime/wp316-tsw-signed-capacity-asan-full-v2.log
build/tmp/runtime/wp316-tsw-signed-capacity-app-full-v2.log
build/tmp/runtime/wp316-tsw-signed-capacity-source-freeze-v2.sha256
```

本次不补造失败加载后的GPR／FLAGS、缓存cursor／共享全局或完整CRT释放。
空桶首轮尾部操作、16位桶计数回绕、四次深层free与失败停点仍待闭合。
两个原版caller缺口、四caller生产绑定、全部98 CALL／22 RET及最终REVIEW仍未完成；
本次不升级inventory，316继续 `pending_audit`。

### 查帧加载前的缓存登记与未知载荷命中

从独立LST完整复核 `4315D0..43175A`、`431C50..431DE6` 与
`433380..43353D` 的入口及出口。`431C93` 增桶计数，`431CB6` 发布新头；
八次清零及完整key/next写在 `431D0A/431D85` 加载调用之前。
普通零回复走 `431D91..431DA6`，不撤销节点、计数或头；外层只清返回全局。
后续 `431DF0` 只比较packed key，即使载荷未知也可命中并返回node+8。
`433380` 两条失败RET位于最终输出复制之前，但没有真实API及别名输入时，
不将宿主安全失败直接等同于这些原版出口，更不填补失败图像内容。

原 `LegacyTswRuntime::query_cached` 在装载成功后才分配/登记节点，
失败则留下无节点；其顺序与已提交前缀不同。
独立合成加载器只观察同桶variants13/23：进入时节点计数1/2，
累计主流字节仍0/4；cache-only已命中相同record身份，完成后累计4/8。
合成档案的未知变体先保留失败状态，再核节点与身份不被回滚，重复键不重载。
真实TSW端口缺特殊绑定时，首次不伪作返回；后续已登记键的命中只发布
记录身份、packed-key/bucket寄存器及TEST record定义FLAGS，资源头与解码源仍未知。

`proc_1cd5 / wp316-tsw-preload-publication-red` 16秒退出8：两个定向各退出8。
恰三项上述新增断言失败；没有旧断言失败或编译诊断。
生产四项及初版测试两项SHA前后相同，动作17组及seq372局部差分实际执行。
初版red测试包含整文件格式化；随后仅恢复无关格式，保留原快照与SHA，
不将当前测试快照冒充red。最终又加未知尾节点回收前障，不能称该新增断言也经red。

最小生产批次先登记缓存节点，再加载；节点的状态与图像可用性分开，
未知装载不撤销登记，重复键仍命中原节点。宿主加载期间独立持有节点，
图像租约仍是不可变快照，不代表原版物理缓存backing或释放地址保活。
Native lookup的未知载荷命中只恢复已知返回ABI，不提供零图像头/解码源。
现有actor+2548同步代码已在token改变时清除旧字段known位与旧lease，
没有重复增加额外检查。淘汰若需读取未知载荷的 `431F11` 长度，
保留此前前缀，不借空vector减零或补造四个free参数。

`proc_ff24 / wp316-tsw-preload-publication-frozen-gates` 391秒退出0。
冻结六项源码/测试在门禁前后及完成后SHA全部匹配。
两个定向各1/1（0.40/0.51秒）、core200/200（20.49秒）、
ASan200/200（33.04秒）、app206/206（81.20秒）通过，五日志诊断0。
三个LastTest各执行一次17组动作与seq372局部差分，skip=0。
没有运行原版或Windows门禁；不是查帧原版动态差分。
正式日志及冻结清单：

```text
build/tmp/runtime/wp316-tsw-preload-runtime-red.log
build/tmp/runtime/wp316-tsw-preload-battle-red.log
build/tmp/runtime/wp316-tsw-preload-runtime-directed.log
build/tmp/runtime/wp316-tsw-preload-battle-directed.log
build/tmp/runtime/wp316-tsw-preload-core-full.log
build/tmp/runtime/wp316-tsw-preload-asan-full.log
build/tmp/runtime/wp316-tsw-preload-app-full.log
build/tmp/runtime/wp316-tsw-preload-source-freeze.sha256
```

本阶段不模拟首次深层失败后的GPR/FLAGS/ESP、cursor/共享结果全局，
不闭合16位计数回绕、空桶错误、DF=1跨节点写、递归改变缓存与完整四free。
另外两处原版caller、四caller生产绑定、249跨块与98 CALL/22 RET及联合差分仍待完成。
316最终REVIEW未完成；不升级inventory，也不将局部门禁作为Goal完成证据。

### 未恢复查帧现场的有效性与caller传播

加载前登记阶段已发布。随后从LST重新核 `479937..479945` 与
`4315D0..43175A`：父先改CX低16、两参数PUSH及479945返回槽；
唯一正常callee RET在43175A。4315D0仅首全局4CF840读取，不是通用失败出口。
host查询可能先初始化、淘汰或发布节点，其失败既不能证明入口停止，
也不能证明默认EAX/ECX/EDX全零。原Native失败回复却指定4315D0，
父又在检查returned前复制零GPR，因而将未恢复状态误报为物理现场。

独立反例 `proc_93f5` 14秒exit8，定向1/1失败：仅七项新增断言失败，
一项Native停止位置及两DF×三ESP的父checkpoint，无旧断言退化或编译诊断。
五生产源与初版测试SHA前后相同；17动作及seq372局部差分实际执行。
原red完整文件/SHA保留；此后新增的有效性断言、四caller与首MOV三输入不称为red。

新回复以 `physical_state_known=false` 明确未知。实际TSW未知加载不造停止地址，
正常loaded miss/record-only hit仍提供已知返回。父不复制未知回复的默认零，
保留已确认的GPR/ESP/DF checkpoint，明确结果physical_state_known=false、
flags_known=false；EIP/stop地址0表示不可用，不是guest在零地址发生故障。
父不执行RET清栈或actor+2548写，不回滚已发生host缓存操作。
保留的寄存器与访问计数不是失败后真实CPU捕获，不能拿entry代替深层现场。

首全局不可读仍是精确4315D0/token4CF840，port未调用。
另三个controlled输入供应首MOV读值0/1/FFFFFFFF，于4315D5、首PUSH前停止；
MOV只改变EAX、不改ESP或FLAGS。显式有效零值不得被当成未知；
这些是由LST推得的供应输入，不是原版运行差分或完整加载失败恢复。
共享caller回收传播child有效性，只有已知RET目标/ESP才能续父正常路径。
四caller用完整owned fixture和实际TSW适配器核未知传播；原三个调用方文件的
!returned退出已有正确顺序，没有重复增加guard，也不将snapshot测试称为生产绑定。

首新快照 `proc_1fe2` 的定向/core/ASan已完成，审查发现测试块空行问题后
受管停止；实际状态killed，APP未形成完整通过证据。修正格式并补齐向量后
另冻结v2，不能把首快照结果直接借给最终源码。
`proc_67cb / wp316-tsw-opaque-state-final-frozen-gates` 192秒exit0：
定向1/1（0.53秒）、core200/200（20.15秒）、ASan200/200（33.20秒）、
app206/206（81.28秒）通过；四日志诊断0，六项SHA前后及完成后匹配。
三LastTest各执行17动作及seq372局部差分，skip0；没有原版或Windows执行。

```text
build/tmp/runtime/wp316-tsw-opaque-state-red.log
build/tmp/runtime/wp316-tsw-opaque-red-test-snapshot.cpp
build/tmp/runtime/wp316-tsw-opaque-directed-v2.log
build/tmp/runtime/wp316-tsw-opaque-core-full-v2.log
build/tmp/runtime/wp316-tsw-opaque-asan-full-v2.log
build/tmp/runtime/wp316-tsw-opaque-app-full-v2.log
build/tmp/runtime/wp316-tsw-opaque-source-freeze-v2.sha256
```

本批次只纠正报告有效性，不恢复首次失败真实GPR/FLAGS/ESP、SEH、Win32/CRT内部。
cursor/共享全局、DF=1别名、计数回绕、空桶四free、真实堆与四caller绑定仍待核。
249跨块、98 CALL/22 RET、联合差分和316最终REVIEW尚未完成；inventory不变。

### 装载返回后重新读取共享缓存节点

随后重新完整读取 `4315D0..43175A`、`431C50..431DE6` 与 `431DF0..431E93`。
431E4C/431E60逐次发布共享游标4DACDC，完整miss留0，hit停在命中节点。
431CD8将已登记节点设为游标，431D0A把其+8传给特殊loader；
431DBF在CALL后重读游标，431DCC从该节点+18取长度，431DCF/431DD4发布该节点+8，
431DDA把重读长度加入当前4DAD0C，u32回绕。431DE0/外层43174A仍按miss TEST1。
原C++虽以shared_ptr避免跨回调借list节点，却继续返回原节点、累计原payload长度，
不能代替原共享游标被后续调用改变的行为。

独立controlled输入：FFFF/0外层4字节，FFFF/10内层8字节，同桶0、限额7FFFFFFF。
内层嵌套load或非头hit后，外层重新选中内层，又计8，结果为内层record、ECX8/EDX16，
不是原record或总12；原外层payload仍按其key留存。嵌套FFFF/20完整miss清游标时，
不能替换回原节点继续，已装载字段/两个节点与既有总8保留；未恢复零地址backing、
故障或SEH，不造物理CPU。尺寸/stream是供应callee输入，不是原版40AD10捕获。

`proc_98e8` 17秒exit8，两定向各1/1失败：恰五新增断言，runtime的load/hit/miss三项、
实际TSW端口的load/miss两项；五生产与两初版测试SHA前后匹配，旧断言无退化、诊断0。
ACT17与seq372局部差分实际执行。red完整文件保留；此后节点移除、父continuation两例与
精确新状态断言没有参加该red，不扩写为所有最终向量都曾失败。

Runtime现在持共享weak节点选择，查找推进/命中/miss和登记按上序更新。
loader仍向原合法驻留节点发布字段，再重读选择，按它返回payload与重复计量，不修原BUG。
节点实际移除前resident失效，Host lease不能使已删除guest地址可读。
选择缺失/已删除/载荷未知时停止为不可恢复状态，不借零长度或正常旧节点回包。
该模型只覆盖节点选择；不冒充allocator期间暂存的桶地址或完整物理内存/别名。

实际TSW适配器与父pipeline本批次只读冻结：它们已从query.frame_owner消费返回，
新两父向量实际调用现有production continuation，正常只写内层record并绑定其lease；
未知不清CALL、不写actor、不回滚已提交cache。对应测试恢复field_4a、resource/lease、
render与progress，不污染后续场景，也不称fixture为四caller真实生产绑定。

`proc_5ab4 / wp316-tsw-shared-cursor-frozen-gates` 394秒exit0：
runtime定向1/1（0.48秒）、battle定向1/1（0.54秒）、core200/200（20.18秒）、
ASan200/200（32.85秒）、app206/206（82.32秒），五日志诊断0，七SHA前后及完成后匹配。
三LastTest各实际执行ACT17/seq372，skip0；没有原版/Windows执行，不是查帧动态差分。

```text
build/tmp/runtime/wp316-tsw-shared-cursor-runtime-red.log
build/tmp/runtime/wp316-tsw-shared-cursor-battle-red.log
build/tmp/runtime/wp316-tsw-shared-cursor-runtime-red-snapshot.cpp
build/tmp/runtime/wp316-tsw-shared-cursor-battle-red-snapshot.cpp
build/tmp/runtime/wp316-tsw-shared-cursor-runtime-directed.log
build/tmp/runtime/wp316-tsw-shared-cursor-battle-directed.log
build/tmp/runtime/wp316-tsw-shared-cursor-core-full.log
build/tmp/runtime/wp316-tsw-shared-cursor-asan-full.log
build/tmp/runtime/wp316-tsw-shared-cursor-app-full.log
build/tmp/runtime/wp316-tsw-shared-cursor-source-freeze.sha256
```

又完整核431F80..432006：431F9E读取长度，431FAC按项减总量，四free后431FEE写next游标。
若上述两个ready节点的四次free全正常返回且无重入/别名，16减8减4应剩4，不是无条件0。
本阶段当时host clear_cache仍重置总量，不能证明此原版路径；新增移除向量只核host owned节点失效，
不称为431F80整体副作用或四free验证。后续计数修正单独记录如下，未反向修改本阶段冻结或red范围。
40AD10已读至40AE1D，含AIL服务、allocator、文件/decoder/free；不据这些CALL推断实际递归。
DF=1、count16回绕、空桶四free、首次失败实际CPU、四caller绑定、249跨块/98 CALL/22 RET、
联合差分、316最终REVIEW与完整Goal均未完成；inventory保持pending_audit。

### 宿主清缓存后的残余计量与未知量传播

继续完整核431F80..432006、431960..431A10、433BF0..433C30与4315D0初始化。
431F8F先清桶head，431F97发布游标；431F9E取+18长度，431FA7保存next，431FAC减总量。
四次free及三次共享游标重读之后，431FE7减word count、431FEE写next；末尾没有total=0。
若所有free正常返回且没有重入/别名，两个known节点8/4、此前total16应留4。
关闭及重开DF=0 REP区域为[4DAD28,4FB0C8)、[4CF988,4DACD4)，均不含4DAD0C；
下一8字节miss应ECX8/EDX12。433BF0显式写buffer/其它计数，不写该total，
但不据此排除未知free回调或间接alias。DF=1首REP会反写经过该total，仍在未恢复物理范围。

独立红向量先用既有API编译：clear/close残余与reopen各两例；known bucket0/unknown bucket1/
known bucket3保留确认前缀4而非0，后续query不以它代capacity，close/reopen仍未知；
实际TSW adapter的clear/close两个消费者反例。
`proc_03fd / wp316-tsw-clear-balance-red` 17秒exit8，两定向各1/1失败：
恰七runtime/两TSW端口新增断言失败，无旧断言退化、diagnostic0；五旧生产/两初版测试SHA
前后匹配。ACT17与seq372局部差分实际执行。两个完整red测试snapshot保留。

候选Runtime现在减每个已知host owned payload一次，保留重复计量与u32减法回绕；
首未知长度后不推导其后扣量，stored bytes只是最后确认的prefix，known=false。
query_cached在需要total的容量比较前以cache_balance_unavailable停止，不造CPU或恢复零量；
close/reopen不重置未知性；find_cached/direct不依赖该量，不被无关禁用。
保持原有host生命周期清理而非声称原版未知长度读/四free已执行；节点数表示host所有权。
现有Native TSW查询和父pipeline只读冻结，已有非返回传播即可消费新状态，没有重复guard。

后补known getter/新状态精确断言、8-12=FFFFFFFC及signed容量/再加8=4、find/direct非依赖，
及两父continuation向量，没有参加上述red。旧移除向量的默认零量改为prefix8/unknown，
不借此前宿主零量假设反改原指令。父未知例保留CALL三槽/last PUSH479945，不写actor+2548、
resource/lease或progress，也不补实际停点；测试恢复原resource/render/progress。
对应状态与callee数据来自controlled输入，不是原版递归/清理捕获或四caller真实生产接线。

七项source冻结与362行/17864字节源码差异已完整审阅；
`proc_75a6 / wp316-tsw-clear-balance-frozen-gates` 407秒exit0：
runtime定向1/1（0.57秒）、battle定向1/1（0.56秒）、core200/200（20.41秒）、
ASan200/200（36.16秒）、app206/206（81.20秒），五日志diagnostic0，七SHA门禁前后/完成后匹配。
三LastTest各action1/reset1/skip0，本轮没有原版或Windows运行，不是原版清理动态差分。
材料前缀为`build/tmp/runtime/wp316-tsw-clear-balance-`：两个red日志/两个完整snapshot、
old-production/red-tests/source-freeze SHA、source-review diff及五个directed/full日志均保留。
该清理残余批次已提交、推送及TG；完整CRT/SEH、十桶物理游标/count/别名、DF=1、空桶错误、
首次失败真实CPU、四caller接线、249跨块/98 CALL/22 RET、联合差分、316最终REVIEW与完整Goal未完成。

### 初选空桶的哨兵输入不可用，不能续正常查帧

按完整431EA0..431F7F独立复核：unsigned16最大计数只选择一次，平数保留低编号；
431EDA signed JL退出，反侧进入431EEB桶base。431EF1/431EF3令EAX/共享游标=base，
431EFA取base+4，head=0时431EFF跳431F11，仍从base+18减量并按+8/+C/+10/base四次free。
后三次free参数来自前次之后重读的共享游标，不能用初始保存的bucket/node替代。
桶0的base是4CF848，不是零；不能把这里称为guest零地址fault，也不声明原版一定不返回。
真正非空桶尾删后431F56 DECword、431F67发现计数0即正常退出，不能误添第二轮哨兵释放。
本阶段不补哨兵payload/page、四CRT回复、共享游标实际写/重读、count16回绕或首次失败CPU。

基线ee4d6acd，五旧生产/两初版tests冻结。先用现有API编译的proc_5536 17秒exit8：
两个定向各1/1失败，五runtime/五Native TSW端口新增断言失败，旧断言无退化、两日志diagnostic0，
旧生产/初版tests SHA前后匹配；两个完整red snapshot保留，ACT17与seq372局部差分实际执行。
反例是fresh total0/head0、limit0/80000000/FFFFFFFF，以及known清理残余4/head0、limit0/4。
残余4/limit5的相反侧仍正常装载8bytes并得到ECX8/EDX12，不能把空表一律禁止。
这些controlled状态验证生产adapter的使用，不是原版空桶捕获或递归发生证明。

Runtime在初选空桶、缺少哨兵length/free载荷时显式cache_bucket_payload_unavailable，
不跳过淘汰、不调用loader或发布新节点，不改确认容量。它表示模型在缺输入处停止，
不是原版错误码；计量known表示停止前checkpoint，不是四free之后的原版物理内存。
Native TSW lookup和父pipeline只读冻结，既有非返回传播已消费该状态，不重复加guard。
不把回复零字段作为失败CPU：physical_state_known/flags_known=false，EIP/stop0是不可用标识。
后补enum精确断言与五父continuation向量没参加red；fresh三例/残余两例均保留CALL三槽、
ESP=prefix-12/last PUSH479945，不写actor+2548/render/resource/lease/progress，测试恢复保存状态。
父仅有该controlled方向/检查点，不称四caller真实接线或全部DF/故障前缀已覆盖。

源码差异265行/12878字节已逐项读完；七source冻结proc_6044 394秒exit0：
runtime/battle定向各1/1（CTest总0.70/0.67秒）、core200/200（20.87秒）、ASan200/200（33.47秒）、
app206/206（81.51秒），五日志diagnostic0、七SHA门禁前后/完成后匹配；
三LastTest各action1/reset1/skip0，未运行原版/Windows，本范围不是原版空桶差分。
材料前缀build/tmp/runtime/wp316-tsw-empty-sentinel-，包括旧生产/初版tests、新冻结SHA、
两个完整red snapshot、两个red日志、五directed/full日志与source-review diff。
该空桶批次已提交、推送及TG；当时count16/allocator暂存/物理别名、四free/SEH仍未完成。
首次失败真实现场、四caller接线、249跨块/98 CALL/22 RET、联合差分与316最终REVIEW仍未完成，315/422不变。

### 分配前独立16位计数及空head残留

完整查帧登记、淘汰和清理路径再次按LST核对：431C93 INCword先于431CA8 allocator CALL；
FFFF递增为0。431C9B发布桶游标、431CA1/431CA4保存旧head，431CAD在分配返回后重读游标。
431EB7 CMPword/431EBF JBE按unsigned16选最大，平数保留低桶。431F56 DECword后，
431F67先按减后word0返回，再决定是否继续容量比较；host list未空也不能继续删。
431F80..432006空head直接跳431FF5，不DECword；正常四free之后才431FE7 DECword。
DF0关闭/重开两REP不覆盖4DACE0..4DACF4，不能以close/clear把残留原计数归零。
分配返回后的共享桶游标、STOSD/DF、free重入/别名与完整CRT合同仍未闭合。

基线e71232b0，七旧生产/两初版tests独立冻结。proc_9c41 17秒exit8，两个定向各1/1失败，
恰五runtime/一Native新增断言失败，无旧退化、两日志diagnostic0、旧生产/初版tests SHA前后匹配。
六runtime场景为1次前缀、1次后clear、1次后close、FFFF/10000/10001次前缀；
非零word让空桶5胜出或平数胜出，不能按host节点数删桶6；10000回绕0相反侧原测试通过。
用guest身份登记临时阻止节点身份分配，只占身份、不提供physical页、不读该范围；释放lease后恢复。
这检验allocator之前的静态计数前缀，不证明原版连续失败、malloc0、异常、实际堆页或失败CPU。
两个完整red snapshot及SHA保留，限定ACT17/seq372差分实际执行，不冒称新计数的原版动态差分。

生产按独立u16保存计数，miss分配前递增，不因身份不可用回滚；unsigned选桶不用list.size。
已知payload/无别名宿主清理逐节点模16减量，空head不复位，close/reopen不补零。
尾删按减后word0返回；word仍非零但host头空时停缺哨兵输入，不再伪作成功。
host节点数getter仍只计驻留节点。首unknown长度后的计数只保留确认前缀，不是原版释放后word快照；
既有unknown累计量阻止后续容量依赖。宿主移除不证明四free正常、原版游标/head或SEH完成。
Native TSW/父pipeline/presentation header和guest身份登记实现均只读冻结，不增加重复传播guard。
后补精确status、FFFF前缀加两节点使word1而host2、已知清理后残留及三父向量均未参加red。
三父向量覆盖不清理/空head clear/close，保留CALL三槽、ESP=prefix-12/last PUSH479945及actor/lease/progress；
physical_state_known/flags_known=false，EIP0仅标不可用，测试恢复保存状态，不称四caller真实绑定。

新九source冻结proc_4abf 399秒exit0：runtime/battle定向各1/1（CTest总0.84/0.69秒），
core200/200（21.63秒）、ASan200/200（33.14秒）、app206/206（81.08秒）。
五日志diagnostic0，九SHA前后及完成后匹配，三LastTest各action1/reset1/skip0。
计数源码差异470行/22530字节；后续审计撤回此前单次/八段截断read的“完整无截断”声明。
此前30段补读的121..140仍截断，“30段全部完整”声明也撤回。
发布后曾按15范围尝试补读602行，但单行129及141..200、561..602等回包仍有字符截断；撤回“15范围无截断／全部602行审阅完成”声明。随后在2026-10-02按原diff的连续30行窗口、Unicode长行126/128/129分段实际补读602行；受管脚本重组断言通过、回包无字符截断，`git show`差异与原diff逐字cmp为0。该发布后补审完成不证明历史提交前曾读完；SHA、cmp及index单独仍不能替代实际阅读。
未运行原版/Windows，未借旧空桶green。
材料前缀build/tmp/runtime/wp316-tsw-count-prefix-保留red日志/snapshot、旧/新SHA、source diff和五green日志。
该计数批次已提交、推送及TG；完整共享全局/allocator游标、DF别名、四free/SEH、首次失败真实ABI、
四caller接线、249跨块/98 CALL/22 RET、联合差分、I5及316最终REVIEW与完整Goal均未完成。

### 普通物理帧选择与字流尺寸写回

4332A0/433380/433540普通路径未按块头声明数拒绝完整descriptor；低层读取现不再以count作边界，
仍保存声明元数据、保留物理偏移/字节与解压长度检查，不把缺字节/未知API/五槽缓存输入补齐。
旧生产proc_eaa5实际8秒exit8，Archive/Runtime恰各三项新增失败，相反侧/旧断言通过，无诊断，
九旧生产/两初版tests SHA相同，两完整snapshot保留。后补FFFFFFFF物理偏移边界未参加该red。

Native后补夹具初版缺pure virtual方法而编译失败，v2漏最后zero row header而六组失败；
LST401C49要求行命令结束后再读下一行头，修正为完整18-byte流，未放宽生产或期望。
第三版六门禁虽成功，但人工核LST发现16位尺寸期望误按descriptor，不能用green证明错误合同。
431DB1把record传401C70，401C9F按主流depth分支；401B94/401BA3把流宽高写回record+C/+E。
独立期望从descriptor31/32×7/8改为流头1×1，proc_cda9实际13秒exit8，恰六新增失败、旧断言0、
diagnostic0，九production/初版新测试SHA前后同，完整snapshot匹配，限定差分实际执行。

Runtime成功转换后按已验证的流depth选尺寸来源，字节流保留descriptor、字流用header，
不按容器bpp误判，不增零/空回复或未知地址guard。Native/父pipeline/renderer/registry仍只读。
后补bit15与8位容器承载16位流，最终十二组实际查询端口检验miss/hit ABI、宽高、全部decoder字节/lease；
这六组类型对照与bit15未参加初版red。受控资产/端口不是原版动态差分或四caller真实接线。

十四source第四版proc_94fb实际414秒exit0：三定向各1/1（0.19/1.16/1.11秒），
core200/200（24.04秒）、ASan200/200（38.28秒）、app206/206（84.77秒）。
六日志diagnostic0、十四SHA前后/完成后同，三LastTest各action1/reset1/skip0，未运行原版/Windows。
材料前缀build/tmp/runtime/wp316-tsw-declared-count-与wp316-tsw-word-dimensions-保留各版冻结、red/green、snapshot；
不改写原始capture、旧freeze，也不借v3green关闭新增尺寸合同。该限定阶段已完成审查、提交、推送及TG。
五槽缓存/I/O顺序、共享全局/allocator游标、DF/别名/四free/SEH、首次失败真实CPU、四caller、
249跨块/98 CALL/22 RET、联合复放/I5及316最终REVIEW/完整Goal仍未完成，315/422不变。

### 法术图像五槽准备的局部有界停点（尚未验收）

431D19..431D49只对资源6001..9000在缓存节点登记后启用431AA0的五槽准备；
旧生产直接按商/余数读完整物理帧，误称已完成原版I/O/共享描述符顺序。
旧生产冻结的两个定向各失败：Runtime6001/9000和战斗真实端口6001首次/重复查询
恰四项新增断言失败；旧生产定向日志未见旧断言退化。新实现仅保留五槽key/age
在I/O前的写入与signed32满槽选择，首次未知准备I/O处明确停住；
资源9000的key0先与全零槽命中，不提前执行普通physical0拒绝。
受控六文件夹具与真实TSW根路径的端口及479940父级CALL检查点定向已各通过，
父级不凭未知回复写回渲染来源或制造首次失败EIP/ESP/FLAGS；重复键仍可命中先登记的record。
冻结源码的core/ASan各200/200、app206/206通过，六源码SHA后核匹配、三日志诊断0；
工作候选420行、首份暂存候选421行及最终暂存421行均已按30行窗口和Unicode长行分段无截断补读，最终暂存与提交差异逐字cmp=0；已提交89ca521b并推送，TG脚本exit0。该历史停点当时会使
有效法术图像暂时不能正常加载，故不能声称五槽成功路径或完整战斗兼容，
也不能据此关闭316/最终REVIEW。原始API回复、描述符255边界、重入、DF与
allocator真实owner仍未闭合；`load_direct()`的宿主物理读取不代表431C50返回。

### 法术图像非调色板帧的原顺序读取（新切片，限定门禁通过，待发布）

`431A50`按2Ch预读`all_magic.tsw`索引；已登记节点的6001..9000路径依431AA0选五槽，在有效非8位块头后按声明数逐条读36字节descriptor，并以真实宿主seek返回位置覆盖首DWORD；433540从已准备的槽取帧，不退回直接Archive物理查找。8位会请求未绑定487C10调色板分配，count256跨槽以及短读、重入、DF1、真实失败CPU均不补造，继续显式未知；key0资源9000父级仍停在CALL检查点。该Linux宿主路径只证明受控输入的文件字节与输出一致，未捕获原版Win32 API回复。

旧生产四源码SHA冻结，真实6001首帧新断言在`proc_c7f3`恰一项失败；改动中16位限定路径对新增24位合成帧在`proc_3f1e`再恰一项失败，随后按`4332E0/4332E5`纠正。真实6001 variant0/1、6251/6331两个24位资源、6137的131帧末帧、合成16/24位双帧与战斗TSW端口、资源9000父级停点已在定向验证；`proc_a6e1`及`proc_9c8a`各1/1。曾因大数组内嵌Runtime出现战斗测试段错误，移至堆持有后先仅旧“6001必停”断言失败，改为9000后定向通过；不能单凭时序给出段错误根因。v3八份源码和测试/CMake身份冻结且后核匹配；新配置core 200/200、重跑完整ASan 200/200、app 206/206退出0，三份LastTest各实际执行17动作及seq372限定差分，日志诊断/跳过匹配均为零。中断的首轮ASan只有199/200且无退出证据，不借它作完整门禁；独立ASan战斗1/1后新受管完整重跑才作为门禁。当前尚待最终暂存差异全文审阅、提交、远端与TG；原版正常图像跨调用联合差分、失败API/allocator、DF/别名/四free、四caller与I5仍未完成；316保持`pending_audit`，315/422不变。
