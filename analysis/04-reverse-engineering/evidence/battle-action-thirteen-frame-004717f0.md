# 战斗动作十三逐帧演出 `0x004717F0`

状态：历史工作包210为`platform_adapted`。工作包282正在回收坐标callee；下述历史门禁不验证本轮修改，本轮完整发布门尚未完成。

## 完整权威范围

权威LST主体为`0x004717F0..0x00471AC5`，proc至endp共317行、204条实际指令、9个call、13个跳转、12个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一真实caller是`0x004539B0`的动作十三分支，ECX为group-A行动者，显式参数为group-B目标。

## typed语义

实现严格保留动作记录三字段初始化、更新失败零返回、帧资源读取、低字宽高、bit0翻转、目标偏移两分支、signed word坐标、32位减法、八倍runtime gate步进、仅比较横向终点、每帧固定音频调用、runtime gate递增、完成帧清零与零/一返回。完成分支按最终目标坐标绘制并清零line raster与152字节动作记录；未完成分支按当前raster增量绘制。

物理状态复用`LegacyBattleTargetPhaseState`动作记录和line-raster块、`LegacyBattleGroupAActionExecutionState`角色字段及shared frame-source owner。审计同时消除结构内`+0x29B4`重复字段，统一为`turn_target_x_offset`。已关闭的动作更新、frame provider和line-raster callee改为typed直连；坐标查询`0x004783B0`已直连共享叶函数；两个未审坐标查询、音频和软件绘制仍保留窄callee port。action-dispatch动作十三不再调用整个`0x004717F0` opaque地址。

## 工作包282坐标调用回收

`0x004718EE`把`var_14`低WORD作为X输出、`var_10`低WORD作为Y输出。
`0x004718E2`令EAX为Y输出地址，EDX保留前一`0x00478400`返回值，ECX重装为目标角色。
两笔输出覆盖前一offset查询的低WORD；不能把前一callee的EAX直接沿用，也不能预读两个源坐标。
输出仍在后续原指令处做WORD减法和符号扩展，形成正向raster终点。

调用现场通过同一个`LegacyBattleActionDispatchContext`提供两个兼容栈token，根dispatch无需复制另一组值。
未捕获时默认零仅表示缺少地址记录，不是原程序寄存器证据；真实动态栈捕获仍为`blocked_runtime_oracle`。
门读取或第二次坐标读取失败保留帧准备、已经写出的WORD与到达寄存器，阻断raster、音频、绘制和清理。
根dispatch传播嵌套停止结果，不清临时记录、不发布完成消息、不修改完成帧效果。

第十一轮core定向`1/1`覆盖该直连及直接调用停止测试；第十二轮core定向`1/1`覆盖根dispatch两分支停止传播。
第八轮ASan定向`1/1`通过，覆盖随后新增的正常数值、根dispatch门读取停止和第二读停止矩阵，日志无编译或sanitizer诊断。完整门禁尚未运行。

完成分支`0x00471A7A rep stosd`耗尽ECX；已修正C++错误保留最后绘制callee ECX的行为。
新增非零绘制ECX/EDX回复测试，要求正常清理后ECX为零、EDX仍保留该回复。
第十轮ASan定向`1/1`通过，日志无编译或sanitizer诊断，覆盖此项修正；完整发布门尚未完成。

## 历史工作包210验证记录

测试覆盖actor原访问点typed-stop、frame读取typed-stop、fallback坐标、非零offset坐标、bit0翻转、零步未完成、首步完成、精确owner清零、旧callee地址零调用和三处production caller后续行为。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`210/422 = 201 platform_adapted + 9 assembly_exact + 212 pending_audit`，SHA256为`7b2090d4a74387607b52ee59b1faea21ce4b9a866179adeb32ddc44d0a839e7d`。动态差分因原版行动者、目标、动作流、帧资源、坐标callee、音频与绘制寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
