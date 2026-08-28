# 战斗文字消息逐帧协调 `0x00469960`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00469960..0x00469D10`，从proc到endp共440行、306条带机器码和真实助记符的实际指令、15个静态call、27个跳转、23个局部/返回标签和1个返回点，没有外部`FUNCTION CHUNK`。唯一caller是战斗逐帧协调器`0x00453200`，调用位置严格位于消息阶段之后、packed-row效果链之前。

15个callsite固定为动作更新1次、矩形1次、九宫格1次、常量色垂直渐变7次、文字绘制4次和节点释放1次。动作、矩形、九宫格和渐变直连已关闭typed实现；字体文字与释放保持窄边界，并显式保留EAX/ECX/EDX。函数无参数且caller忽略返回值，但各窄边界仍观察权威调用前寄存器。

## 2. 第一轮：活动节点绘制与计时

函数从共享链头读取首节点，逐项先取`+0x08+6`作为文字Y、`+0x04+16`作为文字X，再按flags执行：

- bit31：复用共享动作记录，写动作`0x233B`和variant 0；绘制`10,10,620,32,-8,-8,-8,1`矩形，再以矩形返回EAX高word和动作`field_4a`低word绘制`14,14..626,38`九宫格；
- bit30：绘制`10,10,620,32`、颜色`0xFF`的固定渐变；
- bit0：把文字X改为18；
- bit1：按权威SAR/LEA/SHL/SAR链，以byte长度计算`315-10*floor(length/2)`；bit0与bit1同时置位时bit1后执行并覆盖bit0；
- 16-bit计时按i16判断，只有大于0才执行动画、文字和`dec word`。

活动动画按bit4、bit5、bit6顺序独立执行，不合并为互斥分支：

- bit4：宽度项按`(8-SAR(length,1))*11`的32-bit回绕链计算；X为0时先写640，冻结门为0才把X与`227+宽度项`做SAR平均；渐变位于`X-宽度项,10,176,32`，文字Y固定16；
- bit5：冻结门为0时把现有X与`227+宽度项`做SAR平均，不执行bit4的零X初始化；渐变和文字Y同bit4；
- bit6：`value_14`先按`(value_14+32)`的CDQ/SUB/SAR链向零除2，X固定为`227+宽度项`，渐变Y为`value_14-6`，文字Y为新`value_14`。

文字调用固定使用字体token、当前target surface、计算后的X/Y、节点文字token、颜色`0xFFFF`、字号16和尾参数0。只有文字返回后才递减16-bit计时；计时从1减到0的节点会在同一次调用的第二轮立即进入过期处理。

## 3. 第二轮：过期动画与原位摘链

第二轮重新从live链头开始，以“链头槽token或前一保留节点token”维护待写next的位置。计时按i16大于0的节点直接保留；其余节点依次执行：

- bit5：冻结门为0时X加80，绘制渐变与Y=16文字；signed X小于640时立即保留，否则继续测试后续位；
- bit4：冻结门为0时X减80，绘制同类渐变与文字；signed X大于0时立即保留，否则继续；
- bit6：冻结门为0时`value_14 -= value_08`，随后无条件把`value_08`按u32左移一位；绘制纵向渐变和文字，signed Y大于`-32`时保留。

没有命中保留门时，函数先把当前节点next写入前一链位置，再调用释放边界，最后从刚写入的链位置继续，不跳过连续过期节点。typed动态存储仅在释放边界返回后删除该token。所有循环保持原无界链遍历，不增加现代节点上限。

## 4. live状态、typed-stop与寄存器

链头继续复用`LegacyBattleStartupResetRecord::block_5214f8[0]`，36-byte节点继续由`LegacyBattleStartupState::text_messages`唯一承接；本工作包不建立第二份链或渲染缓存。`0x0053C00C`复用主帧`render_abort_latch`唯一owner，并在每个原始动画访问点live读取：任意非零值冻结坐标推进，但bit6步长翻倍、绘制、计时和释放判断仍按原顺序执行。

面板动作记录复用胜利奖励owner，颜色渐变复用选择提示owner；framebuffer、矩形、九宫格和像素格式均复用主帧现有owner。未知链token只在第一轮或第二轮首次真实节点字段访问点typed-stop，保留此前节点绘制、计时、坐标、链和寄存器副作用。矩形、九宫格或渐变typed失败在对应真实绘制点停止；后续文字、计时、摘链及全部帧尾不执行。

直接渲染callee不公开x86寄存器结果，因此聚合请求为矩形、九宫格和动态渐变调用序列保留显式返回快照。七个是静态渐变callsite而不是动态执行上限；多节点遍历可消费任意数量快照，缺省快照不会越界或形成现代节点上限。九宫格资源严格由矩形返回EAX高word与动作记录低word组成；每个后续文字或释放窄边界观察前一callee的live快照，不用统一伪返回覆盖。

## 5. caller回收

主帧在消息阶段正常完成后直接调用typed文字消息协调器；子typed-stop阻断packed-row、头顶动作、对话、调试状态面板、倒计时和全部后续帧尾。旧`post_render_stage_3`枚举数值保留为`reserved_text_message_frame_slot`，生产代码零调用；新增文字绘制和节点释放服务追加到枚举尾部，既有值不平移。

## 6. 验证

定向测试覆盖空链、活动计时、bit0/bit1覆盖顺序、bit4滑入与同帧过期推进、bit5冻结、bit6纵向步长翻倍、过期链头摘除、释放顺序、未知链token停点、bit31面板壳、主帧直连与reserved槽零调用。枚举机械检查锁定旧槽值23、原尾值173以及新增尾值174/175。

定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码构建零warning；app仅有既有ALSA开发库CMake提示。

原版动态链节点、字体/文字surface、释放器、动作/矩形/九宫格/渐变framebuffer、live冻结门变化及主帧联合寄存器捕获后端尚不可用，`original_diff_verified`为`blocked_runtime_oracle`。
