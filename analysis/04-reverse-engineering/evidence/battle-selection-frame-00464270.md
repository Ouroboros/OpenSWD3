# 战斗选择帧 `0x00464270`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00464270..0x00464C6F`，从proc到endp共1154行、747条实际指令、45个静态call、73个跳转、49个局部/默认标签、12个`retn`，没有外部`FUNCTION CHUNK`。函数后的30项message压缩表也已审计：1–8、27、30映射十条有效路径，9–26、28、29和表内默认槽归并到selector 10。

唯一静态caller位于已关闭主帧协调器。45个call中，比例填充面板、纵向状态面板、prepared动作帧、角色目标准备、动作摘要、列表框、列表内容和网格列表帧八类已关闭callee已直连；展开后的其余对象、菜单、文字和选择callee通过窄平台端口保留。

## 2. 入口、完成角色替换与释放

message完整dword等于103时立即返回。否则读取queued角色；queued为0且target-ready不等于1时返回0。其他路径先以`queued-8`按u32计算group-A token并查询角色完成状态，code 0或一过物理十角色范围在首次真实对象call停止。

查询返回1时按原顺序：

1. 清五项选择指针token、message、三项cache gate、runtime gate；置target-selection gate和animation phase 5；
2. 处理当前queued角色；
3. 按live signed group-B count以模式0重置对象，不增加八对象上限；
4. 在`group-A count-1`范围扫描十槽actor order。空槽接收queued并清queued；候选查询不等于1时与queued交换；
5. 保留扫描后的ESI，供message 1文字索引继续使用。

随后无条件以live queued查询释放条件；返回1时先调用释放callee再清queued。所有这些副作用完成后才置selection display gate并进入message表。

## 3. message 1：比例面板、文字与动作摘要

queued为0直接返回。animation frame B不等于6时，runtime gate为0会从两项pointer origin减16/48发布面板原点；pointer activity signed不小于150或上一鼠标坐标落在`[50,570] x [30,360]`外时，把原点夹到`(290,160)`，越界鼠标路径额外调用anchor绘制。

frame B按signed域上限6，animation phase按signed域下限0。随后直连已关闭六级比例填充面板；typed-stop保留此前原点、gate和夹值。frame B signed小于6时递增frame并递减phase后立即返回，保留callee之后的寄存器覆盖顺序。

frame达到6后配置文字行与颜色。角色标签索引来自入口/完成替换流程留下的ESI，不从queued重新计算；十槽typed owner在首次真实索引读取停止。文字token按16字节步长生成，长度使用signed右移居中，再直连已关闭action摘要。摘要复用启动主/次颜色、九byte权限、三项动作代码/文字和静态动作token，恢复四固定行、三动态行、可用性清零与选中覆盖；子typed-stop保留标签绘制并阻断摘要尾。

## 4. message 2、4、8、27、30：列表与网格

五条路径进入时都置selection cache C：

- message 2先直连列表框，依次绘制三项基础动作框、当前分类框、底色与九宫格，并把frame A置10、frame B按signed域加2夹到7；仅完成后frame B等于7且frame A等于10时再直连最多七行列表内容，完成名称/数值、当前行双绘制、双矩形及底部说明。row-limit byte随后替换EAX低byte、按i8符号扩展到lower-panel auxiliary；signed大于7才直连纵向状态面板。任一列表框/列表内容子typed-stop保留cache C与已完成绘制前缀，并阻断auxiliary和cache A/B；
- message 4直连网格列表帧：五项顶部动作框后绘制面板，仅flags bit15行计入七行上限，当前行完成双矩形、共享说明和输入发布；隐藏行只推进iterator且不增加modern扫描上限。正常返回才以u16 row limit发布auxiliary并用unsigned大于7决定纵向面板，X为414；任一grid子stop保留cache C及已完成前缀并阻断auxiliary和cache A/B；message 27保留alternate grid opaque边界，X为402；
- message 8绘制narrow列表；message 30绘制mode grid；
- 正常尾统一置selection cache A/B。纵向面板typed-stop发生在auxiliary和cache C之后、A/B之前。

三处纵向面板调用固定动作、Y坐标、middle count、共享fill offset与selector，并直连既有typed action updater、frame provider与blitter。

## 5. message 3：角色标记与目标轮转

先配置文字font，再从共享动作workspace读取`5*(queued-8)`项决定group-A或group-B路径。无现代边界；只在首次真实workspace访问停止。

target-selection block等于1时扫描整组：

- group-B对象完成查询返回0时，依次构造快照、模式1重置、读取原点；原点双word全零用矩形中心，否则按i16偏移；随后直连prepared动作帧；
- group-A先读取对象内两项门，任一等于1跳过；完成查询返回0时构造快照、模式1重置并按中心绘制；
- live signed count不加现代上限，第九个group-B或第十一个group-A真实对象访问停止。

block不等于1且当前group-B完成时，target cursor递增并按live signed count回绕1，再从九项target map读取新对象。每次发布target index与one-based actor code；遍历次数达到count则把message写1。target map index 9在真实读取点停止。

published actor不是全1且共享pre-frame gate B为0时构造当前标记：group-B顺序为重置、快照、原点；group-A one-based顺序为快照、原点、重置并把Y偏移加10。动作6额外查询目标可用性。最终动作号按target-action available选择，按循环索引直连prepared动作帧。原记录步长为`0x98`，第8项恰从`0x004FDC58`开始；因此只持久化前8项独立记录，第8项前16字节每次与lower-panel bottom/top/aux/aux-index四个typed dword互相装载写回，余下`0x120`字节保存第8项尾部和第9项，严格保留物理重叠而不建立副本。selection input gate等于1时，标记之后调用选择提示。

## 6. message 5、6、7与默认路径

message 5固定绘制动作框；message 7以面板原点加`12/8`和alternate selection绘制。message 6只有两个actor gate都为0时才执行：对`0x0053BF1D`物理byte OR `0x40`，即typed u16 owner高byte OR `0x40`而保留另一byte；随后置cache A/B并清message和queued。

message 0、大于30、103、以及9–26/28/29均保持权威默认返回，不调用表内绘制路径。有效message的ECX为压缩selector；表内默认ECX固定10。

## 7. typed owner、caller回收与验证

选择帧owner只承接两项pointer origin、display/secondary gate、已关闭绘制的栈局部状态、八项独立动作记录及后两项非重叠尾区。动作摘要主/次颜色和组A profile继续由启动状态唯一持有；列表框面板动作记录直接复用主帧协调器owner，列表框四项动作框和网格列表帧五项动作框复用同一偏移动作帧state port；列表内容复用启动颜色、组A说明记录/index、输入门与candidate owner，网格列表帧另复用target argument及主帧面板记录；两者的MAPS和128字节共享文字只接受外部现有owner span；原suppression字段与final-actor pre-frame gate B同址，现直接复用后者唯一owner。十项标签索引现复用启动状态中`0x004A75C8`起的单一物理视图：启动路径只访问前四项，选择帧保留后六项相邻读取。五项选择指针直接复用输入分派的selection workspace；物理控制word直接复用输入分派的retreat control word；两项角色原点word也复用输入分派内由已关闭target-selection entry配置callee写回的唯一owner。message、queued/published角色、actor order、动作workspace、两组数量、其余输入cache/动画、菜单选择、target map、debug gate与target runtime也继续复用既有唯一owner。

global reset通过输入分派owner同步原234项写程序覆盖的控制word，并只同步选择帧owner内的display gate和secondary gate；同址pre-frame gate B只通过final-actor owner清零；五项选择指针、启动映射中的标签视图、pointer origin与输入owner中的actor origin未被原写程序覆盖，保持原值。

主帧协调器原frame-stage槽保留相同枚举数值并改为reserved，交互可用发布后直连本实现。完成角色路径原目标准备callee槽也保留相同枚举数值并改为reserved，五项指针及message/cache/runtime清理后直连已关闭角色目标准备。任一子typed-stop保留此前副作用，并阻断本帧余下选择流程、画面效果和全部后续帧阶段；原动作摘要、列表框、列表内容和网格列表帧opaque槽也分别保留相同枚举数值并改为reserved；六个reserved槽均保持零调用。

定向测试覆盖：message 103和queued零早退、group-A一过前、完成角色替换及actor-order交换、message 1比例动画、文字居中、完整动作摘要及profile子stop、message 2完整列表框/列表内容、资源/矩形/共享文字子stop、signed动画夹值、七行上限与i8低byte、message 4完整网格列表帧、隐藏行无上限扫描、双矩形/共享文字子stop及纵向面板stop、message 5/7/8/27/30固定调用、message 6物理byte OR、message 3第九个group-B对象、target map index 9、共享pre-frame门提示、prepared动作帧stop、动作记录第8项四dword物理重叠与第9项尾区、global reset覆盖范围及主帧caller传播。

当前缺少原版两组角色对象、未关闭callee共享副作用、文字表内容、五项动态指针目标、动作记录/帧资源联合状态、动态栈scratch地址、battle侧MAPS/共享文字owner实装及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
