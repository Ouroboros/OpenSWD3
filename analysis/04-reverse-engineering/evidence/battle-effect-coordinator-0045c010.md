# 战斗效果总协调步进 `0x0045C010`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威函数为`0x0045C010..0x0045D17A`，从proc到endp完整1915行、68个静态call站点、114个条件或无条件分支与59个局部标签，无外部FUNCTION CHUNK。

唯一静态caller是已关闭主帧协调器`0x00453200`的一处调用。caller原先以opaque完成门表达；现已删除该枚举和测试桩，改为直接组合typed实现。子typed-stop阻断后续固定帧；普通返回值不等于1时仍只把共享UI低word OR 1。

十二个唯一callee中，单体效果帧`0x004582B0`八处与群体效果帧`0x00458DE0`五处已关闭并全部直连；其余十个actor查询、状态、配对、反馈与奖励callee继续通过统一效果call port保留窄边界。

## 2. 入口与当前角色分组

入口先测试共享UI dword的bit15，未置位直接返回0；bit15置位后再测试bit0，bit0已置位同样直接返回0。两条路径均不访问角色或效果状态。

随后读取唯一`LegacyBattleActorMetricState`中的当前角色完整dword：

- 无符号值小于8进入组B当前角色路径，基址`0x00525508`、步长`0x2B28`、物理容量8；
- 其他值先低32位减8后进入组A当前角色路径，基址`0x005029D0`、步长`0x2F34`、物理容量10；
- 异常当前角色不增加现代限制，在第一个真实actor查询点typed-stop；
- actor查询返回值只取低word并按i16符号扩展，目标token运算保持低32位回绕。

当前组A路径在actor查询后才读取十槽参数数组，因此第11项在原参数读取点停止并保留查询副作用。参数对象mode不再作为额外物理副本传入：typed caller从当前角色的唯一组A/组B对象状态读取并交给已关闭效果函数。

## 3. 当前组A四类路径

组A由global gate、效果mode与目标side组合为四大类：

1. global gate非零且效果mode精确为1：先直连群体效果，返回非1立即返回0；side为组B时按动态signed组B数量遍历，跳过状态1角色，其余角色发布处理槽、活动latch、三项反馈、可选全屏`0xFFFF`填充和完成计数；最后清共享反馈与完成计数。
2. global gate非零且效果mode不为1：按scan limit低word扫描目标组。组B扫描先执行actor状态查询；状态1可扩展scan limit并在required count大于1时递减。其他状态按u16 delay/threshold扩展限制并直连单体效果。组A扫描额外保留两个actor guard。每个成功槽只在处理槽仍为`0xFFFFFFFF`时发布，完成数按signed大于等于required count收束。
3. global gate为零且效果mode精确为1：按side直连群体效果。组B目标发布pair高word、处理槽、反馈、组A发起计数、dirty与actor发布；组A目标发布pair低word、无条件反馈和处理槽。
4. global gate为零且效果mode不为1：按side直连单体效果。组B目标额外执行配对callee；组A目标保留独立反馈源、反馈actor和不发布dirty的全屏填充路径。

完成的global路径把完成计数清零、scan limit重装为1，并按LST分别清18槽主记录与三组18槽反馈数组。单目标路径只清18槽主记录及对应共享值，不扩大清零范围。

## 4. 当前组B四类路径

组B使用另一组global gate和效果mode：

- global gate为零、效果mode不为1时，side决定单体效果目标组。目标组B只发布pair高word，成功反馈可全屏填充但不发布组A dirty latch。目标组A发布pair低word，并依次保留对象复制、反馈、配对、最终角色查询、焦点release判定和奖励行调用。
- global gate为零、效果mode精确为1时，目标固定为组A，不受单体side影响。失败路径仍执行共享奖励查询；成功路径执行对象复制、pair低word、共享反馈、最终角色查询，但不执行单体配对和成功奖励行。
- global gate非零、效果mode精确为1时，先直连群体效果，再按动态signed组A数量扫描。状态1或两个actor guard任一为1时跳过；其余项保留对象复制、正反馈值发布、配对、活动latch、群体计数、可选全屏填充、奖励行和精确完成计数相等门。
- global gate非零、效果mode不为1时，按side扫描组A或组B。scan limit和delay均为u16回绕域；组A保留双guard和对象复制、奖励行，组B不执行这两项。两侧都只在处理槽为`0xFFFFFFFF`时计数，完成门是与共享目标计数精确相等。

异常动态数量或scan limit不加现代迭代上限。组A第11个、组B第9个只在真实actor访问点typed-stop，保留此前效果、反馈、framebuffer与计数副作用。

## 5. 反馈、framebuffer与固定清理

反馈callee返回精确1时，函数按各路径原有差异发布反馈actor、组A发起计数、组B staged计数、dirty latch或actor槽，然后以内存字节`0xFF`填充`2 * Rect.right * Rect.bottom`。typed实现对唯一`LegacyFramebuffer`写入对应u16 `0xFFFF`像素；请求范围超过owned物理像素时先保留完整owned前缀，再在首次不可用像素停止。

共享反馈primary、secondary和packed reward高word按原路径分开清理。双对象数值转场关闭后，三处caller删除旧token并直接组合typed实现；primary与动作累计值、secondary与单体/群体辅助奖励、packed reward高word与效果步进分别共用唯一typed端口。`0x005202A8`的固定`0xAB0`字节清零恰好对应18个`0x98`主记录；三组反馈数组也各固定18 dword。先前单体8槽与群体10槽的重复建模已纠正为同一18槽虚共享状态，主记录、备用记录、公共渲染字段与奖励数组只保留一份物理typed存储。actor发布数组`0x00502984`也与战斗初始化重置收敛为同一18槽虚共享端口，初始化写`0xFFFFFFFF`与本函数按索引发布不再维护两份副本。单体与群体效果进一步共用动画横向命中的八槽u16计数及共享XY，协调器多重组合仍只有一份物理状态。

全局重置`0x0045B630`写到的18槽主记录、八条精确`0x98`强度效果记录、两组模式、参数数组、计数器、反馈actor与活动latch已从未映射字节像回收，直接同步同一协调器状态；强度record0前28字节同时供已关闭攻击顺序移除作固定一过尾源。重置程序未写的scan limit、delay和反馈数组保持不变。

## 6. 测试与动态差分

定向测试覆盖双UI入口门、单体/群体18槽共享与第19槽停点、当前组A/B越界、组A单目标双side、组A staged与group-wide路径、组B单目标双side、组B群体固定组A不对称、组B staged与group-wide路径、动态组B第9项停点、三处双对象数值转场caller直连、完整framebuffer填充、组A发起计数、组B自目标不发布dirty、初始化actor发布槽物理别名、全局重置物理别名及主帧caller直连。

定向`1/1`、独立AddressSanitizer `1/1`、Linux core `188/188`和Linux app `194/194`通过。当前缺少原版两组完整角色对象、九个剩余callee共享副作用、动态数量与scan limit修改、反馈数组、framebuffer地址、Rect与寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
