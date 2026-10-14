# 战斗资源列表实际条目数 `0x00470A10`

状态：`pending`。

历史还原分类为`pending`。
当前条目计数及两个必要网格调用方的通用协议已回收。
完整角色列表查询、普通/替代网格、字体及主帧接口族仍`pending`。
源码提交：`5a60ef7d`。本页固定状态与定向验证不代表原版联合差分。

## 1. 唯一真值、范围与ABI

完整LST SHA256：
`701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b`。
叶范围`00470A10..00470ABB`，原行213761..213865：
105行、60条指令、1call、18jump、1RET，无外部chunk。
ECX为实际资源列表；两栈DWORD为类别/raw mask和可写WORD地址。
保存EBP/ESI/EDI，RET8；机器暂存器不构成业务计数回包或成功门。

实际opcode caller恰好两处：

- `00465B72`，普通网格`sub_4659C0`，完整503行/337指令/27call/11jump/1RET。
- `00465FC2`，替代网格`sub_465E50`，完整357行/238指令/20call/5jump/1RET。

独立推导先于C++审计；本轮又完整补齐叶与两个父的所有指令及出口。
早期read输出有截断，不以范围、SHA或cmp替代语义阅读。
归一化空白的三份review.lst保留机器码续行、全部标签、ABI及proc/endp。
生产417行、测试1017行及其完整staged差异均已逐页复核。

## 2. 实际参数、结果与资源所有权

`count_legacy_battle_actor_resource_list`借实际list、完整DWORD类别与调用者u16。
只返回既有访问状态；删除Count Request/Result、actor token、输入/返回寄存器、
数量副本及nodes_visited/positive_matches/extra_matches诊断计数。
类别是原业务筛选输入，保留其映射与raw mask语义，不再携带callee编号或参数数组。
节点、根、当前链头继续属于同一list；调用者持有计数WORD。
不复制容器，不搬owner，不读名称，不写selected或next root。

## 3. 逐基本块行为

`470A12..470A5E`精确比较类别0..5，分别映射为
`10/0C/1001/0800/2000/08000000`；其余完整DWORD作为原始掩码。
归一化先于`470A60`实际根发布，叶不清调用者初始WORD。

`470A69..470A79`先访问current.next，发布该next到head，再按零正常耗尽。
有效root.next0保留初值并令head0；零/未知root在原访问点失败。
撤掉旧`while(head!=0)`对零root的伪正常返回，不添加成功兜底或扫描上限。

`470A7B/80`依次短路category DWORD与mask、模式BYTE mask05。
通过后依次读取signed tertiary WORD、unsigned primary WORD、signed secondary WORD。
`470A94/96`以i32计算`tertiary-primary+secondary`，范围-131071..65534。
仅严格正值在`470A9C`直接增加实际WORD；每次增量独立按16位回绕。
不按ID0300排除，不提前按WORD求和，不读取筛选失败节点的数量。

`470A9F`精确比较最终mask与2000：类别4及raw DWORD2000都执行额外块。
不论普通门是否通过，`470AA7`重读actual head，`470AAD`读BYTE+4D bit5。
该位由唯一capacity WORD bit13表达，真时`470AB3`再直接增加同一计数WORD。
同节点可增0/1/2次，普通增量先于额外读取；不能累计到副本后一次发布。

`470AB8..470ABB`为唯一正常出口，不尾部复位根。
缺list、未知节点沿原list_owner/resource_owner状态；
保留根发布、已累计WORD与后续已发布head，不回滚、不隐藏失败。

## 4. 两个实际网格与选择帧

普通网格字体宽度前继之后借live queued的实际party list及live类别。
父清共享panel WORD后直接累计；成功后发布根、scroll+1并查询行。
本次同步计数叶不写queued，紧邻根发布消费同一实际对象；普通后续行查询仍缓存party，
其LST逐站重读queued的迁移尚待下一切片。正常零仍查询；原显示上限与耗尽尾发布保持。
删除initialize_rows操作、计数脚本回复、输出地址token、嵌套回包及计数诊断。
其它普通行查询/字体协议的既有兼容边界仍待迁移。

替代网格同样在字体前继之后借live list，清共享WORD并传类别4。
成功后重读queued并发布根；第八次成功查询仍先于七行显示上限。
删除仅计数使用的scripted兼容字段、actor比例/token、操作与子回包/计数。
选择帧message27撤去该兼容字段传递，始终借`startup.party`。

统计失败沿既有group_a_actor_typed_stop，保存叶的resource_query_failure。
保面板/标题/字体及0或2累计，阻断未执行的根复位、行绘制、输入门与target修改。
coordinator只撤本叶initialize_rows映射；共用list/narrow初始化标签保留。
message4纵向面板测试改用八个实际条目；message27实际空链发布零辅助值并走原后缀。

## 5. 7332次新增固定状态执行

叶6992次：

- 筛选6776：11类别/raw mask×11节点mask×7模式BYTE×2数量条件×4额外BYTE。
  raw mask含10/2000/80000000/80000004/FFFFFFFF；不截断DWORD。
  模式00/01/02/04/05/80/FF；额外BYTE00/20/80/FF覆盖关/开/非目标/混合位。
  ID0300参与计数，100字节名称不读取，邻接WORD和资源存储保持。
- 数量200：10 signed/unsigned边界×5初值×2类别×2额外门。
  派生量覆盖0、1、-1、65534与-65536，保符号及零扩展；初值0/FFFF/FFFE/8000/BEEF。
- 七真实WORD别名：根primary、当前secondary、后继primary/secondary/tertiary、
  当前capacity的1FFF与3FFF。结果BEF1/2/1/2/2/2001/4000。
  前一次写入影响后继数量及BYTE+4D读取；不将u32 head伪装为u16引用。
- 八空链/失败：缺owner、零/未知root、未知首candidate、普通/双增量后失败、
  跳过后失败、有效空链；保真实初值、已发布head、selected、next root和owner。
- 一个512候选长链：每节点双增量，FFFE+1024回绕为03FE，全部遍历。

普通网格270次：六类×十槽×四资格240、十槽两种失败20、live前继10。
替代网格70次：十槽四资格40、两种失败20、live前继10。
字体前继向量使用剩余字体Port的既有测试observer改变queued；
实际计数叶与共享WORD均执行，不使用脚本计数回包，不称为原版字体验证。

三个outer测试由legacy_battle_setup实际runner调用，新增helper由各outer调用。
旧链头/计数、两网格七/八行、名称数量、选择帧及actor_frame_316回归同步通过。

## 6. 实际门禁与纠错

`proc_ed77`成功，398秒/exit0：

- core legacy_battle_setup：1/1，11.54秒。
- core actor_frame_316：1/1，25.58秒。
- ASan legacy_battle_setup：1/1，19.33秒。
- ASan actor_frame_316：1/1，28.10秒。
- SDL实际136/136重新链接，12份validated source身份通过。

首`proc_01fb`64秒/exit1：测试引用不存在的vertical_panel_calls。
改为实际共享WORD/辅助值断言后完整重跑，保留原日志与差异。
迁移脚本首`proc_2363`把五条断言误记为六条；普通网格已写部分保留，
`proc_af3a`按实际范围恢复成功，没有生产失败或测试门伪通过。
source checker首`proc_4663`仅错误要求独立文档的2000带0x前缀；
修正该字面断言后`proc_f020`9秒/exit0，不改源码或门禁结果。

实物前缀`build/tmp/runtime/resource-list-count-*`包含LST、独立推导、双向审查、
完整review/staged差异、纠错、四门与SDL日志、12源码身份及发布修订。
未跑本轮全量UT、Windows、实际MON、原版联合oracle或游戏。

## 7. 历史与未验边界

历史第198项曾登记core188/188、app194/194及双生成SHA：
`6c5586aaab691a5a1576cf43cdd9419129384afe85290c0d53509de99f714828`。
这是历史还原记录，不是本轮门禁或完整INVOKE验收。
3eda365b根发布迁移见[链头发布](battle-actor-resource-list-commit-00470900.md)。

原版资源链、输出WORD和两个caller联合捕获仍`blocked_runtime_oracle`。
列表/字体/父通用参数、寄存器、计数与Port继续pending，完整SDL链及续玩未验收。
不改变B10 0/422、Workpack316 pending_audit、B11或全项目scope=false。
