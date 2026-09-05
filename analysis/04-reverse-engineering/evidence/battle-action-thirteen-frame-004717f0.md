# 战斗动作十三逐帧演出 `0x004717F0`

状态：`platform_adapted`。完整LST、typed实现、action-dispatch caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 完整权威范围

权威LST主体为`0x004717F0..0x00471AC5`，proc至endp共317行、204条实际指令、9个call、13个跳转、12个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一真实caller是`0x004539B0`的动作十三分支，ECX为group-A行动者，显式参数为group-B目标。

## typed语义

实现严格保留动作记录三字段初始化、更新失败零返回、帧资源读取、低字宽高、bit0翻转、目标偏移两分支、signed word坐标、32位减法、八倍runtime gate步进、仅比较横向终点、每帧固定音频调用、runtime gate递增、完成帧清零与零/一返回。完成分支按最终目标坐标绘制并清零line raster与152字节动作记录；未完成分支按当前raster增量绘制。

物理状态复用`LegacyBattleTargetPhaseState`动作记录和line-raster块、`LegacyBattleGroupAActionExecutionState`角色字段及shared frame-source owner。审计同时消除结构内`+0x29B4`重复字段，统一为`turn_target_x_offset`。已关闭的动作更新、frame provider和line-raster callee改为typed直连；三个坐标查询、音频和软件绘制保留窄callee port。action-dispatch动作十三不再调用整个`0x004717F0` opaque地址。

测试覆盖actor原访问点typed-stop、frame读取typed-stop、fallback坐标、非零offset坐标、bit0翻转、零步未完成、首步完成、精确owner清零、旧callee地址零调用和三处production caller后续行为。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`210/422 = 201 platform_adapted + 9 assembly_exact + 212 pending_audit`，SHA256为`7b2090d4a74387607b52ee59b1faea21ce4b9a866179adeb32ddc44d0a839e7d`。动态差分因原版行动者、目标、动作流、帧资源、坐标callee、音频与绘制寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
