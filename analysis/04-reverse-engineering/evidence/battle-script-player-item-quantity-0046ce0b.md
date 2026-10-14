# 战斗脚本增加道具与登记奖励

状态：`pending`。已废弃GPT重建实现，本轮逆向尚未开始。

以下保留历史分析与证据；旧结论、完成记录和测试结果不作为本轮验收。
原版采集事实保留，所有解释和实现结论须重新核实。

## 历史资料（重新核实前不生效）


实现源码`6aa4ffa3`。脚本53直接借用世界共享物品栏和现有MON数据库，
删除对应callee枚举和通用调用；节点由实际容器分配并拥有。
本切片为`platform_adapted`、`unit_tested`、`fixed_state_tested`；
脚本父协议和数量接口族继续`pending`，不标`assembly_exact`。

## 1. 权威范围与归属纠正

LST父函数`00469D20..0046DF35`；本块为`0046CE0B..0046CE5C`，
完整35个物理行、22条机器指令、1个call、无跳转、1个RET。
唯一数量调用在0046CE1B，参数为item和精确DWORD1；没有音频后缀。

此前数量证据、模块和清单将该调用写为脚本51，归属错误，本批纠正为53。
脚本51在0046CDCE..0046CE0A调用47F3A0，脚本55从0046CE5D开始，均未随本批迁移。
该批发布时，13个45D180物理调用有10处对应6个直接C++调用位置；
菜单三站当时未接通。后续52e53a9b接通462FBC/463ACC/463B60，十三站对应九位置，见
[菜单实际物品数量](battle-menu-player-item-quantity-0045d180.md)。
父协议仍在，不宣称完整接口族关闭，也不把后续验证倒填到本脚本批次。

## 2. 输入、共享状态与原顺序

CE0B读取入口cursor+2的item WORD；MOV CX只替换低WORD，
直接数量输入保留继承的高WORD，数量查找仍用完整低WORD，不做3FFF掩码。
完整MON函数476DB0..47720F共586行：476E9F显式AND ECX,FFFF，
目录只观察低WORD；477167的126特例也只比较WORD。目录不是完整参数回显。

CE11在数量前保存sample WORD；使用保存槽，数量前继可以改写live sample。
真实数量及MON正常后才执行原奖励槽访问；既有无法表示槽停止仍发生在数量之后。
顺序为实际payload发布→保存live cursor→INC保存槽数量WORD→
从保存cursor+2重新读item→INC live sample WORD→写保存槽item→
保存cursor加4并返回1。地址及游标使用DWORD回绕，数量与sample使用WORD回绕。
不复用初始item，不把最终cursor+4建立在再次读取的live cursor上。

SDL唯一聚合绑定明确传`world_item_lists_`；测试Fixture明确拥有同资源物品列表。
本块不使用ScriptPort继承的默认隐藏库存，不增加第二个拥有者或业务包装。
其他脚本对Port库存的借用尚在，不据此宣称整条脚本状态已统一。

## 3. 实际失败及提交前缀

数量叶的实际分配、定义或节点访问失败映射既有`player_item_typed_stop`，
奖励槽、计数、item及cursor后缀均不执行，没有新增生产停止条件。
真实分配失败发布head0并保留旧拥有节点；定义停止保留已发布新节点及零数量。
数量正常但槽不可表示时复用既有`shared_state_typed_stop`，保留真实数量结果。
后继脚本读取停止保留真实item、payload及槽数量递增；live sample、槽item及cursor
后缀不提交。最初operand短读发生在数量之前，不分配节点或访问MON。

## 4. 定向测试与失败纠正

新增66次脚本执行：40个十槽/WORD组合、14个已有实际节点的signed数量组合，
4个槽10/FFFF与item0/非零组合、3个真实分配/定义/链访问失败，
前继改写、后继短读、DWORD cursor回绕各1次，以及2个入口短读。
另同步既有槽10用例，共67次相关执行。

核对零item固定payload且不分配、不加载，非零实际拥有节点、定义bit与数量，
97/98/FFFF/7FFF/8000的signed阈值及WORD回绕、实际块析构释放、
保存槽与live counter、前继重新定位cursor及更换operand、完整后继失败前缀。
所有本块执行都无generic script调用。MON通过既有IO字节夹具，未执行实际脚本资产或MON资产验证。

首轮proc_852a编译失败，测试三处误用allow_allocation；改为既有allocation_enabled。
第二轮proc_6297的core失败30条目录断言，错误期待完整DWORD回显。
完整MON LST确认AND FFFF后修正测试期望，生产业务不改；失败日志保留。

## 5. 最终验证与边界

最终proc_07e1经仓库脚本执行并通过：

- battle.legacy_battle_setup：core1/1，10.86秒；ASan1/1，16.59秒。
- `./build.sh app --build-target openswd3`：实际重新链接SDL程序，未启动游戏。
- 四文件哈希与实际门禁源码一致，完整暂存差异与已逐段审查差异一致。

目标为openswd3_battle_legacy_battle_setup_tests，精确regex为`^battle\.legacy_battle_setup$`。
测试入口实际调用新增函数，不能把注册1/1写成只有一个用例。
保留既有测试窄化warning；对应旧源码未改，SDL没有warning或error。
来源审计proc_b243核对完整物理块、顺序、枚举回收、实际绑定、身份与门禁。
日志及完整正反向审查保存在`build/tmp/runtime/script-player-item-quantity-*`。

父callee/操作编号、参数数组、寄存器请求回复、轨迹、计数及Port继续待回收。
Windows、原版动态差分、实际脚本资产、完整SDL战斗链及全游戏未验证。
整体范围穷尽性仍false，清单分类及整体GOAL均未完成。
