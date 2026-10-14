# 战斗角色带标记资源的名称与数量查询 `0x00470F70`

状态：`pending`。

当前切片：源码`84d3f985`已迁移实际查询与唯一网格消费，定向验证通过。
完整列表、网格、字体及其它父接口族继续`pending`，不增加整族验收数。
历史函数还原的`platform_adapted`分类保留，不升级`assembly_exact`。

## 1. 权威范围与业务输入

权威LST为`0x00470F70..0x00470FD5`，proc至endp共60行、36条实际指令，
2个call、4个跳转、2个返回，无外部chunk。原ECX借角色，四个栈DWORD，
RET10h；第一参数未读，第二是完整DWORD序号，第三、第四分别是文字和数量目标。
唯一实际call为`00466020`；父`465E50..466181`共357行。

先独立写入LST基本块、顺序、出口和测试向量，再读取本批C++与测试。
材料为`build/tmp/runtime/flagged-resource-display-lst-analysis.md`、
`flagged-resource-display-sub_470F70.lst`及`flagged-resource-display-sub_465E50.lst`。
权威原LST SHA256：
`701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b`。

实际接口借`LegacyBattleActorListQueryState*`、DWORD序号、可写BYTE span
和数量WORD引用；仅返回既有访问状态与语义`found`。
删除叶请求结构、actor寄存器token、输入/返回寄存器、访问/命中诊断计数，
不以操作编号、参数数组、伪查询回复或额外Port包装实际查询。

## 2. 扫描及全部出口

470F76先发布next根到current，局部DWORD筛选序号从0开始。
470F84..F8E先读当前节点next并写回current，再处理next0。
正常耗尽返回`completed/false`，文字与数量均不写；真实节点访问失败沿既有
`resource_node_typed_stop`，不能冒充耗尽。缺实际list沿`list_owner_typed_stop`。

470F96测试候选`+4C` WORD bit13；命中才递增序号，但470F9D在每个非空
节点之后均比较完整DWORD。序号0可在首个未标记候选陈旧成功，已经增加的
序号不被后续未标记节点清除。保留DWORD回绕、原扫描与无匹配耗尽，不增加上限。

470FAA..FB3按首NUL复制名称及终止符，目标终止符后的字节保持。
20-byte caller边界适配仍在原复制时点返回`list_text_typed_stop`，不截断。
旧C++用整个std::string长度判定边界并清空整块目标；两处差异已纠正。

470FB9在复制后重读实际共享current，再470FC4读tertiary WORD，470FC8
加secondary WORD，470FCD只写数量WORD。保留位型与16位回绕。
真实名称目标别名可改变head；后继查找失败保留已写文字和旧数量，复用原节点
访问失败状态，不回滚、不新增状态。只有文字及数量完成后`found=true`。
next、selected、节点内容与资源存储所有权不由本叶修改。

## 3. 唯一网格消费与转发回收

父计数返回后465FC7/65FE6、每次查询465FF8/466020和耗尽后466147/66166
均重读live队员并借实际资源。沿既有对象访问失败状态处理合法视图缺口，
不回退到初始队员、不生成默认列表。首轮C++仍固定借初始party，完整复核发现
后已纠正并重跑全部本批门禁；首轮日志和九源SHA以before-live-actor前缀保留。

查询直接写原20-byte文字和数量WORD，正常耗尽保留上次输出后才发布实际根。
466031在成功后检查显示行数低WORD是否已到7；第八次成功保留节点和输出，
不绘制、不尾发布。真实失败保持发布/面板/标题/字体前缀，阻断行与收尾后继。

名称主色直接取实际WORD；flags10字体路径436AD0→436410→4358C0中，
435991明确AND FFFF，高位不进入颜色像素，取消查询模拟ECX高位来源。
46606F按signed WORD格式化为`%2d`；名称、数量和选中重绘仍在各原时点读颜色。
选中行成功后仍发布完整序号与输入门。

删除网格本查询脚本回复、嵌套回包、`row_query_calls/scanned_rows`、数量目标
token，以及共用query枚举和主帧纯转发映射。初始化、字体、绘制及其余寄存器/
计数/Port继续待迁移。选择帧message27用实际空链哨兵验证原正常返回后缀。

## 4. 固定执行与实际门禁

新增184次固定执行，纳入既有注册入口：

- 叶111次：7数量位型×3名称×5序号=105；缺根、缺next、空文字span3；
  复制后真实head别名2；缺list1。
- 网格73次：十实际槽位×7数量位型=70；DWORD滚动回绕/缺序号2；
  20-byte真实复制边界失败1。核对signed显示、短名称尾部、耗尽输出和选择发布。

同步既有叶和consumer发布、十槽网格满页/耗尽、真实七行/第八项、选择帧
及其余已登记菜单测试。旧纯查询callback的光栅故障放在已有文字绘制前继，
不在资源查询伪造副作用或回复。

最终实际执行：

- `./build.sh core --build-target openswd3_battle_legacy_battle_setup_tests
  --test --test-regex '^battle\.legacy_battle_setup$'`：1/1，11.79秒。
- core actor目标`openswd3_battle_actor_frame_316_tests`，regex
  `^battle\.actor_frame_316$`：1/1，31.85秒。
- `./build-asan.sh`同两目标/regex：各1/1，18.56/31.79秒。
- `./build.sh app --build-target openswd3`：136/136实际重新链接SDL程序。
- 九份已验证源码SHA、完整351行生产/755行测试差异、相同暂存差异、
  whitespace与双向审查通过； source review `proc_c2f3`成功。

门禁日志、身份清单、双向审查和完整差异位于
`build/tmp/runtime/flagged-resource-display-*`；最终门禁进程为`proc_2699`。
未执行全量UT、Windows、实际MON、游戏或原版联合差分。

## 5. 历史与剩余边界

3eda365b已删除470F76根发布的叶EAX转发和提交计数，见
[资源链头发布](battle-actor-resource-list-commit-00470900.md)。
旧audit_order201的core188/188、app194/194及历史inventory
`201/422 = 192 platform_adapted + 9 assembly_exact + 221 pending_audit`
只属于原还原记录；历史生成SHA256为
`83d366d03421276e5ce1e0580e59cdce4a115028c0d4493d763a5d12f5afb40a`。
这些记录不替代本批门禁，不前移315/422、316或B11。

原版资源链、名称、数量与caller联合捕获仍缺失，保留
`blocked_runtime_oracle`。本批固定状态与链接不证明实际资源生产、完整SDL
战斗生命周期、游戏运行或原版动态差分。全项目清理仍未完成。
