# 战斗角色启动计数递增与标记发布（00478AC0）

状态：`pending`。已废弃GPT重建实现，本轮逆向尚未开始。

以下保留历史分析与证据；旧结论、完成记录和测试结果不作为本轮验收。
原版采集事实保留，所有解释和实现结论须重新核实。

## 历史资料（重新核实前不生效）


## 权威范围与访问顺序

LST范围`00478AC0..00478AD1`，半开终点`00478AD2`。
18字节、3条指令、0个callee、0个分支、1个普通RET，无外部chunk。

```text
00478AC0  66 FF 81 74 2A 00 00           inc word ptr [ecx+2A74h]
00478AC7  C7 81 E0 2A 00 00 01 00 00 00 mov dword ptr [ecx+2AE0h],1
00478AD1  C3                             retn
```

先读WORD，按16位回绕写回，再写完整DWORD1，最后读取返回地址。
标记写不读取旧值。INC保留CF，更新其余算术flags；MOV/RET不改算术flags。
EAX/ECX/EDX不被叶函数修改。

## 当前接口与共享字段

`increment_legacy_battle_actor_start_gate`接收实际WORD/DWORD指针和访问描述。
两个字段由调用方借自现有角色，不创建影子存储：

- 组A：`action.group_a_action_execution[index]`。
- 组B：`startup.group_b_lifecycle[index].action_execution`。
- 字段分别是`start_gate`、`start_gate_latch`；runtime reset仍映射原字段。

删除本操作的owner/view/resolver、请求数组、寄存器请求回复、地址常量、
模拟栈、调用轨迹、访问/调用计数和纯转发执行函数。
结果只含状态、可选的已观察旧WORD、可选的已提交新WORD和标记提交事实。
父结果外层optional表示该操作确已执行，未执行不生成默认成功。

四种既有停止顺序保持：读失败不观察值；WORD写失败只观察旧值；
标记写失败保留计数提交；RET失败保留两个字段提交。
空指针只表示调用方没有对应真实字段，并在原读或写边界报告失败。
不把未映射宿主字段解释成原版任意地址必然故障。

## 五处原调用与后继

```text
00456FE1 → 00456FE6  组A选定敌方目标
0045707A → 0045707F  组A扫描敌方候选
0045786B → 00457870  组B扫描队员候选
00457E1C → 00457E21  组B选定队员
0046DD4A → 0046DD4F  脚本case78
```

组A扫描保留两次终态查询；第一次!=1且第二次=0才递增。
故障保留早先角色写入，并抑制当前扫描进度及后缀。
选定路径的失败EAX取实际前驱：终态!=1保留终态返回；终态=1且
`53C02C!=0`保留该DWORD；重新选择路径才是signed SI地址算术的结果。
旧实现统一制造`1381*目标索引`，现按`456F3C/456F4A/456FD7`区分。
成功后的`CMP53C010,1`覆盖INC flags，无需叶函数回传CPU状态。

组B扫描先清原ESI所指队员的实际叠加绘制字段，再递增其启动门。
故障保留字段清零，阻断`53BD7C`递增；先前扫描提交保留。
清零通用调用已删除，父故障不再使用其虚构EAX。
当前验证见[实际叠加标记](battle-group-b-final-overlay-clear.md)。
选定路径的两个C++分支合流到同一实际递增块，对应原`457E00`。
目标发布后重读完整`53BCFC`，不能沿用终态查询前缓存的对象地址。
地址`5029D0+index*2F34`按32位回绕解析双方现有存储。
失败EAX为`1007*index`；后续公共路径不再依赖叶函数寄存器回复。

脚本先完成清表和共享前缀，再读当前目标代码、解析
`5229E0+code*2B28`的完整回绕地址并修改实际字段。
尚存的父帧协议需要局部前驱，所以调用方保留`EAX=345*code`和ECX地址，
仅在实际WORD提交后按旧值/新值计算INC flags，CF沿用地址SUB。
**EDX是`46DCD0`零扩展读取的队员代码，而非`345*code`**：
`46DD3D`将EAX写成345倍目标代码，不写EDX；`46DD40`将ECX写成
1381倍目标代码，`46DD43`再形成对象地址。旧证据与测试的
EAX=1381倍、EDX=345倍结论均已撤回。
故障抑制后继gate发布、frame调用和游标推进，保留清表等前缀。

## 字段交叉引用

`+2A74`直接访问：`46F8C8/473C1B/4745B8/4758AD/4786D0/478AC0/
478AE0/478AEF/47D4ED`。
`+2AE0`直接访问：`47887B/478AC7/478B0A/478B50/47D5CE`。
其他读取、重置及衰减仍引用同一共享状态，属于各自后续迁移范围。

## 验证与限制

独立推导及当前测试覆盖WORD边界、邻接数据、完整DWORD写入、双方真实
存储、重复实时写入、四个访问阶段、缺失字段、原目标失败前驱、
扫描部分提交及完整选择回绕。最终定向门禁已实际执行：

- Linux core：`battle.legacy_battle_setup`，1/1通过，11.70秒。
- Linux core：`battle.actor_frame_316`，1/1通过，27.52秒。
- AddressSanitizer：`battle.legacy_battle_setup`，1/1通过，16.33秒。
- AddressSanitizer：`battle.actor_frame_316`，1/1通过，28.41秒。
- Linux SDL：`openswd3`目标实际编译、链接成功；未启动游戏程序。

五份最终日志均无编译警告、测试失败或sanitizer错误。十二份源码及测试
的SHA256与执行前后清单一致。日志位于`build/tmp/runtime/`下的
`start-gate-increment-direct-{core,callers-core,asan,callers-asan,sdl}.log`，
身份清单为`start-gate-increment-direct-validated-source.sha256`。

中断和失败日志均保留为历史证据，不用于本轮验收。先前敌方扫描测试
采用终止查询默认返回1，会按原分支跳过递增；最终夹具明确提供四次
返回0，覆盖两名角色各两次前置查询和第二名角色的四个访问失败阶段。

历史Workpack309的199/199、205/205、十轮core及旧inventory哈希属于
原寄存器协议版本，不代表本轮验证。B10游标继续315/422；父链协议、
phase资源、完整内存别名、B11、Workpack316/318和完整游戏未升级。

分类仍为`platform_adapted`。缺少原版异常字段页、RET异常栈页、联合
调用与SEH捕获证据，原版动态差分仍是`blocked_runtime_oracle`。
未启动原版或OpenSWD3游戏程序。
