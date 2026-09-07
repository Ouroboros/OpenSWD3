# 战斗动作十三逐帧演出 `0x004717F0`

状态：`platform_adapted`。完整LST、typed实现、action-dispatch caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 完整权威范围

权威LST主体为`0x004717F0..0x00471AC5`，proc至endp共317行、204条实际指令、9个call、13个跳转、12个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一真实caller是`0x004539B0`的动作十三分支，ECX为group-A行动者，显式参数为group-B目标。

## typed语义

实现严格保留动作记录三字段初始化、更新失败零返回、帧资源读取、低字宽高、bit0翻转、目标偏移两分支、signed word坐标、32位减法、八倍runtime gate步进、仅比较横向终点、每帧固定音频调用、runtime gate递增、完成帧清零与零/一返回。完成分支按最终目标坐标绘制并清零line raster与152字节动作记录；未完成分支按当前raster增量绘制。

物理状态复用`LegacyBattleTargetPhaseState`动作记录和line-raster块、`LegacyBattleGroupAActionExecutionState`角色字段及shared frame-source owner。审计同时消除结构内`+0x29B4`重复字段，统一为`turn_target_x_offset`。已关闭的动作更新、frame provider、`0x00478400`绘制偏移查询、`0x00478470`基准坐标查询和line-raster callee均改为typed直连；音频和软件绘制继续保留窄callee port。绘制偏移查询复用Group-B lifecycle/action-composition canonical owner，两个原局部dword槽先清零，再严格按X后Y只写低16位。任一偏移低word为零时，`0x004718EE`按X后Y顺序把目标canonical坐标写回同一槽；Y读取失败保留已提交X与零高word。双非零时，`0x0047191D`把基准X/Y依次写入原`var_8/var_C`槽，再按完整dword与偏移、动作记录扣减合成终点；base查询保留X/Y输出指针token高word、目标ECX、最后一次偏移CMP flags和X后Y部分提交。任一坐标leaf typed-stop均阻断raster、sample、render、runtime gate推进和完成清理。action-dispatch动作十三不再调用整个`0x004717F0`或内部`0x00478400/0x00478470` opaque地址。

测试覆盖actor原访问点typed-stop、frame读取typed-stop、X非零/Y为零的fallback canonical坐标、零高word局部槽、canonical/绘制偏移/基准坐标三类Y故障的X部分写入和后缀阻断、双非零base查询的X/Y token高word与CMP flags、Group-B覆盖与镜像后的非零offset、bit0翻转、caller EAX/ECX/EDX/ESI与flags、零步未完成、首步完成、精确owner清零、旧`0x004717F0`与`0x00478400/0x00478470`地址零调用和production后续行为。验证：定向`1/1`、AddressSanitizer `199/199`、Linux core `199/199`、Linux app `205/205`和连续10轮core全部通过；对应stderr为空，源码零warning且未发现sanitizer finding。动态差分因原版行动者、目标、动作流、帧资源、坐标callee、音频与绘制寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
