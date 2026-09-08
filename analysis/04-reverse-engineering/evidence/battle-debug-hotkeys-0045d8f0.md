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

逐帧协调器删除第六个前置opaque完成门并直接组合本函数；子返回0保留音乐、前置stage、角色预处理、metric和顺序副作用，然后在surface lock前返回。typed-stop也在同一点阻断后续surface、选择、绘制、输入和截图阶段。F9和F2的两处文字发布已在原调用点直连共享文字消息入链，原文字槽只保留reserved枚举值。H/J旧`adjust_actor`槽同样改为reserved名称并保留ordinal，生产路径对`0x004785A0`零opaque调用。

## 8. 测试与动态差分

定向测试覆盖总门关闭只保留P、左右Control短路、19类DIK查询顺序、E键零返回、F1/X/K/F9/F2切换与精确延迟、Z/D/F/V动态两组循环、AI跳过、C键重定向与陈旧索引、缺失角色帧停点、W键完整126 dword记录重置、第19项发布停点、H/J canonical坐标写入与双键恢复、首轮及后续CMP flags、leaf typed-stop部分提交、delta与P后缀抑制、P异常值及F3/F5双音频顺序。逐帧caller测试覆盖零返回前缀、共享选择值继续影响角色优先路径和旧完成门删除。

发布验证执行定向、独立AddressSanitizer、Linux core、Linux app、连续十轮core、changed-range格式、inventory双生成及完整release审计；最终计数与日志摘要同步记录在模块文档。

当前缺少原版完整键盘轨迹、剩余callee对象状态、Sleep墙钟、完整Group-A/Group-B actor、可写与异常内存页、音频/文字/角色副作用以及四处H/J caller联合寄存器/SEH捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
