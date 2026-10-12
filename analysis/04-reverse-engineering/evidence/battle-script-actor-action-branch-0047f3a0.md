# 战斗脚本51指定敌人动作的实际共享字段

## 范围与结论

实现源码为`31c3a08ea5ded27bf53f6899c58c1b409bafa7de`，三文件已提交并推送。
脚本51直接写敌人实际当前动作分支与保留分支，删除47F3A0操作枚举和通用请求。
没有新增转发包装、参数数组、回复、调用计数或并行字段表。
本切片为`platform_adapted`、`unit_tested`、`fixed_state_tested`，父链仍pending。
额外角色地址、原版差分和完整游戏未验证，不标`assembly_exact`。

## 完整原始范围

唯一行为依据为`swd3.exe_export_for_ai/swd3.exe.lst`的机器字节与指令。
完整叶0047F3A0..0047F3B0共11行，无call或条件分支，RET4；唯一物理caller0046CDEE。
脚本51的完整块0046CDCE..0046CE0A共30行，只有此一call。
独立分析及访问测试在对照现有C++之前记录。

原序列为CDCE读取入口cursor+4的BYTE；CDD2读取+2的WORD并MOVSX；
计算525508+signed selector*2B28并调用叶。叶读取参数低BYTE，
47F3A4先写actor+2A95，47F3AA后写actor+2A94，最后返回。
脚本再读取共享cursor，加6、写回并返回完整DWORD1。
脚本+5没有读取；不把参数扩宽为WORD，不重建没有业务消费者的ECX高24位。

完整47F380..47F39A共18行说明保留值非零会覆盖当前值；
47997E..4799E7块共50行说明当前BYTE零扩展后DEC/CMP99进入角色呈现分支。
因此称当前/保留动作分支，不称计数器、动画速度或推测某个游戏界面。

## 共享数据与原地址别名

八个正常敌人使用startup.group_b_lifecycle实际拥有数组，当前值在已有
base_initialization.field_2a94，保留值在已有runtime_reset.field_2a95。
SDL把battle_runtime_.group_b_lifecycle绑定到battle_group_b_storage_.actors()，
脚本借同一startup；既有角色frame/reset同样读写这些拥有字段。
此次没有SDL源码改动；实际链接是编译接线证据，不能代替游戏执行。

WORD FFFF sign-extend为-1，原基址5229E0的+2A94/95为525474/475，
确实属于共享reset.block_525470[1]低两BYTE。实现先写高BYTE再低BYTE，
保留原DWORD高两BYTE及相邻状态，生命周期为空也不构造默认actor。
此前group_b_token的负值停止在此已被实际共享写入替代。

其它signed索引在已有拥有范围之外继续使用既有group_b_actor_typed_stop；
8/9额外地址涉及CRT/PE区域，尚未建立本操作的真实所有权及运行保护证据。
这里不是原版不可达证明，也不是全部原地址兼容验收。
正常角色缺少实际lifecycle及第一写入不在已有object_writable_bytes范围时，
复用已有actor访问停止，阻断字段与cursor后缀；不存在独立新增成功替身。
首写2A95需要extent至少2A96，此后较低2A94也必在同一可写范围内。
不增加第二独立故障旗标或观察回调来模拟两次真实字段写。

## 实际定向执行

新入口在test_battle_script_dispatch中实际注册，共78次执行：

- 八角色乘BYTE0/1/6/7F/80/FE/FF，共56次：只改所选两个字段，其它角色和邻接状态保持。
- FFFF选择乘同七BYTE，共7次：实际reset别名，高WORD及相邻DWORD保持，无敌人替身。
- extent0/2A94/2A95/2A96/2B28，共5次：第一高地址写失败或两字段成功。
- 实际lifecycle缺失1次：已有访问失败，cursor后缀阻断，无generic请求。
- script容量2/3/4/5，共4次：先在+4失败，容量5即成功，不读取+5；失败字段与cursor保持。
- 8/9/7FFF/8000/FFFE共5次：记录已有未拥有地址边界，不声称原版同样失败。

SDL唯一实际script入口消费status和return_eax，未消费子ECX/EDX/flags；
父层寄存器诊断仍存在并待回收，不以保存虚构寄存器来代替共享数据。

## 验证及保留失败

首轮proc_2bb4在10秒后编译失败：新测试未引入局部compat::u8，出现7条类型错误。
只补实际类型别名；生产业务实现不变。原日志保留，后续ASan/SDL当轮未执行。
最终proc_0506耗时164秒成功，经仓库脚本执行：

- core battle.legacy_battle_setup，1/1，11.44秒。
- ASan同一入口，1/1，18.12秒。
- SDL openswd3实际重新链接；三份格式后源码SHA256一致。

只跑相关目标；没有全量UT、Windows门禁、原版或游戏启动。
两份测试日志保留既有2562行size_t至u32窄化warning；SDL无warning/error。
完整生产71行、修正测试192行已逐段审查；最终及暂存diff与审查稿逐字节一致。
proc_a9cb在3秒后通过原指令、字段顺序、别名、实际绑定、测试注册、源码身份审计。
当前重算清单保持父接口pending和范围穷尽性false，不新增整项验收。

临时原始摘录、分析、审查稿、日志及身份manifest位于
`build/tmp/runtime/script-actor-parameter-*`；不是游戏资产验证材料。

## 剩余工作

父层仍有其它操作编号、参数包、寄存器request/reply、轨迹、计数、Port及资源调用链。
本批仅回收这一物理业务调用；关联
[脚本总入口](battle-script-dispatch-00469d20.md)和
[整体迁移](invoke-migration-progress.md)，不能视作全项目INVOKE完成。
