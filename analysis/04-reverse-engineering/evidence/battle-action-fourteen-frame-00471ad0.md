# 战斗动作十四反向逐帧演出 `0x00471AD0`

状态：历史工作包211为`platform_adapted`。工作包282正在回收坐标callee；下述历史门禁不验证本轮修改，本轮完整发布门尚未完成。

## 完整权威范围

权威LST主体为`0x00471AD0..0x00471D56`，proc至endp共284行、187条实际指令、9个call、10个跳转、9个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一真实caller是`0x004539B0`的动作十四分支，ECX为group-A行动者，显式参数为group-B目标。

## typed语义

实现严格保留动作记录固定action id与base variant一、special mode外部字段、更新失败零返回、帧资源读取、双坐标分支、目标到行动者的反向line-raster、signed word坐标、32位减法、八倍runtime gate步进、只比较横向终点、每帧固定音频调用、动作记录mode flags绘制、runtime gate递增、完成清零及零/一返回。

物理状态继续复用`LegacyBattleTargetPhaseState`动作记录和line-raster块、`LegacyBattleGroupAActionExecutionState`角色字段及shared frame-source owner；新增`+0x0316`的唯一typed字段，与既有`+0x0318`共同计算行动者终点。已关闭的动作更新、frame provider和line-raster callee直接复用typed实现；坐标查询`0x004783B0`已直连共享叶函数；两个未审坐标查询、音频和软件绘制仍保留窄callee port。action-dispatch动作十四不再调用整个`0x00471AD0` opaque地址。

## 工作包282坐标调用回收

`0x00471B82`把`var_14`低WORD作为X输出、`var_10`低WORD作为Y输出。
`0x00471B76/0x00471B7A`分别令EDX为Y输出地址、EAX为X输出地址，ECX重装为目标角色。
这与动作十三不同：不能沿用前一offset查询的EDX。
输出覆盖前一offset查询低WORD，之后按原WORD减法和符号扩展形成反向raster起点。

两个兼容栈token由同一个`LegacyBattleActionDispatchContext`传入；根dispatch直接转交该context。
默认零仅表示未捕获，不是原程序地址或寄存器证据。真实动态栈捕获仍为`blocked_runtime_oracle`。
查询停止保留帧准备、第一WORD与到达寄存器，阻断raster、音频、绘制和清理；根dispatch也不得发布消息`0x62`或完成帧效果。

第十二轮core定向`1/1`通过，覆盖直连、既有完成路径及根dispatch两条坐标分支的第二读停止。
第八轮ASan定向`1/1`通过，覆盖随后新增的正常数值、根dispatch门读取停止和第二读停止矩阵，日志无编译或sanitizer诊断。完整门禁尚未运行。

完成分支`0x00471D0C rep stosd`耗尽ECX；已修正C++错误保留最后绘制callee ECX的行为。
新增非零绘制ECX/EDX回复测试，要求正常清理后ECX为零、EDX仍保留该回复。
第十轮ASan定向`1/1`通过，日志无编译或sanitizer诊断，覆盖此项修正；完整发布门尚未完成。

## 历史工作包211验证记录

测试覆盖actor原访问点typed-stop、frame读取typed-stop、fallback坐标、非零offset坐标、variant一、反向raster起终点、零步未完成、首步完成、动作记录flags、精确owner清零、旧callee地址零调用及production完成后的视觉、消息和actor状态。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`211/422 = 202 platform_adapted + 9 assembly_exact + 211 pending_audit`，SHA256为`681cf10be42d9ea88171534807933d2d4b73f4083c715c186c873b3272edb3f7`。动态差分因原版行动者、目标、动作流、帧资源、坐标callee、音频与绘制寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
