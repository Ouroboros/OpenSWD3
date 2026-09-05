# 战斗组A动作执行 `0x0046F8C0`

历史状态：`platform_adapted`。工作包282复核发现记录重叠与状态投影缺陷，当前修正尚未收敛。第6节旧门禁不是本轮放行证据；本轮进度及未决项见第7节。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046F8C0..0x0046FEEE`，从proc到endp共668行，其中637个非标签物理行、386条实际指令、11个call、48个跳转、31个局部标签、5个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall，两个栈参数分别为目标actor token和0x98记录槽索引，所有出口以`retn 8`弹栈。

入口先检查actor word启动门和dword执行完成门；任一命中直接返回0。特定跳过状态、actor `+0x0D9C` BYTE标志与早退latch组合会先查询目标，查询返回0时写早退latch 1并立即返回该零寄存器前缀。

## 2. 记录准备与基础flag

第二跳过状态为1时把actor两个位置word固定写为0x140和0x136。函数随后写actor `+0x2AAC`，按特殊模式或记录高字节bit1设置force gate，并构造首个0x98记录：资料值写首dword，动作类型通常为0x28；profile mode为1时置共享profile gate并选择0x29，跳过状态下回到0x28，否则复制actor word；非profile模式下可按alternate mode选择0x30或0x31。

目标与首记录交给准备callee后，actor flag bit1会把两项临时值复制到第二0x98记录、发布运行时0x4000，并消费bit1、临时值和可选force bit。运行时0x4000再驱动第二记录查询；callee返回1时清force、bit9和运行时0x4000。

actor flag负位发布两个共享全局；bit3可先按bit10执行七个signed word颜色初始化，然后清bit10和bit3、置运行时0x8000并清所选slot。bit2调用目标重置并把actor flag清零；bit0清motion、共享motion和slot，调用目标模式1后清flag。所有slot按原`slot * 0x98`寻址，越界只在首次实际clear或访问处typed-stop。

## 3. 活动slot与绘制

运行时0x8000未置时返回0。活动slot首dword默认取actor复制word，临时primary非零时覆盖；slot `+0x08` DWORD清零。首dword为零时直接把slot完成dword写1。

首dword非零且actor `+0x0D9C` BYTE bit0为1时，按actor位置、源目标偏移、Y辅助值和记录token调用准备绘制callee；只有返回1才发布actor完成gate、slot完成dword和slot word `+0x5A` bit0。否则走另一目标记录callee。slot word bit0随后触发目标模式1调用并清该word。

slot完成dword不为1时，解引用actor `+0x254C`的帧，按其WORD尺寸、actor绘制位置与`+0x26A4`渲染flag调用blitter后返回0；最后一项参数为零，不代表帧尺寸为零。完成dword为1、首dword非零、`+0x0D9C` BYTE bit0未置且motion signed word大于-32时，先发布三项motion共享值；渲染flag命中0x2C时三项清零并把motion夹为-32。资源token在此后首次解引用，缺失则typed-stop；存在时按资源尺寸、渲染flag和资源值绘制，motion减4，特殊模式1再加4，随后返回0。

## 4. 完成清理与激活倒计时

没有活动motion且actor完成gate不为1时返回0。完成时依次清`+0x0338`、`+0x0468`、当前slot、`+0x06C8`和`+0x0760`五个0x98块；当前slot与后两者可重合，仍执行原五次清理，把四个目标索引写全1，清motion、actor完成、早退latch和运行时flag，并把motion辅助word写1。

共享profile gate为零时读取物品效果唯一owner中的激活byte。激活为零先清效果flag bit0并完成；激活大于零只减1，在完整清理前缀后返回0。共享profile gate非零则绕过倒计时直接完成。完成路径递增共享低byte计数，清actor profile mode和共享profile gate，返回EAX/EDX均为1。

## 5. owner与caller回收

每actor动作执行状态承载局部记录、位置、motion与资源视图。本轮已把主记录重叠字段和索引记录改为唯一存储，详见第7节。历史实现把actor `+0x2AAC/+0x267C`投影到`LegacyBattleActionDispatchState`，完成字段复用startup party进度状态；这些投影还需按真实actor读写链修正，不能据旧测试宣称所有权已关闭。效果flag与激活byte借用物品效果状态；跳过状态通过frame coordinator绑定只读span。

全程序三处静态caller均位于已关闭行动调度器；typed源码将其收敛为两个直接调用位置，覆盖普通行动case 1及共享后段。目标token按side选择，槽索引固定0。旧完整函数地址调用生产零次，子callee以明确参数和寄存器的窄action port保留。

## 6. 验证状态

单元测试覆盖首actor typed-stop、两项入口门、目标查询早退、基础完成清理、激活倒计时、第二记录bit消费、颜色七参数与slot清理、bit0目标模式、渲染bit0完成、motion资源typed-stop和slot越界。行动调度回归覆盖两种side普通攻击、阻塞效果、framebuffer越界及旧opaque零调用。

历史工作包184验证结果：定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning，app仅有既有ALSA提示。inventory连续双生成逐字节一致，稳定为`184/422 = 175 platform_adapted + 9 assembly_exact + 238 pending_audit`，SHA256为`769db821e21fdc38842ae86bb1cbbf882b1a3453689c411e6fec2c3c8abfa506`。原版组A actor完整状态、目标actor、资源记录、九类callee副作用及三处caller寄存器联合捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。

## 7. 工作包282修正中

主记录`+0x0338`现在唯一承载`.field_24/.field_28`、`.field_5a`（含高字节）、`.field_76/.field_78`及`.field_7a..field_86`。删除原独立数值、标志、辅助坐标和颜色成员；颜色读取仍按WORD符号扩展。callee写回后消费数值/辅助Y以及颜色signed WORD边界的测试已加入。

`+0x0468`统一使用`turn_action_record`。索引0/1借用具名effect记录，8/9借用具名special记录，2..7仅保留六条中间记录；索引表是调用期借用指针，不是另一份持续记录数组。删除原索引数组及第三、第四记录副本，保留五次原序清理，包括同址重复。

组B仍使用`secondary_record`及其原端口类型，但其真实位置是`+0x03D0`：`0x00475AC5/0x00475CC2/0x00475CCA`明确区分它与组A的`+0x0468`。core22曾因过早移除该类型编译失败，随后恢复独立的组B存储；未把两组记录合并，也未新增同步副本。十索引测试同时检查`+0x03D0/+0x0500`和其他未选中记录不被清理。

core21/ASan13定向`1/1`通过，覆盖主记录数值与颜色修正；core23/ASan14定向`1/1`通过，覆盖记录别名与组B隔离。后两者测试耗时2.54/4.26秒，日志无匹配编译或sanitizer诊断，`git diff --check`通过。尚无本轮全量门禁或独立审查放行。

`+0x04C0`现已统一到turn记录`.field_58`，敌我音效及召唤清理均不再使用独立sample副本；core24/ASan15定向通过。声像寄存器与重读测试见回合门/组B行动十七证据。

### 资料缓冲与标志修正

`0x0046F8F0/0x0046FC37/0x0046FD18`三处BYTE测试现读取`+0x0D9C`，不再误用`+0x26A4`渲染flag。第四处`0x0046FD58/0x0046FD5E`仍读取`+0x26A4`并测试`0x2C`；不能把四处一起替换。

完整资料块`+0x0D90..+0x0DB7`现在唯一属于组A执行状态。删除final-processing的资料缓冲/独立actor_flags，以及执行状态中`+0x0D94/+0x0D9C/+0x0DA4/+0x0DB2`四个独立标量；同名读取方法直接读取资料BYTE/WORD，WORD写入保留相邻半字。方法不引入构造函数或继承，aggregate断言通过。组B资料仍属于其action-configuration，不借用组A资料视图。

最终处理、链表资料应用、资源选择、资料准备、角色清理和startup reset都写入实际执行状态缓冲；不再靠逐字段同步让影子值追上加载结果。`0x004750DA rep stosd`现在只清一个资料块，缺少后续pre-effect视图时已保留这十次DWORD清理。

真实MON编码测试覆盖加载后预检查、flag低BYTE与相邻高字节区分、相反渲染flag输入、最终处理条件、粒子抑制、动作WORD和有符号Y来源，以及邻接WORD保留。core27曾因新增夹具漏`0x8000`运行门发生两项断言失败，并发现缺少path初始化警告；修正夹具后core28/ASan18定向`1/1`通过（2.59/4.35秒）。再修正资料应用停止点寄存器并加入清理前缀测试，core29/ASan19均`1/1`通过（2.91/4.76秒），无匹配编译/sanitizer诊断，diff check通过。

仍未完成：actor `+0x2AAC/+0x267C/+0x2AB0`投影、帧指针失效路径、跨入口绑定及完整寄存器/所有权双向收敛。本函数不能按历史状态或本轮定向测试冒充全部验收。
