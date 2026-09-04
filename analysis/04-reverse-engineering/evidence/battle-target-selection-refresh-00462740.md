# 战斗目标选择状态刷新 `0x00462740`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00462740..0x004640F1`，从proc到endp共2820行、1662条实际指令、95个静态call、140个跳转、110个局部/默认标签、37个`retn`，没有外部`FUNCTION CHUNK`。函数后的两张间接表也已完整审计：主表压缩200个message为20个有效状态和一个默认槽；message 1的动作表压缩36种动作及默认槽。

唯一静态caller位于已关闭目标选择进入函数。95个call覆盖光标定位、菜单面板、角色提交/刷新、目标校验、三类角色属性查询、目标回退、样本、效果记录、两组角色清理及选择显示刷新。

## 2. 公共入口与主跳表

入口只在target-ready完整dword等于1时执行；否则返回该dword并保留caller ECX/EDX。通过后清selection runtime gate，EAX按u32计算`message-1`。message 0或大于200直接返回并保留caller ECX；200项表内默认message返回压缩selector 20。

有效主状态为1、2、3、4、5、7、8、27、30、98、100–104、110–113、200。实现保留每个状态进入时的压缩ECX、未被覆盖的caller寄存器，以及`jl/jle/jb`各自的signed或unsigned域。

## 3. message 1与动作子跳表

selection input gate为0或animation frame B小于signed 6时直接返回；越过阈值后清输入gate、目标阻断和目标缓存，置target gate，并保存原action kind。action大于等于signed 6时，从`0x004FE5C0 + action*2`读取word重映射；非零才替换action kind。

该物理读取跨越独立前缀、既有startup dword/word、十dword角色记录token块和后缀字节。typed owner按真实地址拼接，不复制已存在的startup存储；动作37在第一个未审计相邻word读取停止。

动作表保持原分组：

- 1–4进入普通目标、分类列表、装备分类和窄列表，按原顺序设置光标、目标映射与面板；
- 5、22、28、32、34–36写共享动作workspace、刷新角色并发布提交/角色五dword记录；
- 6、21、23、29、31、33回到message 3；17进入message 6；24同时置目标阻断；25只置动作模式低byte bit；26清辅助选择；
- 27和30打开对应grid面板。

动作完成后，仅pre-frame gate B为0且live共享message为3时直连已关闭组B目标轮转，再直连已关闭输入记录预置并把animation缓存写回`4/0/0`。目标轮转typed-stop保留动作与message前缀并阻断记录预置。其余13个原调用点也按各自原顺序统一直连：四项写复用主帧输入归一化records唯一owner，原opaque刷新槽保留reserved数值且零调用。动作callee可改写queued角色，因此每次实际角色call和后续记录store都动态重读owner。

## 4. message 2、4、8、27、30：候选与属性链

message 2先消费hovered category；否则在输入gate、双候选gate、候选参数和queued角色通过后，依次执行主动作验证、目标查询、属性A/B/C。属性A低word进入角色五dword记录，属性B可置目标缓存，属性C决定立即目标、默认回退或alternate回退。alternate剩余量unsigned至少4时直连已关闭组A目标轮转：复用连续八dword候选表的后五项物理视图，按queued角色零基值无上限匹配并发布一基目标。

message 4先消费hovered equipment；普通路径以current equipment校验目标。校验失败时按`0/2/3`与callee输出word决定是否显示提示，随后播放失败样本。校验成功且特殊override为1时，低word非零提示失败；高word1/2进入message 5的两行选择；全零直接提交动作15。其他结果复用属性链。

message 8固定以类别0/模式1查询；返回AX全1时直接返回。message 27固定类别4，成功后执行默认目标准备。message 30先以signed grid selection与u16行上限比较，超限或类别5校验失败都播放失败样本；成功后进入属性链。

角色五dword记录不增加十角色现代上限；group-A token、记录token与callee参数全部按u32公式生成，在首次真实对象call或记录访问停止。

## 5. message 3、5与7：提交、效果和目标轮转

message 3在published角色有效且queued/cleanup条件满足时提交当前action，清queued角色并刷新对象。动作30先写动作3、解析效果值，再按override决定动作13或停止旧效果；动作4调用角色特殊处理。随后按live group-B count清对象，再固定清四个group-A对象和四个marker；原循环不加现代上限。

清理后重置动画、grid和三项cache并刷新显示。cleanup gate为1时重新查询已提交角色；返回1则改写动作5、应用并刷新，再写角色五dword记录。末尾只清既有三个startup dword与共享word。

message 5提交动作15，把目标效果低word、行选择、高word偏移和group-A count按原u32公式组合为效果记录索引。效果记录直接复用`0x0053AF30`动作workspace的word/dword物理视图；按行保留Y坐标270/340、旧样本停止、镜像X和样本启动顺序，并把最终低word写入按actor code索引的结果word。

message 7未到alternate limit时固定动作25，并在pre-frame gate B为0时直连已关闭组B目标轮转；该函数按target cursor和live group-B count跳过完成对象，恢复原选择重置、一基published actor与耗尽message 1行为。随后把共享message写3、动画缓存写`0/0/4`并预置输入记录。count大于物理八对象时在第九次真实对象call或第九项target map读取停止，不添加步进上限。到limit时提交动作99并清选择缓存。

## 6. message 98–113与200

message 98清四byte transition control、stage、timer和aux byte，发布message 99与actor byte全1。message 100、102、103以signed `>=20`推进，101以transition state非零且timer signed `>=30`推进；101只替换EAX低byte为actor byte，保留EAX高24位。actor byte全1时按可选word播放样本并进入102，否则播放另一固定样本并进入110。

message 104无条件置completion并清stage。message 110按transition mode分支；mode 1把actor byte按i8符号扩展后查询group-A对象，必要时设置模式，再播放样本进入111；其他模式清状态并回101。111–113只在signed timer大于20时执行，且三条路径故意清不同状态并分别写101、112、113。

message 200先发布完整重置前缀，再按live group-B/group-A count清对象；随后清动画/cache/target-ready、刷新显示并按cleanup gate保存published角色。两组循环只以原signed count控制，在首次真实越界对象call停止。

## 7. typed owner、caller回收与验证

新增target-selection runtime owner只承接尚未建模的物理状态；完整LST交叉扫描确认`0x0053BCEC`继续是既有共享message owner，本函数、战斗启动与最终角色步进均复用该单一存储，global reset按原写集合清零。已提交角色改为复用debug状态内`0x0053BD50`唯一owner，不再保留runtime副本；message、queued/published角色、动作workspace、角色五dword记录、菜单缓存、计数、镜像、补充人数与startup尾dword继续复用既有唯一owner。`0x0053BFBC`统一复用final-actor pre-frame gate B，选择帧不再保留第二份suppression存储。global reset按原234项写程序只清真实覆盖字节：未被reset写到的重映射、transition和数组尾部保持原值。

目标选择进入函数原刷新槽保留相同枚举数值并改为reserved，ready不足或queued短路时直接调用本实现。默认组B目标和alternate组A目标两个原opaque槽也各自保留reserved数值；全部reserved槽生产代码零调用。两处提示文字已在原调用位置直连共享文字消息入链，并复用启动状态唯一链头与动态节点owner；原文字槽只保留reserved数值。各类子typed-stop均按原caller返回并阻断各自输入、message或动画尾路径。

定向测试覆盖主跳表默认域、message 1阈值与物理重映射、live共享message门、组A/组B轮转及子typed-stop、动作5前缀停点、hovered 2/4、message 3提交与第九个group-B对象、message 5效果物理视图、message 7完整轮转/发布/记录尾部与第九项target map、message 8/27/30、98/101的AL行为、100/102–104、111–113的阈值差异、110符号扩展、200重置前缀、global reset字节范围以及唯一caller传播。

## 8. `0x00478330`八处目标选择写入

工作包278关闭`0x0046292B`、`0x004629F3`、`0x00462B02`、`0x00462ED5`、`0x00463108`、`0x00463335`、`0x00463A50`、`0x00463CF0`八处物理call。各分支都把完整dword `1`写入当前组A角色`+0x2AE4`，并直接复用最终角色状态中的availability owner。caller按原路径把目标码、角色索引或相邻callee残值线程化为leaf入口EDX；任何写停止都保留已到达的message、选择与缓存前缀，EAX为1、ECX为角色token，且阻断相应动作提交、轮转、动画与输入尾部。

当前缺少原版两组角色对象、22类callee共享副作用、动态栈scratch地址、target map/效果workspace完整动态内容、transition动画后端及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
