# 战斗菜单提交动作和替换效果时消耗实际角色资源

源码dd7ae08f回收462F5F、4633D1、463A03三处菜单通用资源调用。
状态为`platform_adapted`、`unit_tested`、`fixed_state_tested`。
完整菜单父与释放叶接口族继续`pending`，不标`assembly_exact`。

## 1. 权威范围和返回消费

父462740..4640F1共2820行；主case3/4/5精确块339/553/226行。
释放叶470E20..470F65共165行、100指令、3call、19跳转、3返回，无外部chunk。
头提交470900..47090C共8行，无条件复制next head到current。
六个原物理caller为453F5B、45544C、462F5F、4633D1、463A03、47086C。
本批仅三个菜单站；另三caller及叶寄存器请求/回复/计数仍待后续迁移。

三个出口RET8，但两个栈DWORD未被叶读取，没有业务参数语义。
462F5F返回不消费，后继重读已提交角色，再读取真实override并处理数量和清理。
4633D1只消费AX，写效果低WORD并零扩展为样本item；样本后重读live模式/效果并早返。
463A03先写效果低WORD，再提交动作15及输入、数量、效果后缀。

## 2. 实际资源与顺序

菜单直接借startup.party十槽内的actor_list和workspace，不复制链节点或另建同步字段。
核心帧与消息处理的真实绑定均传同一startup.party；输入和菜单进入沿原绑定转交。
删除菜单resolve_action_effect_value、资源测试兼容分支、子结果寄存器和调用计数。
既有release helper返回optional WORD；present-zero保留原正常零出口，只有真实失败为nullopt。
原范围及资源失败沿已有状态传播，没有新增叶停止条件或默认成功回复。

先提交head，selected零仍返回零。非零时破坏性遍历，以resource id WORD匹配，
命中后处理实际selected对象并写workspace。bit10低BYTE先减，高BYTE保持；
减后非零跳数量，归零或原零继续。category bit7清或bit27置才允许数量更新。
secondary优先、tertiary回退、primary独立递减，全部保留WORD存储；允许路径清selected。
secondary/tertiary同时零即真实erase，即使primary非零；随后提交head、按命中位置
回放前驱并重链。未删除读取selected位13，删除后读取前驱位13决定返回零或resource WORD。
叶output_word在最终位13抑制后才返回，不能把首次workspace写入当作最终结果。

同ID匹配不会替换selected身份；删除仍按第一次命中的位置回放，保留原脱链行为。
删除head后重链访问未拥有节点时，保留已释放节点和已发布head，不恢复owner或伪造成功。

## 3. 真实失败和调用方

三个菜单站都区分正常零与已有未知节点失败。
case3/4失败保持已提交动作、角色和清queued前缀；case5失败发生在动作15提交之前。
失败阻断尚未到达的数量、样本、效果和清理；九个数量失败仍核对已减资源的真实前缀。
原菜单进入与确认输入的四个成功/失败向量同步执行。
输入夹具改借startup.party，消除独立队伍副本；其余生产输入条件及断言保持。

## 4. 独立向量和纠错

test_menu_actor_resource注册1125次新菜单执行：

- 1080矩阵：三站、十槽、三种计数、三种category、两种数量、两种最终返回条件。
- 18次首/中/尾节点删除，selected与前驱位13相反，primary非零仍真实释放。
- 6次未知head/selected、3次正常零selected、3次遍历空next。
- 6次bit10低BYTE零/FF、6次同ID实际对象及原重链、3次释放后重链失败。

既有283数量菜单、四个进入/输入调用方及其它setup测试同步通过。
撤回纯释放叶上任意改写committed的两个旧callback向量，改为实际提交actor9并读其override；
原数量证据的四个前继向量应解释为两个真实样本模式读取与两个已提交角色override。

首case4宽摘录混入下一主case27，618行原稿保留，最终范围纠正为553行。
proc_e991编译通过但两个输入断言失败，原因是独立party副本；改借实际startup后通过。
公开叶第四request有默认值，三参数调用合法；此前接口不一致猜测已撤回。
首轮删除第三位置仍有后继，不能算尾节点覆盖；修正为next零并重新执行门禁。
所有失败、初次green、错范围、身份与纠错记录保留于build/tmp/runtime/menu-actor-resource-*。

## 5. 验证范围和未决项

proc_877c最终门禁通过，全部使用仓库脚本：

- core setup目标、精确battle.legacy_battle_setup，1/1，12.18秒。
- ASan同一目标与regex，1/1，19.60秒。
- SDL程序实际133/133重新链接，未启动游戏。
- 五份源码SHA一致，162行生产差异和599行测试差异完整阅读并与暂存逐字比较。

proc_aa7d来源审计通过；完整差异、冻结源码和双向记录保存在
build/tmp/runtime/menu-actor-resource-*。实际日志为core-final、asan、sdl和source-review。
本批未执行全量UT、Windows、实际MON资产、游戏运行、原版差分或完整SDL战斗生命周期。
叶默认request、寄存器/计数、另三caller和完整菜单其余操作/参数包/Port/别名继续pending。
原叶历史验证见[资源释放](battle-actor-resource-release-00470e20.md)，
本次菜单数量顺序见[实际物品数量](battle-menu-player-item-quantity-0045d180.md)。
不改变315/422、Workpack316、B11和全项目范围穷尽性false；整体Goal未完成。
