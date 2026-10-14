# 敌方角色收尾时读取实际参数

状态：`pending`。

源码：`c9585d99`。本批回收最终角色步进的`0045AD2E`查询站点。
该父接口族继续`pending`；本证据不关闭完整角色帧、SDL行动链或原版差分。

## 1. 独立LST范围与顺序

父函数`0045AA00..0045ADEC`完整465行；叶`0047F910..0047F917`
只有`mov ax,[ecx+2A0C]`和`retn`，没有业务失败返回或其他调用。
原AX被替换为实际WORD，EAX高WORD继承描述符token；EDX和FLAGS不变。
下一计数函数只消费参数低WORD，故真实函数直接接受WORD，不构造寄存器回复。

六个物理调用地址为`00454665`、`004546F1`、`00454736`、`0045AD2E`、
`00466AC8`、`0046CD58`。本批仅处理第四处，脚本50已处理最后一处。
行动三处与摘要一处尚待迁移；物理call扫描排除`__thiscall`声明。

第二参数完整DWORD不等于1时才进入敌方分支；全1索引在任何调用前返回0。
帧呈现必须完整返回1，随后顺序为：

1. `0045ACE3`读取所选敌人的共享资源坐标WORD；完整返回后，
   `0045ACF2/0045ACF9`分别以WORD回绕累加。
2. `0045AD02`取描述符，`0045AD07`发布其token；首次读取byte+20。
3. bit20h置位在`0045AD14`以WORD加20，否则`0045AD1E`以WORD加3。
4. `0045AD2E`读取该敌人参数WORD；`0045AD39`以该键累计1。
5. `0045AD3F`才移除攻击顺序；随后维持闭区间低byte递增、signed完成比较、
   结束门发布和reset分支。

完整摘录与独立分析位于`build/tmp/runtime/final-actor-profile-call-context.lst`
和`build/tmp/runtime/final-actor-profile-lst-analysis.md`。

## 2. 实际字段与资源

直接借已有`startup.group_b_lifecycle[index].action_execution.profile_value`。
同一记录此前供坐标读取使用，其完整返回已证明角色和资源绑定存在；
没有增加重复null预检、影子参数、默认角色或同步赋值。
既有`profile_value_read_accessible`在实际参数读取点消费；访问失败使用已有
`actor_base_state_missing`，保留坐标、描述符与延迟前缀，并阻断后续计数和门发布。
这是已有字段访问合同，不把纯读取叶改成带业务失败回包的接口。

删除查询编号、通用参数数组调用和三处测试模拟回复。
固定计数已由实际根与PMR节点持有；本批同时删除测试中不再消费的分配回复和令牌计数器。

计数叶`00477710..0047777C`完整63行，先比较根的WORD键。
清零根与参数0相等时直接累计根数量，不分配节点；非零缺键则分配20字节节点，
写键与数量，再递增根键。原有饱和、链访问和分配失败行为保持。

组B帧直接调用本函数并传既有startup；子失败先返回，后续映射、槽、选择字与门不清。
组A分支不消费新读取点。已有实际帧呈现返回测试也改为实际字段赋值。
这属于核心绑定的静态核验与定向运行，不等于完整SDL战斗链验收。

## 3. 汇编推导的测试

新增52组执行：

- 8敌人槽×6个WORD值：0、1、7FFF、8000、BEEF、FFFF。
  验证实际选中来源、邻居保留、无符号键、坐标和延迟回绕、计数及移除顺序。
- 4组描述符前继改写实际参数：成功、已有字段访问失败、真实PMR分配失败、
  攻击顺序记录访问失败。观察坐标已累计而延迟尚未增加的前继状态；
  读取新8001参数后才累计，失败保留已执行前缀并阻断计数或其后继。

零键断言固定根数量，其他键断言真实动态节点；分配失败借既有
`FixedNodeMemoryResource`制造`bad_alloc`，不注入通用分配回复。
既有全1索引、帧/坐标失败、描述符0、终止与reset、真实帧呈现返回测试继续运行。

首轮`proc_a83d`仅core运行，失败1条：新增测试误用`attack_order_typed_stop`。
实际已有状态为`attack_order_remove_typed_stop`；修正预期后重跑全部最终门禁。
首轮日志保留于`build/tmp/runtime/final-actor-profile-core.log`，不作最终通过证据。

## 4. 最终验证

受管进程`proc_f8e4`成功，最终源文件与发布文件两份SHA-256一致：

- `./build.sh core --build-target openswd3_battle_actor_frame_316_tests --test --test-regex '^battle\.actor_frame_316$'`：1/1，26.28秒。
- `./build-asan.sh --build-target openswd3_battle_actor_frame_316_tests --test --test-regex '^battle\.actor_frame_316$'`：1/1，28.87秒。
- `./build.sh app --build-target openswd3`：实际重新链接SDL程序，未运行app UT或游戏。

最终日志`build/tmp/runtime/final-actor-profile-{core,asan,sdl}-final.log`无警告或错误。
身份记录为`final-actor-profile-formatted-source.sha256`与`final-actor-profile-source-check-final.log`。
完整源码、测试差异及单条预期修正保留为`final-actor-profile-*-review*.diff`；
staged差异与完整已审查差异逐字节相同。

## 5. 验收限制

清单仍为874生产文件、27项保留分类和293项待处理，范围穷尽性仍为false。
最终角色父协议还保留帧、描述符、继续与重置查询及相关通用请求/回复/端口；
其他参数消费者尚待迁移。没有新增整族验收项，也不宣称B11、316或完整游戏已完成。
