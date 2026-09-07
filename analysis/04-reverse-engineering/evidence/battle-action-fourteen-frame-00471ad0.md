# 战斗动作十四反向逐帧演出 `0x00471AD0`

状态：`platform_adapted`。完整LST、typed实现、action-dispatch caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 完整权威范围

权威LST主体为`0x00471AD0..0x00471D56`，proc至endp共284行、187条实际指令、9个call、10个跳转、9个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一真实caller是`0x004539B0`的动作十四分支，ECX为group-A行动者，显式参数为group-B目标。

## typed语义

实现严格保留动作记录固定action id与base variant一、special mode外部字段、更新失败零返回、帧资源读取、双坐标分支、目标到行动者的反向line-raster、signed word坐标、32位减法、八倍runtime gate步进、只比较横向终点、每帧固定音频调用、动作记录mode flags绘制、runtime gate递增、完成清零及零/一返回。

物理状态继续复用`LegacyBattleTargetPhaseState`动作记录和line-raster块、`LegacyBattleGroupAActionExecutionState`角色字段及shared frame-source owner；`+0x0316`与既有`+0x0318`共同计算行动者终点。已关闭的动作更新、frame provider、`0x00478400`绘制偏移查询、`0x00478470`基准坐标查询和line-raster callee直接复用typed实现；音频和软件绘制继续保留窄callee port。绘制偏移查询复用Group-B lifecycle/action-composition canonical owner，两个原局部dword槽先清零，再严格按X后Y只写低16位。任一偏移低word为零时，`0x00471B82`按X后Y顺序把目标canonical坐标写回同一槽；双非零时，`0x00471BB1`把基准X/Y分别写原`var_C/var_8`槽，再合成反向raster起点。该路径EAX高word保留绘制偏移leaf装载的X输出指针token，ECX/EDX高word分别来自基准Y/X输出指针token，flags来自Y偏移CMP；Y侧typed-stop保留X提交并阻断反向raster、sample、render、runtime gate推进和完成清理。action-dispatch动作十四不再调用整个`0x00471AD0`或内部`0x00478400/0x00478470` opaque地址。

测试覆盖actor原访问点typed-stop、frame读取typed-stop、fallback canonical坐标、零高word局部槽、基础非零offset、绘制偏移X故障的零提交、基准Y故障的X部分提交、双非零base查询的反向局部槽与三类寄存器高word、frame-source EDX、行动者ESI、caller flags与完整后缀抑制、variant一、反向raster起终点、零步未完成、首步完成、动作记录flags、精确owner清零、旧`0x00471AD0`与`0x00478400/0x00478470`地址零调用及production完成后的视觉、消息和actor状态。验证：定向`1/1`、AddressSanitizer `199/199`、Linux core `199/199`、Linux app `205/205`和连续10轮core全部通过；对应stderr为空，源码零warning且未发现sanitizer finding。动态差分因原版行动者、目标、动作流、帧资源、坐标callee、音频与绘制寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
