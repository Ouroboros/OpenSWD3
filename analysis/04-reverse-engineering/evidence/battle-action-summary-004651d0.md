# 战斗动作摘要 `0x004651D0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x004651D0..0x00465474`，从proc到endp共314行、210条实际指令、14个静态call、16个跳转、13个局部标签、1个`retn`，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭选择帧message 1尾部。

六个唯一callee中，动作模式刷新已经关闭并直接组合；字体清零、字体样式、原始字节文字绘制、组A特殊门和动作可用性五类callee继续使用窄typed端口。

## 2. 入口、字体与组A对象前缀

入口以live queued actor code覆盖EAX；零值立即返回，ECX/EDX保留入口值且不配置字体。非零时固定依次发布字体参数0与`0xFFFE`，随后以`queued-8`的u32值计算组A对象步长。

对象索引只在首次读取actor内profile token时typed-stop；token为0则在紧接的kind dword解引用点停止。kind完整dword等于`0x38`时调用特殊门，完整EAX为0才把九byte权限域的第4项置1。组A profile token/kind建立在启动状态的单一typed owner中：组A对象reset时清零，profile配置callee只有显式发布时写入。

两次字体callee和可选特殊门的完整返回寄存器进入后续已关闭动作模式刷新。刷新继续读取live startup mode、party mapping、option source、queued actor和输入状态；任一子typed-stop保留全部字体、profile查询、权限及刷新前缀，并阻断所有摘要行。

## 3. 四项固定动作

动作模式刷新完成后固定扫描四项：

1. 依次读取九byte权限域第1..4项；
2. 权限byte替换EDX低byte，ECX先写一基行号，再只替换CX为主色或次色；
3. 行Y为`origin_y + index*24`，X按原`index>>2`公式计算，四个合法index均为`origin_x`；
4. 文字token不建立副本，直接复用动作模式静态21项表的第16..19项；
5. 每行以style 4绘制，权限完整byte严格等于1才用主色。

当前action kind等于一基行号1..4时，先把字体样式设为`0xF000`，再在`x-1,y-1`以style `0x10`和主色重绘同一token，最后恢复`0xFFFE`。高16位按原局部寄存器来源保留，不把word颜色合理化为全dword赋值。

## 4. 三项动态动作

动态段从共享三项action token与三个连续u16 action code读取；token为0时跳过可用性查询、普通绘制、选中重绘和权限修改。非零时按queued组A对象调用动作可用性callee，并读取权限第6..8项。

只有callee完整EAX等于1且权限byte完整等于1时，以主色绘制；其他情况都把次色写入查询返回ECX低word、保留ECX高16位，并在绘制前把对应权限byte清0。普通动态行固定X=`origin_x+48`，首行Y=`origin_y+24`，步长24，style 4。

当前action kind 6..8匹配本行时，无论普通行是否可用，都会以主色在`x-1,y-1`执行style `0x10`选中重绘并恢复字体样式。该非对称行为保持原逻辑，不擅自让禁用项失去选中覆盖。

## 5. 寄存器与返回

固定普通行在文字call前保留`EAX=x, ECX=font, EDX=token`；固定选中行保留`EAX=x-1, ECX=font, EDX=0`。动态可用普通行保留`EAX=surface, ECX=font, EDX=x`，禁用普通行保留`EAX=x, ECX=font, EDX=token`；动态选中行保留`EAX=y-1, ECX=font, EDX=surface`。

进入动态段时重新装载原始Y到EDX，并把EAX设为`6-0x00524419`的u32结果；不能沿用固定行最后callee返回。三项token全零时，这两个陈旧值穿过循环，最终只把ECX改为字体owner并进入最后一次`0xFFFE`样式call。函数返回该最终callee完整EAX/ECX/EDX。

## 6. 共享owner与caller回收

启动阶段原`frame_value_a/b`已更名为主/次文字颜色，并继续承接原两次颜色查询写；动作摘要不建立颜色副本。九byte权限域、三项action token/code继续由启动reset唯一owner持有。固定动作token继续由动作模式静态21项表唯一持有。

选择帧原`draw_action_summary` opaque槽改为reserved并保持生产代码零调用。message 1在角色标签绘制后直接组合本实现，累加子端口次数，完整传播EAX/ECX/EDX；profile或动作模式子typed-stop映射为选择帧专用停止并阻断调用点返回尾。

## 7. 验证与动态差分

定向测试覆盖queued零早退、负组A索引、零profile token、四固定行token/颜色/几何、固定选中覆盖、两动态行可用/禁用分歧、次色陈旧高word、权限清零、禁用动态项仍选中、特殊门、动作模式子stop、动态空尾寄存器及唯一caller传播。

验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。

当前缺少原版组A profile内容、五类callee共享副作用、字体状态、文字surface和EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
