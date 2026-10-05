# 战斗帧鼠标输入与目标解析 `0x0045FC60`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`、`actor_frame_snapshot_reclaimed`、`actor_frame_resource_reclaimed:3/3`。

## 1. 完整范围

权威LST主体为`0x0045FC60..0x00460BF7`，从proc到endp共1917行、1165条实际指令、31个call、189个跳转、118个局部标签、27个`retn`，没有外部`FUNCTION CHUNK`。

31个callsite完整归类为：1次已关闭热点命中查询、6次既有样本播放窄边界、3次已关闭TSW命令流像素命中、1次选项角色资格查询、6次角色选择配置、1次组B候选查询、3次已关闭typed角色当前帧边界查询、3次已关闭typed角色帧资源准备、3次角色镜像查询、1次组B模式查询和3次组A候选查询。热点、样本、像素、角色当前帧边界与角色帧资源准备直接复用已关闭typed语义；surface adapter只解析帧资源typed返回token，不对应额外原版callsite且不修改模拟EAX/ECX/EDX，其余六类尚未关闭callee继续归单一帧输入typed端口。

函数没有参数。唯一caller是战斗逐帧协调器。入口EAX/ECX/EDX只在若干早退路径保留；函数按路径返回0或1，完整ECX/EDX会直接成为相邻逐帧输入分派的入口寄存器。

## 2. 鼠标入口门与热点owner

入口先保存当前鼠标X/Y。只有当前坐标同时等于两项战斗前帧坐标，且pre-frame gate B为0时立即返回EAX 0，ECX/EDX保持caller入口值。

坐标改变时先把pre-frame gate B写1、独立pointer activity gate写0；同坐标但gate B非零则保留原gate。该门与右侧按钮/目标命中使用的mouse action gate是两个物理全局，不能折叠。两项战斗前帧坐标随后无条件更新为当前值。

原`0x004C8BEC`是选择热点链头，不建立第二份标量owner。现代直接以热点vector非空作为同一条件，调用已关闭严格开区间命中查询；命中时把零基索引和非零选择owner token发布到输入共享状态，miss发布零token。查询后按原时点重读live鼠标。

消息值大于30时在switch范围检查处返回0并保留此前ECX。0到30先按权威31-byte间接表装载ECX；只有case 0、1、2、3、4、5、8、27、30进入有效分支，其余值统一默认返回0。

## 3. case 0、1与30

case 0在`0x0045FD14/16`以组A排除低byte和组B数量作**signed jge**比较；成立则直接跳`0x004602A3`默认返回0，**不写**selected option。否则要求Y严格位于`0x182..0x1E0`；越界时走`0x0045FDD6`，先把selected option写全1再返回0。组Acount按i32 signed正数循环，从`0x004A75C8`开始逐项读取十项party source映射，随后从`0x004A75A8+source*4`读取横向值：来源0..7对应八项party offsets，8..17与紧随的十项映射dword同址，来源18首次落入尚无共享owner的`byte_4A75EC+4`而typed-stop；X严格位于回绕的`offset-0x18..offset+0x74`时发布`index+8`低word、EAX=1、ECX=index+8并保留EDX为命中的映射地址。未命中逐项令ECX+1、EDX+4：左界未过保留EAX=offset，左界已过但未过右界则EAX已加`0x74`，在下一次未绑定读取前必须保留此差异；耗尽时将selected option写全1、EAX=0并保留已推进的ECX/EDX；越过十项来源或来源17后的未绑定邻接byte表时在首次真实访问typed-stop。此前通用C++只保留前四项映射、未保留该分支EDX。无命中或非正count正常写全1并返回0。另一个旧差异是把计数门和Y门合并，错误清除了前者的选择字；两项均已在共享前缀中修正，完整门禁等级另记。

case 1要求active actor非零。面板矩形严格使用live origin：X为`origin+0x0A..origin+0x76`，Y为`origin+0x28..origin+0x88`；按`0x36`列宽和`0x18`行高计算八项索引。索引必须signed小于`startup extra+5`，permission byte非零。索引4以上还以`active-8`组A对象token和物理选项role id调用资格callee，并在callee后动态重读permission。有效项变为一基selection，变化时播放样本`0x2E`，再按extra+5夹上界并发布双gate。

case 1无效矩形先清mouse action gate与selected option，再按case 0同一party owner遍历全部成员；每个命中都覆盖selected option，不提前退出，最后仍返回0。

case 30按三列、每列五行严格矩形计算`5*column+row+1`，变化时播放样本，先发布live值再以signed比较夹到10。完全miss清mouse action gate和selected option全1。

## 4. case 2、4、5、8与27

case 2先把equipment hover写全1；Y严格位于`0x82..0xA0`时按`0x2A`列宽扫描，再发布hover并置pre-frame gate B。主列表要求X严格位于`0xE0..0x180`，以`0x14`行高计算一基行；`panel scroll+row`与BDF2低byte的i8符号扩展作signed比较。有效变化播放样本并发布列表选择、interaction 0与双gate。

case 4与27使用同一十项hover/列表框架，但横向上界分别为`0x19C`和`0x194`；一基行加scroll后与BDF4的u16零扩展作signed比较。列表miss先清mouse action gate；record15 held按i32 signed大于等于1时立即返回。否则清menu action和三项临时选择，只有BDF4 unsigned大于7才处理右侧四个严格按钮。

case 2、4、27的右侧按钮分别保留不同X/Y边界。命中依次发布interaction 1、2、3、4；完全miss严格按原路径清interaction、menu action与mouse action gate。

case 5在X`0xC4..0x178`内按`0x16`行高选择两行；case 8在X`0xE0..0x199`内按`0x18`行高选择七行，并把一基行与BDF3低byte的i8符号扩展作signed比较。变化时均播放样本，成功发布双gate，miss只清mouse action gate。

所有矩形使用unsigned严格开区间；边界点不命中。加法、减法和选择值均保留u32回绕。

## 5. case 3组B目标扫描

case 3先检查目标阻断dword、目标抑制byte和阻断word。随后按`active*5-40`的u32回绕索引首次读取启动reset物理表；越界在该访问typed-stop，入口鼠标副作用已经可见。

表值为0时选择组B路径：

1. 以i32 signed动态组Bcount正向遍历，每个对象调用配置mode 0；callee后重新读取count，不增加modern上限。
2. 从`count-1`按i32 signed逆向扫描对象token。
3. 候选查询完整EAX不等于1时，先直接组合`0x004784A0` typed角色当前帧边界查询，再在原`0x004605D9`组合`0x00478620` typed帧资源准备。snapshot两条正常早退继续使用共享四dword局部块旧值；帧资源leaf正常返回后才以返回token解析surface。零token在caller真实对象读取点typed-stop；非零对象首dword为零普通未命中；只有首dword非零才把目标动作可用写1并进入像素扫描。
4. 外层与内层都按`0,2,4,6`。每个点先动态查询镜像；普通点为`mouse+(inner,outer)`，镜像X为`width+2*origin_x-mouse_x-inner`。
5. 每点直接调用已关闭TSW命令流像素命中；短源只在该helper真实读取点typed-stop。零命中当点清mouse action gate，非零命中立即发布`candidate+1`、selected target、配置mode 1和双gate。
6. selection等于6时再查组B模式；完整EAX为0把目标动作可用清零，但不撤销目标发布。

无候选返回0并保留此前角色配置和像素查询副作用。

## 6. case 3组A目标扫描

启动表非零时先按i32 signed动态组Acount对全部组A对象配置mode 0，再对live selected target对象补一次mode 0。随后计算：

```text
remaining = group_a_count
remaining -= startup final_subtract_word
remaining -= startup supplemental_count_word
```

三步均为u32回绕。

remaining unsigned不小于4时，从`remaining-1`按i32 signed逆向读取最终角色owner的十项actor order。每个actor index先真实读取组A对象`+0x2B00/+0x2B04`共享完成槽；任一精确等于1跳过，否则查询组A候选。order或完成槽只在实际访问typed-stop。

有效候选执行8×8逐点扫描。若候选正是`active-8`且selection不等于2或3，可见像素不能发布，但原程序仍继续剩余点；modern不提前退出，完整保留最多64次镜像与像素call。扫描后清live selected target marker；鼠标Y位于闭区间`0x16A..0x1B6`且X位于actor offset闭区间`offset..offset+0x7C`时，可绕过像素发布该actor并置marker。order耗尽直接返回0。

remaining小于4时改按`group_a_count-1`直接逆向扫描组A对象。可见目标同样服从当前actor排除；成功发布后把marker前四byte作为一个物理dword清零。扫描耗尽后先清live selected marker，再在同一Y闭区间正向扫描紧凑party source：完成槽按紧凑索引访问，X offset与marker按source索引访问，发布actor code为紧凑索引加1。

marker、source、offset、actor order和组A完成槽分别只在首次原始访问typed-stop，已完成的角色配置、候选callee、像素查询及前缀写不回滚。

## 7. caller回收与全局重置

唯一逐帧caller删除最后一个前置opaque stage并直连本typed实现。音乐查询/提交留下的完整EAX/ECX/EDX进入本函数；普通返回的ECX/EDX直接进入相邻逐帧输入分派。typed-stop保留音乐与本函数前缀，阻断输入分派、角色预处理、metric、surface和全部后续帧。

case 3的三处原点准备先直接组合`query_legacy_battle_actor_frame_snapshot`，随后分别在`0x004605D9`、`0x004607F6`与`0x00460A0A`组合`prepare_legacy_battle_actor_frame_resource`。三处共享同一个四dword snapshot局部块，但帧资源复制与frame token来自各actor唯一十八槽动作记录owner。前后两个逆序caller分别重建`EBX=index, ESI=actor, EDI=0`；actor-order caller重建EBX槽token、ESI=1、`EAX=actor_index*0xBCD`与最后一次SUB flags，并在完整8×8 miss后把EDI=8携带到下一候选。snapshot正常早退不清局部槽；任一leaf typed-stop保留候选查询、动作更新、frame lookup、动作记录复制前缀、actor frame-token提交和已完成的X/Y/width前缀，阻断surface解析、像素查询、目标发布与当前/剩余actor后缀。旧`prepare_actor_origin`枚举ordinal保持reserved且生产调用数为0；surface adapter接收typed帧资源返回token，frame coordinator继续复用既有action dispatch、action updater和frame provider注入，不创建平行actor owner。

状态复用：当前鼠标来自输入归一化owner；party source与offset来自启动owner；permission、extra、启动模式表和两个减数来自启动/reset owner；组A数量与组B数量来自metric owner；active、published、actor order与组A完成槽来自最终角色owner；selection、interaction、mouse action、selected option、热点token与样本混音来自输入分派owner；热点链只保留vector owner。

新增状态只承载此前未命名的战斗前帧鼠标、独立pointer activity、菜单几何/行选择、阻断值、目标索引、十byte marker及边界。全局重置只同步权威LST实际写入的前帧鼠标、列表初值、当前equipment、scroll、origin和三项阻断值；未在reset写集合中的hover、行限制、边界、marker和其他选择保持入口值。

## 8. 验证与动态差分

定向测试覆盖：同鼠标早退寄存器；热点首命中；case 0 party映射；case 1 permission与样本；case 2按钮；case 4 signed负held；case 5/8行高与signed byte；case 27独有按钮边界；case 30网格；组B逆向像素命中和selection 6模式；组A大列表actor-order命中、完整64点miss后的下一候选EDI=8、组A小列表直接命中及首marker dword清零；同active不可选时完整64点调用；三处typed frame snapshot与typed帧资源准备、各caller返回地址/EBX/ESI/EDI/ESP、actor-order重算EAX与SUB flags、两次updater/provider、typed返回token解析、surface reply哨兵不污染Group-B首mirror与Group-A次mirror寄存器、Group-B首dword普通未命中、Group-B非零token/object-unreadable在action kind 6可达场景的mirror与action-six前停止、Group-A零token及非零token/object-unreadable保留reset/configure与一次mirror前缀、REP与frame-token fault后缀抑制、共享局部块正常早退、reserved槽零调用、重叠local参数读取停点、provider失败与输出X写typed-stop后缀抑制；actor order、启动模式和图像短源typed-stop；全局重置别名；逐帧caller阻断。

当前缺少原版鼠标/菜单全局、八类未关闭角色callee、两组角色对象、surface记录、TSW命令流、热点链、样本及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

Workpack 316后续审计发现SDL旧`battle_actor_metrics_`在战斗初始化后仍为默认0，尽管启动owner `battle_runtime_.enemy_count/party_count`已由部分`prepare_legacy_battle_setup`发布；这会让case 0的有符号计数门错误走零人分支。现在每次实际`frame`回调前从启动owner把这两项live数值映射到metric视图，保留其他metric字段；该修正定向core 1/1、Linux core／ASan各200/200及Linux／Windows app各206/206通过，仍待新SDL实测。第二次旧SDL日志只证明旧二份状态下的当次输入返回，不能推断修正后的case 0轨迹；后续增加旧默认零人早退与live一敌一人进入party-source的跨视图反例，定向core 1/1、Linux core／ASan各200/200及Linux／Windows app各206/206通过，仍不等于生产回调实测。紧接着将简化setup已计算的四项紧凑来源索引与四对signed阵型锚点映射到共享`battle_runtime_`启动owner，避免live人数门进入扫描时误读默认0；合成三人及真实战斗98资产测试已补，定向core 1/1、Linux core／ASan各200/200及Linux／Windows app各206/206通过；尚无第三次实际SDL运行。启动期额外角色材料化仍未闭合，不能以同步动作证明原版完整启动。

## 9. Workpack 316 的生产前缀接线（未验收）

权威LST `0x0045FC60–0x0045FCA8`依次读鼠标X/Y及前帧X/Y：均相同且`dword_53BFBC=0`时走零返回；否则只有坐标变化才先写`dword_53BFBC=1`与`dword_53BDA8=0`。接下来的`0x0045FCAD`**先读**热点链头`dword_4C8BEC`，随后`0x0045FCB2`写前帧X，`0x0045FCBA`写前帧Y；缺热点owner时不允许提前写这两项。core和SDL共用提取出的首门：core在热点链头读取后才写前帧坐标；SDL此前缺热点视图，先保守停在`0x0045FCAD`；现按[世界选择热点链证据](world-choice-hotspot-chain-0040db40-0040dbc0.md)借用同一`world_dialogs_.messages[].choices` owner，以与世界帧相同的顺序构建当次临时热点视图。非早退路径先读取此视图、再写前帧坐标，调用已关闭的严格开区间首命中查询，发布同一输入选择状态，按LST `0x00460BF8..0x00460C3E`跳表，消息6、7、9..26、28、29及大于30直达`0x004602A3`默认零返回、进入上层下一CALL入口前缀；非默认case 1空角色在`0x0045FE3C/3E`走默认返回，非空在`0x0045FE44`前停；case 3在`0x00460527..4D`按`ESI=1/DI=0`比较阻断位、音乐抑制与选择字，成立走默认返回，否则在`0x00460556`动作槽首读前停；case 5/8在`0x00460270..7E`／`0x004602E6..F4`横向严格未命中时写共享鼠标动作门为0并走默认返回，命中时分别停在`0x00460280/F6`行扫描前；case 2在`0x0045FFCF`将`hovered_equipment`写全1后停在`0x0045FFD9`垂直分支前，case 4在`0x0046011A`将相邻`hovered_secondary`写全1后停在`0x00460124`前；case 27/30仍在已读状态后的`0x0045FCEF`比较前typed-stop；case 0则读取SDL既有`battle_message_state_`、共享actor metrics、最终角色排除数与输入选择字，沿`0x0045FD07..0x0045FD44`有符号计数门、严格Y门及组A数量门运行：可证明零返回时进入上层下一CALL `0x0045323E → sub_45F2A0`入口前缀；进入角色来源映射时复用启动期同一十项映射与八项水平偏移，原序扫描并发布共享输入选择；可映射的零／一返回也进入上层下一CALL入口前缀，越界分别在首次`0x0045FD49/4B`读取前typed-stop。同坐标且旧gate为零时不触碰热点，零返回并进入上层同一下一CALL入口前缀；该前缀按序清共享菜单动作，精确latch=1在`0x00453243`前typed-stop，否则读真实消息／排队角色／共享对话链，按signed小于2、角色非零、链空三门分流，键盘路径依序查询DIK2..9并在首个按下的键分支前typed-stop；无键按下与非键盘路径借用共享输入状态及记录1执行三门／latch按位或，再按有符号记录9门发布记录0两项，停在`0x0045F5E0`记录2首读前。SDL借用现有归一化鼠标、最终角色pre-frame gate及共享帧输入状态端口，不注入测试坐标；目前`release_display_and_world_for_battle_entry()`与`close_world_map_view()`仍为空实现，战斗入场时该对话链的清理生命周期尚未证明与原版一致。其后用户Windows开场战斗日志已实际执行SDL帧回调，见`build/vm/wp316-sdl-frame-user-run/openswd3-2026-10-03_18-15-50-20916.log`；该次门返回`input_gate_eax=0`、`hotspot_queries=0`，最终停在输入分派记录2首读前；仍未接完整消息阶段。首门定向`battle.legacy_battle_setup` 1/1及该版完整Linux core／ASan各200/200、app206/206通过；热点前缀定向1/1、该版完整Linux core／ASan各200/200及app206/206通过。新增case 0共用前缀定向1/1、完整Linux core／ASan各200/200和app206/206通过；随后修正大于30的默认返回和非零消息停点，定向core 1/1及完整Linux core／ASan各200/200、app206/206通过。新增case 0映射扫描共享前缀经LST复核补齐来源8..17物理同址别名、横向严格边界以及两种未命中后EAX差异，最终该版定向core 1/1与完整Linux core／ASan各200/200、app206/206通过；`clang-format --dry-run --Werror`与`git diff --check`通过。另新增默认消息跳表共用前缀，LST 31项映射分类与所述六项默认消息的core行为定向1/1、完整Linux core／ASan各200/200及app206/206通过；`clang-format --dry-run --Werror`、`git diff --check`通过。新case 1/3/5/8共享门控及SDL绑定的定向core 1/1、完整Linux core／ASan各200/200及app206/206通过，格式与diff检查通过；新case 2/4悬停复位前缀定向core 1/1、完整Linux core／ASan各200/200及app206/206通过，格式与diff检查通过。前述构建及CTest本身不包含实际SDL战斗`frame`回调运行；用户独立运行才证实本次生产帧回调与首个停点，不覆盖非默认输入或角色帧，不能据此前缀关闭P0。
