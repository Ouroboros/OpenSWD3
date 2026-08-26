# 战斗群体效果记录帧协调器 `0x00458DE0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威函数为`0x00458DE0..0x004599AF`，完整1350行、71个静态call站点、80个`loc_`标签，无外部FUNCTION CHUNK。24个唯一callee。唯一caller为尚未关闭的`0x0045C010`，共有5个静态调用点。

入口读取actor token、参数对象token、source value、18槽slot index和group-wide mode。第八十二项caller审计确认主/备用物理工作区各为18个固定槽；主/备用记录与上一项共用固定152字节布局：

```text
primary:   0x005202A8 + slot * 0x98
alternate: 0x004FE600 + slot * 0x98
```

slot越界在首次主记录complete读取typed-stop，且发生在本函数把共享reward value清零之前。参数对象、角色、resource owner继续用`compat::u32` token。

## 2. 主记录建立和首次owner访问

每次有效slot入口先把共享reward value清零。主记录complete为0时：

1. 写source value、zero value和global mode等于1的snapshot；
2. 初始化主记录；完整EAX为0直接返回0，不清主记录、不清备用记录；
3. 按两个u16 key查resource owner；
4. lookup callee EDX只覆写DX为主record pan，并立即播放sample；
5. 主record pan清零；
6. 此时才首次读取`[owner]`，零owner typed-stop保留sample和pan清零；
7. 发布owner value token，再查询argument offsets。

因此本函数主资源与上一项不同：sample发生在resource owner首次解引用之前，pan高word来自lookup callee EDX，不来自owner token。

## 3. offset AND门与镜像坐标

argument offsets两个低word必须**同时非零**才查询base coordinates并做完整u32相加。任一低word为0时，X/Y都保持0，不保留另一项非零offset。

随后读取record base offset、render flags和由render高word+width adjustment低word拼成的width value，再首次直接读取参数对象mode。

参数mode为0时：

- 完整render flags bit0翻转；
- 仅当record base offset非0才改为`u16(resource width)-record base`；
- width adjustment非0时只覆写width value低word为`resource width-low16(width value)`，高word继续来自render flags。

全局flip mode为1时再次翻转render flags；仅当前base offset非0才按原record base重算；原width adjustment非0时恢复原高/低拼接值。

坐标分支：

- width value低word或record Y adjustment非0：当前X/Y任一低word非0时，X做完整u32减width value，Y只减低word；
- 两者都为0：先清X/Y；flip时先写`640-resource width`低word；base offset非0时再按参数mode选择`160-base`或`480-base`，并保留mode 0+flip只覆盖X低word为`160-original base`、Y低word写`235-base Y`的非对称路径。

最终共享X/Y只取本地低word的signed解释。

## 4. collision、绘制与主记录公共尾

查询animation mode。完整EAX等于1时查询参数对象坐标，X完整减base offset；当前X低word为0时改为完整`0-base offset`。collision固定把两个Y参数和最终slot参数都传0，只传两项signed X、mode低word和两个共享坐标token。完整EAX等于1时status OR 1且complete写1。

resource render使用共享signed X/Y、owner u16宽高、本地render flags，最后参数固定0。resource value即使为0也调用release，随后释放owner。

主资源成功尾：

- rendered-primary counter低32位加1；
- 三个record word发布到对应共享word；
- 两项suppression写1；
- 调用共享发布callee。

## 5. 备用记录的pan清零非对称

alternate active完整值等于1时，从主记录复制source/resource辅助值并初始化备用记录；失败直接返回0，不清备用记录与active。备用status非0时完整覆盖主status后清零。

资源查询后，owner完整EAX只覆写AX为备用pan并播放sample。随后机器码清零的是**主record pan**，不是备用pan；备用pan保持原值。此后才首次读取owner value并直接读取参数对象mode。

参数mode为0只翻转render flags最低byte的bit0；本函数备用路径不应用global flip。render最后参数固定0；value和owner都无条件release。备用complete完整值等于1时active清零，备用record本体此时不清。

## 6. status群体发布

status bit1即word bit`0x0002`置位时，alternate active先写1，随后整个status word清零。

重读status：

- signed负值：唯一共享战斗消息/阶段写1，battle gate清零；该dword与startup、动作和预帧路径共用typed端口；
- bit3置位且bit`0x1000`置位：先清`0x1000`。group-wide mode等于1时首次直接读取参数对象mode；mode等于1按signed组B数量逐角色调用状态发布，否则按signed组A数量；group-wide mode不等于1时只对入口actor调用一次；
- bit3置位且bit`0x0400`置位：把record七个u16按i16符号扩展后直连已关闭颜色初始化器，恢复其EAX/ECX/EDX尾寄存器，清该bit并把共享颜色初始化门写1；
- bit2即word bit`0x0004`置位：group-wide mode等于1时，group-A-special等于1选择组A，否则选择组B；非群体模式只发布入口actor。最后整个status word清零。

组A status发布在caller内先读取每个角色的两个独立guard dword，任一等于1即跳过；两者都不等于1才调用eligibility，完整EAX为0才发布actor。组B只要求eligibility完整EAX为0。

组A固定基址`0x005029D0`、步长`0x2F34`、10槽；组B固定基址`0x00525508`、步长`0x2B28`、8槽。数量越过物理槽域只在首次对应actor访问typed-stop，保留此前角色的完整前缀。

## 7. 群体A奖励路径

status bit0或bit`0x10`任一置位进入奖励。group-wide mode等于1时，以下条件选择组A：

```text
(group-A-reward-mode == 1 && reward-summary-gate == 1)
|| group-A-special-mode == 1
```

每个组A角色先检查两个caller guard，再要求reward-gate完整EAX不等于1。

- group-A-special不等于1：调用mode-one reward，输出auxiliary word和packed dword高word；
- group-A-special等于1：eligibility完整EAX不等于1才继续；status bit0置位时先发布actor，再调用普通reward，保留既有auxiliary和packed高word。

每个实际处理角色都把reward offset独立重置0。基础reward取AX signed扩展，signed大于等于9999夹到9999；仅`-1`改为0且不发布基础行。auxiliary行ID为`0x2367`，packed-high行ID为`0x2366`，每项按原顺序发布值、offset和mode 1。

auxiliary行发布后全局auxiliary word清零；packed-high行发布后packed dword高word清零。因此每角色数组最终写入的辅助与高位值都是0，total按u32回绕累加，display total取当前角色total。

若group-A-reward-mode与reward-summary-gate都等于1，再发布角色汇总。汇总的total为完整dword；辅助与高位参数虽然低word为0，但分别保留最后奖励callee ECX与EAX高word，不合理化为全零。

## 8. 群体B与单体奖励路径

未选择组A时按signed组B数量循环。eligibility完整EAX为0才处理；status bit0置位时先发布actor，随后始终调用mode-one reward。

组B的reward offset只在函数入口初始化一次，**不会按角色重置**。因此后一角色继承前一角色基础行和auxiliary行累计的offset；基础reward为`-1`时也不清此前offset。auxiliary与packed-high全局仍在各自行发布后清零，角色数组辅助/高位写0，total累加；组B不调用角色汇总。

非group-wide路径：

- status bit0置位时先发布入口actor并清battle gate；
- 直接读取参数对象mode，等于1调用mode-one reward，否则调用普通reward；
- reward offset从0开始；
- auxiliary与packed-high发布后清对应全局；
- 不写逐角色数组，只把基础reward按u32加到全局display total。

所有奖励路径结束后整个主status word清零。

## 9. 两次final gate和成功收束

final gate word按i16大于0时执行第一次gate：

- 保留当前EAX高word；
- 只以主记录第二lookup key覆盖AX；
- 把final-gate latch写1；
- 直连已关闭全角色数值步进`0x0045BD90`，参数为`(stale-EAX-with-key, primary complete)`；
- 子函数恢复入口完整ECX，返回0立即返回0，角色typed-stop阻止后续收束。

随后要求主complete完整值等于1且alternate active为0，否则返回0。

final gate word仍大于0时执行第二次gate：

- 保留第一次步进恢复的入口ECX高word；
- 只覆盖CX为同一lookup key；
- 第二参数固定1；
- 忽略普通返回值，但角色typed-stop仍阻止后续收束。

成功尾按顺序把rendered-primary counter清零、清同槽备用152字节record、active清零并返回1。主record、共享X/Y、奖励数组和suppression不清。

## 10. callee、测试与动态差分

原24个唯一直接callee中的`0x0045BD90`与`0x0045D3E0`已关闭并直连；其余22个直接callee继续通过专用typed token端口发布完整EAX/ECX/EDX与输出。全角色步进内部两个尚未关闭actor callee复用同一端口。第八十二项已关闭唯一总协调器caller的五处调用并改为直接组合，同时把本函数与单体效果函数的公共记录、渲染字段和奖励数组收敛为同一18槽虚共享状态。

定向测试覆盖：

- slot在reward清零前typed-stop；
- 主初始化失败不清备用record；
- 主owner零token在sample与主pan清零后停；
- 参数对象在owner value和offset查询后的停点；
- offset AND门、mode 0坐标非对称、EDX sample高word、无条件双release；
- collision固定双Y和slot零；
- 备用sample后清主pan但保留备用pan；
- 备用AL翻转、双release、complete与成功尾清record；
- group-wide状态按参数mode选择组B及七项i16颜色初始化、共享门与尾寄存器；
- 组A双guard状态发布；
- 组A奖励数组清零与汇总陈旧ECX/EAX高word；
- 组B`-1`基础reward下跨角色累计offset；
- 单体普通reward和旧辅助/高位行消费；
- 第11个组A角色保留前10角色前缀后typed-stop；
- 两次全角色步进的EAX/恢复ECX来源、第一次0返回早退、第二次普通返回忽略与角色typed-stop传播。

当前缺少原版双记录、18角色对象、22类剩余直接callee与效果步进内部两类actor callee共享副作用、resource owner、参数对象、sample、eligibility/reward表、奖励行对象和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
