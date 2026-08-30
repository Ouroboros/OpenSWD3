# 战斗动作四与四百零四复合效果 `0x004745B0`

状态：`platform_adapted`。完整LST、typed实现、双caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x004745B0..0x00474B52`，proc至endp共582行、343条实际指令、12个call、32个跳转、22个局部标签、3个返回点，没有外部`FUNCTION CHUNK`。两个caller均位于动作dispatch：普通动作4分支直接调用；特殊动作404在阶段低十五位等于二时调用。二者都以选中group-A行动者为this、选中group-B目标token为唯一栈参数，并只在本函数返回一后继续发布目标、效果分数、画面提交与后续清理。

入口按word检查起始门，再按完整dword检查执行完成值是否精确为一；任一命中均无副作用返回零。通过后置完成latch，主记录external mode先清零，field5A bit9存在时再置一；随后以profile word加1500和特殊variant配置主记录，并通过带记录、frame token、flags与坐标引用的窄port调用待审主更新。

主记录field5A bit1先处理可选转身记录。field24非零时发布runtime gate bit14，把field24与field28复制到`+0x468`转身记录action ID和base variant；field5A bit9再次置主记录external mode。随后只清bit1，破坏性清field24与field28，保留其余field5A位。gate bit14存在时通过携带转身记录引用的窄port调用待审逐帧更新；严格只在返回一时整word清主field5A、清external mode并清gate bit14。

主field5A bit15发布共享negative flag一并清negative reset，但不消费该位。bit2路径先整word清field5A，再发布gate bit15并以目标为this调用待审刷新。bit3路径无论bit10是否存在都会消费bit3、发布bit15、清motion word、整word清field5A并清完整效果记录；bit10存在时先把唯一颜色初始化gate置一，并将field7A、7C、7E、80、82、84、86按signed word直连已关闭七参数颜色初始化，再仅清bit10。因为bit3最终整word清零，同word bit0不会继续执行。独立bit0路径发布bit15、清motion与field5A，调用待审目标事件并清完整效果记录。

函数每帧无条件把主记录field64、field66、field68作为三项word快照直连已关闭画面刷新函数；刷新内部继续持有唯一surface、snapshot与pending owner。只有runtime gate bit15置位才进入效果段。

`+0x630`效果记录action ID默认取行动者runtime word、base variant写零；主记录field24非零时以field24与field28覆盖。行动者`+0xD9C`低bit0为零时，通过带记录、frame、flags与坐标引用的窄port调用待审效果更新，参数保留主记录field76/78。该bit为一时走另一待审效果callee：X按`positionX-targetXOffset+sourceXOffset`、Y按`positionY+auxiliaryWord-primaryRecord.drawY`计算，另传signed sourceY；返回非一立即返回零。返回一后按原顺序给效果field5A或bit0，置主动作记录与效果记录完成位并清motion。

效果field5A bit15发布共享negative状态；bit2以目标为this调用待审刷新后整word清field5A；bit0先清motion和共享motion word，再整word清field5A并调用待审目标事件。随后在原frame解引用点查询效果frame并发布共享frame token。效果未完成时，仅在gate bit14未置时按signed draw X/Y、当前flags、frame尺寸及资源value04绘制；bit14抑制该层。

效果完成且gate bit14未置时，先清主记录与目标记录field24。直接效果模式下，motion word按signed与负三十二比较：大于负三十二时发布三份共享motion，以效果frame绘制并word减八；特殊模式精确为一时又加八，保留原净零进度。该路径当帧返回零。小于等于负三十二时motion清零。最终仅当主记录完成位精确为一且motion word为零时收尾。

收尾按原顺序整记录清主记录、目标记录、转身记录和效果记录，不清附效果记录；清`+0xFCC`起连续`0x4C0`字节工作区，四个目标索引写全一并清motion。进度owner只在原`+0x2AB0`写点检查，缺失时保留此前清理前缀。随后清进度、runtime gate和尾word，completion byte按八位回绕加一，清共享profile mode并返回一。

实现复用第226项主、目标、转身、效果记录与延迟连续工作区唯一owner，只新增`+0xD9C`直接效果模式字段。颜色初始化和画面刷新两个已关闭callee直接typed调用；主更新、转身更新、目标刷新、目标事件、两种效果更新与绘制保留窄port。普通动作4和特殊动作404两个production caller均删除整函数opaque调用。

测试覆盖双入口门、field24/28转身记录转移、bit9 external mode、bit14完成门、bit10 signed颜色初始化、bit3优先级、每帧画面刷新、普通效果fallback绘制、直接效果坐标与返回门、效果bit0事件、负八淡出、bit14绘制抑制、四记录与连续工作区清理、completion byte回绕、frame原访问点typed-stop、普通动作4与特殊动作404双caller及旧地址零调用。新增测试最初在普通和AddressSanitizer运行中分别暴露累计测试栈溢出；将大型行动者、Fixture、port与dispatch结果移出函数栈后，定向测试与独立AddressSanitizer均通过且findings为零。Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`227/422 = 218 platform_adapted + 9 assembly_exact + 195 pending_audit`，SHA256为`796f78d208b0f774040fa836b55a8d431d7f24eee72e290a6f406e2d5e5bbb05`。动态差分因原版四记录、frame、颜色、画面刷新、绘制、目标事件、两类待审效果callee与双caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
