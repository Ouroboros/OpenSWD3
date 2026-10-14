# 战斗获得道具时的实际物品节点所有权

状态：`pending`。

源码：`f597100a`。本批删除新增物品数量链的通用分配请求，实际节点由共享物品容器拥有。
状态为`platform_adapted`、`unit_tested`、`fixed_state_tested`；整条接口族继续`pending`。
返回payload身份、父级诊断和通用端口仍在，不能据局部接线算作整项完成。

## 1. 完整LST与调用边界

权威范围`0045D180..0045D242`，完整109个物理行、2个机器call、7个跳转、6个局部标签，
无外部FUNCTION CHUNK。cdecl两个DWORD参数，保存EBX/ESI/EDI，创建分支另保存EBP。
完整摘录及独立推导见`build/tmp/runtime/player-item-quantity-function.lst`
和`player-item-quantity-lst-analysis.md`。

13个物理调用点为00454776、00454C90、00457F4D、00457FF8、0045E9D1、0045E9F6、
00462FBC、00463ACC、00463B60、004672C6、00467860、00467982、0046CE1B。
已有九处物理调用对应五个C++调用位置：行动、敌方角色帧、结果整理、消息、胜利奖励。
这些调用在f597100a显式传已有物品共享状态和MON数据库。当时其他四处未接通。
其中0046CE1B属于脚本53，先前写为51的归属错误。
后续6aa4ffa3直接连接脚本53并删除该操作枚举，见
[脚本增加道具](battle-script-player-item-quantity-0046ce0b.md)；当时其余三处未接通。
后续52e53a9b接通菜单462FBC/463ACC/463B60，十三物理站对应九个直接C++位置，见
[菜单实际物品数量](battle-menu-player-item-quantity-0045d180.md)。

两个原callee分别为`0045D1B3`的固定B0分配和`0045D1DA`的MON定义初始化。
分配callee编号、参数数组、EAX回复及动作通用Port依赖已删除；MON直接使用既有实际加载接口。
其他行动和召唤分配协议未随此节点迁移删除。

## 2. 查找、数量与返回

入口0045D187只测试item低WORD；零直接返回固定头别名payload004A994C，
不访问真实链、分配器或MON。非零先比较固定别名004A9940的item WORD，再沿offset0链接遍历。
匹配使用完整WORD，不做3FFF掩码。未知非零身份在首次实际节点访问处复用既有停止状态。

0045D204..217分别MOVSX两个数量WORD，以i32相加后JGE99。
总和>=99时不写；完整selector DWORD精确1递增B，其他值递增A。
只存WORD，保留负数、7FFF/8000解释和FFFF回绕，不增加夹值。
正常出口返回所选拥有节点的物理payload身份，即legacy_token加0C；别名为固定身份。
这些元数据继续被尚待回收的父结果消费，不重建分配器寄存器回复。

## 3. 实际分配、初始化与失败前缀

原顺序为保存旧head→分配B0→发布head→清零44 DWORD→写旧链接及item→加载定义→写数量/标记。
现代实现一次`std::pmr::list::emplace_front`实际分配并初始化拥有节点，
然后发布其共有身份、保存的旧链接和item WORD。节点构造不引入业务回调；
MON前继看到已发布且数量清零的实际节点。B0保持逻辑布局大小，主机分配还包含容器及元数据。

资源身份复用已有`next_battle_level_item_node_token`序列，未增加平行物品表或假分配。
真实分配回调看到旧head；即使回调改写当前head，新节点链接仍使用分配前保存值。
MON接口收到完整item参数，定义成功或原非停止返回后才写数量1，selector1另OR定义bit15。

原分配null先发布head0，再在首次清零访问故障。实际`std::bad_alloc`同样发布head0，
复用`allocation_typed_stop`，不释放旧拥有节点或恢复旧head，不执行初始化。
删除“模拟成功后再做主机分配”的双重分配及`host_allocation_typed_stop`。
MON停止保留已拥有且发布的新节点、旧链接、item及已有定义写入，阻断数量与标记后缀。

玩家、队伍、角色和VM暂存节点统一使用`LegacyWorldItemNodes`。
世界共享状态将同一内存资源传给玩家、4条队伍和64条角色列表；排序仅在原列表内splice。
VM跨列表splice要求双方使用同一资源。当前生产没有替换默认PMR资源；非默认测试显式
给库存和VM暂存链相同资源，验证创建、反向转移、文本释放和实际分配释放。
本批没有增加VM行为包装、分配失败模拟或新的生产停止条件。

## 4. 实际状态测试与证据纠正

两个旧灵契完成夹具的00487C10回复仍供后继物品分配消费，先前“死回复”归属不成立。
它们现在随实际物品分配迁移删除；不把固定次数链的真实PMR分配与物品节点混为一项。
行动23、敌方完成、消息99、结果奖励和胜利奖励改用实际节点身份。
结果整理、结果判断及核心帧的分配失败由真实内存资源产生，不按callee伪造null。

直接数量测试执行45次：3个零WORD、35个signed总和/完整selector组合、2次新节点头插，
实际链命中、未知链、真实分配失败各1次，以及2种MON前继/停止路径。
另检查拥有块数量、共享容器析构释放、精确item WORD、旧head观察和保存链接。

VM真实资产入口读取TALK1.DAT的764F记录，选择道具971；既有RealPorts的定义接口
仍使用固定字节夹具，不是实际MON资产验证。该记录新增一件道具并把实际节点
从暂存链转入库存；随后固定状态负增量将它转回并释放。
检查两列表分配器相等、拥有块1→0以及两链最终为空。
这一后继改写是固定状态向量，不能称为第二条原始资产记录。

## 5. 验证与失败记录

首轮`proc_d887`编译通过，core三条断言失败：两个父夹具仍模拟分配null，
消息99仍期待模拟地址。修正夹具后`proc_ae92`全部通过；继续审查及格式整理，
最终`proc_1ab8`通过。源码审计`proc_14d1`核对25文件身份、完整差异、顺序及实际门禁。

核对CTest注册发现最初VM regex仅执行默认入口，新增非默认分配器用例未运行。
补跑`proc_e3eb`的实际资产入口后通过；未把默认入口通过当作资产用例通过。
最终实际验证为：

- battle.legacy_battle_setup：core1/1，10.43秒；ASan1/1，18.03秒。
- world_map.legacy_world_story_vm及其_real：core2/2，0.14/0.54秒；ASan2/2，0.44/0.86秒。
- world_map.legacy_world_item_lifecycle：core1/1，0.00秒；ASan1/1，0.02秒。
- world_map.legacy_world_save_restore：core1/1，0.04秒；ASan1/1，0.10秒。
- `./build.sh app --build-target openswd3`：实际重新链接SDL程序，未启动游戏。

构建和测试均经仓库脚本指定相应build-target与上述精确test-regex。
日志为`build/tmp/runtime/player-item-quantity-*-reviewed.log`、`*-asan.log`、
`*-published.log`及`world-vm-real-{core,asan}.log`。
25份发布源码与`player-item-quantity-formatted-source.sha256`一致；
完整staged差异与已逐段审查的生产和测试差异一致。

保留既有save-restore初始化/符号转换及两处战斗测试转换warning，未改其行为。
已修改文件出现的旧warning对应源码片段与父提交一致；没有把warning当作error或声称零警告。
未执行Windows门禁、原版动态分配差分、完整SDL战斗链或全游戏验收，不标assembly_exact。

## 6. 待处理范围

五个已直接连接调用位置、物品容器真实分配及相关消费者类型适配已验证。
脚本53数量操作在6aa4ffa3直接连接；菜单三站在52e53a9b直接连接，
283次菜单和4次实际调用方执行、定向core/ASan各1/1及SDL链接通过。
十三原物理站已接通；父callee编号/参数数组/寄存器回复/调用计数/Port仍待回收。
SDL与世界物品状态的完整游戏行为不能由链接和UT推断。
清单仍保持范围穷尽性false；完整接口族与整体GOAL均未完成。
