# 战斗角色模式资源数量累计 `0x00471080`

状态：`pending`。已废弃GPT重建实现，本轮逆向尚未开始。

以下保留历史分析与证据；旧结论、完成记录和测试结果不作为本轮验收。
原版采集事实保留，所有解释和实现结论须重新核实。

## 历史资料（重新核实前不生效）


历史还原状态：`platform_adapted`。本批完成累计器及必要调用方的通用协议迁移。
完整列表、字体、网格和主帧接口继续pending；不增加整族验收。

## 实际炼符资源数量累计

源码`1930c4159a8f1a518bc8ef1140264f3bee382062`直接借list和caller数量WORD。
删除CountRequest/Result、actor/output寄存器token、输入/返回寄存器、输出副本及
nodes_visited/matches。先发布根，随后清实际WORD，每个合格节点直接累计。
保留category bit27→非0300→BYTE mask05的短路，tertiary先取再加secondary；
节点内和跨节点均按WORD回绕。不读取名称，不改selected/next，不转移资源所有权。

唯一真实caller00466342直接借live queued队员的实际list，数量局部改为u16。
删除脚本计数分支、数量目标token、子回包与调用计数；共用grid计数枚举、主帧计数操作
和纯映射同步删除。选择帧message30回收该网格的兼容开关传递，继续借startup.party。
数量低WORD决定十格来源与末尾共享row-limit；成功零继续原绘制路径。
真实失败沿已有resource_query_failure和group_a_actor_typed_stop保留累计前缀，
阻断根再发布、行绘制、输入门及target修改；不新增停点或成功兜底。

## 1. 完整权威范围与ABI

LST SHA256：`701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b`。
目标`00471080..004710CD`，原行214619..214660；42行、25条带机器码指令、1call、
5jump、2局部标签、1RET，无外部chunk。唯一callee为资源链头发布00470900。
全LST按机器码和call指令扫描只有00466342一个caller，位于sub_466190。
完整父`00466190..004664FB`，原行194512..194892，共381行。
目标和父全文均实际完整阅读，完整生产301行、测试591行及相同staged差异均已阅读。
差异cmp和源码身份只绑定审查实物，不能代替该语义阅读。

ECX为list；保存EBX/ESI，RET8。第一栈DWORD完全未读，第二为可写WORD地址。
正常EAX0、ECX输出地址、EDX0300是机器合同，不是额外业务结果。
父后继重新准备actor、坐标、cell/primary及文字参数，不消费这些计数寄存器回复。

## 2. 入口、循环与全部出口

471084先发布next→current；471092再清caller实际WORD。
471097/9D读取current.next；4710A1先发布next→current，4710A7才判断零。
真实根next0正常输出0、current0；null list沿原list_owner错误且保旧输出。
未知/零根在原节点访问点失败，保已发布根与零输出；未知候选先发布current再失败。
已经累计的数量不回滚，不把非法根当成空链成功。

4710A9读取category DWORD bit27，4710B2排除id WORD0300，4710B8读取BYTE mask05。
任一关闭直接回循环，不读取数量。4710BE先取tertiary WORD，4710C2加secondary，
4710C6直接加输出WORD。负位型仍参与，两级WORD回绕保持；4710C9回循环。
4710CB/CD为正常耗尽唯一出口，不复位根，不增加扫描上限。

原C++先读secondary后读tertiary已按指令纠正。真实signed/unsigned对应WORD别名
可看到入口清零和每次增量，并影响后续节点数量读取；未用WORD引用访问DWORD链头。
实际list拥有节点与链头；caller拥有输出WORD，不另建输出影子或叶回调。

## 3. 唯一父的实际消费

4662EE发布primary WORD；466318发布该次live角色根。
46631D再次读取queued，466323构造数量地址，46632D压入未读零参数，466342累计。
466347再次读取queued，466365发布后继根；随后清group容量并置target1。
var_1C原为入口高WORD清零的DWORD局部，但叶只写低WORD；46639C/A0明确AND FFFF。
低WORD与primary按非负范围比较，正常零仍画十格；4664E8/EE末尾加共享WORD允许回绕。
primary、两根发布、count和每group查询仍重新借live队员。

## 4. 独立固定执行与实际门禁

新增285次执行，数量不是CTest项目数或完整性证明：

- 叶196：七category、四id、七mask组合，包含高bit、全1、固定0300和关闭门。
- 叶14：七数量位型及各自追加FFFF+2尾节点，核两级回绕和输出相邻WORD guard。
- 叶5：输出别名为root.secondary、first.secondary/tertiary、second.secondary/tertiary，
  最终10/10/6/21/21证明实际清零、部分累计和数量读取先后。
- 叶9：缺list、未知根/候选、数量累计后未知next、三种筛除后失败、正常空链与零根。
- 叶1：512个合格节点的有限真实链，数量3584，容器地址/长度和所有权保持。
- 网格40：十槽×三筛除与合格结果，核实际范围、行来源、末panel WORD及字体端口边界。
- 网格20：十槽×统计前/部分统计后失败，保primary文字/数量与0/7累计，阻断后继。

同步三个旧叶调用、真实网格断言与reply数组；既有七数量位型×十槽、live队员变化、
关闭输出/耗尽保旧值、容量失败、两组页推进和canonical message30回归保持。

实际门禁proc_3440共394秒、exit0：

- core setup 1/1，11.03秒；core actor_frame_316 1/1，25.51秒。
- ASan setup 1/1，18.47秒；ASan actor_frame_316 1/1，27.85秒。
- SDL136/136重新链接；九份已验证源码身份一致。

source审查proc_bf45为10秒、exit0；日志与身份：
`build/tmp/runtime/mode-resource-count-{setup,actor}-{core,asan}.log`、
`mode-resource-count-sdl.log`、`mode-resource-count-source-review.log`、
`mode-resource-count-validated-source.sha256`。
LST摘录、独立推导、双向review、production/tests及staged差异都在同目录的
`mode-resource-count-*`材料。两个edit重复匹配拒绝均以具体调用上下文修正，
未伪造编译结果；误猜测试runner路径不影响实际构建/测试，导航错误保留。

## 5. 历史与证据界限

3eda365b已删除471084的叶寄存器转发与提交计数，见
[资源链头发布](battle-actor-resource-list-commit-00470900.md)。
旧还原的core188/188、app194/194和203/422是历史门禁与进度，未用本批定向记录替代。
旧inventory SHA为`060968b2e3ee56d46b89501fbfa0bd0c0d15e1ff5d394634bd35b40b909b2f4d`。
当时的脚本计数兼容分支、DWORD副本和寄存器断言已由本批实际WORD路径替代。

未运行全量、Windows、真实MON、游戏或原版联合差分；动态原版资源链、输出WORD和
寄存器联合捕获缺口继续为`blocked_runtime_oracle`。
完整列表查询、字体、父参数包/寄存器/计数/Port、真实资源生产和SDL生命周期仍pending。
全项目scope仍未穷尽，不增加验收数，不前移B10 315/422、Workpack316、B11或实际续玩。
