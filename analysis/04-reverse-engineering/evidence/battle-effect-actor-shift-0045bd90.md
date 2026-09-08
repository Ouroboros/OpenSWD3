# 战斗效果全角色数值步进 `0x0045BD90`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威函数为`0x0045BD90..0x0045C00C`，从proc到endp完整299行、8个静态call站点、19个条件或无条件分支与17个局部标签，无外部FUNCTION CHUNK。

三个静态caller均已关闭：单体效果帧`0x004582B0`一处，群体效果帧`0x00458DE0`两处。三个caller全部删除`0x0045BD90`地址token并直连typed实现。

## 2. 入口phase推进

入口用一次`push ecx`同时保留caller完整ECX并形成四字节scratch局部，然后：

1. 读取共享phase低word；
2. 共享调用计数按u16加1回绕；
3. snapshot完整方向dword；
4. phase按i16判断。

phase为正时，函数将其算术右移一位，把半值按u16加到共享累计word，发布半值为新phase，把调用计数清零，并按方向选择actor delta：方向精确为0时取负半值，其他完整dword取正半值。phase为1时半值与actor delta均为0，随后进入完成路径。

phase非正时不改phase、累计word或计数清零，直接使用此前共享actor delta。共享actor delta非零即进入全角色路径，最终返回0。

## 3. 全角色路径

函数固定先遍历组A再遍历组B：

- 组A基址`0x005029D0`、步长`0x2F34`、物理容量10；
- 组B基址`0x00525508`、步长`0x2B28`、物理容量8；
- 两组数量复用唯一`LegacyBattleActorMetricState`，每组入口按完整dword作signed非正门；
- 索引是每轮末尾加1的i16，随后符号扩展并与动态重载的完整signed数量比较；
- 每个角色直接组合独立`0x00478600` current-coordinate typed leaf，把当前可变arg0与入口ECX scratch的地址按原顺序交给leaf；
- leaf仅写两个局部的低word，高word与未到达写入的旧值均保留；X写入后发生的Y读取、第二指针读取或Y写入停止保留X前缀；
- leaf完成后重新读取共享actor delta，按低32位模加到完整arg0；
- 随后直接组合`0x004785C0`的typed坐标发布leaf：先写X/Y低word，再把更新后的`actor+0x0D50..+0x0D6F`按八个有序dword复制到`+0x0D70..+0x0D8F`；
- publication成功后才重新读取当前组数量并推进索引；typed-stop保留当前leaf部分提交并抑制剩余actor、后续组及caller后缀。

arg0与scratch跨组A、组B及所有角色持续携带，不按角色重置。异常数量不增加现代循环上限；第11个组A或第9个组B只在current-coordinate leaf首次真实角色解引用点typed-stop，保留此前所有查询、加法、typed publication与动态数量读取副作用。

四个`0x00478600`物理callsite `0x0045BE39/0x0045BE99/0x0045BF5A/0x0045BFBA`已从效果call port移除，直接解析startup/action/lifecycle canonical owner。两条循环体都保留`arg_0/var_4`局部地址、Group-A现有EDX与Group-B第二局部指针EDX入口位形、索引SUB flags、正常返回寄存器及六类停止前缀；单体父级一处与群体父级两处调用均从各自frame state显式传入这对语义局部token，不使用零默认值。四个`0x004785C0`物理callsite也继续直接组合typed publication，不向port发送两个callee token。

## 4. 完成路径

共享actor delta为0时：

1. 将threshold word按i16符号扩展；
2. 只取arg0低word并按0..65535正值与signed threshold作`jle`比较；
3. 低word小于等于threshold时跳过累计消费和completion latch发布；
4. 否则读取共享累计word并按i16判断；非正值不消费，正值每次最多消费30；
5. 累计word只写低word，与其相邻的共享packed reward低word不被覆盖；
6. 完成消费的方向与phase路径故意相反：方向精确为0时actor delta为正，非零时为负；
7. 非零消费量再次执行完整组A、组B路径，全部成功后才把completion latch发布为1；
8. 最后只有完整completion mode精确等于1才把phase word重装为`0x01A4`。

完成路径固定返回1。函数返回前恢复入口完整ECX；EAX是0或1，EDX保留路径相关threshold、累计低word、actor delta或最后typed publication结果。publication开始前的完整u32 ADD flags被精确构造并传入leaf；leaf自身不改flags。

## 5. 单一物理状态与caller回收

新`LegacyBattleEffectShiftStatePort`由效果端口与动作端口虚继承，统一保存phase、调用计数、方向、累计word、相邻packed reward、actor delta、threshold与completion latch。群体效果帧删除自己的组A/组B数量、packed reward、final gate word和latch副本；单体效果帧删除packed reward与final gate word副本。两类效果帧现共用同一角色数量与效果步进状态。相邻双对象数值转场关闭后，kind 4尾部也通过该端口只替换packed reward高word，低word继续由本状态唯一保存。

动作分派到单体效果帧的适配器覆盖共享状态getter，直接转发父动作端口的唯一metric与effect-shift状态，不创建临时副本。

全局状态重置`0x0045B630`写到的actor delta、方向、threshold和completion latch地址已从未映射字节像回收，直接清零同一effect-shift状态；原234项写序不变。

单体效果caller保留lookup低word覆盖caller EDX高word、完整完成值作为completion mode与入口ECX、子返回0提前结束及typed-stop阻止后续清理。群体效果第一个caller先发布completion latch，再用当前EAX构造arg0；子返回0提前结束。第二个caller使用第一个callee恢复的ECX构造arg0，完整mode固定1，且按原逻辑忽略普通0返回，但typed-stop仍阻止后续清理。两类父caller都保存完整`LegacyBattleEffectShiftResult`，并分别映射Group-A/Group-B容量、current-coordinate与coordinate-publication停止，同时公开真实EAX/ECX/EDX并阻断后续清理。

## 6. 测试与动态差分

定向测试覆盖phase正/一/非正、u16调用计数回绕、两种方向非对称、累计word回绕与packed reward保留、组A后组B、固定token与步长、current-coordinate六类typed-stop、局部低word与高word保留、跨角色arg/scratch、低32位加法回绕、完整SUB/ADD flags、Group-A/Group-B EDX入口差异、typed publication真实owner写入、threshold signed域、每次最多30、completion latch时机、精确mode一重装、入口ECX恢复、路径相关EDX、第11个组A与第9个组B停点、查询及publication部分提交与后缀抑制、三个父caller直连、普通0返回差异及全局重置物理别名。四处current-coordinate generic端口调用断言为零。

历史关闭门为定向`1/1`、独立AddressSanitizer`1/1`、Linux core`188/188`和Linux app`194/194`。Workpack 287 REVIEW 1接入typed publication并回收四处物理setter后，最终门为战斗定向`1/1`、Linux core `199/199`、ASan/UBSan `199/199`和Linux app `205/205`，日志零源码warning与sanitizer诊断且stderr为空。

当前缺少原版两组角色对象、异常栈与坐标内存页、DF/寄存器/SEH、getter共享副作用、动态数量、phase/累计/threshold/latch与四个publication caller联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
