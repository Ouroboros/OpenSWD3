# 战斗菜单应用行动配置后记录队员的标记数值

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。
实现源码：`e237b6e7eea9f9a975003c8b99e8babcf89ed22e`。
本批回收四个菜单47D8B0调用站；完整菜单父链仍pending，不标`assembly_exact`。

## 1. 独立原指令与范围

父462740..4640F1完整2820行，95个静态call、140个实际跳转、110个局部/
默认标签、37个plain RET，无外部chunk。四站分别为message2的462C9D、
message4的463475、message30的4637C2、message8的463906。
第五站46ACD9位于脚本父469D20，已在此前迁移。

完整叶47D8B0..47D8C5共15行，无栈参数、call或写入，两个plain RET。
MOV AX[ECX+0DAE]，TEST AH80；真AND EAX7FFF，假XOR AXAX。
假分支保留EAX高WORD；四菜单caller随后都AND EAXFFFF。
因此完整发布DWORD是真分支低15位或假分支0，无需保留机器寄存器回复。

初跳转宽匹配176包含36个jumptable注释；按机器指令重新计数纠正为140。
独立记录完整物理范围、ABI、前继与出口后再审计C++和测试。

## 2. 读取对象、实际持有者与原序

叶前重读53BD54，并保存actor-8；对象为5029D0+(actor-8)*2F34。
已有startup.party[index].final_processing.profile_buffer覆盖actor+D90
十DWORD；0DAE即DWORD7高WORD。直接借用现有缓冲，不增加字段或副本。
GroupAStorage配置借同一final_processing；其余profile重叠视图仍待审。

先清真实selection_target_cache，再把数值写入共享block_520e90保存行
column3。后继bit1可重读live角色并写column2；两行不能混淆。
真实队列插入继续借同一50 DWORD来源，复制五字段后才清相应行。

四条前继分别保持：

- message2在hover/确认/候选门、470380非零及4705C0之后重读对象。
- message4在470AC0非零之后重读override对象；4717E0完整1仍走独立路径。
  普通路径再保存实际角色并读取标记WORD。
- message30保留signed grid与零扩展WORD行上限、470AC0 mode5非零门。
- message8仅以4705C0低AX等于FFFF早返，其余值进入实际读取。

删除query_actor_property_a操作、专用通用寄存器形态和五处测试假回复。
后三条路径原先沿用前继前的角色，现按LST保存实时对象供读取及后续fallback。
没有新增Port、Adapter、参数包、寄存器回包、计数、纯叶回调或成功兜底。
未拥有角色仍复用group_a_actor_typed_stop，在cache及辅助写入之前保留前缀。
这不证明原DWORD地址回绕别名或未知地址故障等价性。

## 3. 实际验证

新增424次菜单执行：400次为四路径×十实际槽×十个WORD：
0000/0001/7FFE/7FFF/8000/8001/ABCD/F00F/FFFE/FFFF。
检查完整DWORD发布、全十槽profile DWORD、邻居行与后继可见前缀。
同400次随后执行真实攻击队列插入，检查五字段复制、WORD截断、copy-before-clear
及只清本行。其他尚未迁移的父操作仍使用既有测试Port，不代表整父已迁移。

八次真实源前继修改角色及profile；装备路径用旧对象override真、新对象假，
验证实际重读。十二个未拥有角色向量和四个真实前继早返保留旧数据及失败前缀。
纯叶不设置mutation或stop夹具。既有属性链测试同步删除五个假回复，
两个剩余父轨迹期望由五调用改为四调用。

proc_f12e首次构建因测试错误字段hovered_primary失败；纠正为hovered_equipment。
原错误日志保留；proc_d593初轮core通过，最终门重新确认全部当前向量及源码身份。
proc_d636最终门成功294秒：core setup1/1 10.62秒、ASan setup1/1 18.01秒，
SDL实际重新链接，三源SHA256及whitespace通过。
proc_72e1源码审计成功3秒：完整原指令范围、四站、实际来源/发布、向量、
真实消费、门日志、冻结差异及已验证源码身份通过。

实际命令使用build.sh core、build-asan.sh及build.sh app公共入口，
定向target为openswd3_battle_legacy_battle_setup_tests，
regex为`^battle\.legacy_battle_setup$`，SDL target为openswd3。
原outcome第122行u16→u8与script第2565行size_t→u32 warning保留。
首次清单复制误用中间groups.tsv，审查发现后改用正式invoke-migration-progress.tsv；
错误差异留存，正式表与生成产物逐字校验。
proc_03f2生成清单成功7秒；proc_9d76证据审计成功17秒，源码/远端身份、
可复现874/324/27/293清单、范围false、无关分类与新增文档链接通过。
首次审计读取不存在的顶层manual_scope失败，改为核对全部实际元数据。
没有全量UT、Windows、原版差分、真实MON验收或游戏运行。

临时指令、独立分析、双向复核、差异、hash、日志与审计脚本保留在
build/tmp/runtime/menu-profile-marked-word-*及check-menu-profile-marked-word-source.py。

## 4. 剩余范围

五个47D8B0物理父点已全部使用实际WORD；不据此关闭五个完整父调用链。
47D8D0/47D880菜单余量各四站；MON源处理、其他操作编号、参数包、父寄存器
报告、轨迹、计数、Port、共享别名、全部出口与完整SDL生产帧仍待迁移。
B11、Workpack316及全项目Goal未完成。相关范围见
[菜单父](battle-target-selection-refresh-00462740.md)、
[脚本标记数值](battle-script-party-profile-word-0047d8b0.md)、
[项目清单](invoke-migration-progress.md)。
