# 战斗调试叠加层 `0x0045DEE0`

状态：`pending`。

## B11：角色摘要读取实际WORD

45E2F1从ActionDispatchState.current_actor_index做WORD符号扩展，
不再借overlay.battle_selector。前一文字回调改写后按原读取点显示。
45E210的优先角色DWORD与53BD54显示字段保持区分。
符号边界、回调向量与门禁状态见
[实际角色与优先角色](battle-frame-effect-00453580.md#21-b11画面效果借实际角色word及优先角色dword)。

## 1. 完整LST范围与调用图

权威函数为`0x0045DEE0..0x0045E576`，从proc到endp完整736行、460条实际指令、45个call站点、15个分支和15个局部标签，无外部FUNCTION CHUNK。

唯一caller是逐帧画面协调器`0x00453200`。45个call站点分为：

- 15次导入`wsprintfA`，现以固定255字节typed缓冲和等价十进制格式化直接实现；
- 18次文字绘制，保留字体、surface、坐标、前景色`0xFFFF`和高度`0x10`的窄端口；
- 4次字体style/reset；两处动作种类与两处启动计数查询均直接借实际WORD并返回状态及可选观察值，其余对象查询继续按现有实现处理。

尚未迁移的对象解析及surface接口仍使用`compat::u32` token；动作与启动计数查询直接借实际字段。

## 2. 总门与字体生命周期

调试叠加总门完整dword严格等于1时才执行正文。正文前固定：

1. 字体style参数写`0xFFFE`；
2. 字体reset参数写0。

正文正常结束后无条件再执行：

1. 字体reset参数写0；
2. 字体style参数写`0xFFFE`。

总门不等于1时只执行末尾两项。正常返回完整保留末次style的EAX/ECX/EDX。任何正文typed-stop或原始signed除零点都不会执行字体恢复尾。

## 3. 两组角色文字

组B循环从动态数量和固定对象基址开始，每项严格按以下顺序：

1. 解析对象并立即发布返回token；
2. 查询生命文字局部；该未初始化栈局部只在函数入口接收一次显式陈旧snapshot，callee写回后成为后续角色的陈旧值，后续callee未写时不会恢复入口snapshot；
3. 从解析token的真实`+0x54`读取等级u16；缺失typed owner在此停止；
4. 从Group-B startup lifecycle的`action_composition.action_kind`直接组合typed动作种类word查询，结果只保留低word；
5. 从同一lifecycle的`action_execution.start_gate`直接组合typed启动门word查询，caller随后只保留低word；
6. 以CP950格式`生命:%-3d 鎖定:%-1d 命令:%-1d lv:%-2d`格式化；
7. 在`X=10`、`Y=10+index*20`绘制。

组B每次文字绘制后重新读取动态数量。对象token为`0x00525508 + index*0x2B28`的低32位。

组A循环按`0x005029D0 + index*0x2F34`生成父级token，直接读取十槽`group_a_action_execution[index].action_kind`及同槽`start_gate`，然后以完整unsigned WORD执行CP950格式`鎖定:%d 命令:%-3d`，坐标为`X=520`、`Y=10+index*20`。两个查询均无寄存器请求回复、解析或计数。父级动作查询失败仍保留首行敌方数量或后续行重载队员数量，RET失败替换低WORD；EDX保留字体/上一行绘制结果。启动查询失败沿用已读动作值及EDX。每次绘制后重新读取动态数量和下一角色实际字段。

不增加现代数量夹值或循环上限。

## 4. 三行动态顺序区

组B行结束后依次绘制三组CP950标题与数值：

- `攻擊順序:`位于`X=10`、`Y=20*groupB+10`；随后按动态`groupA+groupB`读取18条启动记录的`+0x00`，X从100按20递增；
- `我方等待順序:`位于`X=10`、`Y=20*groupB+30`；随后按动态组A数量读取十项最终角色顺序，X从120按20递增；
- `排列順序:`位于`X=10`、`Y=20*groupB+50`；随后按动态两组总数读取18项选择顺序，X从100按20递增。

每次值绘制都重新读取组B数量计算Y；每轮末尾重读对应动态上界。第19条启动记录、第11项最终角色顺序和第19项选择顺序分别只在原始首次真实读取处typed-stop，保留此前全部角色文字、标题和值绘制。

## 5. 当前角色、状态文字与宽度规则

动态顺序区后先在`(240,70)`绘制CP950 `目前角色:%d SEL:%d`，参数分别复用唯一角色优先索引和叠加层选择值。

组B像素标记后再按固定顺序绘制：

- `(240,30)`：CP950 `無敵:%d UM:%d 戰鬥:%d`；
- `(240,50)`：`fMenu:%d mMove%d`；
- `(240,90)`：`MsD:%d dRole1:%d CanS:%d`；其中第一项只取低byte，后二项只取低word；
- `(240,110)`：`MS:%d Stop:%d mStop%d`；
- `(420,30)`：`A.Lockc:%d `；
- `(420,50)`：`TswMem:%dK`；缓存字节按signed除1000并向零截断；
- `(420,70)`：`wLl:%d iMn:%d`；分别按i8和i16符号扩展；
- `(450,90)`：`bf:%d`；
- `(240,10)`：`FRAME:%d`；先执行signed `1000 / divisor`。

所有普通`%d`参数均按机器码的符号扩展或完整32位位形解释，不改用无符号显示。frame divisor为0时在原始`idiv`位置typed-stop；此前`bf`文字已经格式化和绘制，FRAME缓冲写入及字体恢复均未发生。

## 6. 组B双行像素标记

每个动态组B对象先调用位置查询，把两项输出写入共享i16 X/row；未写输出保持上一对象值。随后：

1. 位置callee后读取startup共享矩形的`Rect.right`，与signed row执行低32位乘法；
2. 直接读取startup enemies对应唯一progress owner的`actor+0x2A12`低word，以startup timing唯一阈值计算`trunc(progress/threshold*62)`，只保留返回低word；
3. 宽度非零时按列递增；
4. 每列先写`row*right + X + column`，再重读共享`Rect.right`并写对应下一行；
5. 两个像素均固定写u16 `0xEEEE`。

共享矩形与逻辑画布接线见[入战矩形证据](battle-window-rectangle-runtime-binding.md)。
此前使用framebuffer裁剪几何的说明已撤销。

每个像素只在原始实际store点检查owned framebuffer。第一行成功而第二行越界时保留第一行像素；失败对象不计为完成。宽度0不访问framebuffer。对象完成后重新读取动态组B数量。

## 7. 单一typed状态与全局重置

函数直接复用：

- 角色metric的两组数量和唯一角色优先索引；
- 启动状态的18条完整`0x1C`记录，以及Group-B八槽动作种类word和启动门word；
- 最终角色的当前角色、已发布角色、十项顺序、预帧移动门与两项frame gate；
- 动作状态的packed actor counter低byte，以及Group-A十槽动作种类word和启动门word；
- 跨预帧与调试显示的唯一共享message state；
- 效果协调器的组A反馈actor；
- 调试快捷键的显示总门和无敌切换值；
- 逐帧唯一owned framebuffer。

仅新增此前未建模的18项选择顺序、255字节共享文字缓冲、战斗选择/模式、选择低word、锁计数、缓存、世界等级、初始模式、战斗帧、帧率除数及两项标记坐标；当前角色、fMenu、mMove、MsD低byte和MS不建立副本，分别读取最终角色当前值、共享message state、最终角色预帧门、动作packed actor counter和最终角色已发布值。标记行宽直接读取唯一framebuffer几何，不复制平行字段；标记宽度直接复用startup enemies进度和timing阈值，不复制角色字段或保留旧`query_marker_width`调用。叠加调用门由独立虚共享gate port持有，撤退提交、逐帧协调器和全局重置使用同一存储。全局重置按原固定写集合清叠加调用门、选择顺序、战斗选择、初始模式和战斗帧；当前角色、fMenu、mMove、MsD低byte和MS由既有owner按同一reset program同步；未写的显示总门、文字缓冲、战斗模式、选择低word、锁计数、缓存、世界等级、帧率除数与标记坐标保持入口值。

## 8. caller回收

逐帧协调器在内部bit17检查之后读取叠加门。该门归独立虚共享gate port唯一持有，撤退提交可在成功时清零，全局重置也同步清零。机器码只有门完整等于1时调用本函数；旧实现把条件反向并保留opaque调用，现已纠正为精确等于1时直接组合typed叠加层。

子typed-stop保留音乐、角色预处理、metric、角色顺序、surface绘制、HUD、对话、双倒计时和叠加正文前缀，随后阻断结果判定、上下文提示、颜色累加、临时surface与截图。两处动作种类调用现直接借实际WORD；原Group-B返回地址为`0x0045DF5F`，Group-A为`0x0045DFDA`。查询编号、寄存器协议、token解析、栈轨迹、计数、转发及reserved查询槽已删除；当前证据见[动作种类查询](battle-actor-action-kind-004786b0.md)。其后的两处启动计数查询已直接借用双方角色实际WORD，返回状态及可选读值；本查询的寄存器请求/回复、栈轨迹、调用计数、转发函数及reserved编号均删除。原返回地址为`0x0045DF6C`、`0x0045DFE7`，调用前动作种类已经mask并保存，查询成功后完整WORD零扩展用于文字。字段/返回失败保留已完成前缀与父级实际残值，抑制当前行格式化、绘制及剩余循环；上一行回调修改下一角色后必须实时读取。当前向量、门禁与限制见[启动计数查询](battle-actor-start-gate-004786d0.md)。其余调试操作与资源仍待迁移。Workpack 288 REVIEW 1把`0x0045E270`标记坐标站点改为直接读取Group-B lifecycle action-execution canonical坐标到固定`0x0053BF4A/0x0053BF4E` scratch；旧位置查询端口槽改为reserved并保持生产零调用。六类current-coordinate stop保留已完成正文及X scratch前缀，阻断进度宽度、row-offset与全部标记像素。组B进度或阈值读取stop发生在位置查询与row-offset计算之后、任何标记像素之前；旧宽度端口槽同样仅保留reserved兼容编号且生产零调用。正常返回寄存器按原caller保持未消费。

## 9. 测试与动态差分

定向测试覆盖总门关闭字体尾、两组完整文字、CP950字节、固定坐标与格式、生命局部跨角色陈旧继承、低byte/word和signed参数、完整EAX/ECX/EDX尾、解析token发布、`+0x54`读取停点、双方动作与启动WORD实际共享读取、首行/后续行父级失败值、字段/RET停止后缀、十槽Group-A边界、三类动态顺序、callee后数量增长、typed current-coordinate与进度宽度、两类标记旧端口零调用、Y读取停止后的X scratch部分提交、进度读取停点、双行标记、第二行越界前缀、signed缓存除法、frame divisor零点、全局重置别名及逐帧caller阻断。

当前缺少原版字体/文字callee、两组角色对象及查询副作用、共享255字节缓冲、TSW缓存计数、完整叠加全局、framebuffer和寄存器联合捕获后端，`original_diff_verified`为`pending`。
