# 战斗角色动作种类查询（0x004786B0）

状态：`pending`。

## 权威范围

完整LST范围`004786B0..004786B7`，8字节：

```text
004786B0 66 8B 81 6C 2A 00 00  mov ax,[ecx+2A6Ch]
004786B7 C3                    retn
```

无分支、调用、写入或外部chunk。完整WORD替换AX，高EAX、ECX、EDX与
flags保持；字段读取先于RET。全LST直接`+2A6C`访问47处，四处物理调用
分别为`004539E6`、`00455D97`、`0045DF5A`和`0045DFD5`。
独立推导先于当前C++审查，见`build/tmp/runtime/action-kind-direct-lst-analysis.md`。

## 共享字段与接口

函数借实际`const u16*`，返回状态及`optional<u16> value`。
队员借`action.group_a_action_execution[index].action_kind`；敌方借
`startup.group_b_lifecycle[index].action_composition.action_kind`。
沿用既有角色字段及其生命周期，不新增镜像、缓存或写回。

删除查询地址常量、token解析、Owners/View、寄存器Request/Reply、
模拟栈、访问和调用计数，以及调试转发函数和reserved查询编号。
四处调用均直接借字段。外层optional表示是否执行查询，内层表示是否观察到值。

字段缺失/不可读时返回原字段失败且无值；读取后RET不可访问时返回原
返回失败并保留观察值；两者同时不可访问时先报告字段失败。查询不写字段。
双方动作的原入口索引检查保留；调试在每行实际读取时识别缺少角色字段，
不会提前拒绝整轮或抹掉已绘制行。敌方startup及lifecycle沿用原可空合同，
缺少存储只能在原读取边界失败，不伪造动作零。

## 四处调用的追溯

- 队员`004539E6→004539EB`：入口索引算术EAX为`index*BCD`。
  原DI接AX并mask，现直接消费完整WORD；之后才调用终态及各动作。
  字段失败保留算术值，RET失败替换其低WORD，均不执行终态或动作后缀。
- 敌方`00455D97→00455D9C`：入口算术EAX为`index*565`。
  原AND FFFF后分派动作，现直接消费完整WORD；100返回1，0与未识别值
  返回0，不访问无用的目标。其余动作及顺序保持，字段/RET失败保留原前缀。
- 敌方调试`0045DF5A→0045DF5F`：先解析对象、查询数值并读取level，
  再依次读取动作与启动WORD、格式化及绘制。父级字段失败保留resolved
  token，RET失败只替换AX；EDX保留完整unsigned level，ECX仍为角色。
- 队员调试`0045DFD5→0045DFDA`：动作WORD、启动WORD、格式化、绘制。
  父级失败EAX沿用首行敌方数量或上一行重读的队员数量；RET失败仅替换AX。
  EDX保留字体/上一行绘制结果。每行即时读取，不按byte、bool或signed缩窄。

敌方父级后续五处旧接口仍消费入口EDX。该值留在父级
`opponent_action_entry_edx`并在入口保存一次，不通过查询请求或结果转发；
查询不再带机器状态。父级其余寄存器接口及资源仍待迁移，不因此验收父链。
调试父级对外失败寄存器也继续在实际调用点维护，leaf没有寄存器协议。

## 测试与门禁

独立向量覆盖完整WORD 0/1/FF/100/7FFF/8000/FFFF、字段与邻居不写、
双方实际字段分离和实时重读，以及缺字段、不可读、RET失败和双失败优先级。
双方动作测试保留原字段/返回失败与后缀抑制；敌方直接改写实际字段后
重新查询，验证100与高WORD默认分支，以及startup/lifecycle缺失失败。

调试新增双方七种WORD乘三个执行阶段的向量：第一行绘制后改写第二角色，
检查即时完整数值、unsigned文字、两类失败、父级高EAX/ECX/EDX与已完成行。
字段失败不伪造观察值；RET失败保留已读值；当前行启动查询和绘制均抑制。

setup注册leaf与调试测试，actor-frame注册双方动作测试。
最终实际门禁均通过：

- core setup 1/1，10.01秒；actor-frame 1/1，23.99秒。
- ASan setup 1/1，15.39秒；actor-frame 1/1，24.99秒。
- SDL `openswd3`实际编译链接，未启动游戏。

均通过仓库构建脚本指定目标及精确测试正则执行。最终ASan setup日志
保留一条既存outcome测试133行u16→u8窄化warning；其余最终日志无warning，
无编译错误、测试失败或sanitizer报告。

测试迁移曾漏保留两处仍用于敌方动作执行的父级EDX输入，既有断言检出后
恢复到父级字段，原断言保持，再执行上述完整定向门禁通过。
源码复核也将该父级值固定为入口保存一次，不在后续路径重复读取上下文。

日志与源码身份前缀为`build/tmp/runtime/action-kind-direct-`，后缀为
`core.log`、`callers-core.log`、`asan.log`、`callers-asan.log`及`sdl.log`。
十一份源码测试的`validated-source.sha256`与最终身份核验一致；
`publication-check.log`核对实际门禁、完整leaf、全部四处调用、47处字段
访问、旧协议清除、父级EDX保存、实际测试注册及inventory边界，全部通过。
最后从入口和全部出口双向复核，本切片内无未解释差异；不扩展到父级其他操作。

## 验收边界

保持`pending`。原版完整内存别名、异常页/SEH及联合动态捕获
仍缺少后端，差分继续`blocked_runtime_oracle`。
父级操作编号、参数包、寄存器接口、Port及资源继续pending；
本切片不提升B10整体、B11或WP316/317/318的验收状态。
