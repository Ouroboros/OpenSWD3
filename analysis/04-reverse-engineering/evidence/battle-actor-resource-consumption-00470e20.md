# 战斗执行动作时扣减队员资源并释放耗尽节点

状态：`pending`。已废弃GPT重建实现，本轮逆向尚未开始。

以下保留历史分析与证据；旧结论、完成记录和测试结果不作为本轮验收。
原版采集事实保留，所有解释和实现结论须重新核实。

## 历史资料（重新核实前不生效）


源码5ad2a9aa移除470E20叶的寄存器接口及计数，回收六处原物理调用消费。
状态为`platform_adapted`、`unit_tested`、`fixed_state_tested`。
完整动作、资源列表、菜单与头提交接口族仍`pending`，不标`assembly_exact`。

## 1. 独立权威范围与接口

470E20..470F65共165行、100指令、3call、19跳转、3返回，无外部chunk。
470900..47090C共8行，直接把next head复制到current。
原RET8的两个栈DWORD不被读取；ECX是借用角色对象，不是业务编号。
六个原call为453F5B、45544C、462F5F、4633D1、463A03、47086C。

叶只借实际list和workspace，结果只保留现有status与最终u16数值。
删除默认request、actor寄存器token、EAX/ECX/EDX回包和八个物理调用/写入计数。
两次头提交直接写共享head；其它头提交caller与接口不在本批验收范围。
实际遍历位置仍用于重链，不把业务位置当作诊断调用计数删除。

## 2. 全部基本块与资源身份

470E26先发布head；selected零正常返回数值0，并保持旧workspace。
470E3D..470E64每次破坏性推进current，以candidate和selected的资源WORD匹配。
next零先发布current零，再正常返回0；candidate与selected访问顺序保持。
入口selected非零且本段无写/无call，保存值不会自行变零，不增加人工回调。

470E69先发布实际selected，470E6F..470E76才访问并写workspace。
旧C++缺workspace时停在candidate；本批改为保留原已发布selected的失败前缀。
同ID匹配不替换selected身份，数量与销毁作用于实际selected节点。

470E7A..470EA8先递减bit10门的低BYTE，高BYTE保持；剩余非零才跳数量。
470EAA..470EBE只在category bit7清或bit27置时允许数量更新。
470EC0..470EF2先secondary、零时tertiary，primary独立非零递减；WORD回绕保持。
允许路径清selected；secondary/tertiary均零才真实erase，primary不决定保留。

470EFC..470F47先保存next、释放、重新发布head，再按首次匹配位置回放前驱。
位置减1后使用signed正值循环，替换旧无符号for条件；不新增次数上限。
删除同ID不同节点仍可使其它已拥有节点脱链，原行为保留。
释放后的重链访问失败保留erase、selected清零和已发布head，不恢复所有权。
470F48..470F65按实际current的位13抑制最终数值；未删读selected，删后读前驱。
原三个正常出口和已有宿主owner/节点失败均重查，不新增停点或成功兜底。

## 3. 六处调用消费与顺序

453F5B动作3消费最终WORD写computed选择值，成功后才清三个选择状态，再执行后缀。
原C++提前清理已修正；失败保持旧选择、computed值和fade，阻断撤退提交。
动作2不调用此叶，其原清理时点保持。
45544C动作27在4728E0完整返回1时才消费，随后重读阻断状态并清pair。
独立分析初稿误把比较条件写0；4539FB明确EBX=1，错误原稿保留，生产比较1未改。
两个动作helper借startup.party原对象，optional的present-zero继续，真实失败才nullopt。
删除脚本兼容分支、资源操作编号、子寄存器结果及计数，四类context夹具同步。

462F5F丢弃WORD；4633D1消费低WORD并零扩展样本item；463A03先写效果低WORD再执行后缀。
菜单只同步必需的两参数调用，既有实际资源、数量、live读取与失败路径保持。
47086C丢弃WORD，成功才清selected/primary required并刷新；失败不刷新容量。
原唯一父45703D后跳45735E重读EAX与后继ECX，不消费释放寄存器回包。
链表父只删除释放子协议及计数；其它刷新寄存器、参数、计数和Port仍待迁移。

## 4. 汇编推导向量与纠错

新增注册52次固定状态执行：

- 动作32矩阵：两站、资源WORD0/34/8001/FFFF、是否耗尽、最终抑制与实际对象身份。
- 动作8次正常零/节点失败：验证发布head、实际erase及尚未执行的选择/fade/提交。
- 叶8次前缀：selected零、空next、未知current/candidate/selected、缺workspace、
  释放后重链失败及缺list；同ID的candidate数量和实际selected身份独立核对。
- 链表4次：关闭资源模式仍刷新、正常零仍清理/刷新、未知head与erase后失败不刷新。

旧四组叶测试改查实际数量、所有权、链头、workspace与最终WORD，移除寄存器/计数断言。
既有1125菜单资源、283数量及四个进入/输入向量随相关setup目标同步执行。
初次core编译因新测试缺命名空间失败；补本地using及两处显式u16转换后通过。
链表新向量误期望成功刷新保留secondary required，按原刷新实际清零修正夹具。
原失败日志、分析初稿与源码身份保留，不修改生产行为迎合期望。

## 5. 验证与边界

所有构建/测试使用仓库脚本，六job并发，原测试锁与真实资产锁保持：

- core actor目标，精确battle.actor_frame_316：1/1，26.98秒。
- core setup目标，精确battle.legacy_battle_setup：1/1，11.26秒。
- ASan同两目标：各1/1，29.67/17.74秒。
- SDL实际136/136重新链接；未运行程序，未把构建当作生命周期验证。
- 13份最终源码SHA、whitespace、524行生产与533行测试差异及完整暂存复核通过。

proc_d7c2、proc_bb81、proc_19e3分别取得core、ASan/SDL及来源审计通过记录。
原初次失败及全部门禁日志、独立分析、双向追溯和冻结差异在
build/tmp/runtime/actor-resource-release-*；来源审计脚本为check-actor-resource-release-source.py。
五个既有测试窄化warning未扩入本批；本批13文件没有新增warning。
未执行全量UT、Windows、实际MON、游戏运行、原版联合allocator/寄存器差分或完整SDL链。
共享别名与原版动态fault不由固定节点验证关闭；其余父架构继续逐项迁移。
不改变315/422、Workpack316、B11或全项目范围穷尽性false。
