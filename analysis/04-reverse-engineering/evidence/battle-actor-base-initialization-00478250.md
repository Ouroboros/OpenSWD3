# 战斗角色公共前部初始化 `0x00478250`

状态：`platform_adapted`。完整LST、全部三处物理xref、64次写入顺序、寄存器残留、typed停止点与caller直组装均已收敛。

## 1. 完整权威范围

唯一行为真值为`swd3.exe.lst`。完整主体是`0x00478250..0x004782FF`，从`proc`到`endp`共63个物理行、35条实际指令、0个call、0个跳转、0个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。

本函数为thiscall，入口`ECX`是角色对象。它把this保存到`EDX`，保存非易失`EDI`，完成公共前部初始化后返回完整this。正常返回寄存器固定为`EAX=this`、`ECX=0`、`EDX=this`；`EDI`恢复为入口值。

## 2. 精确访问顺序

全部对象写入严格按以下顺序发生，不能按偏移排序或合并成整对象清零：

1. `+0x2A56/+0x2A5A/+0x2A5E/+0x2A62`四个dword依次写`0xFFFFFFFF`；此时`EAX=0xFFFFFFFF`、`ECX=this+0x2A56`。
2. `EAX`清零，`ECX`改为`this+0x2630`。
3. `+0x29A2` word写`0xFFFF`。
4. `+0x2668` dword写`15`，`+0x266C` dword写`1`。
5. `+0x2958` word写零。
6. `+0x2A0A/+0x2A68/+0x2A6A`三个word依次写`4/2/0x18`。
7. `+0x2954/+0x2956/+0x2A66/+0x2A0E/+0x2A6C`五个word依次写零。
8. `+0x2A94` byte写零。
9. `+0x26BC` dword写`0x062B062B`，`+0x2584` dword写零。
10. `+0x2630/+0x2634/+0x2638/+0x263C`四个dword依次写零。
11. `ECX`改为`0x29`，`EDI`指向`this+0x10`，`rep stosd`按低地址到高地址依次清零41个dword，即精确覆盖`+0x10..+0xB3`的164 bytes。

合计53次dword写、10次word写和1次byte写，共64次物理写。`+0x10..+0xB3`最后一个dword覆盖原definition说明token；typed owner因此只在最后一个dword真正写成零后清除对应说明bytes。若此前任一写入停止，旧说明owner保持不变。

## 3. typed owner与停止点

`LegacyBattleActorBaseInitializationFields`承接此前尚无owner的零散公共字段；definition、说明bytes、动作文本、目标槽、回合/动作字段继续复用各角色已有的唯一typed owner。组A和静态单例使用`LegacyBattleActorBaseInitializationOwner`聚合这些物理字段；组B直接把已有`action_composition`与`action_execution` owner作为同一对象的视图传入，不复制第二份业务状态。

`initialize_legacy_battle_actor_base()`按原指令顺序逐次检查对象可写前缀，并对动作文本和definition独立检查typed视图长度：

- 第一至第四目标槽停止时，保留已完成槽，返回`EAX=0xFFFFFFFF`、`ECX=this+0x2A56`、`EDX=this`。
- `xor eax,eax`之后的任一直接字段或动作文本写停止时，保留全部已完成前缀，返回`EAX=0`、`ECX=this+0x2630`、`EDX=this`。
- 41-dword definition清零中途停止时，保留已清dword，`ECX`为尚未执行的dword数，`EAX=0`、`EDX=this`；最后token dword未完成时不清说明bytes。
- 正常完成才返回`EAX=this`、`ECX=0`、`EDX=this`。

没有空对象、短对象、短文本或短definition的防御性继续路径；全部在原始写访问处typed-stop。

## 4. 三处物理xref与caller回收

完整LST只有三处物理xref：

- `0x00451875`：`0x00451870`把固定单例token `0x00521598`装入`ECX`后尾跳本函数。
- `0x0046E494`：组A元素构造`0x0046E490`调用本函数。
- `0x00475564`：组B元素构造`0x00475560`调用本函数。

三处均已删除本函数的opaque构造调用：

- 组A先直接初始化公共前部；只有完整成功后才清尾部word、分配56-byte说明记录并清零。
- 组B先直接初始化公共前部；只有完整成功后才分配并发布独立`+0x0C`的164-byte资源记录。
- 单例包装器直接初始化固定对象的typed state；静态入口只有完整成功后才调用`_atexit`注册析构包装器。

公共前部typed-stop会阻断三类caller的所有后续副作用。组A/组B元素分配端口不再含`construct_base`；单例对象生命周期端口只保留尚待`0x00478300`工作包关闭的析构边界。

## 5. 双向追溯

- `0x00478250..0x0047825C`：保存this/EDI，建立目标槽与definition地址并置`EAX=-1`。
- `0x0047825F..0x00478267`：四目标槽全一。
- `0x0047826A..0x0047826C`：清EAX并建立动作文本地址。
- `0x00478272..0x004782E4`：按原序写10个word、1个byte和4个直接dword。
- `0x004782EA..0x004782F2`：四个动作文本dword清零。
- `0x004782F5..0x004782FA`：41-dword definition清零。
- `0x004782FC..0x004782FF`：返回this并恢复EDI。

C++到LST反向追溯覆盖全部35条指令、64次物理写、三处xref、definition token所有权、正常返回和每类typed停止寄存器。

## 6. 验证与动态差分

独立定向测试覆盖正常64写、全部四个目标槽边界、可达的`+0x2A68/+0x2A6A/+0x2A6C/+0x2A94`高偏移直接写边界、全部四个动作文本dword边界及41个definition dword边界；caller聚合测试覆盖组A、组B和单例的正常直组装、后缀顺序与公共前部typed-stop阻断。

验证：定向测试`2/2`、AddressSanitizer`197/197`、Linux core`197/197`、Linux app`203/203`及连续10轮完整core全部通过，源码零warning，无sanitizer finding或runtime error。inventory连续双生成逐字节一致，正式计数为`276/422 = 266 platform_adapted + 10 assembly_exact + 146 pending_audit`，SHA256为`6ef0923452f60cf13342d07e9bd6ef861b3b5100602169723c1ee53b61679cd0`。

当前没有原版组A/组B/单例完整对象字节、异常写访问、definition说明堆所有权及三caller联合寄存器捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。该阻塞不影响完整35条指令的静态与typed闭环。
