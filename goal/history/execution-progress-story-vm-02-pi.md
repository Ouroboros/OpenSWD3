# 执行历史：剧情VM第二段

状态：冻结历史；不得作为当前执行状态或行为真值。

来源：重构前`execution-plan-pi.md` v853第1901..2450行，剧情VM P2中段。

完整性与当前资料入口见[`../execution-history-index-pi.md`](../execution-history-index-pi.md)。当前状态见[`../execution-state-pi.md`](../execution-state-pi.md)。

---

    对外进度为已实现90/198、已验收70/198；内部workpack当前47/146，即
    `5 assembly_exact + 42 platform_adapted + 99 pending_audit`。

- 剧情VM P2第四十八组`0x00429AD2` / opcode64完成独立闭环。LST只以`rep stosd`把
    `word_4ACE70`起32个dword/64个word/128字节全部写成`CFCF`，不修改opcode63建立的interval、
    remaining、cursor或left/top快照；随后走共享`+2`尾、发布previous并同调用继续。四raw alias、
    64个不同初值、完整状态保留、table owner缺失和`0x7FFE`精确尾均通过。资产锁定8条物理记录/
    8 probes，TALK1/2/3分布`3/1/4`，全部raw`0x0040`、长度2；TALK1真实记录回放通过。剧情VM定向
    3/3、Linux core 186/186、Linux app 192/192均exit 0，SDL主程序完成链接；未启动游戏EXE。
    生成器`py_compile`及双重生成幂等通过，workpack hash为
    `aa7d60e832dad3f2a21c1a755d3ad3acb12d7166b42cc9d431bc04f1b492da78`。Windows依v290留到P3。
    对外进度为已实现91/198、已验收71/198；内部workpack当前48/146，即
    `5 assembly_exact + 43 platform_adapted + 98 pending_audit`。

- 剧情VM P2第四十九组`0x00429AE8` / opcode65完成独立闭环。LST原样读取selector、不替换
    `FFF0`；lookup命中才调用已审计`sub_40D610`，按Path/活动对象条件完成地表与空间对齐、MAPS
    flags `0x80` patch和对象槽清空，再写role index、清新party槽、清Talk、清flag`0x4000`并置
    `0x80`；缺失角色静默消费。两路均走共享`+4`尾、发布previous并yield。SDL现按新party槽后count
    的顺序同步post与live frame状态；REVIEW发现并修正了只写加载期post副本的问题。四raw alias、
    raw`FFF0`、`FFFE`受控角色、path0、aligned活动对象、nullable MAPS失败点、live owner缺失、满8槽、
    selector截断与`0x7FFC`精确尾均通过。资产锁定109条物理记录/110 probes，TALK1/2/3/4分布
    `52/1/21/35`，全部raw`0x0041`、长度4；TALK1 GUID3真实记录回放通过。共享helper+剧情VM定向
    4/4、Linux core 186/186、Linux app 192/192均exit 0，SDL主程序完成链接；未启动游戏EXE。
    生成器`py_compile`及双重生成幂等通过，workpack hash为
    `15a4fadb7b3a9d9948394feda09b143e2e659d0f58de4248662d94afe6060c66`。Windows依v291留到P3。
    对外进度为已实现92/198、已验收72/198；内部workpack当前49/146，即
    `5 assembly_exact + 44 platform_adapted + 97 pending_audit`。

- 剧情VM P2第五十组`0x00429B14` / opcode66完成独立闭环。LST把selector、Path、Talk、action、
    base、variant、flags七个u16零扩展后调用`sub_40D790`并无条件忽略返回：缺失运行角色只清同GUID
    MAPS flags bit7；命中角色则扫描八物理party槽，按条件清surface/整格对齐/空间摘链，写运行角色七
    字段、更新MAPS Talk/Path/flags、标记surface，左移party indices与对象槽并减count。现代仅对真正
    提前的owner/count/direction/surface失败typed-stop；party未命中diagnostic、missing-source MAPS
    diagnostic，以及已完成party移除的MAPS/空间diagnostic仍消费。post/live对象槽与count同步完成。
    四raw alias、raw`FFF0`、独立`FFFE`、missing fallback、完整命中、party未命中、MAPS/空间诊断、
    方向失败、live count缺失、MAPS runtime缺失、截断与`0x7FF0`精确尾均通过。资产锁定100条物理
    记录/100 probes，TALK1/2/3/4分布`48/0/22/30`，全部raw`0x0042`、长度16；TALK1 selector9
    真实记录回放通过。共享helper普通/真实+剧情VM定向5/5、Linux core 186/186、Linux app 192/192
    均exit 0，SDL主程序完成链接；未启动游戏EXE。生成器`py_compile`及双重生成幂等通过，workpack
    hash为`604d5833972e7e0e03b8246beba44ab5dd856e65ef81c2b1f89f269a61c32d7b`。Windows依v292留到P3。
    对外进度为已实现93/198、已验收73/198；内部workpack当前50/146，即
    `5 assembly_exact + 45 platform_adapted + 96 pending_audit`。

- 剧情VM P2第五十一组`0x00429B62` / opcode67完成独立闭环。LST以operand bit15保存两阶段：
    初次执行保存duration与accepted-frame clock并自修改置bit15；后续以u32回绕`current-start`比较，
    `elapsed<=duration`原地等待，只有严格`>`才清bit15、推进4并同调用继续。初始化、等待、完成三路
    均发布previous67；原C++算术和自修改正确但三路漏发previous，已修正并更新opcode16/62/63/64
    同调用进入67的最终previous断言。四raw alias、严格等于/加一、跨`0xFFFFFFFF`回绕、截断、阶段一
    与完成`0x7FFC`精确尾均通过。资产锁定1118条物理记录/1130 probes，TALK1/2/3/4分布
    `460/232/273/153`，全部raw`0x0043`、长度4、源phase bit均清；TALK1 duration2000三阶段真实回放
    通过。剧情VM定向3/3、Linux core 186/186、Linux app 192/192均exit 0，SDL主程序完成链接；
    未启动游戏EXE。生成器`py_compile`及双重生成幂等通过，workpack hash为
    `5b333e74a3645930dfd01a43343a8e8fb36796f2bed586f5ccb32d6c4ea5efeb`。Windows依v293留到P3。
    现代显式opcode仍为93；对外进度为已实现93/198、已验收74/198；内部workpack当前51/146，即
    `6 assembly_exact + 45 platform_adapted + 95 pending_audit`。

- 剧情VM P2第五十二组`0x00429BB5` / opcode68完成独立闭环。LST先把`FFF0`替换为Talk source
    GUID；lookup命中时对完整flags dword只清`0x400`，缺失时以替换后的GUID调用`sub_40D460`，其余
    字段/map全`FFFF`、flags OR0/AND`FBFF`。两路推进4、发布previous并yield，MAPS GUID缺失只诊断。
    四raw alias、完整32位flags、`FFF0`命中/缺失、独立`FFFE`、完整11参数fallback、selector截断与
    `0x7FFC`精确尾均通过。资产锁定78条物理记录/78 probes，TALK1/2/3/4分布`24/6/7/41`，全部
    raw`0x0044`、长度4；TALK1 GUID1真实记录回放通过。剧情VM定向3/3、Linux core 186/186、Linux
    app 192/192均exit 0，SDL主程序完成链接；未启动游戏EXE。生成器`py_compile`及双重生成幂等通过，
    workpack hash为`0919e57af0227eacd460f4782f77cf87b3f545f6474c957832db9f4f4bf72fcf`。Windows依v294留到P3。
    对外进度为已实现94/198、已验收75/198；内部workpack当前52/146，即
    `6 assembly_exact + 46 platform_adapted + 94 pending_audit`。

- 剧情VM P2第五十三组`0x00429C37` / opcode69完成独立闭环。LST先把`FFF0`替换为Talk source
    GUID；lookup命中时对完整flags dword只置`0x400`，缺失时以替换后的GUID调用`sub_40D460`，其余
    字段/map全`FFFF`、flags OR`0400`/AND`FFFF`。两路推进4、发布previous并yield，MAPS GUID缺失
    只诊断。四raw alias、完整32位flags、`FFF0`命中/缺失、独立`FFFE`、完整11参数fallback、selector
    截断与`0x7FFC`精确尾均通过。资产锁定103条物理记录/103 probes，TALK1/2/3/4分布`22/3/7/71`，
    全部raw`0x0045`、长度4；TALK1 GUID1真实记录回放通过。剧情VM定向3/3、Linux core 186/186、
    Linux app 192/192均exit 0，SDL主程序完成链接；未启动游戏EXE。生成器`py_compile`及双重生成幂等
    通过，workpack hash为`4d51e570092a5887c5b5b3b3a7c2e998fcc2a663587e4de5c15f2d8cb011d933`。Windows依v295留到P3。
    对外进度为已实现95/198、已验收76/198；内部workpack当前53/146，即
    `6 assembly_exact + 47 platform_adapted + 93 pending_audit`。

- 剧情VM P2第五十四组`0x00429CBC` / opcode71完成独立闭环。LST原样lookup selector，命中后才读取
    slot并写`0x004B9F68 + slot*0x98`；缺失不读slot但仍推进6。两路发布previous71并yield。原C++
    错误整条预验、same-call且漏previous，已修正并更新story100全部71真实帧边界。四raw alias、
    slot`0/1/3/FFFF`、literal`FFF0` missing无slot、命中截断、`FFFE`和精确尾均通过。资产331条/
    338 probes，分布`127/22/138/44`，全部raw`0x0047`、长度6；真实GUID191/slot0回放通过。
    剧情VM3/3、Linux core186/186、Linux app192/192通过；workpack hash为
    `db469bd69fc5b9a1b67f923aa00658ad32e1756985ecc33518be8f50f42c8ad0`。未启动游戏EXE。
    现代显式opcode仍95；对外进度为已实现95/198、已验收77/198；内部workpack54/146，即
    `6 assembly_exact + 48 platform_adapted + 92 pending_audit`。

- 剧情VM P2第五十五组`0x00429D0F` / opcode72完成独立闭环。LST按raw selector lookup，命中清
    `field_3c`、缺失不写；两路推进4、发布previous72并yield。原C++错误same-call且漏previous，
    已修正并更新story100全部72真实帧边界。四raw alias、literal`FFF0` missing、`FFFE`、selector
    截断和精确尾均通过。资产329条/336 probes，分布`126/21/138/44`，全部raw`0x0048`、长度4；
    真实GUID191回放通过。剧情VM3/3、Linux core186/186、Linux app192/192通过；workpack hash为
    `4d2db303269f4b1d5b1ef8fbc58f44d85d7880ce5ab07a93eccce87e453f4892`。未启动游戏EXE。
    现代显式opcode仍95；对外进度为已实现95/198、已验收78/198；内部workpack55/146，即
    `7 assembly_exact + 48 platform_adapted + 91 pending_audit`。

- 剧情VM P2第五十六组`0x00429D43` / opcode74完成独立闭环。LST按red/green/blue step、
    signed countdown顺序清零，保留current/target；推进2、发布previous74并same-call继续。原C++
    四字段与顺序正确但漏previous，已修正。四raw alias精确尾、空owner与ordinary same-call均通过。
    资产13条/13 probes，分布`1/2/1/9`，全部raw`0x004A`、长度2；TALK1真实精确尾回放通过。
    剧情VM3/3、Linux core186/186、Linux app192/192通过；workpack hash为
    `85783b97cea56774abda76cf623031ed90fd75f785b8d0c273dd3c265f9d202f`。未启动游戏EXE。
    现代显式opcode仍95；对外进度为已实现95/198、已验收79/198；内部workpack56/146，即
    `7 assembly_exact + 49 platform_adapted + 90 pending_audit`。

- 剧情VM P2第五十七组`0x00429D70` / opcode75完成独立闭环。LST以raw selector lookup后无条件
    调`sub_42E5A0`；合法域协调角色sub-cell/槽/surface/空间链并置flags bit31，推进4、发布
    previous75并same-call继续。现代复用`suspend_legacy_world_story_role`；原版missing index -1越界
    与固定全局owner收敛为typed失败。四raw alias literal`FFF0`、`FFFE`精确尾、空owner和截断均通过。
    资产82条/82 probes，分布`19/43/18/2`，全部raw`0x004B`、长度4；TALK1 GUID181回放通过。
    剧情VM3/3、Linux core186/186、Linux app192/192通过；workpack hash为
    `8bfb0eb9afe63ec3c66a35f2f08b94a3b55ab826c4afbfa2688c8f336423c23f`。未启动游戏EXE。
    现代显式opcode增至96；对外进度为已实现96/198、已验收80/198；内部workpack57/146，即
    `7 assembly_exact + 50 platform_adapted + 89 pending_audit`。

- 剧情VM P2第五十八组`0x00429DA6` / opcode76完成独立闭环。LST分阶段读取双selector，仅第一
    参数替换`FFF0`；以wrapping中心点计算朝向，先更新/刷新第一角色action，再调`sub_42E5A0`
    挂起，成功+6、发布previous76并same-call继续。原C++整条预验、owner检查过早且漏previous，
    已修正；两处index -1越界收敛为typed失败。四raw alias第一missing、第二截断、第二literal
    `FFF0`、owner延后、`0xC04C`精确尾均通过。资产449条/450 probes，分布`181/140/56/72`，
    全部raw`0x004C`、长度6；TALK1 GUID191朝向GUID1回放通过。剧情VM3/3、Linux core186/186、
    Linux app192/192通过；workpack hash为`e39ece177a57aa87908d2972ce677c031f75818ab4591d3b2907e1d0d20ef082`。
    未启动游戏EXE。现代显式opcode仍96；对外进度为已实现96/198、已验收81/198；内部workpack
    58/146，即`7 assembly_exact + 51 platform_adapted + 88 pending_audit`。

- 剧情VM P2第五十九组`0x00429F7B` / shared opcodes77/78完成独立闭环。两者先解析支持
    `FFF0`的selector；命中后77才读取payload并写`value|0x8000`、固定长6，78写零、固定长4；
    两者清wait_remaining、刷新、发布previous并same-call继续。原C++整条预验且漏previous，已修正；
    missing原版使用陈旧`var_40`推进，现代typed-stop。两opcode×四raw alias missing、77载荷截断、
    77/78精确尾均通过。77资产442条/447 probes、分布`137/109/113/83`；78资产4条/4 probes、
    分布`1/3/0/0`；全部长度及raw核验通过，TALK1两条回放通过。剧情VM3/3、Linux core186/186、
    Linux app192/192通过；workpack hash为`ce5a41ffc05e907c2c90ba1cd99f969082a5d1d94541255f4db6aa684873b97f`。
    未启动游戏EXE。现代显式opcode仍96；对外进度为已实现96/198、已验收83/198；内部workpack
    59/146，即`7 assembly_exact + 52 platform_adapted + 87 pending_audit`。

- 剧情VM P2第六十组`0x0042A0A6` / opcode79完成独立闭环。handler先分配/清零/初始化0xB4
    节点，再分阶段读取action/variant、四坐标与signed movement；坐标word内左移4，平方和按i32
    wrapping，x87精度计算速度，最后前插并+16、发布previous79、same-call继续。现代接入已有
    `world_moving_actions_`生命周期，以临时list/splice保持资源顺序。四raw alias精确尾、ordinary
    same-call、七截断点、空owner、零距离NaN、负movement、平方溢出NaN均通过。TALK线性目录
    为0条/0 probes，使用`asset_absence_verified`，不把2个候选CFG节点冒充资产。剧情VM3/3、
    Linux core186/186、Linux app192/192通过；workpack hash为
    `1c60762974a4d613c6d9531b320b01a8119f629fd922504f668345df88f9645a`。未启动游戏EXE。
    现代显式opcode增至97；对外进度为已实现97/198、已验收84/198；内部workpack60/146，即
    `7 assembly_exact + 53 platform_adapted + 86 pending_audit`。

- 剧情VM P2第六十一组`0x0042A1EF` / opcode80完成独立闭环。LST确认读取32位
    `dword_4A1360`、以`0xDFFFFFFF`只清bit29，经共享尾写回、+2、发布previous80并same-call。
    现代直接映射`text_control_flags`；四raw alias精确尾、ordinary continuation与
    `TALK1.DAT@0x00004520`真实记录通过。资产锁2256条/2256 probes，四文件分布
    `609/453/507/687`，全部raw `0x0050`、长度2。剧情VM3/3、Linux core186/186、Linux app
    192/192通过；workpack hash为`048a4c63426f262db518938f644a7d8fccd1fb3848f0bd58d6223d6b9a42c59b`。
    未启动游戏EXE。现代显式opcode增至98；对外进度为已实现98/198、已验收85/198；内部
    workpack61/146，即`8 assembly_exact + 53 platform_adapted + 85 pending_audit`。

- 剧情VM P2第六十二组`0x0042A200` / opcode81完成独立闭环。handler先分配/清零/初始化
    0xB4节点，再分阶段读取action/variant、raw signed target X与encoded Y；默认X按signed target
    `>320`选择760或-120，Y只取low15，bit15则覆盖为current X=target、motion=0x8000。现代用
    临时list/splice前插既有`world_role_head_actions_`生命周期。四raw alias精确尾、signed X
    320/321/-1、四截断点、空owner与ordinary same-call通过；真实回放覆盖TALK1普通右侧和
    TALK2稀有bit15路径。资产锁1888条/1888 probes，分布`488/340/398/662`，全部raw `0x0051`、
    长度10；3条bit15特殊记录均在TALK2。剧情VM3/3、Linux core186/186、Linux app192/192通过；
    workpack hash为`2cdeac1106718dca980d4612830791bf76e4d8a3788cf49eae01117bccd5ef0b`。
    未启动游戏EXE。现代显式opcode增至99；对外进度为已实现99/198、已验收86/198；内部
    workpack62/146，即`8 assembly_exact + 54 platform_adapted + 84 pending_audit`。

- 剧情VM P2第六十三组`0x0042A2C6` / opcode82完成独立闭环。handler按action ID/base variant
    首匹配头像节点；motion bit15置位写10000，否则signed current X<=320写-1、>320写+1。
    空链在不读任何operand时仍+6；非空先读ID，只有首次ID命中才读variant；ID全miss不读variant
    仍静默+6。现代仅在固定全局head访问点增加typed owner失败。四raw alias精确尾、重复key首节点、
    bit15、signed X、ordinary same-call、空链unsafe advance、ID miss、variant截断、owner缺失和
    `TALK1.DAT@0x0000614D`真实回放通过。资产锁1889条/1889 probes，分布`489/340/398/662`，
    全部raw `0x0052`、长度6。剧情VM3/3、Linux core186/186、Linux app192/192通过；workpack
    hash为`d072856ecfbeeb2c53f82b57bf044017d9b986a2640f37dcec3a2ae63ed59f61`。未启动游戏EXE。
    现代显式opcode增至100；对外进度为已实现100/198、已验收87/198；内部workpack63/146，
    即`8 assembly_exact + 55 platform_adapted + 83 pending_audit`。

- 剧情VM P2第六十四组`0x0042A341` / opcode83完成独立闭环。合法ID先删除全部同低字节
    packed-row节点，再分配节点并按真实顺序读取color、X/Y/width/height，数组分配后才读取mode；
    四字段清bit0。矩形门只要求X/Y非负与右/下界，不增加正width/height条件。mode1/2/其他分别
    建立`0x4000/0x0800/0x8000`，行offset为width-2/0/0、length固定2。现代用临时list/vector
    owner收敛unchecked分配与negative-height巨大分配。四alias精确尾、全部同ID删除、三mode、七截断
    副作用、invalid-ID早消费、四矩形失败、零/负尺寸、owner缺失和TALK1真实110行mode1回放通过。
    资产锁1879条/1879 probes，分布`485/337/396/661`，全部raw `0x0053`、长度16；mode1/0/11
    为`935/938/6`。剧情VM3/3、Linux core186/186、Linux app192/192通过；workpack hash为
    `e5d0b1620194094c4c1d27174fbdb735caa86decebc519095a9f113cfea6788c`。未启动游戏EXE。
    现代显式opcode增至101；对外进度为已实现101/198、已验收88/198；内部workpack64/146，
    即`8 assembly_exact + 56 platform_adapted + 82 pending_audit`。

- 剧情VM P2第六十五组`0x0042A54C` / opcode84完成独立闭环。handler先读ID；invalid ID、
    空链和ID miss都不读operation并静默+6。首匹配后operation0/1丢弃旧高mode并替换为
    `0x2000/0x1000`，operation2释放首节点。其他operation原版把陈旧`var_44` OR入ID；现代不
    伪造局部值，返回typed failure且不改节点/IP/previous。四alias精确尾、首匹配、op0/1/2、
    same-call、所有staged缺失、owner缺失与真实op0/op1/op3回放通过。资产锁1879条/1879 probes，
    分布`485/337/396/661`，全部raw `0x0054`、长度6；operation0/1/3/8为`910/963/3/3`，
    6条真实3/8记录均锁定typed-stop。格式化后剧情VM3/3、Linux core186/186、Linux app192/192
    通过；workpack hash为`cf816274d245e393f011df1b268e2e1135580a050c323d8bbb07a84436c9e20c`。
    未启动游戏EXE。现代显式opcode增至102；对外进度为已实现102/198、已验收89/198；内部
    workpack65/146，即`8 assembly_exact + 57 platform_adapted + 81 pending_audit`。

- 剧情VM P2第六十六组`0x0042A611` / opcode85完成独立闭环。handler固定执行clear framebuffer、
    primary present、AIL service，再进入CD preflight与`%Q`文件名解析。CD checker拒绝不推进IP，
    但common join仍发布previous85/yield；成功路径消费到terminator后提交raw filename并让出。
    本轮修正旧C++把terminator恰好结束在`0x8000`误判为失败，并补齐audio service、preflight、
    previous publication与side-effect顺序。SDL以配置data root和typed video backend替代CD、固定
    `video\\swd3\\`路径与Bink裸owner。四alias精确尾、preflight拒绝、缺terminator，以及真实
    `OPENING.bik`/`Demo.mpg`精确尾回放通过。资产锁11条/11 probes，分布`6/2/1/2`，全部raw
    `0x0055`和`%Q`终止；另锁定1条177-byte opaque记录。格式化后剧情VM3/3、Linux core186/186、
    Linux app192/192通过；workpack hash为
    `39d6ae542f4d9dc454cf979d245fdf5a5b7b61a563c4d12b5cea7990cf932b11`。未启动游戏EXE。
    对外进度为已实现102/198、已验收90/198；内部workpack66/146，即
    `8 assembly_exact + 58 platform_adapted + 80 pending_audit`。

- 剧情VM P2第六十七组`0x0042A673` / opcode86完成独立闭环。handler先访问0xB4头像动作
    链head；空链不读operand。非空链按完整32位action ID/variant与脚本零扩展u16比较，仅改写
    首个exact match。operand按old ID、old variant、new ID、new variant分阶段读取；new ID先写，
    new variant缺失时保留部分写。空链/ID miss仍+10、发布previous86并same-call继续。typed list
    owner只替代固定裸全局和parallel sentinel。四alias精确尾、首匹配、new ID变化、same-call、
    32位key比较、空链、ID miss、三阶段截断、owner缺失与两条真实记录通过。资产锁34条/34 probes，
    分布`4/22/5/3`，全部raw `0x0056`、长度10；真实`10002/18→10002/24`与
    `10001/22→10001/54`精确尾回放通过。格式化后剧情VM3/3、Linux core186/186、Linux app192/192
    通过；workpack hash为`04083a23810039d65ce5ca3457a43d5e684ecc97147d01ca6bcfad2bb6f80262`。
    未启动游戏EXE。现代显式opcode增至103；对外进度为已实现103/198、已验收91/198；内部
    workpack67/146，即`8 assembly_exact + 59 platform_adapted + 79 pending_audit`。

- 剧情VM P2第六十八组`0x0042A6CB` / opcode87完成独立闭环。handler从+2扫描unaligned u32
    目标到`FF00FF00`，调用secondary RNG按`(FFFF/count)*count`阈值执行每轮two-raw拒绝采样，
    再由同文件窗口helper执行audio、target data offset、IP0和0x8000窗口替换，发布previous87并
    same-call继续。空表原版在RNG状态访问前unsigned DIV0，现代以明确typed-stop隔离且不伪造目标；
    缺sentinel、RNG owner和I/O失败均按原副作用阶段收敛。四alias精确尾、固定seed index1、空表、
    缺sentinel、owner缺失、load失败与两条真实表通过。资产锁5条/5 probes，分布`3/1/0/1`，
    四条3-target长18、一条6-target长30；真实TALK1选`0x28995`、TALK4选`0x2FEB3`。格式化后
    剧情VM3/3、Linux core186/186、Linux app192/192通过；workpack hash为
    `597b0f03e016dd5b2988fa2f766ac9700a2fc7a41ad8d4c6b036731fd6a1f7d5`。未启动游戏EXE。
    现代显式opcode增至104；对外进度为已实现104/198、已验收92/198；内部workpack68/146，
    即`8 assembly_exact + 60 platform_adapted + 78 pending_audit`。

- 剧情VM P2第六十九组`0x0042A727` / opcode88完成独立闭环。handler严格先调用
    `sub_40F500`清packed-row效果链，再调用`sub_40F570`清头像动作链，之后才读取signed i16
    battle ID并提交`sign_extend(id)|80000000`请求；移动动作链保持。修正旧C++在释放前执行
    whole-record/三owner合并预检及漏发previous88；typed owner/operand失败按原访问点保留此前
    已完成释放。四alias精确尾、四个signed边界、分阶段owner、释放后截断、移动链保留与两条
    真实记录通过。资产锁52条/52 probes，分布`20/7/8/17`，全部raw`0x0058`、长度4；真实
    battle98/290回放分别提交`0x80000062`/`0x80000122`。格式化后剧情VM3/3、Linux core186/186、
    Linux app192/192通过；workpack hash为
    `66243f68234f2345294c1280a92a006567187f2f4b562d4e869eaf738faed886`。未启动游戏EXE。
    现代显式opcode保持104；对外进度为已实现104/198、已验收93/198；内部workpack69/146，
    即`8 assembly_exact + 61 platform_adapted + 77 pending_audit`。

- 剧情VM大阶段Windows门禁规则完成纠偏：P0/P1/P2/P3各自完成时必须独立执行Windows LLVM
    app完整门，前一阶段结果不得替代。P1边界提交`a24145a`已在隔离worktree补跑192/192并exit0。
    当前HEAD首次补门暴露Story VM测试二进制`0xC00000FD`栈溢出，以及resource DB持有TALK句柄时
    二次ifstream受Windows共享模式拒绝；测试Fixture改为单一heap storage，real opcode21/22记录改为
    初始化DB前预读。修复后Linux/Windows Story VM均3/3，当前HEAD Windows LLVM app192/192并
    exit0；历史与当前均未启动游戏EXE。该结果不替代P2完成时必须重跑的独立Windows门。

- 剧情VM P2第七十组`0x0042B287` / shared opcodes91/162完成独立闭环。opcode91读取u16
    record index并把`FFF0`替换为context source GUID；opcode162只接受变量11/12并读取完整u32
    动态index，非法selector或zero仅固定消费。共享路径按MAPS `+20`相对目录以u32回绕计算entry，
    复制32 bytes、把首个`%Q`的percent改零，再按`sub_40BAA0`固定buffer规则替换两个默认姓名前缀；
    合法replacement保留NUL后尾bytes，短replacement跨相邻全局读取以bounded fallback适配。目录/记录
    越界与缺terminator按原访问阶段typed-stop，缺terminator保留已完成copy。四alias、FFF0且index0合法、
    variable11/12、invalid/zero、u32回绕、staged失败、精确尾与real MAPS对照均通过。资产锁1184条/
    1203 probes：opcode91为1180/1199，分布`388/239/295/258`且有33条FFF0；opcode162为4/4，
    分布`3/1/0/0`且均selector11。真实TALK1显式index782和variable11→782得到相同姓名。格式化后
    剧情VM3/3、Linux core186/186、Linux app192/192均exit0。workpack双生成hash为`b610350b0e756879545f7502b26f9bcd28426b4a5967107e1e19c12018dabe36`。
    未启动游戏EXE；Windows依阶段规则留到P2完成门。现代显式opcode增至105；对外进度为已实现
    105/198、已验收95/198；内部workpack70/146，即`8 assembly_exact + 62 platform_adapted +
    76 pending_audit`。

- 剧情VM P2第七十一组`0x0042A756` / opcode92完成独立闭环。handler将+2 u16零扩展后按
    u32执行`dec`与`+30`，selector1..4映射bit30..33，selector0回绕到bit29；invalid诊断只经
    `nullsub_1`，所有仍在owner内的invalid selector继续写。`sub_40DC80`按`bit&7`取mask并对
    bit index执行signed `sar 3`定位byte；现代0x400-byte owner保留selector0及1..8162，其中
    8162写bit8191，8163起在原始裸写点以`global_bit_index_out_of_range` typed-stop，且不能经
    u16 helper截断。四alias、0/1/4/5、8162/8163/FFFF、operand截断、精确尾与same-call均通过。
    线性资产0条/0 probes；四raw全文件102处双字节候选均非指令入口，使用asset-absence证据。
    格式化后剧情VM3/3、Linux core186/186、Linux app192/192均exit0。workpack双生成hash为
    `1473b4842fceb8186fff291ad13ad6f725e6c4e850989a58647d33d5342193eb`。未启动游戏EXE；
    Windows依阶段规则留到P2完成门。现代显式opcode增至106；对外进度为已实现106/198、已验收
    96/198；内部workpack71/146，即`8 assembly_exact + 63 platform_adapted + 75 pending_audit`。

- 剧情VM P2第七十二组`0x0042A792` / opcode93完成独立闭环。handler独立恢复+2 u16
    零扩展后的u32 `dec/+30`，selector1..4清bit30..33，selector0回绕清bit29；invalid诊断仍只经
    `nullsub_1`且owner内selector继续。`sub_40DCB0`按`bit&7`取单bit mask、signed `sar 3`
    定位byte，并以`FF-mask` AND只清目标bit。0x400-byte owner最后安全selector8162；8163起在
    原始read/write点typed-stop，不改flags、IP或previous。四alias、0/1/4/5、8162/8163/FFFF、
    operand截断、精确尾、单bit保持与same-call均独立覆盖。线性资产0条/0 probes；四raw全文件
    41处双字节候选均非指令入口，使用asset-absence证据。格式化后剧情VM3/3、Linux core186/186、
    Linux app192/192均exit0。workpack双生成hash为
    `891b193d222ff828007273f9b6b3a2787f5f2e308b96f9db26a60930ccc094c4`。未启动游戏EXE；
    Windows依阶段规则留到P2完成门。现代显式opcode增至107；对外进度为已实现107/198、已验收
    97/198；内部workpack72/146，即`8 assembly_exact + 64 platform_adapted + 74 pending_audit`。

- 剧情VM P2第七十三组`0x0042A7CE` / opcode94完成独立闭环。机器无operand：读取完整
    `dword_4C9A18`、OR bit1、IP+2后以ESI0进入common join，发布previous94并yield。修复旧C++
    combined 94/95 case漏发previous94，只拆分94而未提前修改95。原dword操作仅影响低字节bit1，
    modern复用已集成的u8 scene-runtime owner，保留其他低位；typed owner缺失在原读取点停止。
    四alias、bit保持、owner失败、精确尾及真实记录通过。资产锁39条/39 probes，全部raw`005E`、
    长度2，分布`14/7/3/15`；真实TALK1 `0x49F4`精确尾回放`A5→A7`并发布previous94。格式化后
    剧情VM3/3、Linux core186/186、Linux app192/192均exit0。workpack双生成hash为`9e8998a333349cff72a9890ae9cd1502efee0392a8ecc8b3d5c9c121a7c35bd3`。
    未启动游戏EXE；Windows依阶段规则留到P2完成门。现代显式opcode保持107；对外进度为已实现
    107/198、已验收98/198；内部workpack73/146，即`8 assembly_exact + 65 platform_adapted +
    73 pending_audit`。

- 剧情VM P2第七十四组`0x0042A7EE` / opcode95完成独立闭环。机器无operand：读取
    `dword_4C9A18`、AND `FFFFFFFD`只清bit1、IP+2后以ESI0进入common join，发布previous95并
    yield。旧numeric case已有低位clear/yield但漏previous95，本组独立修正。原dword低位操作映射
    到已集成u8 scene-runtime owner，保留其他低位；owner缺失在原读取点typed-stop。四alias、
    bit已清保持、owner失败、精确尾与真实记录通过。资产锁25条/25 probes，全部raw`005F`、长度2，
    分布`9/5/2/9`；真实TALK1 `0x4A22`精确尾回放`A7→A5`并发布previous95。格式化后剧情VM3/3、
    Linux core186/186、Linux app192/192均exit0。workpack双生成hash为`50202cd05af6e02117a66f6656e8df01c310a61c25577e28f12b84363b35b4f1`。
    未启动游戏EXE；Windows依阶段规则留到P2完成门。现代显式opcode保持107；对外进度为已实现
    107/198、已验收99/198；内部workpack74/146，即`8 assembly_exact + 66 platform_adapted +
    72 pending_audit`。

- 剧情VM P2第七十五组`0x0042A80E` / opcode96完成独立闭环。机器先写frame interval70，
    再消费可选`%`/`*`独立prefix和首个`%Q`终止的filename；原32-byte临时copy域内扫描改为
    bounded typed失败，保留interval先行与精确窗口尾。记录消费后先audio，再走CD-style preflight；
    result2路径一次audio、无start/previous，正常与open失败路径第二次audio、actual start后共同发布
    previous96并yield。SDL以配置data root、大小写不敏感`Video/`解析及真实`LegacyAniActivity`
    start替代Win32 CD/文件/DirectDraw；archive/header/frame1/palette backend3/3通过，world-frame ANI
    stage已由后续opcode97包接通。最终LST复审删除了机器中不存在的scene staged读取；scene
    live owner留在异步activity port。四alias、四prefix、32-byte边界、截断、preflight、open失败、无
    artificial scene stop及精确尾通过。资产锁16条/16 probes，全部raw`0060`，分布`2/2/9/3`；真实
    TALK1 `0x43FA` `%*expv.ani%Q`和TALK2 `0xD39F` `*memory.ani%Q`精确尾回放。格式化后
    剧情VM3/3、Linux core186/186、Linux app192/192均exit0。workpack双生成hash为`54b0429903f1a40cd64f6a70b55f2e8c4f2ea6d1a81e924a6b430406416b2714`。
    未启动游戏EXE；Windows依阶段规则留到P2完成门。现代显式opcode增至108；对外进度为已实现
    108/198、已验收100/198；内部workpack75/146，即`8 assembly_exact + 67 platform_adapted +
    71 pending_audit`。

- 剧情VM P2第七十六组`0x0042AD3C` / opcode97完成独立闭环。机器只读取ANI header
    `+0x0E`写入的active extent：active时不推进、发布previous97并yield；inactive时先IP+2、恢复
    frame interval35，再发布previous97并same-call继续。modern直接查询`LegacyAniActivity::is_active`，
    不新增nullable VM owner。四alias跨active/inactive两帧、normal same-call和精确尾通过；精确尾完成
    副作用后下一fetch按合同返回`instruction_out_of_range`。资产锁14条/14 probes，全部raw`0061`、
    长度2，分布`2/2/8/2`；真实TALK1 `0x4408`跨两帧精确尾回放。为保证谓词真实可完成，本组把
    world-frame `ani_activity_004154a0` stage接入actual activity update，映射dialog/packed-row/head-action
    三blocker，逐帧同步scene/process，复用assembly-audited RGB ending并在finalize恢复interval35。
    原递归scene redraw以start前world snapshot/下一帧live composition最小适配；`%`成功start按机器清黑。
    格式化后剧情VM3/3、ANI backend3/3、Linux core186/186、Linux app192/192均exit0。workpack双生成hash为
    `995f2fa71a740ac6b85986370a5cfdc02ed9b9092642c4921ef441d0ccb1e8ad`。未启动游戏EXE；Windows
    依阶段规则留到P2完成门。现代显式opcode增至109；对外进度为已实现109/198、已验收101/198；
    内部workpack76/146，即`8 assembly_exact + 68 platform_adapted + 70 pending_audit`。

- 剧情VM P2第七十七组`0x0042C7EA` / opcode98完成独立闭环。机器不读取名义`+2 u16`
    payload，只把物理脚本指针与u16 IP各+4，保存新指针后以ESI0进入common join，发布previous98并
    yield；不是same-call no-op。modern同样不检查或使用payload。四raw alias的正常记录、完整四字节
    精确尾和仅剩两字节opcode的未读payload尾通过；后者仍完成IP=`0x8002`、previous98与yield，下一
    调用才typed fetch失败。资产锁3条/3 probes，全部raw`0062`、长度4，分布`0/1/2/0`；TALK2/3三条
    机器未读payload分别为`0190/006C/0001`，均以完整精确尾真实回放。格式化后剧情VM3/3、Linux
    core186/186、Linux app192/192均exit0。workpack双生成hash为
    `a6c41dee65c432f589e66d36e844ec449a2668b9e63fafc12e553bbd0a12be4d`。未启动游戏EXE；Windows
    依阶段规则留到P2完成门。现代显式opcode增至110；对外进度为已实现110/198、已验收102/198；
    内部workpack77/146，即`9 assembly_exact + 68 platform_adapted + 69 pending_audit`。

- 剧情VM P2第七十八组`0x0042AD75` / opcode99完成独立闭环。机器先读signed ANI phase
    dword，再零扩展读取`u16(+2)` threshold并作严格有符号`>`比较；`phase<=threshold`不推进、
    发布previous99并yield，`phase>threshold`推进4、发布previous99并same-call。handler不检查active
    extent，phase为0的inactive状态仍按比较等待。modern直接映射已集成`LegacyAniActivityState::phase`
    live i32 owner，保持start 1/-13、逐帧递增与finalize归零；threshold缺失在phase查询后的原operand
    访问点typed-stop。四raw alias、负启动相位、u16最大阈值、缺operand访问顺序和精确尾通过。资产锁
    139条/139 probes，全部raw`0063`、长度4，分布`0/44/64/31`；threshold范围1..350、共111种，
    TALK2/3/4四条代表记录跨等值等待和大一完成真实回放。格式化后剧情VM3/3、ANI backend3/3、
    Linux core186/186、Linux app192/192均exit0。workpack双生成hash为`e966a88434e3133522c81d0936abea8ed48f6e298e7c45c7f01b5bc61cfca455`。
    未启动游戏EXE；Windows依阶段规则留到P2完成门。现代显式opcode增至111；对外进度为已实现
    111/198、已验收103/198；内部workpack78/146，即`9 assembly_exact + 69 platform_adapted +
    68 pending_audit`。

- 剧情VM P2第七十九组`0x0042B3B0` / opcode100完成独立闭环。机器在selector替换或lookup前
    staged读取`u16(+2)` selector与`u16(+4)` Talk script number；`FFF0`映射current source GUID，
    `FFFE`保持helper-native受控角色规则。命中live role时只写`talk_script_id`；missing时以Talk值、
    OR0、AND FFFF和其余FFFF sentinel提交精确MAPS role-source patch。两路均推进6、发布previous100
    并same-call，Talk值不做范围或sentinel验证。modern复用已集成typed MAPS database port，保持
    无world-session owner或source GUID不存在时无伪造写入。四raw alias、`FFFF`值、双operand截断、
    精确尾和四TALK文件代表记录通过。资产锁192条/192 probes，全部raw`0064`、长度6，分布
    `49/14/47/82`；Talk范围0..6909。格式化后剧情VM3/3、MAPS依赖3/3、Linux core186/186、
    Linux app192/192均exit0。workpack双生成hash为
    `2eb063a44643f25b7fc1e2c743115b665b910274515f26bbec2197380a600303`。未启动游戏EXE；
    Windows依阶段规则留到P2完成门。现代显式opcode增至112；对外进度为已实现112/198、已验收
    104/198；内部workpack79/146，即`9 assembly_exact + 70 platform_adapted + 67 pending_audit`。

- 剧情VM P2第八十组`0x0042B43B` / opcode101完成独立闭环。机器只读取`u16(+2)` selector；
    `FFF0`映射current source GUID，`FFFE`保持helper-native受控角色规则。lookup命中时只对live role
    `flags`执行`OR 04000000`，保留其余31位；missing静默消费，不诊断也不提交MAPS fallback。
    两路均推进4、发布previous101并same-call。modern的typed role record以布局断言锁定flags在`+0x10`，
    无需平台转换或新端口。四raw alias、bit28 skip首匹配、selector截断、其他位保留、精确尾和
    四TALK文件代表记录通过。资产锁126条/126 probes，全部raw`0065`、长度4，分布`39/38/11/38`；
    40种selector范围0..1061。格式化后剧情VM3/3、role lookup依赖1/1、Linux core186/186、
    Linux app192/192均exit0。workpack双生成hash为
    `d051c5ec33cf669aa18d6b0c1c6ab0ceb34d324d3ddc2c59a2fcea6ab0a2b3e6`。未启动游戏EXE；
    Windows依阶段规则留到P2完成门。现代显式opcode增至113；对外进度为已实现113/198、已验收
    105/198；内部workpack80/146，即`10 assembly_exact + 70 platform_adapted + 66 pending_audit`。

- 剧情VM P2第八十一组`0x0042C567`共享opcodes102/103/117/136/140/145/146/174完成独立闭环。
    内部跳表分别选择`0040/0020/0010/1000/0800/2000/0100/4000` mask；机器先读selector并lookup，
    再读boolean。`FFF0`把受控index低16位当GUID key继续lookup，不是current source或直接controlled role；
    `FFFE`仍由helper直接选择受控角色。live路径按clear mask、任意非零set、clear surface、mark surface
    顺序执行；missing路径按false=`OR0/AND(FFFF-mask)`、true=`OR mask/AND FFFF`提交MAPS patch。
    modern复用typed surface和MAPS owner，owner/footprint失败在原helper点typed-stop并保留已提交flags与
    partial clear。八mask×四alias×两类boolean、FFF0/FFFE、skip首匹配、分阶段截断、missing双mask、
    partial failure及精确尾通过。资产锁683条/685 probes，六个变体有记录，145/174以asset absence和
    synthetic锁定；只有opcode103观察到58条`FFF0`。格式化后剧情VM3/3、surface/MAPS依赖3/3、Linux
    core186/186、Linux app192/192均exit0。workpack双生成hash为`0a455f788581ae9ffe944858d8d7381ae4fbcc944f339344357952c76723d5b7`。未启动游戏EXE；
    Windows依阶段规则留到P2完成门。人工语义增至134行，现代显式opcode增至121；对外进度为已实现
    121/198、已验收113/198；内部workpack81/146，即`10 assembly_exact + 71 platform_adapted +
    65 pending_audit`。

- 剧情VM P2第八十二组`0x0042B47E` / opcode104完成独立闭环。机器先清text control bit28，
    再依次读取并sign-extend写入`+2/+4`两个i16布局值，最后推进6、发布previous104并same-call。
    REVIEW发现旧case错误地一次性预检两个operand，且成功路径漏发previous；现按原访问点分两级typed-stop，
    保留缺第一项前已清bit、缺第二项前已清bit并写第一项的副作用。四raw alias、i16最小/最大、两级截断、
    精确尾和既有dialog消费/复位链通过。资产锁125条/125 probes，全部raw`0068`、长度6，分布
    `46/48/18/13`；31种pair，第一项范围-80..52、第二项-120..-20。格式化后剧情VM3/3、Linux
    core186/186、Linux app192/192均exit0。workpack双生成hash为
    `56a45509e2499fed1cf892e3ad7413559cdd337ffc67d91c43cedc4f80b4f969`。未启动游戏EXE；
    Windows依阶段规则留到P2完成门。现代显式opcode保持121；对外进度为已实现121/198、已验收
    114/198；内部workpack82/146，即`10 assembly_exact + 72 platform_adapted + 64 pending_audit`。

- 剧情VM P2第八十三组`0x0042B4B9` / opcode105完成独立闭环。机器只执行text control u32
    `AND F7FFFFFF`、推进2、发布previous105并same-call；modern新增直接typed owner case，无operand、helper、
    条件分支或平台适配。四raw alias、其他位保留、same-call successor与精确尾通过。资产锁806条/
    806 probes，全部raw`0069`、长度2，分布`308/173/145/180`。格式化后剧情VM3/3、Linux
    core186/186、Linux app192/192均exit0。workpack双生成hash为
    `e2ded255146f2237a45dc0dc65659c11e4c12a2e215f55200c1e7b7eecf6ea49`。未启动游戏EXE；
    Windows依阶段规则留到P2完成门。现代显式opcode增至122；对外进度为已实现122/198、已验收
    115/198；内部workpack83/146，即`11 assembly_exact + 72 platform_adapted + 63 pending_audit`。

- 剧情VM P2第八十四组`0x0042B4CA`共享opcodes106/154完成独立闭环。机器先读`+2` u16
    threshold，再按normalized opcode选择主/副picture-action父对象；selected链为空直接完成，非空只读取首节点
    `+0x49` u8，即typed action `packed_ap_state`高字节。`byte<=threshold`不推进并yield；空链或严格
    `byte>threshold`推进4、发布previous并same-call。modern复用`LegacyPictureActionLists`，缺binding在operand后
    的父对象访问点typed-stop。两个变体各四raw alias、主副链选择、相等/严格大于、空链、threshold256、
    operand/runtime顺序及精确尾通过。opcode106资产锁60条/63 probes，全部raw`006A`、长度4，分布
    `21/8/14/17`，25种threshold范围2..110；opcode154以asset absence和synthetic锁定。格式化后剧情VM/
    picture-actions定向4/4、Linux core186/186、Linux app192/192均exit0。workpack双生成hash为`d145e17bd011fa7aa5498103c845fcb310024edc4b7b8f1aa531e7066501e8cc`。
    未启动游戏EXE；Windows依阶段规则留到P2完成门。人工语义增至135行，现代显式opcode增至124；
    对外进度为已实现124/198、已验收117/198；内部workpack84/146，即`11 assembly_exact +
    73 platform_adapted + 62 pending_audit`。

按用户指令，本组提交推送后暂停P2并将`pi-execution`合并到`main`；合并后只独立调整CTest并发，
不进入下一handler。恢复P2时下一停点为`0x0042B50F` / opcode107，现有导航语义与既有实现均不继承完成状态。

- `pi-execution`的`a48601c`已在确认`origin/main`为其祖先且零分叉后，以`--ff-only`合并并推送到
    `main`；合并时两branch tip/tree完全相等。随后按用户指令在`main`独立优化完整门：Linux/Windows
    CTest统一读取`OPENSWD3_TEST_JOBS`，默认8，Linux拒绝非正整数；编译仍使用既有并发。
    首轮Windows并发暴露真实竞态，5/192失败；仅加同exe锁后另一组5/192失败。最终按CTest JSON锁定
    31组同测试二进制多invocation，并给33项真实资产测试追加`legacy_real_assets`全局锁，保留Win32
    exclusive archive语义而不伪改生产共享模式。最终Linux core186/186、Linux app192/192、Windows LLVM
    app192/192均exit0。`build.bat`保持CRLF且未加入pause；未启动游戏EXE。

- 剧情VM P2第八十五组`0x0042B50F` / opcode107完成独立闭环。机器按selector→lookup→threshold→
    action顺序读取；FFF0替换context source，helper-native FFFE解析controlled role。threshold高于packed低
    字节上限或lookup失败时空诊断并消费；合法且packed高字节低于threshold时不推进并yield，其余+6后
    same-call；所有路径经common join发布previous107。修复旧C++ whole-record预检提前threshold及两类路径
    漏previous。四raw alias、FFF0/FFFE、等上限等待、等threshold完成、非法上限、lookup失败、两阶段截断、
    精确尾与四库真实回放通过。资产锁294条/296 probes，全部raw`0x006B`、长度6，分布
    `64/27/36/167`；35种selector、20种threshold、67种pair。Story VM3/3、Linux core186/186、app192/192
    均exit0；workpack双生成稳定hash为
    `b42d4ff6c9ac6719824e245531b1d327a183f55b27b7ce706d0c9e57ad234b94`。
    未启动游戏EXE。现代显式opcode保持124；对外进度为已实现124/198、已验收118/198；内部workpack
    85/146，即`12 assembly_exact + 73 platform_adapted + 61 pending_audit`。

- 剧情VM P2第八十六组`0x0042B5F2` / opcode108完成独立闭环。机器先读/写u16 X，再读/写
    u16 Y，只有两项均提交后才按无符号边界独立把`X>639`或`Y>479`替换为16；成功+6、
    previous108、same-call。现代复用已集成的`dialog_anchor_left/top` one-shot owner；修复原缺失case，
    下游共享dialog同帧消费后恢复`8000/8000` sentinel。四raw alias、`639/640`、`479/480`、FFFF、
    两阶段截断、精确尾及下游dialog链通过。线性TALK零记录，使用`asset_absence_verified`。Story VM3/3，
    Linux core/app完整门待最终验证；workpack双生成稳定hash为
    `a94c72c6bc052fc4da30fb9eec3ebdb4c951f0429c5fb22b25740adcca260fef`。未启动游戏EXE。现代显式
    opcode增至125；Linux core 186/186与app 192/192完整门exit0通过；对外进度为
    已实现125/198、已验收119/198；内部workpack86/146，即
    `13 assembly_exact + 73 platform_adapted + 60 pending_audit`。

- 剧情VM P2第八十七组`0x0042B63C` / opcode109完成独立闭环。恢复变长角色列表的逐项
    路径步进、缺失角色跳过和消费后让出合同；复用已闭环story-path owner，并在原危险点保留
    typed失败。67条真实记录及代表性count1/count18回放通过。Story VM 3/3、Linux core
    186/186和app 192/192通过；workpack双生成稳定hash为
    `951b34e144bfcf449d764fd8f598d8de1c4b8ffc29c80b2b95efa94c9e2c9ad3`。未启动游戏EXE。
    现代显式opcode增至126；对外进度为已实现126/198、已验收120/198；内部workpack87/146，
    即`13 assembly_exact + 74 platform_adapted + 59 pending_audit`。

- 剧情VM P2第八十八组`0x0042B6A5` / 共享opcodes110/111完成独立闭环。恢复从角色1
    开始的bit30扫描及两条互反条件：命中分支按同文件target重载，顺序分支固定消费且不读取
    target；两路均发布previous并same-call。opcode110线性资产零记录；opcode111锁定24条记录/
    24 probes，代表记录的顺序和重载两路回放通过。Story VM 3/3、Linux core 186/186和app 192/192通过；
    workpack双生成稳定hash为
    `808be682a78cd80c5d05a2d95f2145c36210ed943d1ce188da55a46f92621f16`。未启动游戏EXE。
    现代显式opcode增至128；对外进度为已实现128/198、已验收122/198；内部workpack88/146，
    即`13 assembly_exact + 75 platform_adapted + 58 pending_audit`。

- 剧情VM P2第八十九组`0x0042B70C` / opcode112完成独立闭环。恢复packed-row与
    role-head两条action链的短路等待：任一相关链非空则原地让出，两链为空只推进两字节但
    仍让出；moving-action链不参与。线性资产锁定9条记录/9 probes，代表记录的等待与完成
    两帧回放通过。Story VM 3/3、Linux core 186/186和app 192/192通过；workpack双生成稳定hash为
    `75cc397503e8470c0f418eb6178a571016a51eedb0c8f1fc3d33c8d14d5e8dd2`。未启动游戏EXE。
    现代显式opcode增至129；对外进度为已实现129/198、已验收123/198；内部workpack89/146，
    即`13 assembly_exact + 76 platform_adapted + 57 pending_audit`。

- 剧情VM P2第九十组`0x0042B723` / opcode113完成独立闭环。恢复六字节音效请求：
    只读取u16 sound id，固定消费但不读取末尾padding；复用已闭环sample wrapper，播放返回不
    参与流控，正常路径发布previous并让出。线性资产零记录/零probes，以asset absence和四alias、
    operand截断、未读padding尾、精确尾锁定。Story VM 3/3、Linux core 186/186和app 192/192通过；
    workpack双生成稳定hash为
    `9febc1905a830a79e4b11940d9cf1ca1c789236fa6ef114dee7a07cf6c2d34e5`。未启动游戏EXE。
    现代显式opcode增至130；对外进度为已实现130/198、已验收124/198；内部workpack90/146，
    即`13 assembly_exact + 77 platform_adapted + 56 pending_audit`。

- 剧情VM P2第九十一组`0x0042B739` / opcode114完成独立闭环。恢复场景音乐stream
    请求的分阶段状态写入、既有transition同步、control位派生及same-call合同；实际
    LegacyStreamManager已接入原transition槽。157条真实记录/159 probes及代表记录回放通过。
    Story VM 3/3、Linux core 186/186和app 192/192通过；workpack双生成稳定hash为
    `f5e176856582bf643502b2ece14b7658a307e7e76fc7144fd8c9e7c6a8fc14ba`。未启动游戏EXE。
    现代显式opcode保持130；对外进度为已实现130/198、已验收125/198；内部workpack91/146，
    即`13 assembly_exact + 78 platform_adapted + 55 pending_audit`。

- 剧情VM P2第九十二组`0x0042B7FC` / opcode115完成独立闭环。恢复u16音量档
    零扩展、上限11、不可达负夹分支、stream100 wrapper返回忽略及跨帧让出合同；实际
    LegacyStreamManager已接入窄port。线性资产零记录/零probes，109处raw字样均非证明入口，
    以asset absence和synthetic锁定。Story VM 3/3、Linux core 186/186和app 192/192通过；
    workpack双生成稳定hash为`0265c2a06b9dfaf4537069c25742887f05c8f59912fb073ee62dace5b0bb5c12`。
    未启动游戏EXE。现代显式opcode增至131；对外进度为已实现131/198、已验收126/198；
    内部workpack92/146，即`13 assembly_exact + 79 platform_adapted + 54 pending_audit`。

- 剧情VM P2第九十三组`0x0042B83A` / opcode116完成独立闭环。恢复入口count冻结、
    末尾count重读、逐项角色解析与位置调度、受控角色标记、16位坐标/IP及same-call合同；
    复用已闭环story-path owner并在missing/typed失败原危险点停止。30条真实记录/30 probes、
    117个子记录及代表count1回放通过。Story VM 3/3、Linux core186/186及app192/192通过；
    workpack双生成稳定hash为`6d05a604f0fee35de7df00b3a5cf1932fd60c8e86cb0b76adb636ec6747b5807`。
    未启动游戏EXE。现代显式opcode增至132；对外进度为已实现132/198、已验收127/198；
    内部workpack93/146，即`13 assembly_exact + 80 platform_adapted + 53 pending_audit`。

- 剧情VM P2第九十四组`0x0042B8E6` / opcode118完成独立闭环。恢复按消息raw role index
    映射GUID并删除全部匹配对话、连续match predecessor保持、角色gate清零、text/caption/node
    释放及低15位计数零夹顺序；无效record index在原GUID裸读点typed-stop，保留此前删除。
    1669条真实记录/1669 probes及TALK1连续GUID 0→1记录回放通过。Story VM 3/3、
    Linux core186/186及app192/192通过；workpack双生成稳定hash为
    `2d0f63e21ceb4f44e0f74d203fa336e53686a563f6b3db059f6e00554c834136`。
    未启动游戏EXE。现代显式opcode增至133；对外进度为已实现133/198、已验收128/198；
    内部workpack94/146，即`13 assembly_exact + 81 platform_adapted + 52 pending_audit`。

- 剧情VM P2第九十五组`0x0042B9C2` / shared opcodes119、139完成独立闭环。
    两者共享selector解析与首匹配消息扫描，分别等待flags bit0/bit15；wait路径发布previous、
    service audio并yield，完成/空链/miss/lookup失败均+4 same-call。119/139锁定835/15条真实
    记录及两条variant跨帧回放；四raw alias、`FFF0→FFFD`、完整u32受控index和两类精确尾通过。
    Story VM 3/3、Linux core 186/186及app 192/192均以exit 0通过；workpack双生成稳定hash为
    `42c45c7d9693fccba91491c76eb6a825233fbafb19b3694c07b57442221d54c0`。
    未启动游戏EXE。人工语义增至136行，现代显式opcode增至135；对外进度为已实现135/198、
    已验收130/198；内部workpack95/146，即`13 assembly_exact + 82 platform_adapted + 51 pending_audit`。
