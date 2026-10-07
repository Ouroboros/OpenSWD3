# 战斗调试快捷键总处理 `0x0045D8F0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围与调用图

权威函数为`0x0045D8F0..0x0045DED6`，从proc到endp完整731行、436条实际指令、48个call站点、53个分支与44个局部标签，无外部FUNCTION CHUNK。

唯一caller是逐帧画面协调器`0x00453200`。48个call站点由三类组成：

- 19次原始DIK键查询`0x004372D0`；该callee已关闭，现直接读取同一256字节typed键盘快照；
- 8次导入`Sleep`；保留可观察阻塞时长，通过平台延迟端口执行；
- H/J四次角色坐标调整`0x004785A0`；现全部直接组合其typed leaf；
- 其余17次音频、文字、角色对象与数值调用继续通过窄typed端口表达，物理对象地址只作为`compat::u32` token。

## 2. 调试总门、Control分流与尾键

入口调试总门完整dword不等于1时直接跳到P，只查询P，不查询或执行Control、H、J。总门等于1时，Control判定严格为：

1. 先查询左Control DIK `0x1D`；
2. 左Control非零时不查询右Control；
3. 左Control为零才查询右Control DIK `0x9D`；
4. 两者都为零时直接跳到H/J尾部。

总门开启但Control未按下时共查询左Control、右Control、H、J、P五键。P位于调试总门两条路径汇合后的共享尾部，不需要Control。

## 3. Control组合键顺序

Control块按LST固定顺序查询：F3、F1、X、K、Z、D、F、V、F9、E、C、F5、F2、W。不能按功能类别重排。

- F3：调用一次音频输出暂停。
- F1：阻塞200毫秒，再把共享dword按`old==0 ? 1 : 0`切换。
- X：阻塞200毫秒，以同一精确规则切换另一共享dword。
- K：共享消息latch为0时先写1，随后无条件以固定坐标、颜色和文字token发布调试文本。
- Z：阻塞200毫秒；第一轮对动态组A逐对象执行三参数重置与单参数重置，第二轮重新从0扫描动态数量并发布固定配置。
- D：阻塞200毫秒；逐项读取组A两组AI dword，只有两者都不等于1才发布`(80,-10,-10)`。
- F：阻塞100毫秒；同一筛选发布`(500,-5,-5)`。
- V：阻塞200毫秒；逐个动态组B发布`(10,0,0)`。
- F9：阻塞200毫秒；对共享速度执行`(signed)(old+1) % 2`，保留负值余数和32位回绕，不合理化为bool。
- E：立即返回EAX 0，不查询C/F5/F2/W，也不执行H/J/P尾键。
- F5：依次暂停音频输出，再以固定战斗音乐路径token和mode 0重启音乐。
- F2：阻塞200毫秒；以完整dword低byte的bit1切换文字模式，同时发布对应固定文字，高24位保持。

所有动态角色循环都在callee后重新读取数量，不增加现代上限。

## 4. C键状态清理与陈旧索引

C键先执行以下固定写序：

1. 只对共享状态word低word OR 1，高word保持；
2. 清选择gate；
3. 清七dword选择工作区；第一项与角色优先索引共用唯一typed存储；
4. 清两项最终角色帧门，并同步动作路径同址门；
5. 把角色优先索引写`0xFFFFFFFF`。

特殊重定向gate精确等于1时，先清gate，再查询特殊对象的AX并按i16符号扩展。随后两类对象token都按机器码的32位LEA/shift乘法生成，不替换为现代数组索引。第一个对象callee后重新读取角色优先索引，再生成第二个对象token；第二个callee后再次重读该索引。

动作阻断gate精确等于1时先清gate，再以当前索引生成第三个对象token并调用重置。特殊重定向gate不等于1时，当前索引保持此前`or ecx,-1`形成的`0xFFFFFFFF`；这是原始陈旧寄存器路径，不修复。

缺少角色帧typed owner时，只在原始动作阻断gate读取处typed-stop；此前状态word、工作区、帧门、优先索引和可选重定向callee副作用全部保留。

## 5. W键全体状态重置

W键先按动态组B数量扫描。每项先查询对象状态；返回不等于1时才按顺序：

1. 发布对象索引；
2. 清同索引反馈值；
3. 发布`(30000,0,0)`。

第19次真实18槽发布store才typed-stop；第19个对象状态查询已经发生，前18项副作用保持。

完整扫描后按LST顺序：

- 把共享反馈计数写为当前动态组B数量；
- 写目标ready与两项最终角色帧门为1，并同步动作同址门；
- 清十项最终角色顺序、十dword辅助块；
- 把18个`0x1C`物理记录的126 dword全部清零，再逐记录把`+0x00`写`0xFFFFFFFF`；
- 反馈actor word写`0xFFFF`；
- 清完成计数、重定向gate、选择辅助gate、重置gate、排队角色与消息；
- 角色优先索引写`0xFFFFFFFF`。

`LegacyBattleStartupResetRecord`扩展为精确`0x1C`字节布局，W键和全局重置不再只清部分字段。

## 6. H、J与P尾键

H与J受调试总门约束，位于可选Control块之后；P位于总门关闭与H/J完成路径汇合后的共享尾部：

- H：先按Group-A当前无符号count，再按Group-B当前无符号count直接组合`0x004785A0` typed leaf。每项X低word为`+10`、Y低word为0；两组完整结束后才把共享actor delta写10；
- J：同序直接组合typed leaf，每项X低word为`-10`、Y低word为0；两组完整结束后才把actor delta写-10；
- H/J同时按下时先完整加10，再完整减10，角色坐标按word模65536恢复，最终共享delta为-10；
- P：按`old==1 ? 0 : 1`切换截图请求，异常非1值统一写1。

Group-A token为`0x005029D0 + index * 0x2F34`，Group-B token为`0x00525508 + index * 0x2B28`。首轮leaf flags来自`count - 0`，后续轮来自`index - count`；EAX为当前group count，ECX为actor token，EDX保留尾部入口高word且DX在成功调用中被Y参数清零。

leaf停止立即映射为`actor_coordinate_adjustment_typed_stop`，保留此前actor和当前X已提交前缀，并抑制余下actor/group、delta提交和后续J/P。Control+E仍在H/J/P之前返回0。普通尾固定返回EAX 1；完整函数只有E键返回0和普通尾返回1两种正常返回。

## 7. 单一typed物理状态与caller回收

本函数直接复用既有唯一typed owner：

- 组A/B动态数量、角色优先索引与角色发布数组；
- 最终角色帧门、选择gate、排队角色和十项角色顺序；
- 单体/群体效果协调器的反馈计数、完成计数与反馈actor；
- 效果步进actor delta与共享战斗消息；
- 组A/B帧与结果判定共用的唯一结果latch；
- 启动状态的18个完整`0x1C`记录和反馈数组；
- 世界玩家控制的速度模式；
- 逐帧截图请求与战斗mode flags。

该状态定义已抽到无循环依赖的共享state port；撤退提交直接读取同一battle mode并清同一调试重置门，不建立动作分派副本。H/J坐标写入复用startup party和Group-B lifecycle action-execution的canonical `LegacyBattleActorCoordinatesState/View`，不建立调试专用角色数组。

全局重置同步清本函数已映射且属于原固定写集合的状态，未写入的F1/X/K/F9/F2/P开关保持入口值。

逐帧协调器删除第六个前置opaque完成门并直接组合本函数；子返回0保留音乐、前置stage、角色预处理、metric和顺序副作用，然后在surface lock前返回。typed-stop也在同一点阻断后续surface、选择、绘制、输入和截图阶段。K和F2的两处文字发布已在原调用点直连共享文字消息入链，原文字槽只保留reserved枚举值。H/J旧`adjust_actor`槽同样改为reserved名称并保留ordinal，生产路径对`0x004785A0`零opaque调用。

## 8. 测试与动态差分

定向测试覆盖总门关闭只保留P、左右Control短路、19类DIK查询顺序、E键零返回、F1/X/K/F9/F2切换与精确延迟、Z/D/F/V动态两组循环、AI跳过、C键重定向与陈旧索引、缺失角色帧停点、W键完整126 dword记录重置、第19项发布停点、H/J canonical坐标写入与双键恢复、首轮及后续CMP flags、leaf typed-stop部分提交、delta与P后缀抑制、P异常值及F3/F5双音频顺序。逐帧caller测试覆盖零返回前缀、共享选择值继续影响角色优先路径和旧完成门删除。

发布验证执行定向、独立AddressSanitizer、Linux core、Linux app、连续十轮core、changed-range格式、inventory双生成及完整release审计；最终计数与日志摘要同步记录在模块文档。

当前缺少原版完整键盘轨迹、剩余callee对象状态、Sleep墙钟、完整Group-A/Group-B actor、可写与异常内存页、音频/文字/角色副作用以及四处H/J caller联合寄存器/SEH捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

## 9. 特殊actor动作目标caller直连

工作包296关闭C键重定向路径中的`0x0045DBE3`物理call，固定actor token为`0x004E80FC`，真实返回地址为`0x0045DBE8`。入口EAX、EDX和flags继承角色优先级更新后的caller状态；leaf只替换AX，返回后才符号扩展动作目标、定位Group-B角色并执行重置和action-block清理。

调试状态保留专属特殊actor canonical动作目标；旧查询ordinal改名为`reserved_query_special_action_target`并保持生产零调用。字段或RET停止保留按键状态、C键重定向及优先级更新前缀，抑制目标重置、action-block和公共清理。测试覆盖固定token、字段token、真实返回地址、EAX高word、ECX/EDX、flags和typed-stop边界；生产`0x004786E0` raw调用为零。

当前缺少原版特殊actor与完整Group-B对象、优先级callee共享副作用和该caller寄存器、flags与SEH联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

## 10. B11真实键盘与共享状态接线

基线47427136。实现前已分为两批：本批接真实键盘、共享状态和调用失败传播；
下一批接角色记录、音频与文字端口。完整热键仍须两批验证，不改变B10游标。

重新完整读取731行45D8F0及67行4698E0，复核453252调用与453257返回判定。
SDL仅在前帧、指标及排序正常返回后进入本函数。借用原始键盘快照和世界的
4CAE98字段，精确1判定不读取调试结构中独立夹具使用的默认值。
F1/X/F9、C的共享状态清理、H/J实际角色坐标、P截图请求均复用现有owner。
等待使用SDL_Delay，保持每个调用的100/200毫秒及先后顺序。

E键正常返回0时，SDL按45325B..45325D返回，不能伪装成typed-stop；
普通返回1后停在45325E锁表面准备处。本批不执行表面锁、绘制或截图写盘。
上游45F2A0仍未接的输入分支继续先停止，例如默认绑定的F1可进入记录19分支；
本批不跳过该前缀，因此不宣称所有按键在所有输入状态下已可实机使用。

外部端口未绑定时默认返回明确失败；不再以空回复冒充完成。每个真实callsite
立即传播停止，保留先前状态写入与已正常返回的调用。W的状态查询失败时不发布
索引；数值调用失败时保留此前索引与反馈写入。K/F2保留先写的开关或latch。
文字分配未返回时不生成节点；测长未返回时保留已分配和初始化的节点，
不消费回复寄存器，不写长度、flags或链头。通用文字层新增独立call失败状态，
既有正常返回、零分配、文字访问失败的合同不变。

H/J的EDX参数表示尾部入口，不能把排序返回值冒充经过按键查询、等待等调用
后的现场。SDL和核心逐帧caller均显式标记该诊断现场未知，坐标业务仍使用
固定低WORD参数。该标记不参与业务分支，也不是原版CPU捕获证据。

新增15种调用停点向量，覆盖音频两个站点、Z三种调用、D/F/V、W查询与发布、
K/F2的分配与测长。四个共享门值0/1/2/FFFFFFFF覆盖门外P、门内精确等待、
真实两组坐标回绕及H/J恢复、速度切换；另验证默认未绑定端口停止。
既有E键返回、C清理、调试逐帧caller及文字回归仍由setup目标执行。

proc_2de9：定向core/ASan各1/1（4.84秒/8.81秒），SDL构建通过。
仅core/ASan重编译出现既有outcome_resolution测试137行u16到u8警告。
补充诊断未知性后的proc_ae1b：core/ASan各1/1（5.00秒/8.20秒），SDL构建通过；
仍只有同一既有测试警告。生产与测试完整差异已按上述LST逐调用复核。
日志前缀build/tmp/runtime/battle-debug-shared-input-。
未运行游戏，未新增真实资产或原版动态差分。完整热键外部端口、完整帧与
实际续玩未验收；B10仍315/422，WP315已关闭、WP316 pending_audit。

## 11. B11实际角色、音频与文字调用

基线9168bed1。本批完成预定第二部分的SDL回调绑定，仍在45325E前停止，
不绕过45F2A0的既有输入停点，不执行后续表面锁或绘制，不升级B10审计游标。

重新完整核对47D950的83行、47DA10的65行、482DA0的115行、47F150的237行、
47CE80的17行及47F360的10行。对象引用借用已有分配注册表与存档字节，
每次原始记录访问重新借用映射；没有映射或剩余长度不足时保留已写前缀并停止。
物理对象范围仍由既有typed owner表示，不宣称任意guest内存或SEH完全兼容。

Z的即时记录、基础记录、辅助记录和MON数据均使用实际存储：

- 47D950：kind精确1时，三个-1分别从最大HP/MP/SP恢复当前值，合法空引用跳过；
  其他kind对MON先写FFFFFFFF至+4C，再重读引用写FFFF至+64。
- 47DA10：三个正WORD参数写基础记录+26/+28/+16，或MON的+56/+58/+5A。
  基础记录没有额外空引用跳过；第二次访问失败保留第一次写入。
- 482DA0：实际2B00或2B04精确1时立即返回；固定-1把实际辅助首DWORD写56，
  随后重读引用和值。此调用域不进入通用浮点或音效分支，WP399保持待审。

465213..465219证明行动摘要必须解引用actor+8并精确比较DWORD 56。
删除group_a_profiles及group_a_auxiliary_profile_kinds合成缓存，摘要直接读取
四个连续0x60辅助记录，与调试写入共用存档恢复的数据。原两次字体调用仍在前。
辅助映射允许非对齐及跨记录读取，末端不足DWORD明确停止。

D/F直接读取实际action-execution的2B00和startup progress的2B04，
不再依赖Group-A帧中的独立AI数组。47F150借用实际角色及MON字节，保持：

- 正数伤害才检查26D1 bit7、26C0 bit25；再检查2AB8精确1与26C8低byte bit3。
  这些门阻断时不触碰坏记录。
- kind1：2B18精确1且共享4ACBD0无符号非零时，对共享DWORD减法回绕；
  否则当前HP低WORD减法后做有符号WORD上限比较。
- MP/SP先加低WORD、分别比较有符号上限，然后分别非正清零；最后HP非正清零。
  死亡置2ABC，2B00精确1时额外写2A94=6，不调用随机数。
- 其他kind：非零26B4使用DWORD血量；零26B4使用MON的WORD血量。
  保留负WORD恢复上限、WORD与DWORD上限的不同宽度以及各次引用重读。
- 非kind1死亡先读取2A94，置2ABC、清2958；原2A94为0才调用共享random(10)，
  低byte加1后写回。正常返回EAX为死亡1或未死亡/被阻断0。

W状态查询复用已有47CE80实现，仍先查询、再发布索引和清反馈、再修改数值。
C的角色重置已经直接调用实际runtime-reset owner，不新增第二套重置数据。
组B外部基础记录按稳定5213A0、0x20步长定位，不用可变引用值创建新映射。

F3/F5停止同一音频流；F5通过既有音乐路径实现重启，借用实际53C198路径。
K/F2的固定文字按LST映射含NUL的原始字节，长度为6/8/8；未知token明确停止。
节点仍由共享guest分配与既有消息链持有。本批没有文字或音频的实机播放证据。

新增固定状态验证包括：Z三记录链的11个访问失败前缀、MON负参数、精确AI门、
短基础记录及合法空引用；D/F/V/W串联实际记录、查询、死亡随机和发布状态；
47F150的15个逐访问停止、正数门、共享币值回绕、WORD溢出和负上限、两种MON
血量路径及条件随机；记录引用改变后保留已装载引用并在下一处重新读取。
辅助读取覆盖0/56/10038/FFFFFFFF、字体回调改变引用及映射末端边界。

proc_84d6：core/ASan各1/1（4.68/7.49秒），SDL构建通过。
随后只补边界测试，proc_57c5：core/ASan各1/1（4.29/7.05秒）。上述日志无警告错误。
对应日志前缀为build/tmp/runtime/battle-debug-external-ports-final-与
battle-debug-record-boundaries-。

早期红测另外出现一次selection_hint_frame_test:323失败，后续全部门禁未重现。
审查发现该独立夹具的panel_action_record未值初始化，其聚合类型没有默认初始化，
而动作更新读取external_mode等入口字段；这是已确认的夹具风险，但原失败日志
没有字段快照，不能把该风险写成已动态证明的唯一原因。本批未修改该无关测试。

未运行游戏，未增加原版动态差分，不宣称CPU寄存器、任意异常访存或完整热键实机
验收。完整帧、返回、重入与实际续玩仍待完成；B10仍315/422、WP316待审。
