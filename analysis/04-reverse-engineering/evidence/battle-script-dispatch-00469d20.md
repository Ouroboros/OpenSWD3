# 战斗脚本逐帧分派 `0x00469D20`

状态：`platform_adapted`。85路行为、机械矩阵、typed实现、唯一caller、完整门和inventory双生成均已收敛。

## 1. 完整权威范围与ABI

权威LST主体为`0x00469D20..0x0046DF35`，从proc到endp共7454行、4669条带机器码和真实助记符的实际指令、256个静态call、285个跳转、328个局部/返回标签和102个返回点，没有外部`FUNCTION CHUNK`。唯一caller是顶层逐帧协调器`0x0040A570`。

函数无显式参数。入口从`0x0053CE84`读取脚本指针，按脚本首个i16执行`inc`后形成`-1..83`共85项分派。脚本值`0`、`7`、`32`、`38`以及超出`-1..83`的值均走默认出口，不推进脚本指针并返回1。

返回值是顶层四值战斗合同的直接来源：普通等待或继续返回1；结束路径可返回0、2或3。函数保存并恢复EBX、EBP、ESI、EDI；每个callee边界前后的EAX、ECX、EDX必须按原调用点独立保存，不能用统一伪返回覆盖。

## 2. 分派结构与共享状态

脚本物理由`LegacyBattleAssets::script`提供固定0x8000-byte窗口；`LegacyBattleScriptWorkspace::cursor`是`0x0053CE84`的唯一typed cursor owner。实现只以窗口内offset承接指针，任何操作数或变长文字只在原始首次读取点检查，越界时保留此前副作用并typed-stop。

本函数引用138个可写静态状态位置，另有4个固定只读地址/token操作数。角色组A、角色组B、战斗消息、输入选择、攻击顺序、奖励、frame coordinator和startup/global reset继续复用既有owner；`LegacyBattleScriptWorkspace`唯一承接`0x0053CCC8..0x0053CEB8`脚本工作区，`LegacyBattleScriptSharedState`只承接尚无既有owner的函数级共享状态，不复制已关闭物理数组。

69个唯一游戏callee中，角色顺序、组B顺序和攻击顺序插入直接组合既有typed实现；需要平台对象或尚未审计内部业务的角色动作、对象、文件、画面和脚本服务统一经过`LegacyBattleScriptDispatchPort`窄合同。port显式携带对象token、参数、EAX/ECX/EDX和返回寄存器，不暴露原函数地址，也不提前实现`pending_audit`callee内部副作用。

## 3. 公共出口

- 默认出口`0x0046DF2B`：不改脚本指针，返回1。
- 公共帧出口`0x0046DF20..0x0046DF35`：先把入口EBP写入帧门，再调用`0x00453200`，随后返回1。调用返回寄存器在该公共出口不可见。
- 结束出口`0x0046DEB7..0x0046DF1F`：按live signed组B数量逐项调用角色清理，再按live signed组A数量逐项清理；随后清等待word、调用全局重置和脚本收尾，返回0。两个循环每轮重读live数量，不使用入口快照。

## 4. 已审计case：`-1`、`0..10`

### case `-1`

严格执行结束出口。组B基址、尺寸和数量分别为`0x00525508`、`0x2B28`和live signed计数；组A分别为`0x005029D0`、`0x2F34`和live signed计数。零或负数跳过对应循环。清理顺序固定为组B、组A、等待word、全局重置、脚本收尾，最后返回0。

### case `0`、`7`

均走默认出口。不读取后续操作数，不推进cursor，不调用帧协调，返回1。

### case `1`

`0x0053CCDC`低word是带bit15初始化位的等待状态。

1. bit15为0：读取`script+2`的u16，先发布原值到`0x0053CCE4`，再把该值与`0x8000`按u16或后写回等待word；不推进cursor，返回1。
2. bit15为1且低15位为0：清等待word，cursor按u32前进4，设置脚本完成门；若暂存actor非零则发布到共享queued actor，再清暂存actor并返回1。
3. bit15为1且低15位非零：先设置帧门并调用完整战斗帧。若返回1，立即走默认出口，等待word与cursor保持不变。若返回非1，则按case `-1`相同顺序清理双方角色、全局状态和脚本收尾；完成后cursor前进4、清等待word并设置完成门，最终原样返回帧协调结果，因此2/3不能折叠成1。

### case `2`

这是带`%Q`终止标记的动态76-byte命令对象。

首次进入由工作word的bit15判定。bit15为0时先读取`script+2`的actor code并发布文字起点`script+2`，随后申请76 bytes并立即以19个dword清零；零分配并不被原函数拦截，首次`rep stosd`访问点才是typed-stop。actor code按`<=7`组B、`>7`组A分别解析，先读取actor坐标，再对整组10个同类角色执行动作清理，并对目标actor执行初始化。随后按原顺序写动态对象的flags、文字参数、坐标与类型，把工作word bit15置位，清消息状态并设置选择缓存门。

活动阶段先检查共享列表门；非零时跳到公共帧出口，保留此前对象和工作位。门为零时依次清理组A十项动作、清理组A十项第二动作、清理组B十项动作。之后从已发布文字起点逐byte扫描`0x25,0x51`即`%Q`；最多检查offset `0..254`，即使没有标记也按offset255后的两个byte形成新cursor，不增加现代失败上限。cursor和文字起点均更新为标记后位置，32-byte临时文字缓冲按8个dword清零，帧门置1并进入与case44共享的动态命令逐帧路径。

### case `3`

先把帧门写0，调用完整战斗帧；无论callee返回什么都把cursor前进2，再把帧门写1并固定返回1。原callee返回值不传播。

### case `4`

先把`script+2`按i16符号扩展写入共享帧参数，再调用完整战斗帧；cursor前进4并固定返回1。

### case `5`

读取actor code、signed X增量和signed Y增量。actor code按`<=7`组B、`>7`组A解析；先调用坐标读取，把两个signed增量分别加到live X/Y，再调用坐标写入。cursor固定前进8；随后清四个共享坐标word/dword，发布状态word 1，依次重建角色指标、组A顺序和组B顺序。只有共享阶段门精确为1时额外调用完整战斗帧。最后帧门写1并返回1。

### case `6`

使用`0x0053CCD4`的bit15和低15位实现逐帧倒计时。

1. bit15为0：把`script+2`按i16符号扩展后只保留低16位并置bit15，帧门写0，调用完整战斗帧；不推进cursor，返回1。
2. bit15为1且低15位为0：清整个状态dword，cursor前进4，帧门写1，再调用完整战斗帧并返回1。
3. bit15为1且低15位非零：完整32-bit值减1写回，调用完整战斗帧；不推进cursor，返回1。

减法不是只减低15位，必须保留bit15和高word原值的32-bit结果。

### case `8`

先把cursor前进2、帧门写1并调用完整战斗帧，然后把新cursor处的i16当作actor code、`+2`的u16与常量1比较、`+4`的i16作为signed位置上限。第二word不等于1时立即默认返回且保留已经发生的cursor前进和帧调用。

actor code按`<=7`组B、`>7`组A解析并调用live位置查询。若当前位置大于signed上限，cursor从当前记录再前进10并返回1；否则cursor前进6，读取下一opcode并调用脚本准备callee，返回1。两条正常路径对cursor的总推进分别为12和8 bytes。

### case `9`

读取两个actor code并建立目标关系。首actor `>7`走组A主动方路径：保存旧queued actor和首actor，第二actor加1后发布；若第二actor也`>7`，设置组A目标门并在共享10项表对应槽写1。随后初始化主动组A角色，把全局动作状态和按actor索引的状态写1。

首actor `<=7`走组B主动方路径：先清首actor对应u16目标槽，再从第二actor开始寻找第一个callee返回非1的可用目标。第二actor`>7`时按组A live数量循环，数量每轮重读，并要求两个live actor字段都等于1后才查询可用性；第二actor`<=7`时从该索引递增到7，同样跳过callee返回1的目标。成功或越过上限后写回首actor目标槽；后一路还设置两个共享选择门。两路最后都把目标槽高byte按原顺序或入`0x80`、按固定参数插入攻击顺序记录。

函数随后把帧门写0、调用完整战斗帧、再把帧门写1，cursor固定前进6并返回1。所有搜索循环保持原live数量和原无界/上界条件，不增加重试上限。

### case `10`

工作word bit15为0时读取actor code并调用角色状态初始化。actor code `>7`使用组A：调用组A服务，发布`actor-8`到共享word，设置组A和共同选择门，清一个选择状态；actor code `<=7`使用组B：调用组B服务，原actor code同时写共享word和按code索引的dword槽，设置组B和共同选择门，清一个选择状态。两路都把工作word置bit15；若入口已置bit15则跳过全部初始化。

随后无条件调用完整战斗帧，cursor前进4，清工作word与两个选择门，帧门写1并返回1。

## 5. 已审计case：`11..20`

### case `11`

工作word bit15为0时按actor code初始化一次动作。组A依次调用动作准备参数0、动作代码17、以`script+4`传参的动作提交，并插入类型1的攻击顺序记录；组B执行对应三项调用并插入类型2。初始化尾把工作word或入bit15、清完成门，随后帧门写1并调用完整战斗帧。

bit15已置时只检查完成门：不等于1则直接调用完整战斗帧且不推进cursor；精确为1时cursor前进6，清工作word与选择缓存门，不再调用帧协调。完成门必须live读取。

### case `12`

先按live signed组B数量扫描每项共享dword状态，直到遇到首个不等于`0xFFFFFFFF`的值。命中时立即调用完整战斗帧，设置内部等待word为1，不推进cursor。零或负数量以及全部为`0xFFFFFFFF`时才继续。

完成路径若group-B bypass门为0，则发布组B数量低byte、清相邻byte，并仅在消息状态既不等于98且signed小于99时清选中角色清理门并把消息强制写99。随后cursor前进2，把bypass门和帧门写1并返回1；该路径不调用帧协调。

### case `13`

读取actor code和signed Y增量，按`<=7`组B、`>7`组A解析；先读取live X/Y，只对Y做signed wrapping加法，再写回坐标。cursor前进6，清共享坐标工作区，重建角色指标，调用完整战斗帧并返回1。

### case `14`

这是以`0xFFFF`终止的变长actor列表。首次扫描从`script+2`开始逐个u16计数，终止项不计入；计数和两个循环索引均为u16并保留回绕。计数为0时进入共享收尾。

非空时外层逐项解析actor code，按组A/组B地址调用资格查询。callee返回1时递增内层索引；内层达到计数时，cursor跳到`old_cursor + count*2 + 4`，读取该处i16并调用脚本准备callee。外层索引和内层索引的比较均保持u16无符号条件。

共享收尾只有在内层索引不等于总数时才把cursor推进到`old_cursor + count*2 + 8`；相等时清计数和两个索引但保持当前cursor。这个不对称分支必须原样保留，不能统一跳过整条变长记录。

### case `15`

读取actor code和u16参数，帧门先写0。按`<=7`组B、`>7`组A调用同一类角色服务。仅组A路径在`script+4`的bit13非零时把共享u8计数加1；该u8按原回绕。cursor固定前进6，帧门写1并返回1，不调用完整战斗帧。

### case `16`

使用工作word bit15和低15位执行带脚本回调的逐帧计数。

首次进入把`script+2`读为u16、置bit15，再以完整32-bit ECX减1后只写回低word。随后若低15位非零，调用脚本准备callee并传入`script+4`的i16，再把工作word按u16减1，调用完整战斗帧且不推进cursor。低15位为0时cursor前进8、清工作word，再调用完整战斗帧。两路帧门均从0恢复为1并返回1。

### case `17`

cursor前进2并返回1，不改其他状态。

### case `18`

读取actor code和u16参数，按`<=7`组B、`>7`组A调用对应角色服务。仅组A路径在`script+4`的bit13非零时把case15共享u8计数减1；该u8按原回绕。随后cursor固定前进6，帧门写0，调用完整战斗帧，再恢复帧门1并返回1。case15不跑帧而case18必跑帧的非对称必须保留。

### case `19`

入口先把cursor前进2。若共享列表计数word为0，则从新cursor按二word步长扫描：每轮读取`cursor + index*2`的u16，`0xFFFF`终止；非终止时索引加2、计数加1。索引和计数均按u16回绕。

计数精确为1时把新cursor首个i16传给脚本准备callee。其他计数先以完整低word调用共享随机选择，返回值左移一位后作为signed i16索引读取候选并调用脚本准备callee；计数0也保留这条原路径。最后清临时word、计数和索引，帧门写1并返回1。后续脚本位置由callee副作用决定，本函数不另行跨过列表。

### case `20`

读取actor code，按`<=7`组B、`>7`组A解析后以固定参数1调用对应角色服务。cursor前进4并返回1，不调用帧协调。

## 6. 已审计case：`21..30`

### case `21`

与case11共用工作word bit15和完成门。首次进入按actor code初始化动作：组A发布actor、设置对应五dword表首项为1，依次调用动作代码11和参数提交，再插入类型1攻击顺序；组B执行对应动作准备、代码11和参数提交。两路均置工作word bit15、清完成门，帧门写1并调用完整战斗帧。

bit15已置时，完成门不等于1则直接调用完整战斗帧；精确为1时cursor前进6，帧门写0，清工作word与选择缓存门，再调用完整战斗帧并返回1。这里完成路径仍调用一帧，与case11不调用的行为不同。

### case `22`

把`script+2`按i16符号扩展为X增量。先按live signed组A数量逐项读取坐标、以u32回绕加到X并保持Y，再写回；每轮重读live数量。随后对组B执行同一流程并同样每轮重读数量。零或负数量跳过对应组。cursor前进4，清临时word并返回1，不调用帧协调。

### case `23`

读取三项signed actor code：`script+2`是要写目标槽的actor，`script+4`决定主动方分支，`script+6`是候选目标。

`script+4 > 7`走组A主动方：保存旧queued actor，发布主动actor；候选目标`>7`时设置组A目标门并写共享五dword表。随后初始化主动actor，调用目标数据准备、资源/状态查询，并按返回低16位更新五dword表；两个后续查询分别控制选择目标缓存和输入阻塞门。最后发布动作种类2和按actor索引的状态2。

`script+4 <= 7`走组B主动方：先清`script+2`对应u16目标槽，再从`script+6`开始寻找首个可用目标。候选`>7`时按live组A数量随机或顺序推进，只有两个live actor门均为1才查询可用性；候选`<=7`时递增到7并跳过callee返回1的项，每次递增同步发布第三个共享signed操作数。最终写回目标槽，设置两个选择门，给目标槽高byte或入`0x40`，在原调用点直接组合已关闭组B行动对象资料，再插入类型2攻击顺序。资料组合typed-stop保留三个共享操作数、候选扫描、目标槽和选择门前缀，阻断攻击顺序、frame与cursor后缀。

两路汇合后帧门写0，调用完整战斗帧，再恢复帧门；cursor固定前进8并返回1。所有候选搜索保持原live数量、随机调用位置和上界条件。

### case `24`

按live signed组B数量逐项清理，再按live signed组A数量逐项清理；每轮重读数量。随后调用全局重置，并以`script+2`的u16调用完整战斗启动协调器。函数返回1但不在本地推进cursor；新脚本指针由启动callee负责发布。

### case `25`

若成长样本计数word大于0且完成门为0，则把战斗消息写102；否则把完成门写1。随后把live组B数量低byte写发布byte、清相邻byte，设置group-B bypass门，帧门写0并调用完整战斗帧。

帧返回任意非零值时立即默认返回1，保留bypass门和cursor。只有帧返回0时才清bypass门、cursor前进2并返回1。原帧返回2/3在此处不向顶层传播。

### case `26`

与case21共用初始化尾和工作word，但动作代码固定12且不插入攻击顺序。首次进入按组A/组B调用准备、动作代码12和参数提交，置bit15、清完成门并调用完整战斗帧。bit15已置且完成门不等于1时继续调用帧；精确为1时cursor前进6，清工作word、选择缓存门并把帧门写0，不再调用帧协调。

### case `27`

读取actor code和u16参数，按`<=7`组B、`>7`组A调用对应角色服务。cursor前进6并返回1，不调用帧协调。

### case `28`

读取actor code及三个u16参数。组B仅以三参数调用角色服务。组A先调用对应三参数服务，再读取live坐标并写入按组A索引的坐标表；随后调用另一查询，把返回低16位发布到五dword表，并调用一项双word输出查询，将其第二输出按完整i16符号扩展写到按原actor code索引的dword表。cursor固定前进10，清共享word并返回1。

### case `29`

读取actor code并以固定参数1调用对应组A/组B服务。仅组B路径在callee返回后把共享byte加1，按u8回绕；组A不修改该byte。cursor前进4，清工作word并返回1。

### case `30`

帧门先写0，以`script+2`的u16和固定画面source token调用音频/画面请求callee，再调用完整战斗帧。cursor前进4，帧门恢复1并返回1；帧callee返回值不传播。

## 7. 已审计case：`31..40`

### case `31`

入口先把cursor前进2，再从新cursor按二word步长扫描：读取`cursor + index*2`的u16，`0xFFFF`终止；否则索引按u16加2、总数按u32加1。若外部选择值加1大于总数，原程序弹出固定错误消息并直接返回0，不清计数或索引。否则以`cursor + 外部选择值*4`读取i16并调用脚本准备callee，随后清总数和索引、返回1。callee负责后续cursor，函数不跨过列表。

### case `32`、`38`

均走默认出口。不读取后续操作数，不推进cursor，返回1。

### case `33`

申请180 bytes，立即按45个dword清零并调用对象构造；零分配在首次清零访问点typed-stop。对象字段依次接收`script+2/+4/+6/+8`，其中两个位置word原样保留，显示X初值固定`-120`，若目标X大于320则改为760，Y初值0。最后把旧链头写入新对象`+0xB0`并发布新链头，cursor前进10，返回1。

### case `34`

遍历case33共享单链，按对象`+0`等于`script+2`且`+8`等于`script+4`双条件匹配。未命中时cursor前进6。命中时先把对象`+0x9A`写`0xFFFF`；若其signed `+0x98`大于320再覆盖为1。随后cursor同样前进6。未知链token在首次字段访问点typed-stop。

### case `35`

把共享工作dword最低byte或入`0xA0`，cursor前进2并返回1。高24位保持不变。

### case `36`

`script+2 >= 0x100`时仅以`script+4`和固定两个token调用空服务，cursor前进16。

`script+2 < 0x100`时先按节点`+8`低byte遍历共享24-byte节点链。已有同类节点时把共享mask word写1，以`script+4`调用固定服务后cursor前进16。无同类节点时申请24 bytes并按顺序写：类型、参数、四个以清低bit方式偶数化的signed几何word。随后检查X、Y非负且`X+W <= 640`、`Y+H <= 480`；失败路径在原释放点释放当前EAX token，再读取暂存节点四个signed几何word并调用错误服务，最后仍cursor前进16。零分配或未知token均在首次真实字段访问点typed-stop。

合法几何两次都按`height*2`申请word数组；第二块并不使用width，不额外检查零分配。根据`script+6`精确1/2选择初始word和flag，其他值保留入口AX陈旧值。循环按signed height填第一数组、固定写第二数组值2，再把节点头插共享链。所有signed乘法、负尺寸和分配顺序原样保留。

### case `37`

`script+2 >= 0x100`时调用固定服务并cursor前进6。否则按节点类型低byte遍历case36共享链；未命中直接cursor前进6。

命中后根据`script+4`：0把共享mask写`0x2000`；1写`0x1000`；2先原位摘链，再依次释放节点两块数组和节点本身，然后cursor前进6。其他值不写mask，继续复用此前陈旧mask。非删除路径把节点类型低byte与mask按u16或后写回，再cursor前进6。陈旧mask和释放顺序不可现代化。

### case `39`

工作word bit15为0时读取`script+2`并置bit15，按低15位actor code解析角色并读取live坐标。以该坐标为起点，把`script+4..+22`的十个word依次转换为六段路径点，全部加法按u16回绕。原LST末组故意把同一X暂存值写入两个相邻点，并把最后Y写入三个相邻点；不能整理成对称数组。随后把帧序号设1、段号设0。

活动阶段段号`>=6`时cursor前进24，清工作word、帧序号、段号和坐标工作区，返回1且不跑帧。否则用当前段的四个word和帧序号调用插值callee，将输出坐标写回低15位actor；帧序号按u16加1，大于20时段号加1并把帧序号清0。每个活动帧随后重建角色指标、调用完整战斗帧并返回1。工作word bit15、低15位actor和六段×20帧时序必须保持。

### case `40`

`script+2`写目标word；若当前word为0，先复制目标。目标在signed `0..16`时按actor code读取live X，计算`trunc_zero((X+320)/2)`并把它写当前word，平移量为`320-current`。目标不在该范围时，以乘法常量实现当前signed word除3向零取整，把商写回当前word，平移量是商的相反数。

平移量为0时cursor前进4并清全部坐标/计数工作word，帧门写1。非零时先按live signed组A数量逐项读取坐标、以u16回绕加到X并写回，再按live signed组B数量执行同样流程；两组每轮重读live数量。随后重建角色指标、调用完整战斗帧，不推进cursor。signed除法、`X+320`回绕与两组顺序不得修改。

## 8. 已审计case：`41..50`

### case `41`

先清转场阶段门，以`script+2`的u16调用已关闭画面转场，cursor前进4，再把阶段门写1并返回1。

### case `42`

cursor先前进2并把新位置发布为文字起点，32-byte临时缓冲按8个dword清零。随后最多检查文字offset `0..31`寻找`%Q`。命中时仅复制标记前字节到临时缓冲并把扫描长度加2；未命中时保留全零缓冲和长度32。接着按u16索引0..3依次与四个固定16-byte名称比较；命中时以对应固定文字和临时缓冲调用服务，未命中不调用。

随后帧门写0并调用完整战斗帧，cursor更新为文字起点加最终扫描长度，清名称索引，帧门恢复1并返回1。命中时恰好越过`%Q`，未命中时总推进34 bytes；不得继续扫描到现代NUL。

### case `43`

把live组B数量低byte发布并清相邻byte，设置转场门1、消息99、清选中角色清理门、设置group-B bypass门1，帧门写0后调用完整战斗帧。cursor前进2并返回1；帧门没有在本case内恢复1。

### case `44`

无条件读取`script+2`作为actor code和文字起点，申请76 bytes并立即按19个dword清零；零分配在首次清零访问点typed-stop。随后构造文字动作对象，固定flags包含`0x800`、bit3、bit10，并或入case4发布的帧参数及actor code低16位；类型word固定2。

actor按`<=7`组B、`>7`组A解析。两路均读取坐标并清理整组10项动作；组B目标actor另以参数1调用一次清理。对象记录最终坐标，消息状态清0。随后再次清理组A十项与组B十项，从文字起点最多扫描255 bytes寻找`%Q`，无标记也使用offset255；cursor更新到标记/上限后2 bytes，32-byte缓冲清零，帧门写1，清选择缓存与坐标/工作word后返回1。本case不调用完整战斗帧。

### case `45`

跨调用镜像门精确为1时，只把cursor前进2、清镜像门并返回1，不触碰角色。否则先置镜像门1：按live signed组B数量逐项调用固定参数1的角色服务，把X改为`640-X`；再按live signed组A数量逐项根据actor `+0x2B04`是否等于1传0/1，调用同一服务，把X改为`640-X`，并把相邻独立X表改为`624-value`。两组每轮重读live数量。最后cursor前进2并返回1。

### case `46`、`47`

分别把`script+2`按i16符号扩展传给两个不同跨模块服务，随后共用收尾：cursor前进4并返回1。

### case `48`

把`script+2`按i16传给查询服务并暂存。返回精确1时cursor前进4，读取新cursor的i16调用脚本准备callee并返回1。返回非1时cursor直接前进8并返回1，不调用脚本准备。两条长度不可合并。

### case `49`

先把共享u8计数写`0xFF`，再把live组A数量清0，重建角色指标；cursor前进2并返回1。

### case `50`

把`script+2`按i16符号扩展直接计算组B对象地址并调用服务，没有`<=7`或非负检查；越界只在首次真实对象访问点typed-stop。保存callee AX后读取组B首对象坐标，再把该坐标写到同一`script+2`选定对象。cursor前进4，清临时word并返回1。

## 9. 已审计case：`51..60`

### case `51`、`52`

case51先只改入口ECX低byte为`script+4`并把完整陈旧ECX压栈，再以`script+2`的signed值直接寻址组B对象并调用服务；cursor前进6。高24位陈旧参数必须由入口寄存器快照保留。case52按actor code选择组A/组B，以`script+4/+6/+8`三个u16调用对应服务，cursor前进10。

### case `53`

以成长样本计数word作为未检查索引，调用已关闭玩家道具数量函数取得节点token，依次写奖励token数组、数量word加1、道具ID=`script+2`、样本计数word加1，最后cursor前进4。越界只在对应数组首次真实写入点typed-stop，且保留此前调用副作用。

### case `54`

读取三个signed actor code。第二项`>7`时直接cursor前进8。否则先清第二actor目标槽，再从第三项开始按case23相同的组A/组B可用目标规则搜索并写回目标；随后以第一actor调用组B准备服务，根据callee精确返回1给目标槽高byte或入`0x80`，否则或入`0x40`，再插入类型2攻击顺序。最后帧门写0、调用完整战斗帧、清三项临时值、恢复帧门1，cursor前进8。

### case `55`、`56`

case55把`script+2`按signed值减8，与`script+4`的signed值调用服务；仅返回精确1时设置成长过渡门，随后清临时值并cursor前进6。

case56把`script+2`作为组Bactor u16并写入packed状态高word。caller按`script+0x0E/+0x0C/+0x0A/+0x08/+0x06/+0x04`逆序读取六个word，形成原EAX/EDX局部覆盖、actor地址和`1381 * actor`寄存器前缀后直连已关闭typed函数。六个word分别对应资源`+0x66..+0x70`三组主行动道具选项参数：零值跳过actor和资源访问并保留旧word，非零才重读资源token并写入；前五项使用EDX，末项使用ECX。全零参数不得因actor越界提前停止。只有typed函数完成后cursor才前进16、EAX返回1并恢复入口ECX；脚本、actor或资源typed-stop保留真实前缀并阻断成功后缀，旧整函数opaque地址生产调用为零。

### case `57`

把共享控制dword最低bit置1，按固定五个全局参数调用已关闭背景初始化，cursor前进2并返回1。

### case `58`

首actor `>7`时保存queued actor并初始化组A主动方，按第二actor发布目标和组A门，设置动作种类与按actor状态1，cursor前进4。首actor`<=7`时，LST加载的两个固定函数地址均为非零，因此无条件短路原本的目标槽/攻击顺序块，直接cursor前进4；不得把静态不可达块恢复成运行路径。

### case `59`

这是case2/44的第三种76-byte文字动作。工作word bit15控制首次构造；首次进入除读取坐标外还调用一项角色方向/锚点服务，固定对象参数使用`0x10000`并写入额外坐标。它按组A/组B清理动作、扫描最多255 bytes的`%Q`、更新cursor并清32-byte临时缓冲。活动阶段在共享列表门非零时只跑完整战斗帧；门为零时清理双方动作并完成文字扫描。完成尾清工作word、坐标、选择缓存并返回1。

### case `60`

从`script+2`最多扫描255 bytes寻找`%Q`，把长度前缀复制到255-byte临时缓冲；无标记时使用offset255并仍越过后续2 bytes。cursor更新到标记/上限后。随后以不带边界的旧字符串复制/追加顺序构造`music\\`加脚本文字的共享音乐路径，依次停止当前流、以参数0启动新路径、提交固定音量对象。现代实现只在原字符串首次真实越界访问点typed-stop，不预先截断或扩大缓冲。

## 10. 已审计case：`61..66`

### case `61`

从`script+2`起按u16扫描，以首个`0xFFFF`终止并得到元素数；没有现代长度上限，越界只在首次真实脚本读取点typed-stop。随后逐元素按`<=7`组B、`>7`组A调用角色查询，保留两个u16循环计数。

若所有查询的AX都为零，cursor先定位到终止字后的首个word，以该word的signed值调用脚本准备callee，cursor保持在该word，三个计数全部清零并返回1。若任一查询AX非零，则不调用准备callee，cursor改为原位置加`2*count+8`，只清两个循环索引，保留元素数word并返回1。空列表走全假路径，仍读取并调用终止字后的word。两条cursor和清理不对称不可合并。

### case `62`

把`script+2/+4/+6`三个i16分别转为x87 float并发布为三项起点；把`script+8/+10/+12`三个i16同时保留原word并转为三项目标float。`script+14`先在已清高word的ECX中读取为u16，发布为正的分母/剩余帧数。

每项步进均按`(target - trunc_x87(start))/denominator`计算：起点先经已关闭`0x00489654`取得x87向零整数，再做整数差转float和x87除法。cursor前进16，三项目标word清零；第三项步进写完后显式弹出仍留在x87栈中的分母。原除零、NaN/无穷和x87舍入由既有兼容helper承接，不改成现代整数除法。

### case `63`

读取signed门`0x00520FB8`。值`<=0`时cursor前进2；值`>0`时cursor不动。两路随后都把帧门写0、调用完整战斗帧、恢复帧门1并返回1，因此正值形成原地等待。

### case `64`

读取actor code与一个u16参数，按`<=7`组B、`>7`组A调用同一角色查询。仅返回精确1时先清临时word并cursor前进6；返回非1时cursor保持。两路随后都直接调用完整战斗帧，再清临时word并返回1；本case不改帧门。

### case `65`

以`script+2`的u16代码查询玩家道具节点，`script+4`作为u16阈值。节点非零且节点`+8/+10`两个i16符号扩展之和大于等于零扩展阈值时，cursor前进6，清代码word，以新cursor的i16调用脚本准备callee并立即返回1；阈值word和节点token不清，且不调用完整战斗帧。

节点为零或signed和不足时，先调用完整战斗帧，再清代码与阈值word，cursor前进10并返回1。节点访问、signed求和和两条不对称长度必须保留。

### case `66`

同样按`script+2`查询玩家道具节点，但原版不检查零token便读取节点`+8`，因此零节点在首次读取处typed-stop。以`script+4`的u16数量先扣节点`+8`：现有量小于等于需求时该字段清零并把余量留在共享低word；现有量大于需求时按16-bit结果扣减并把余量清零。随后节点`+10`无条件按共享低word做16-bit减法，没有不足检查，可原样下溢。

仅当两个数量word都为零时调用节点摘除服务。之后调用完整战斗帧，清代码和需求低word，cursor前进6并返回1；共享需求dword高word及节点token保持陈旧。

## 11. 已审计case：`67..73`

### case `67`

cursor前进2，设置独立脚本门为1，发布cursor后调用完整战斗帧并返回1；不改帧门。

### case `68`

读取`script+2`的signed actor code和`script+4/+6`两个u16，按signed `<=7`组B、`>7`组A调用角色服务；负code仍走组B并在首次对象访问typed-stop。随后清三项临时值，把帧门写0、调用完整战斗帧、恢复帧门1，cursor前进8并返回1。

### case `69`

把共享控制dword的bit3置1，cursor前进2并返回1。

### case `70`

把`script+2`按u16直接寻址组B对象，以`script+4`作为服务参数，cursor前进6并返回1。参数通过只改AX低word后压入完整EAX，高word保持入口陈旧值；组B code没有范围检查。

### case `71`

先截取live组A数量dword低byte到独立byte，再把共享控制dword的bit4置1、消息状态写103，cursor前进2并返回1。

### case `72`

以`script+2`的i16、`script+4`的低word、`script+6/+8`两个i16及固定1调用已关闭背景初始化，cursor前进10并返回1。`script+4`只改AX低word后压入完整EAX，必须保留入口高word。

### case `73`

把`script+2`目标word减当前signed位置word，再以`script+4`的signed i16执行原生`idiv`，商发布为32-bit步长，当前位置加商的低16位；除零及`INT_MIN/-1`在原除法点typed-stop。帧门随后写0。

商非零时，按live signed组A数量逐项调用坐标读取服务、把X低word加步长低word、再调用坐标提交服务；组B同序处理，每轮重读live数量。之后重建角色指标并调用完整战斗帧，cursor不动、帧门保持0并返回1。

商为零时不调用角色服务和帧，cursor前进6，清坐标对、当前位置、附加临时word、参数word和另一位置word，恢复帧门1并返回1。

## 12. 已审计case：`74..83`

### case `74`

把`script+2`按u16直接形成组B对象地址，并把actor code写入共享packed状态高word。已关闭的typed函数以`script+4`为18-byte载荷首地址，只读取九个偶数byte，并按“重读动态资源token→读取一个脚本byte→写入一个连续资源参数byte”的原顺序更新资源`+0x92..+0x9A`；奇数byte不读取。前八项以EDX承接资源token，末项改由ECX承接资源token并只用DL覆盖EDX低byte；caller传入的初始EDX按原地址算式为`345 * actor`。

只有typed函数完整返回后，cursor才前进22、EAX返回1并由caller epilogue恢复入口ECX。actor、任一偶数脚本byte或资源写入typed-stop都保留packed actor及此前完成的参数写入，并阻断cursor后缀；旧整函数opaque地址生产调用为零。

### case `75`

把`script+2`按u16直接寻址组B对象，以`script+4/+6/+8/+10`四个word按原压栈顺序调用服务，cursor前进12并返回1。四个参数均由只改AX/DX低word的寄存器链形成，完整32-bit高word按各自调用前状态保留。

### case `76`

按u16 actor code的`<=7`组B、`>7`组A选择对象并调用位置服务。组A参数固定为`-9999,9999,9999`，组B固定为`-100000,0,0`；cursor前进4并返回1。

### case `77`

把共享控制dword的bit6置1，cursor前进2并返回1。

### case `78`

这是组A角色的多帧准备命令，默认增量6。阶段word为零时读取`script+2`的u16 actor并直接减8寻址组A，读取`script+4`的i16目标再加1发布。角色查询精确返回1时先提交`0x8000`状态并把位置设为`-9999,9999,9999`；无论查询结果，随后都清queued actor、设置目标、以参数6调用角色准备，清10-dword工作区和126-dword攻击顺序区，并把每个28-byte攻击记录首dword写`-1`。之后发布角色/选择门，调用目标对象服务，置异步门1并清阶段word。所有数组仍按原固定物理范围。

阶段word非零时跳过上述初始化。两路都把帧门写1并调用完整战斗帧；异步门仍非零时直接走default返回，cursor与临时值不动。异步门变零时cursor前进6，清阶段word和actor word并返回1。

### case `79`

读取actor及三个word参数。actor `>7`时减8寻址组A：先以`script+4`的i16、`script+6/+8`只改低word形成的完整寄存器值调用位置服务，再依次调用角色刷新、固定`0x235E`服务、以`script+4`的i16调用动作服务、以固定1调用状态服务。actor `<=7`时只对组B调用位置服务，第一参数改为`-signed(script+4)`，其余两个完整寄存器值保持相同陈旧高word链。最后清四个临时word，cursor前进10并返回1。

### case `80`

把`script+2`按u16直接寻址组B，以`script+4`的i16调用服务，cursor前进6并返回1。

### case `81`

把`script+2`与共享word比较。不相等时复用case54的早退尾，cursor前进8并返回1，不清共享word或临时actor。相等时cursor前进4，清共享word与临时actor，以新cursor的i16调用脚本准备callee并返回1。

### case `82`

先把独立状态dword写2，再读取`script+2`。值精确1时设置共享控制dword bit8、cursor前进4并返回1，状态dword保持2；其他值清bit8、cursor前进4、再把状态dword清0并返回1。临时word两路都不清。

### case `83`

设置共享控制dword bit9，随后物理落入case17共享尾：cursor前进2并返回1。

## 13. 机械矩阵关闭

从权威LST重新提取完整主体得到4680条机器码匹配；其中11条是长数据续字节被正则误判出的`FF/F8/A0/F2`伪助记符，剔除后为4669条真实指令。256个`call`、285个跳转、328个局部/返回标签和102个不同地址的物理`ret`全部位于本函数主体，没有外部chunk。

跳表的85个输入值逐项映射到typed `switch`：`-1..83`各出现一次，无缺失、无重复；原跳表中`0`、`7`、`32`、`38`四项明确走默认帧，范围外输入同样不推进cursor并返回1。所有102个物理返回点按case矩阵归并为三类：默认/普通完成返回1、结束清理返回0、case1完整帧非1路径传播2或3；没有第四类返回合同。跨case共享尾包括case17/83、case54/81及公共帧/结束出口，均按控制流而非物理相邻区间归属。

256个callsite归并为69个唯一游戏callee。每个case章节记录调用条件、调用顺序、参数域和返回寄存器消费；typed测试另证明默认出口零调用、动态对象零分配停止、case5顺序重建、case9攻击顺序直连、case48双cursor、case53先写奖励再停止、case61/62变长与x87、case65/66道具双数量、case73除零、case78异步等待以及结束清理顺序。

## 14. 物理状态唯一owner

绝对操作数机械提取共142个命名位置，其中138个是可写状态，4个是固定只读地址/token。owner按物理职责关闭如下：

- `LegacyBattleStartupState`及其`reset`承接两组角色、数量、攻击顺序50记录、`0x00520E90`共享表、战斗ID、背景、结果前置状态和启动清理块。
- `LegacyBattleActorMetricState`承接18项指标、actor order和组B顺序；case5直接调用现有顺序实现。
- `LegacyBattleFinalActorStepState`、`LegacyBattleInputDispatchState`、`LegacyBattleTargetSelectionRuntimeState`、`LegacyBattleMessagePhaseState`和`LegacyBattleVictoryRewardState`分别承接最终角色、输入、选择、消息和胜利奖励状态。
- `LegacyBattleScriptWorkspace`逐字段承接`0x0053CCC8..0x0053CEB8`：bit15工作word、cursor、六段路径、动态对象token、临时坐标、文字缓冲、三项float/目标/步进、列表计数和临时actor。
- `LegacyBattleScriptSharedState`承接剩余函数级控制位、动态76/180/24-byte对象存储、效果链、脚本异步门及未被既有结构持有的窄服务交换状态。
- 物理地址和动态对象地址全部保留为`compat::u32` token；脚本、对象、链和数组越界只在权威首次真实访问点typed-stop。

## 15. typed实现与唯一caller回收

`include/openswd3/battle/legacy_battle_script_dispatch.hpp`定义0x8000-byte脚本窗口、工作区、共享状态、窄调用ABI和停止状态；`src/battle/legacy_battle_script_dispatch.cpp`显式实现85个case。实现保留bit15跨帧状态、陈旧寄存器高位、u16/u32回绕、signed比较与除法、x87向零转换、变长扫描、动态链节点、释放顺序和每条不对称cursor路径。

唯一caller`0x0040A570`对应SDL `SdlSmokeIdlePorts::step_battle()`。旧固定返回1占位已删除；初始化阶段把battle setup的双方数量、角色资源与坐标发布到同一`LegacyBattleStartupState`，逐帧调用typed分派器并把0/1/2/3原样交回既有`run_frame_iteration`战斗恢复分支。typed-stop记录状态、opcode和offset后保持战斗活动，不伪装成成功退出。生产源码不再含`0x00469D20`函数地址或固定`step_battle`返回1边界。

## 16. 验证与动态差分

定向`battle.legacy_battle_setup`、独立AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过；新源码与SDL app目标零warning编译。inventory生成器连续双跑逐字节一致，正式计数为`161/422 = 154 platform_adapted + 7 assembly_exact + 261 pending_audit`，SHA256为`1ab13a95a22a4175d6663b6a2c0e5c185632078aed386bcd51c1763d664373ce`。

原版138项共享状态、动态对象地址、CRT随机序列、69个callee副作用、framebuffer/音频/文件服务及EAX/ECX/EDX联合捕获后端尚不可同时获得，因此`original_diff_verified`登记为`blocked_runtime_oracle`；这不改变完整LST静态审计、typed实现和现代侧门禁结论。
