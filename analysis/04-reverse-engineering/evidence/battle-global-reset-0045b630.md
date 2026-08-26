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
- 18槽metric、18槽角色顺序、18槽mask、两组数量与角色优先字段映射回唯一`LegacyBattleActorMetricState`；
- 效果全角色步进的actor delta、方向、threshold与completion latch映射回唯一`LegacyBattleEffectShiftState`；
- 三通道颜色累加的九个float与signed计数映射回唯一`LegacyFrameColorTransitionState`；
- 效果总协调器的18槽主记录、两组模式、计数器、反馈actor、参数数组和活动latch映射回唯一`LegacyBattleEffectCoordinatorState`；未写的扫描计时与反馈数组保持不变；
- 八槽group B顺序表不在本函数写集合中，必须原样保留；
- 战斗启动复用的显示surface、敌我启动记录、重置块与记录数组映射回唯一`LegacyBattleStartupState`；
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

定向测试覆盖显示surface零/非零槽、旋转缓存嵌套释放、渲染资源释放、条件分配token零与非零、九阶段call顺序、234项写序散列、3300次物理写、13106字节、标量宽度、重复写、尾部6 dword、little-endian字节像、mapped地址排除、未触及字节保留、metric、颜色累加、effect-shift与effect-coordinator typed别名同步、记录默认值差异、group B顺序表不清零及固定返回0。

当前缺少原版全部全局内存、九类callee共享副作用、旧分配器、音频对象及寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
