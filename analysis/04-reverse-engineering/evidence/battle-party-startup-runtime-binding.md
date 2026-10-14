# 初始队伍构造、重置与配置接线

## 范围与证据等级

本批接通SDL会话内十个队伍对象的基础记录构造，以及每个初始队员的
重置、镜像、基础记录配置和模式计数。关闭时回收同一分配登记。
不包含后续排序、物品与属性、指标、敌方随机进度、补位和完整战斗帧。
B10保持315/422，Workpack316保持pending_audit；B11实际续玩未验收。

行为依据为仓库外已有的`swd3.exe_export_for_ai/swd3.exe.lst`机器指令。
本批是宿主对象与原版布局的有界适配，不升级为assembly_exact或
original_diff_verified。没有运行原版或OpenSWD3游戏程序。

## 原版顺序与实现映射

### 一次性构造：0046E490..0046E4C4

1. 0046E494调用478250。复用已有基础初始化器，直接借用startup/action字段。
2. 0046E49D、0046E4A4依次清2F26、2F18；前者写失败时不分配。
3. 0046E4AB分配38字节。0046E4BC先发布actor首指针，再逐DWORD清14项。
   零分配停在首次写入，ECX为14；不足一DWORD不提交该DWORD。
4. 正常返回actor token，ECX为0，保留分配调用的EDX。

`construct_legacy_battle_actor_group_a_element`增加临时借用视图；旧owned
入口只适配到该实现。`LegacyBattleGroupAStorage`登记完整guest地址范围，
实际字节仍位于`party.configuration.actor_record`，不复制另一个actor。
重复construct不重建已构造对象。非法或已释放登记不返回伪造字节。

actor+0由configuration.actor_record_token持有；+2BC4单独保存secondary。
+10内嵌资料及其说明独立于+0C动态资料，也不同于+0B4、+158、+1FC属性资料。

### 每次入战重置：0047D350..0047D632

`reset_legacy_battle_group_a_for_startup`复用共用的逐次字段重置函数。
每条写指令使用临时ABI图像编解码，立即提交到既有字段；不缓存持久actor图像。
额外存储只承接尚无owner的字节区与标量。
[启动重置接口](battle-actor-startup-reset-semantic.md)随后已删除寄存器
参数、结果、重复完成标记和访问计数，直接借用堆，删除队伍视图中的堆转发。
生产调用方检查完成状态；共享堆释放协议仍待与关闭流程共同迁移。

+26C8借用属性应用状态；+2A87借用粒子phase的模式字节；+2AE4借用
final_actor的availability。既有ABI映射覆盖的动作、进度、配置、坐标保持原归属。
+0FCC未分配的零工作区只接受对应区间的零写；真实已分配区直接写原存储。

动态资料仍依据+2AA0及+0C决定释放。释放失败保留此前REP写入，不清指针，
不继续后续字段。+0基础记录、+2BC4次级指针及+10内嵌资料不由该reset释放。
可用标志不可写和活动字段不可读分别停在0047D5DB、0047D422。

全局dispatch重置保留队伍action、phase及availability，直至原角色reset位置。
原有敌方及粒子分配保留逻辑继续保留。此处不宣称其余全局字段顺序全部关闭。

### 初始队员循环：004521C5..00452277

- 004521DD先完成角色reset。
- 镜像门精确等于1才执行47F900(1)，写渲染门，并将来源X按word从640减、
  anchor按DWORD从624减。所有回绕按对应宽度保留。
- 00452220读取紧凑角色来源索引。基础来源为4AB790+index*38，辅助来源为
  4ACF50+index*60；两种来源不可混用。
- 00452251调用46E730，成功后才到00452258的47CE80查询。
- 47CE80先读+2AB8完整DWORD，精确1返回1；否则检查+26D1的20h位。
  只有返回1才在00452261递增53BEFF的byte，保留255回绕至0。

`LegacyBattleGroupAStorage::initialize_party`按此顺序组合；SDL在敌方准备
之后绑定队伍来源并逐个调用。失败阻断余下队员及后缀。
无效来源索引不提前跳过46E730：以缺失字节视图到实际来源读取点停止，
保留已完成的workspace清零及placement复制。
本helper不自行清计数；原全局清理的45BA4C是另一调用位置。

### 配置与workspace：0046E6A0..0046E720、0046E730

46E6A0先按原顺序清11个word与2F0C DWORD，然后清2BC8的BE个DWORD、
0AF0的4C个DWORD、2B24的29个DWORD。临时bindings在对应位置更新
实际action/final/item/particle字段和七条动作记录。

删除独立的untouched_field_2f0e：它位于原2F0C DWORD内部，不能保持旧值。
2EF8粒子计数及2F26 tick不属于这些区间，继续保留。

53AF70阵形来源与actor+0D50坐标分开。setup已经镜像时先反算原来源，
真正镜像写入仍发生在reset之后。46E730复制primary/secondary placement时
立即发布两个借用坐标视图，早于来源失败和诊断。之后仍调用既有基础记录
复制、来源限幅和状态设置；诊断后不做快照回填，保留回调对活跃字段的修改。

### 缺少角色动作号诊断：00431150..004311B6

读取完整LST及4A7C2C、4A7C44、4A5DC4、4A5DD0字符串。
SDL显示原繁体标题、源文件、行号和正文。按钮ID保持3/4/5：

- 中止：同步关窗后断点，对应4311A6..4311B3。
- 重试：断点，对应4311A0..4311B3。
- 忽略：返回1；API失败0则返回32位0-4，对应43119B..4311A5。

原Win32消息框和INT3分别用SDL消息框和SDL_TriggerBreakpoint适配。
原CP950文字转为SDL要求的UTF-8；原消息框不能直接用于跨平台SDL宿主。
按钮外观和宿主调试陷阱由SDL提供，没有验证其与Windows弹窗的像素一致性。
消息框关闭按钮不作为新的返回分支；取消选择时重新等待有效按钮。
未启动UI，现有证据只有静态分支核对及SDL编译链接。

### 资源释放：00475180

清理继续先释放+2BC4，再释放+0。核心shutdown直接借用唯一的两个指针。
SDL直接实现两组资源释放方法，分别访问各自会话登记；与render辅助缓冲
release接口分开。关闭层通用分派已删除，登记失效后才清canonical指针。
见[关闭时的存储绑定](battle-shutdown-direct-resource-binding.md)。
未知或重复释放明确失败，不当作成功。此处不声明其余资源shutdown已完整实现。

## 测试与验证

新增及修订测试覆盖：

- 构造0/3/4/55/56字节目的区域、首派生word写失败、零分配及寄存器前缀。
- 十条独立基础记录、共享字节读写、重复构造、末尾越界与释放失效。
- reset的基础资料保留、动态资料释放两侧、可用标志写失败及活动值读失败。
- 镜像0/1/2、word及DWORD回绕、紧凑来源2、byte计数回绕及不计数分支。
- 两次配置、构造专属tick保留、availability按原位置清零。
- workspace真实字段与七条动作记录清零，相邻粒子计数保留。
- 诊断观察已发布状态，回调修改坐标与模式后继续读取现场值。
- 缺失或无效来源、reset失败、模式不可读均保留对应前缀并阻断计数。
- 核心shutdown通过登记释放十条记录，并清零真实primary指针。

proc_3d9f先通过core、ASan聚合测试各1/1及SDL构建。
REVIEW补齐workspace绑定后，proc_adcd暴露旧startup断言错误：它要求保留
+2F12缓存ID。依据0046E6D2改为零后，proc_d056三门通过。
其ASan编译仅有既存outcome_resolution_test.cpp:137窄化警告。

最后来源索引停止点修正由proc_1633验证：core/ASan各1/1、SDL链接通过。
日志为`build/tmp/runtime/party-startup-final-{core,asan,sdl}.log`。
之后只增加模式查询返回零的成功路径断言；proc_5d40的core/ASan各1/1通过，
日志为`build/tmp/runtime/party-startup-count-{core,asan}.log`。
最后五份日志无warning、error或sanitizer报告；SDL生产代码未再修改。
定向目标为openswd3_battle_legacy_battle_setup_tests，CTest表达式为
`^battle\.legacy_battle_setup$`；SDL目标为openswd3。
聚合入口实际调用新增reset测试以及构造、workspace、startup、setup、shutdown测试。
本批不以聚合1/1冒称全量core/app测试，也不以SDL链接证明实机续玩。

## 反向追溯与后续边界

构造视图、分配登记、可空借用bindings和访问范围检查属于宿主表示边界；
它们不增加原版游戏状态。所有业务写入分别映射上述构造、reset、workspace、
配置、镜像与计数指令。原始文件及资料借用规则沿用
[来源接线证据](battle-party-source-runtime-binding.md)。

后续按451B10中45227D之后的顺序接排序、物品与属性、指标和敌方进度；
不提前调用随机。完整初始化、帧、返回、再次进入以及原版选档与实际续玩
仍属于当前PLAN的未完成范围，不能由本批替代。
