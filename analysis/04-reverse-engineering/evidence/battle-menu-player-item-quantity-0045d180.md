# 战斗菜单提交动作和替换效果时增加实际物品数量

状态：`pending`。

实现源码`52e53a9b`。回收462FBC、463ACC、463B60三处45D180调用，
删除误名为stop_effect_sample的通用操作，直接借用共享库存和既有MON数据库。
状态为`pending`。
完整菜单父和数量接口族保持`pending`，不标`assembly_exact`。

## 1. 权威范围

父462740..4640F1共2820行，95个静态call、37个RET，无外部chunk；唯一caller462318。
主message3为462E2A..46314B共339行；主message5为4639CF..463C01共226行。
数量叶45D180..45D242共109行、cdecl两个DWORD，完整十三个调用见
[物品节点数量](battle-player-item-quantity-0045d180.md)。
主switch和message1内嵌switch已分开；旧case-eleven摘录错误保留，实际属于message5。
独立分析与精确摘录在`build/tmp/runtime/menu-player-item-quantity-*`，
完整双向记录为`menu-player-item-quantity-bidirectional-review.md`。

## 2. 动作提交后的固定道具

message3保持原早返、动作/角色提交、清queued、对象刷新及输入门清除前缀。
动作完整30时先写动作3并解析效果，随后重新读committed角色和实际位13覆盖条件。
真实覆盖值仅0/1；真写动作13及原特殊计数，假直接增加item0300的数量A。
selector为完整DWORD0；原数量payload没有业务消费者，不重建叶EAX回复。
数量成功后重新读committed和live动作4，再执行原后继、两组对象清理及缓存尾部。
动作4保持独立后继判断，不用动作30的else-if隐藏前继改写。

## 3. 效果替换前返还旧道具

message5保留资源释放、效果低WORD、动作15提交、输入预置、共享门和辅助行清理。
索引为u32(mode−零扩展效果高WORD+group_a_count−1)，只计算并保存一次。
完整mode1/2才进入效果路径；旧item WORD零时跳过数量，非零以完整WORD和selector0增加。
数量成功后才OR规则低BYTE4，再读取live效果和mirror，写item、X300、Y270/340及active1。
mirror完整DWORD等于1才镜像；其他值不镜像。第一样本后重新读mode和发布保存索引低WORD。
第二段是独立完整mode2判断，沿用保存索引，可以再次修改同一物理行。
共尾保留三项cache、效果低WORD、menu WORD、角色结果WORD和统计调用顺序。
固定状态样本前继改写只验证读取时点，不证明原样本生产者会改写mode。

## 4. 库存所有权和真实失败

必需items引用从核心帧经输入和菜单进入传至刷新；消息处理借其既有库存引用。
42个既有核心帧夹具传各自Port库存，未另建同步副本。
实际节点仍由共享PMR容器拥有，数量函数和MON实现未重写；新增父状态只传播已有叶失败。
未知节点保留旧head；真实bad_alloc发布head0并保留旧拥有节点；MON停止保留已发布新节点、
旧link、item和零数量。失败阻断规则、效果替换、样本及未到达的清理尾部，保留提交前缀。
菜单进入和确认输入分别传播已有子失败；资源释放前缀仍发生，不恢复已减数量。

## 5. 实际测试与纠错

新增283次菜单执行和4次真实调用方执行：

- 240次三站×十个signed数量组合×固定头别名/真实链节点×四个完整mirror值。
- 6次节点创建/MON正常或原非停止返回，9次节点/真实分配/MON停止。
- 7次零item/完整mode，8次索引和回绕，5次原早返，4次真实位13条件。
- 当时4次资源/样本前继向量，另4次菜单进入/输入成功与失败传播。
  后续dd7ae08f撤回其中两个纯释放叶任意改写角色的callback，改为实际提交actor9并读其override；
  另外两个真实样本后完整mode读取保持，不能把旧人工资源回调作为原版叶行为证据。

240组已有节点中数量B及定义保持；完整item8001与邻居0001不混，WORD回绕不夹值。
效果高WORD零扩展、完整DWORD计数及mode、保存行、WORD结果和邻居行分别核对。
首编译proc_e5d0发现消息处理缺库存绑定，补齐必要调用方及42个核心帧夹具。
proc_4f9b编译通过但两个输入断言失败：确认路径读record1持续计数，夹具只设置record0。
只补record1，未删除断言或改生产输入逻辑；“入口拒绝非零message”的口头归因已撤回。
原失败和错范围日志保留，最终完整相关入口通过。

## 6. 最终门禁和验收边界

proc_15a9通过，实际全部使用仓库脚本：

- core：openswd3_battle_legacy_battle_setup_tests，精确battle.legacy_battle_setup，1/1，11.35秒。
- ASan：同一目标和regex，1/1，18.59秒。
- app：openswd3目标实际133/133重新链接SDL程序，未启动游戏。
- 14份实际门禁源码SHA一致，生产223行和测试1158行差异完整审查并与暂存逐字比较。

来源审计proc_ecdc通过，核对三站、九个直接C++数量位置、真实绑定、冻结身份和门禁。
日志在`build/tmp/runtime/menu-player-item-quantity-{core-final,asan,sdl,source-review}.log`。
本批未执行全量UT、Windows、真实MON资产、原版动态差分或完整SDL战斗生命周期。
十三个原物理数量站均已接通，不关闭父寄存器/参数包/计数/Port、共享别名及其余资源协议。
整体范围穷尽性仍false，Workpack316和B11验收不变，整体Goal未完成。
