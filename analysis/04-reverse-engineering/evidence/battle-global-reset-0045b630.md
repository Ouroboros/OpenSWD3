# 战斗全局状态重置 `0x0045B630`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威函数为`0x0045B630..0x0045BD0C`，从proc到endp完整637行、9个静态call站点、1个条件跳转与1个局部标签，无外部FUNCTION CHUNK。

三个静态caller都位于尚未关闭的`0x00469D20`，调用点分别为`0x00469E51`、`0x0046BC45`与`0x0046DF0E`。三个caller均不消费本函数返回值；未提前回收。

## 2. 入口资源释放顺序

函数先按固定顺序执行：

1. `0x00451AE0`释放两个战斗显示surface槽；每槽只在token非零时调用对象释放，调用后才把该槽清零；
2. `0x00451730`释放六槽战斗动作旋转缓存；
3. `0x00433D70`释放战斗渲染owner的附属缓冲、主行表与surface行表；
4. 读取全局分配token，仅非零时调用`0x004885A0`。

前三项已有typed实现，当前函数直接组合，不保留opaque helper。第四项只保留尚未关闭旧分配器的窄释放端口。条件分配token在释放调用返回后才由后续固定store清零。

显示surface释放此前内联在已关闭战斗启动函数；现抽为同一`0x00451AE0` typed helper，启动与本函数共用一份实现。

## 3. 固定写程序

条件释放之后没有数据分支。完整LST共得到234个显式写操作：

- `rep stosd`与标量store展开为3300次物理写；
- 合计写入13106字节；
- 值域包括零、全1、1、2、`0x60`、`0xF0`、`0xFA`、`0xF2`、`0x64`、word 6与byte `0x10`；
- dword、word与byte宽度均按机器指令保留；
- 重复写入不合并，例如同一38 dword区域清零两次，尾部资源callee之后再次清零同一全局dword；
- 最后固定清零6 dword区域。

写序元组按`address,size,count,value`固定。完整234项序列的FNV-1a 64散列为`0x970D7E940E1225B2`，定向测试逐项计算并锁定；不是只比较最终内存快照。

## 4. 单一typed存储

已关闭并被其他工作包共享的物理状态不建立第二份数组：

- 渲染owner固定731 dword清零映射回唯一`LegacyBattleRenderGeometry`；
- 18槽metric、18槽角色顺序、18槽mask、两组数量、角色优先字段及其后六dword出队输出记录、待执行动作activation latch映射回唯一`LegacyBattleActorMetricState`；`0x0053AE70`起原七dword清零现完整同步同一owner，启动期重复的组B数量副本已删除；
- 效果全角色步进的actor delta、方向、threshold与completion latch映射回唯一`LegacyBattleEffectShiftState`；
- 三通道颜色累加与初始化的九个float及signed计数映射回唯一`LegacyFrameColorTransitionState`；共享初始化门不在本函数写集合中，必须保留入口值；
- 角色预处理的active、secondary、published、source、action execution、auxiliary与双门映射回最终角色和动作状态；terminal与message映射回startup、动作、效果和预帧共用的唯一共享phase端口；事件工作区只清物理槽`0..9`和`16..95`，其他槽与五dword记录保持入口值；
- 双对象数值转场的primary映射回动作累计值与效果协调器主反馈共用的唯一端口并按原写集合清零；secondary与打包奖励高word不在本函数写集合中，保持入口值；
- 战斗调试快捷键映射的低word状态、重定向gate、battle mode flags、十dword辅助块与重置gate按原写集合清零；F1/X/K/F9/F2/P开关不在写集合中，保持入口值；
- 战斗调试叠加层的调用门、18项选择顺序、战斗选择、初始模式和战斗帧按原写宽度同步；当前角色、fMenu、mMove、MsD低byte和MS分别复用最终角色当前值、共享message state、预帧门、动作packed actor counter与已发布值，不保留叠加副本；255字节文字缓冲、战斗模式、选择低word、缓存计数、帧率除数与标记坐标保持入口值；
- 战斗结果判定复用最终角色排除u16/完成u8和动作phase/packed counter；排除与完成清零，phase只清低word，packed counter只清低byte；唯一结果状态端口的latch、暗化门与强制值清零；结果整理完成双word清零而奖励槽前缀及两项奖励ID保持入口值；全帧暗化delta保持入口值；
- 战斗上下文提示复用共享message、动作消息门/辅助值/两项坐标、最终角色active/pre-frame gate B及启动镜像模式；message、消息门、active、gate B和镜像模式清零，辅助值、坐标、提示计数、静态资源选择和偏移动作持久状态保持入口值；
- 战斗纵向位移的phase与节拍上限映射到唯一state port并清零；节拍计数不在原写集合中，保持入口值；
- 撤退提交两完成门和选择token映射到唯一state port并清零；辅助latch不在原写集合中保持入口值；battle mode/调试重置门与调试快捷键共享，叠加门改由撤退、逐帧和reset共用的独立gate port；
- `0x00524788`的18条记录扩展为精确`0x1C`布局，126 dword全部映射到唯一启动状态并清零，不再只同步五个已知字段；已关闭攻击顺序登记、插入、移除和出队均直接读写同一记录owner，插入尾部两项共享门也映射到startup reset并按原物理写清零；最终角色十项顺序、frame gate、selection gate、排队角色及动作路径同址门同步清零；
- 效果总协调器的18槽主记录、八条强度效果记录、两组模式、计数器、反馈actor、参数数组和活动latch映射回唯一`LegacyBattleEffectCoordinatorState`；强度记录0同时是攻击顺序移除必读的一过尾七dword源，也是攻击顺序出队无界28字节扫描离开18槽后的首个物理区域；未写的扫描计时与反馈数组保持不变；
- 八槽group B顺序表不在本函数写集合中，必须原样保留；
- 战斗启动复用的显示surface、敌我启动记录、重置块、记录数组与镜像模式映射回唯一`LegacyBattleStartupState`；
- 记录首字段的C++默认值为全1，但本函数执行整块清零，显式覆盖为零，不复用构造默认值。

尚未被typed工作包命名的物理全局写入单一按地址存储的字节像。已映射地址从该字节像排除，避免与typed数组形成两个可分叉副本。字节像按小端序写入，未触及地址保持原值。

## 5. 尾部callee与返回

固定写程序主体完成后，依次调用：

1. `0x00431960`；
2. `0x00433010`；
3. 已关闭`0x00485740`停止全部sample；
4. `0x00485710`提交固定音频stream停用；
5. `0x004776A0`执行后置初始化。

两个资源释放、stream停用与后置初始化仍保留窄typed端口，等待各自工作包关闭。停止全部sample直接组合已关闭sample命令。

五个callee返回后，函数把ECX设为6、EAX清零、重复清零一个全局dword，再以`rep stosd`清零最终6 dword。正常返回固定EAX为0，不泄漏任何尾部callee返回值。

## 6. 测试与动态差分

定向测试覆盖显示surface零/非零槽、旋转缓存嵌套释放、渲染资源释放、条件分配token零与非零、九阶段call顺序、234项写序散列、3300次物理写、13106字节、标量宽度、重复写、尾部6 dword、little-endian字节像、mapped地址排除、未触及字节保留、metric及待执行动作latch、颜色值清零与初始化门保留、角色预处理标量与工作区分段别名、双对象转场primary清零及secondary/打包高word保留、调试快捷键、调试叠加、结果判定、奖励槽写入、结果整理、撤退提交、上下文提示与纵向位移共享状态、完整18条启动记录、攻击顺序插入双尾门、移除相邻强度记录与出队七dword输出、最终角色/动作同址门、effect-shift与effect-coordinator typed别名同步、记录默认值差异、group B顺序表不清零及固定返回0。

当前缺少原版全部全局内存、九类callee共享副作用、旧分配器、音频对象及寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
