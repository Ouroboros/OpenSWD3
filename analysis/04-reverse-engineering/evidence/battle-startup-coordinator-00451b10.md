# 战斗启动协调器 `0x00451B10`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围与附件

权威LST主函数为`0x00451B10..0x004527DB`，共1351行、60个静态call站点、48个标签，无外部FUNCTION CHUNK。cdecl单参数按原规则同时保留完整32位battle ID和低16位ID。

主函数直接调用的邻接显示surface helper未在工作包单列：

- `0x00451AE0..0x00451B04`，29行，释放两个旧surface槽并逐槽清零；
- `0x00451A90..0x00451AD5`，41行，逐槽查询高/宽、创建两个surface，随后令EAX为全1并按低地址到高地址写三个`0xFFFF`完成word。

两附件随本caller typed闭合，但正式工作包只计`0x00451B10`一次。

## 2. 入口、阈值与固定reset区

入口先把参数低word写入旧battle ID高半word槽，再调用运行时准备。随后从速度设置调用已关闭`0x0044FFC0`，modern直接调用`publish_legacy_battle_action_threshold`，保留模`2^32`的`(20-speed)*100`。

接着按LST顺序清理固定状态：

- 零填充dword块：`0x26`、`0x14`、`0x3C`、`0x0A`、`0x14`、`9`、`8`、`0x32`及两个`0x12`项块；
- 全1填充dword块：三个`0x12`项块及一个`0x7E`项块；
- 单独清零四槽、五槽、四槽、两槽及其他latch；
- 候选used八字节清零；
- 18条步长`0x1C`记录逐条写`+00=0xFFFFFFFF`、`+0A(word)=0`、`+0C=0`、`+14=0`、`+18=0`；
- 另把固定latch写全1或零。

modern以定长typed数组和字段建模全部块，不以宿主指针模拟旧地址。`LegacyBattleStartupResetRecord`现为精确`0x1C`布局；本函数只修改上述五个已证字段，`+04/+08/+10`保持入口值。`0x32`项零填充块由已关闭攻击顺序插入按十组、每组五dword直接消费并逐项清源，不建立平行暂存区。`0x0052022C`起20个dword同时是消息99过渡控制选择按四行十列读取的唯一40-word表；选择只清命中的半word，启动和全局重置仍整块清零。战斗调试W键和全局重置执行完整126 dword清零时则覆盖全部记录字段。角色优先索引直接写唯一actor metric owner。入口不清组B数量；`00451C86`清的是`0x0053C4B0`特殊行动计数，由共享目标选择状态唯一持有。组B数量直到定义投影时才被覆盖。测试先以非零值污染所有块，再逐块验证最终值与未写字段保留。

## 3. 控制块与队伍发现

四个控制switch先写1，`00451CCD`直接调用公共动作初始化`0040DC00`。
目标`0x004C9708`是对话结束按钮，世界初始化`0040E372..0040E3C5`及对话尾部
`004300D3..0043011A`使用同一记录。它不是`00414AA7`传入的世界光标
`0x004ACD18`，也不是下一页按钮`0x004CADE0`。

初始化只把`+1C/+20/+3C`三个dword置全1，把`+42/+44/+46/+48`四个word
及`+90`dword清零；其余字节保持。`00451CD2`随后读取透明像素双字，
`00451CE5/00451CEF`才发布完整dword动作ID `0x2329`和基本variant `0x0C`，
最后`00451CF9`把该双字保存到`0x004AB8F8`。

原先把这个值称为运行句柄是错误的：`00423752`写入`0x026B`，
`00423773/00423778`按当前像素格式转换它，`0042378B..004237B3`取低word并
复制到高word后写回`0x004CD784`。普通显示初始化`00424CC8/00424CD6`也复制
同一个双字。现更名为`transparent_pixel_pair`及`read_transparent_pixel_pair`；
只纠正命名，保持字段位宽、枚举值、调用顺序及完整32位复制不变。
该值不是surface或其他资源句柄。SDL现有入口已用当前像素格式构造同一双字，
完整startup端口仍需接通。
该独立更名的完整代码差异只有标识符、注释和断言文案变化，未改变表达式、常量或布局。
定向core 1/1及SDL链接通过，日志为
`build/tmp/runtime/battle-startup-color-naming-{core,sdl}.log`；
core仅有既有结果测试137行窄化警告。未扩大测试范围或重复无内存行为改动的ASan验证。

核心通过必需的`battle_control_action()`借用真实记录，删除启动状态的两个私有副本。
旧初始化opaque槽保留为reserved且零调用；`read_transparent_pixel_pair`仅提供颜色快照，
不能承担动作初始化。SDL现有初始化入口直接修改
`world_dialog_runtime_state_.end_dialog_action`，不清整个对话状态、不改下一页动作
或世界光标。完整startup与SDL其余端口仍待接通。

独立测试用0、A5、FF污染记录及相邻记录，逐字节校验指定store、未写字节和邻居保留，
并在读取颜色的回调处验证只完成reset、尚未发布ID/variant。旧实现三项均失败，
红测日志为`build/tmp/runtime/battle-startup-control-action-red.log`。
修正后战斗聚合core/ASan各1/1及SDL链接通过，日志为
`build/tmp/runtime/battle-startup-control-action-{core,asan,sdl}.log`。
core/ASan仅报告既有结果测试137行的u16到u8窄化警告，SDL无warning/error。
SDL颜色来源使用既有`legacy_pack_color_pair(pixel_conversion_, 0, 19, 11)`：
RGB555的`0x026B`恰为这三个分量。转换器对两个相同word分别转换，等价于原初始化
先转换单个word再复制；函数只访问局部像素，转换格式只读，没有额外业务副作用。
计算放在按钮reset之后、ID/variant发布之前，双字写入放在发布之后。
原SDL清零整个战斗状态后没有写入这个颜色，本次补齐该写入。
新增五种转换的独立预期为`026B026B/04D604D6/04CB04CB/012B012B/026B026B`。
这些向量验证颜色生成；不是SDL整条初始化路径或实机像素差分。
颜色定向core/ASan各1/1及SDL链接通过，日志为
`build/tmp/runtime/battle-startup-color-binding-{core,asan,sdl}.log`，无warning/error。

逐字节测试验证核心借用与写入顺序；SDL接线目前只有源码追溯和编译证据，
不据此宣告实际按钮显示、完整战斗或续玩验收完成。

函数清零四字节presence表，但**不清零队伍总数**。它依次查询ID 30、31、32、33；仅返回严格等于1时写presence字节并递增旧总数。之后按累加后的总数门扫描四字节presence，把命中源索引顺序写入映射表。若本轮扫描耗尽仍无命中，先写来源索引4再退出；若已命中最后一槽3，则写3后直接退出，不追加4。

这意味着陈旧队伍总数可大于本次presence数；modern保留入口旧总数，后续只在首次真实越界映射/对象访问处typed-stop，不提前“修复”为本次计数。

查询ID`0xC9`返回非零时只置共享flags低字节bit1；查询`0x1BB0`严格等于1时把默认延迟`0x3C`改为`0x12`。

## 4. 窗口、几何与显示surface

顺序固定为：

1. snapshot窗口矩形；
2. 对固定word对象执行参数`0x10`初始化；
3. 两组三参数查询，低word分别发布；
4. 对固定几何owner与surface源调用已关闭`0x00433DC0`；
5. 写两组逻辑尺寸`320×200`并调用输出配置；
6. 调用`0x00451AE0`释放两个旧surface；
7. 调用`0x00451A90`，每槽均按height→width→create顺序；
8. EAX snapshot为`0xFFFFFFFF`，完成word按索引`0→1→2`发布。

modern直接调用`rebuild_legacy_battle_render_surface`；其typed-stop阻断后续原本会访问无效行表的路径。显示surface由typed token保存，零token仍占一次创建调用。

## 5. `battle.ffd`加载与唯一普通早退

低word battle ID先送入准备callee。随后原函数调用`0x0046E0B0`：按`0x200+battle_id*4`读取FIGTALK偏移，跳到`0x200+offset`，分配并清零固定`0x8000` bytes后读取脚本窗口。该函数已由`load_legacy_battle_script_window`直接承接，窗口只由`LegacyBattleAssets::script`持有，当前指针只保存窗口内offset；生产SDL入口通过总资产加载器保持FIGTALK先于FFD和setup的顺序，不保留旧地址或opaque端口。

之后路径固定为`data_root / "battle.ffd"`。已关闭archive header读取直接复用第106项唯一`0x31F4`绑定对象，以固定只读独占参数打开文件，把`0x2714`字节写入对象`+4`，并把`this+0x1F48`发布到固定scratch输出owner；打开失败仍关闭全1handle，读取返回或短读不形成成功门。旧高层`open_archive`端口已删除，仅三项Windows文件API保持窄平台边界。

无论archive header读取返回0还是1，caller都继续直连已关闭definition record读取。它以同一路径和同一绑定对象重新打开并读取头部，按battle ID低16位的signed计数、前序signed byte累计和variant零定位偏移dword，以`0x2714 + value*0x10C`回绕公式seek，再把固定`0x10C`字节读入启动状态唯一raw记录owner。打开失败、signed门拒绝或短读普通返回均不构成caller成功门，只有count或偏移表真实访问typed-stop阻断后续启动。

caller随后无条件从live raw记录按原物理offset投影definition；enemy count和secondary count只取低16位。旧高层`load_definition`端口已删除，文件open/read/seek/close统一复用同一窄平台边界。

若enemy count为零，函数调用固定失败文本token与低16位battle ID，然后立即返回该callee完整EAX。此前阈值、所有reset、队伍扫描、几何/surface重建和文件加载副作用全部保留；背景、角色和补位阶段均不执行。

## 6. 背景初始化直连

有敌人时先调用无偏随机`random(4)`，结果加1后只写低word背景资源号。随后把definition中的signed旋转除数、动作lowword、B4、B8、随机资源号传给已关闭`0x00451940`。

modern直接调用`initialize_legacy_battle_background`，不保留opaque entry。原caller忽略callee返回，因此普通image load失败仍继续；除零、命令流、旋转或动作缓存typed-stop则在原故障域阻断。返回后再次清零`0x26`项scratch块。

## 7. 敌方组B物化

enemy count按signed正数门进入循环，因已mask为u16，只有零跳过。每项固定：

1. 以基址`0x00525508`、步长`0x2B28`选择组B对象；
2. 调用pending对象reset；
3. 清零`0x29`个dword scratch；
4. 从definition按原stride读role、X、Y、mode；
5. 清record尾dword；
6. mirror mode严格等于1时调用actor mode并令X低word为`640-X`；
7. 直接调用已关闭组B行动配置`0x00475720`，依次执行双记录复制、资源加载、字段发布、profile加载与资源文本释放；
8. mode word严格等于1时调用额外模式callee。

固定记录表`0x005213A0 + index*0x20`由组B元素生命周期八槽唯一持有，startup不再保留role/坐标/runtime副本。caller只写LST实际覆盖的`+0x14/+0x16/+0x18/+0x1C`，保留未知20-byte前缀与`+0x1A`。镜像路径在actor mode返回后只用`mov cx`覆盖下一定义参数低word，因此ECX高word为callee陈旧snapshot；modern组合为`stale_high | role_low`并直接送typed配置。资源加载、profile或释放typed-stop保留此前reset、scratch、记录和mirror副作用，并阻断额外mode、随机进度与后续组A阶段；旧整函数opaque槽保留为reserved且零调用。

definition与组B都只有八槽。第九项在首次actor对象访问处typed-stop，保留前八项全部副作用。

## 8. 初始队伍组A物化

先按映射源把role写入步长32的队伍记录并置active=1。随后只在队伍总数严格为1、2、3、4时写固定坐标：

- 1人：`(527,287)`；
- 2人：`(490,275)`、`(555,370)`；
- 3人：`(504,272)`、`(565,353)`、`(462,224)`；
- 4人：`(526,298)`、`(497,277)`、`(464,217)`、`(588,359)`。

其他数量不写坐标。无论人数多少，caller都读取前四槽坐标计算八个signed偏移；缺席槽因此使用入口陈旧word，modern不得清零。

随后按组A基址`0x005029D0`、步长`0x2F34`逐项：reset；mirror mode为1时调用actor mode、令X=`640-X`并令对应X偏移=`624-old`；再以源索引派生固定`0x38`、`0x60`表token和placement token配置actor。actor mode查询严格等于1时递增陈旧byte计数，按u8回绕。

两个pending全局阶段后，再为每个初始队员依次调用profile、value、palette、name四个callee，固定表步长分别为`0x40`与`0x10`。组A reset同步清唯一profile token/kind owner，以及对象说明记录token与callee可见text-index owner；profile callee只有显式发布时写回profile owner，供已关闭动作摘要在原首次指针与kind访问点读取或typed-stop。说明owner供已关闭列表内容与网格列表帧在各自双矩形之后直连共享文字解析，不复制对象字段或共享文字buffer。

## 9. 三组x87比率

每名初始队员执行三组查询。每组均为：

```text
ratio = low_dword(fistp_qword_trunc((signed numerator / signed denominator) * 56.0f))
```

第一组使用i32；后两组只读取callee输出word并符号扩展。`0x00489654`把x87控制字改为向零后`fistp qword`，只返回低dword。零除、NaN、无穷或qword越界产生integer-indefinite，其低dword为零。

modern以80位`long double`执行同序计算，有限域向零转i64并取低32位，非法域发布零。每个ratio都复制到两张旧表；三个原numerator和最终actor首dword也分别保存。

## 10. 候选补位与陈旧分支word

固定候选查询ID为：

```text
34, 35, 38, 44, 45, 46, 47, 49
```

对应补位role为：

```text
3, 4, 10, 33, 34, 37, 38, 40
```

第一次八项扫描只对非零查询递增`word_53BF0C`，但该word入口**不清零**。分支判断使用“陈旧入口值+本轮命中数”的u16回绕结果；两条分支随后才清零word。

- 分支值大于2：反复`random(8)`，查询失败或used字节为1则无上限重试，直到成功加入两人；
- 分支值不大于2：顺序再扫八项，加入命中者，达到两人即退出。

每个补位记录写role、`X=750`、`Y=310`、active=1；mirror mode为1时X按低word变为`640-750`。随机分支`0x00452511`与顺序分支`0x00452646`都以固定参数1和首个Group-A actor直接组合已关闭`0x00478670` typed选择器，只读取首个actor `+0x00`的角色基础记录token；即使该token与live来源记录token相等，也不得按内容改选`+0x04`。selector返回后保持“先压入所选记录token、再压入当前新角色token”的materialization参数顺序；随后配置当前Group-A槽、激活，mirror mode为0时才调用actor mode。最后递增队伍总数与补位word；随机分支还写used字节。

两处selector的真实返回地址分别为`0x00452516/0x0045264B`。selector typed-stop保留此前候选role、坐标、mirror及基础token发布，阻断当前护援materialization、激活、剩余候选和全部startup后缀。旧seed端口ordinal改为reserved且生产零调用。随机重试不加modern上限，保持原非终止域；端口若违背`random(bound)`合同返回越界值，则在首次候选数组访问处typed-stop。

## 11. 最终阶段与返回

初始组A角色配置后的两个全局阶段均已回收。第一阶段对玩家道具链按u16 item id稳定升序，每次比较先清当前selected count，交换后从head重扫；第二阶段接收第一阶段EAX，依次稳定排序四个队伍道具sentinel链，不清selected count，交换后只重扫当前根。第一阶段typed-stop阻断第二阶段，第二阶段typed-stop阻断资料绑定和补位。

补位后固定调用三个pending全局阶段。每名敌人调用`random(6)`，结果为N就对该组B对象直接调用N次已关闭`0x004755E0`，固定参数零并使用本次动作阈值。第一次调用的EDX继承random callee；`0045274D..0045274F`在后续调用前将EDX替换为已完成次数的低16位，不继承前一次进度返回；整函数旧opaque枚举槽保留为reserved且零调用。资源typed-stop保留此前全部启动副作用和已完成迭代，并阻断组A随机进度初始化。之后按补位后的队伍总数，对每个组A对象直连已关闭`0x00478380`：以固定上界9调用第二套RNG，计算`300 + 150 / (random + 1)`，并只写角色`+0x2A12`低word。startup直接使用`state.party[index].progress`唯一owner；旧`finalize_party_actor` opaque槽改为reserved且零调用。角色进度写typed-stop保留当前RNG、商余数和此前角色写入，并阻断后续角色和正常尾部；固定组A owner越界也在完成本轮RNG与除法后才于原word写访问停止。

正常返回EAX按u32顺序计算：

```text
remaining = party_count
remaining -= final_subtract_word
remaining -= low16(supplemental_count_word)
```

再以unsigned比较：若陈旧party actor mode byte不小于`remaining`，把唯一共享战斗消息/阶段写`0x67`。相邻角色预处理关闭后，该dword与动作、效果和逐帧路径共用`LegacyBattleSharedPhaseStatePort`，不再保留startup副本。该写不改变EAX；无论条件真假都返回同一个回绕`remaining`。测试覆盖等于零时成立及正数域。

该循环的EDX输入已按调用点修正。callee仅在两条状态早退中原样返回入口EDX，
其他路径都会覆盖它；启动caller不消费这个返回。因此现有角色字段结果不受此修正影响。
该结论来自完整callee `004755E0..0047570E`及caller `0045270C..00452769`，
不以字段回归测试冒充寄存器动态差分。敌方记录偏移说明同时按实际store和结构静态断言
修正；结构布局与坐标写入代码原本正确，无须改动。
回归战斗聚合core/ASan各1/1及SDL链接通过，三份日志无warning/error：
`build/tmp/runtime/battle-startup-progress-register-{core,asan,sdl}.log`。
这些回归包含既有重复敌方进度调用及真实资源组合测试，未新增寄存器动态采集。

## 12. 双向追溯

- `0x00451B10..0x00451C53`：参数lowword、阈值、零/全1块与固定latch；
- `0x00451C55..0x00451D8A`：presence、控制块、四ID队伍扫描及映射；
- `0x00451D8A..0x00451E53`：flags、延迟、窗口、几何、逻辑尺寸和两个surface附件；
- `0x00451E53..0x00451EEC`：battle ID、`battle.ffd`及零敌人唯一普通早退；
- `0x00451EED..0x00452018`：随机背景、已关闭背景helper与组B物化；
- `0x00452018..0x0045227D`：队伍记录、1–4人坐标、陈旧槽偏移及组A配置；
- `0x0045227D..0x00452449`：已关闭玩家与四队伍道具排序、四类资料绑定与三组x87比率；
- `0x00452449..0x004526F2`：陈旧word门、随机/顺序两条补位路径；
- `0x004526F2..0x004527A5`：三个全局阶段、敌方随机动作与组A随机进度初始化；
- `0x004527A5..0x004527DB`：两次u32减法、unsigned门、可选`0x67`与EAX返回。

C++到LST反向追溯覆盖1351行、全部48个标签、60个静态call站点、两个邻接附件、唯一普通早退、所有循环和正常尾段。

### B11接线前的入口清零复核

重新核对`00451B33..00451CCD`发现两处旧实现和旧测试共有的错误：

- `00451C17`执行`rep stosd`时EAX仍为零，EDX才是全一。
  直到`00451C23`才执行`mov eax,edx`。因此`0x005242B0`起18个dword
  必须清零，后续三块18个dword和126个dword才写全一。
- `00451C86`写零到`0x0053C4B0`，不是组B数量`0x0053BCE0`。
  入口经共享目标选择状态清特殊行动计数；旧敌人数须保留到定义读取后发布。

新增测试使用0、非零高位及全一敌人数，反复进入同一启动状态。
控制块回调在定义读取前观察旧敌人数与已经清零的特殊行动计数；
相邻transition stage保留原值，定义为零敌人时才把敌人数改为零。
另污染整块`0x005242B0`，核对18项均清零。
修正前定向测试出现四条失败断言，构建脚本退出8，日志为
`build/tmp/runtime/battle-startup-reset-prefix-red.log`。
修正后战斗聚合core与ASan各1/1通过，SDL重新链接通过。
日志为`build/tmp/runtime/battle-startup-reset-prefix-{core,asan,sdl}.log`。
core与ASan只有既有结果测试第137行窄化警告；SDL没有warning/error。
实现直接借用目标选择状态，同一对象连续三次初始化均验证了写入时机。
这次修正不代表SDL完整初始化或再次进入战斗的实机验收已完成。

### B11接线前的队伍扫描复核

`00451D28`取得已经累加并按u32回绕的队伍总数，零值跳过整个映射扫描。
`00451D47..00451D5D`在未命中时递增来源索引，低word大于3便退出内层扫描，
但仍在`00451D74`写入来源索引4；随后`00451D7B`才结束外层扫描。
命中最后一槽3时，递增后同样直接退出，不会再生成一项4。
未写入的映射后缀与后六项相邻数据保持原值。

旧C++只发布命中项，在旧人数大于本轮命中数时遗漏上述索引4写入。
独立列出全部16种在队组合的路径，分别使用旧人数0、1、7、全一和全一减1，
共80组，覆盖总数回绕、内层耗尽、命中最后槽和未写后缀。
修正前定向测试退出8，日志为`build/tmp/runtime/battle-startup-party-scan-red.log`。
实现已按LST恢复“扫描、发布、递增、退出”的顺序。
修正后战斗聚合core与ASan各1/1通过，SDL链接通过，三份日志无warning/error。
日志为`build/tmp/runtime/battle-startup-party-scan-{core,asan,sdl}.log`。
本次为核心启动路径的定向验证，不代表SDL完整初始化或实机续玩已完成。

## 13. 验证与动态差分

定向合成测试覆盖：

- 所有固定reset块、18条混合宽度记录和四个控制switch；
- 完整ID与低word分离、flags/延迟查询、窗口、几何和两个surface释放/创建；
- surface完成word正序写与全1EAX snapshot；
- archive真实路径、固定文件API参数、无效handle关闭、完整/短头部读取、signed计数/variant/前序累计、偏移dword和`0x10C`记录、固定owner/scratch/目标、打开失败仍继续raw投影、定义typed-stop阻断、variant零及零敌人callee EAX早退；
- 背景image load零返回仍继续；
- 两名敌人、共享32-byte组B记录、未知前缀保留、镜像低word、陈旧ECX高word、三callee直连、旧行动配置opaque零调用及资源加载stop；
- 1–4人全部固定坐标和缺席槽不改写；
- 三组比率、负比率与零除integer-indefinite低dword零；
- 玩家与四队伍道具升序、差异化selected count、陈旧EAX和双阶段排序停点；
- 顺序补位、陈旧word触发随机补位、重复随机候选重试；
- 两处补位caller固定参数1、首个Group-A actor基础记录、真实返回地址、token动态别名、selector末尾RET停止对当前materialization与剩余startup后缀的抑制，以及旧seed端口零调用；
- 敌人随机动作次数、补位后组A固定上界9随机进度、完整商余数、进度高word保留、写入停点、旧opaque零调用、u32尾减法和`0x67`门；
- 第九名敌人在前八名副作用后typed-stop；
- battle聚合目标零warning，普通定向与独立ASan定向均`1/1`通过。

`battle.ffd`头部与definition记录读取现分别由`audit_order=107/108`关闭并从caller直连，高层archive/definition加载端口已全部删除；角色、AI和其余全局阶段callee仍各有后续工作包。原版Windows文件handle、完整归档对象与raw记录、全部共享表、18个角色对象及其首双token、异常栈/字段页、窗口surface、随机状态与后续状态联合捕获后端缺失，`original_diff_verified`为`blocked_runtime_oracle`。
