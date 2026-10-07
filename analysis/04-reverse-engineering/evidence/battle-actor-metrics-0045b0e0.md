# 战斗角色metric重建 `0x0045B0E0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威函数为`0x0045B0E0..0x0045B18B`，从proc到endp完整92行、2个静态call站点、4个`loc_`标签，无外部FUNCTION CHUNK。两个call站点都调用`0x004783B0`，REVIEW 1均改为直接组合typed坐标查询，不再经过opaque端口回复。

共有14个静态caller站点。七个已关闭站点位于战斗启动、逐帧协调、角色动作分派、对手动作分派两处和最终角色步进两处；它们全部直接组合本实现并传入既有角色owner。其余七处位于尚未关闭的后续战斗函数，不提前修改。

## 2. 双18 dword清零

入口固定把第一张18 dword角色metric表清零，再把第二张18 dword角色顺序表清零。两次都是`ECX=0x12`、`EAX=0`与正向`rep stosd`；角色数量为零也不跳过清零。后一相邻函数直接覆盖顺序表前缀。

两张物理表只保存在动作、启动与帧协调端口共同虚继承的单一typed状态中。同一组合端口跨多类caller只有一份存储。角色优先索引也与逐帧选择值、启动初始化和调试C/W键共用该owner；已关闭攻击顺序出队证明它还是`0x0053AE70`起七dword输出记录的首项，因此后六dword一并归本state。后续优先阶段只覆盖首项并保留记录尾，全局重置则按原写集合清完整七dword，不再维护独立副本。

## 3. 栈局部初值

函数的首条`push ecx`同时保存callee-saved ECX并提供4字节栈局部：

- `var_4`是保存的caller ECX低word；
- `var_2`从同一保存值的第三byte起始，但callee执行word store，实际覆盖保存槽的完整高word。

两个word别名不预清零。首个角色查询若在输出写入前停止，group B仍观察caller ECX低word；后续角色复用前一次查询留在保存槽中的高、低word。typed状态显式保存入口ECX、两个word别名及其32位兼容token。

## 4. group B循环

先读取group B完整u32数量；只有零跳过。循环从索引0开始：

1. EAX载入word局部token；
2. ECX载入组B角色token，基址固定、步长固定；
3. 栈参数依次表示byte局部token与word局部token；
4. callee返回后把word局部按i16符号扩展到EDX；
5. 重新读取共享group B数量到EAX；
6. 把EDX写入第一张表当前槽；
7. 索引加1，并与刚重新读取的数量作unsigned比较。

callee返回后仍按LST重新读取共享group B数量，再执行store与循环比较；typed坐标查询本身不伪造数量更新。角色token按u32回绕，不增加modern上限。

## 5. group A循环

组A表从索引8开始。入口先计算`group_a_count + 8`的u32结果，并以unsigned `<= 8`决定是否跳过；因此零跳过，接近全1的数量保留原始回绕域。

每轮使用组A固定基址与步长调用同一callee。callee后把word局部按i16符号扩展到EAX，重新读取group A数量到ECX，再写第一张表。索引加1后才计算新的`group_a_count + 8`边界；即使当前typed坐标查询不写数量，这些重读位置仍按原指令顺序保留。

组A从表索引8开始，按原物理布局覆盖该区；callee返回后同样重新读取共享数量，不为异常组B数量建立隔离副本。

## 6. 返回与typed-stop

正常返回保留路径相关完整EAX：

- 无组A迭代时返回`group_a_count + 8`的u32结果；
- 有组A迭代时返回最后一次word局部的i16符号扩展。

函数尾`pop ecx`读取的是已被两个callee word输出覆盖的保存槽：完成过查询时，ECX高word来自最后一次第一个输出，低word来自最后一次第二个输出；无角色时才等于caller入口完整值。EDX不保存：group B尾为最后metric符号扩展，group A尾为最后callee EDX，无角色时保留caller入口EDX。typed-stop不执行函数尾`pop ecx`，保留停止点的callee ECX残值。

第一张typed表只有18槽。异常数量导致第19次写时，callee、局部输出、数量重载和寄存器覆盖已经发生；实现只在该首次真实store处typed-stop，保留此前36 dword清零与18次完整写入。

两个坐标CALL的入口标志来自LST中最后到达的32位`test`或`cmp`。首个group B调用保留`test group_b_count,group_b_count`的`CF/PF/ZF/SF/OF`，并显式标记架构未定义的AF；后续group B、首个group A与后续group A调用分别保留各自循环边界`cmp`的完整标志。selector读取停止时这些入口标志不被伪造为全零；其他坐标停止保留callee内部word `cmp`标志。正常返回则保留最终group A入口或循环边界`cmp`标志。EAX、ECX、EDX和标志同时回写统一metric状态。

## 7. caller回收与测试

已关闭caller删除`0x0045B0E0` opaque边界并直接组合统一实现：

- 战斗启动以敌方与队伍数量发布共享计数，随后保留后两阶段；
- 逐帧协调使用同一端口共享计数，随后保留下一阶段与完成门；
- 角色动作分派、对手动作分派和最终角色步进在原调用点发布各自当前计数，并在callee后按原位置重读共享数量；
- 子函数typed-stop立即阻止caller后续阶段。

定向测试覆盖双表固定清零、caller ECX双word别名、两组token与步长、primary/alternate坐标、i16符号扩展、跨角色保留、组A加8回绕、路径相关EAX/ECX/EDX与最终flags、首个group B `test`的未定义AF标记、首个group A及后续循环`cmp`入口flags、第二源读取停止后的第一次部分写入、缺失owner、第九个组B actor停止、第19次store时机、跨动作/启动端口的单一物理状态及七处已关闭caller回收。全部direct路径的`port_calls`为零，并独立记录`coordinate_query_calls`。

当前缺少原版两组完整角色对象、metric与角色顺序两张物理表、栈地址和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

## B11每帧指标接线：业务数据与诊断输入分离

本批基线634d9357。SDL调用与固定状态验证已完成，原版动态差分仍未取得。

重新核对45B0E0..45B18B全部92行及4783B0..4783F5完整坐标callee。
上一历史断点正确指出SDL未提供完整上游CPU现场；这项限制仍保留。
但它并不使正常业务指标依赖未知入口值，理由如下：

- 45B0E4..45B0FC固定清两表，不读入口ECX/EDX。
- 两个CALL前，EAX来自局部地址、组A入口边界或上一轮已写指标，ECX
  来自当前角色地址。组A还在45B15C覆盖EDX；首次组B的EDX可能仍未知。
- 4783B0的两个正常出口均先写X与Y两份WORD输出，且使EAX/ECX/EDX
  可由角色字段及本次已知输入推导。alternate保留EAX已知高WORD；
  primary在第一次输出后覆盖完整EDX。不依据未知入口值或旧栈WORD分支。
- 45B124和45B169仅在callee正常返回后读取新Y并作i16符号扩展。
  因此正常指标存储不需要给旧栈内容填一个假定值。
- 零角色路径保留旧ECX/EDX；早期失败也可能只完成第一份输出。
  此时不能把诊断元组或保存槽宣称为全部已知。

核心增加诊断有效性标记，默认保留显式输入调用的旧合同；SDL每次调用
主动声明入口寄存器元组未知。两个WORD都写完才证明保存槽已知。
组A进入坐标CALL前已覆盖三个寄存器；组B首个正常CALL也消除入口依赖。
提前失败保守保留未知标记，不用它选择业务分支，不改变写入与停止顺序。
标记为已知仅表示可从模型输入推导，不代表取得原版同run CPU捕获。

平台用统一guest身份分配器保留四字节scratch身份，两输出相距二字节。
该身份在SDL对象内复用，坐标调用无回调、无递归且输出地址不逃逸；不逐帧
累积分配，也不冒充原版栈地址。分配失败停在指标函数入口，尚未清两表。
实际数据仍由同一startup/action角色及同址计数读取，未增加角色副本。

SDL在完整前帧预处理正常返回后执行指标重建；坐标或表写入失败保留前缀。
该批次截止时正常完成后停在45324D角色顺序调用前。后续已接入
[角色排序](battle-actor-order-0045b190.md)，完整战斗帧尚未接通。

测试向量以不同未指定入口残值验证正常两组输出相同，并覆盖primary与
alternate、零角色、首个角色缺失、第一输出完成后第二源不可读、第二角色
失败保留前一指标，以及组A首次失败时寄存器已知但保存槽未知。
proc_d42a：定向battle.legacy_battle_setup在core/ASan各1/1通过，分别
5.01秒和7.91秒，SDL构建通过。测试源在tests/CMakeLists.txt注册，setup
测试入口实际调用test_battle_actor_metrics。日志前缀为
build/tmp/runtime/battle-frame-metrics-live-。

core和ASan构建均报告既有legacy_battle_outcome_resolution_test.cpp:137
的u16到u8转换警告，该文件未改。此次未运行游戏，未新增原版动态差分。

完整源码差异已审查；全文件格式化带入的无关换行已撤回。最初的格式验证
脚本把注释内单引号误识别为字符字面量，断言失败；修正词法识别后，按
保存的差异重建出测试时同一Git blob，确认当前源码token及字符串未变。
验证日志为battle-frame-metrics-format-verification.log。

## B11入战排序接线

SDL入战路径已在额外队员配置后组合实际坐标指标、角色顺序与敌方顺序。
新入口借用startup的同一metric状态，原startup端口入口委托相同实现。
本批只接入启动路径，不补造下述完整帧入口的CPU现场。
见[排序与进度接线](battle-startup-order-progress-runtime-binding.md)。

## 8. Workpack 316 SDL生产入口缺口（历史断点）

用户第二次实际战斗日志`build/vm/wp316-sdl-frame-user-run-v2/openswd3-2026-10-03_19-14-31-66208.log`在输入分派`0x0045FC5B`RET前停下，未执行本函数。后续代码先按LST让预帧早退正常进入`0x00453248→sub_45B0E0`，共享`battle_actor_metrics_`的两张18-dword表在`0x0045B0E4..0x0045B0FE`被依序清零，随后`0x0045B0FE`从同址启动owner的`battle_runtime_.enemy_count`而非metric port默认0读取真实组B数量；非零停在首个坐标CALL`0x0045B11F→sub_4783B0`前，零停在`0x0045B13E`组A数量首读前，包含高位无符号反例。最新定向core 1/1、Linux core／ASan各200/200及Linux／Windows app各206/206通过，但没有第三次SDL实测。当前SDL启动仅部分执行`prepare_legacy_battle_setup`，资源id98显示敌方1／队伍1，不能因为只借用了组B数量owner，就宣称原版`sub_451B10`、后续组A额外成员或坐标callee已经绑定，也不能提前宣称角色metric完成。`0x0045B11F`首次CALLeffect还需要`sub_453200`音乐调用、`sub_45FC60`鼠标输入与`sub_45F2A0`分派及`sub_45D490`预帧依序留下的ECX/EDX，以及真实两字栈局部输出地址；SDL音乐端口目前只返回EAX，不能把默认0视作完整CPU现场，不能在坐标子函数中虚构正常返回。组B对象坐标当前由真实资源的`prepare_legacy_battle_setup`写入共享`battle_runtime_.group_b_lifecycle`，但这仅证明可定位字段，仍需严格绑定该次CALL前现场与完整启动生命周期。
