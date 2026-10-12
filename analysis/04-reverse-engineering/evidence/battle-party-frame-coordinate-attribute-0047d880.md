# 战斗队员自动选敌人后记录坐标抑制标记

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。
实现源码：`45f5a77e4f9c43f4fd51682972ba4711668e8fd5`。
范围仅456844→47D880及必要辅助记录、实际消费者与夹具。
完整456680父链仍pending，不标`assembly_exact`。

## 1. 独立原指令与条件

完整47D880..47D890十行，无参数、内部调用、写入、分支，plain RET。
DWORD load actor+0D94→AND FF→SHR1→AND1，完整EAX只能0/1，即BYTE bit1。
独立重新枚举八call：456844、457B57、457CA9、462CCD、4634A5、4637F2、463936、46AD03。
脚本46AD03先前已迁移；本批仅456844，剩余六父点。
完整456680..45769B提取1795行、101call、84标签、157跳转、一RET，无外chunk。

456829先发布53BF60，再执行456839 readiness；45683E精确1才进入属性读取。
456844查询，456849 CMP EAX,EDI1；假跳456857保留旧DWORD，
真45684D计算index×5，456850写DWORD520E98[index×5]=1。
随后456859 bit0写不同column1。这里不写actor+2AB8或脚本全局53BFF0。
完整进度46E520..46E690共174行；46E545取2AB8，46E550比较完整1，
精确1跳无进度返回，非1继续；这是独立角色状态，不能被属性辅助标记覆盖。

## 2. 共享消费者与真实借用

本父520E98只有这一写入；真实消费者45EE70..45EFAE完整156行。
kind1从(code-8)×20五DWORD行读取；45EF1D复制column2至队列记录+18h。
列1→+14h、列0→+Ch、列3只WORD→+Ah、列4→+4h，
45EF43..53随后清完整五DWORD源行，保留复制与清零顺序。
首次误取45EDF0..45EE69另一个71行入口已保留为unrelated，未用作消费者证据。

生产frame_coordinator既有接线让attack_order_party_sources借startup.reset.block_520e90。
本批直接读actual action-execution BYTE并写同一数组column2，
删除kCallQueryPrimaryAi、invoke参数/回包及错误actor.special_ready置1。
真实2AB8字段与进度/回合门保留，无新owner、包装、counter或停点。
Fixture删除独立attack_order_party_sources数组，借同一startup.reset辅助块。
既有队列插入夹具同步该持有者，不改生产消费者或SDL接口。

## 3. 当前执行与校准

新增279次帧执行：270次十槽×九BYTE×列2旧0/1/AABBCCDD，
6次readiness0/2/FFFFFFFF与属性0/2，3次真实2AB8旧0/1/FFFFFFFF。
九BYTE为00/01/02/03/7C/80/FD/FE/FF；比较完整50DWORD，
同时按实际bit0/bit1更新各自列，假路径保留旧值。
角色2AB8种子AABBCCDD保持；2AB0由原进度置1，精确1角色门抑制属性路径。
原最终45AA00→4750C0仍只清当前profile属性，其它角色保持。

270次帧后再实际插入攻击顺序，验证五个源值、WORD截断、记录身份、
原行清零、邻行隔离及两个真实门清零。是270次真实消费者执行。
同步既有285次bit0执行：剩余bit1模拟回复撤回，观察移到真实readiness前继，
完整辅助快照纳入实际column2变化；旧独立消费与失败路径保留。
既有AI和队列夹具分别改实际属性3与同一辅助块；其它合法角色状态夹具不删除。
新测试局部类型别名在构建前补齐，没有失败门禁或生产行为修补。

## 4. 实际验证与发布

proc_c75b成功125秒：core actor_frame_316 1/1 26.41秒，
ASan同入口1/1 27.78秒，SDL实际重新链接，无warning/error。
两源SHA与空白通过；proc_9da2原leaf/八caller/共享消费者/进度门/
生产借用/注册/测试身份审计成功2秒。
完整生产28行、测试318行差异已读取；stage与已审差异逐字相同。
两个文件228行增加、19行删除，实现已提交并推送。
原指令、独立分析、差异、hash与日志见build/tmp/runtime/party-frame-coordinate-attribute-*。

## 5. 未完成范围

后续d8a93827回收四个47D880菜单站，见
[菜单坐标属性](battle-menu-party-coordinate-attribute-0047d880.md)。
后续371fffc0回收四个47D8D0菜单站，见
[菜单目标限制](battle-menu-party-target-selection-block-0047d8d0.md)。
原八站均读实际BYTE，完整父链继续pending。
五个47D8B0站已使用实际WORD，完整父链仍pending，见
[菜单标记数值](battle-menu-party-profile-word-0047d8b0.md)及
[敌人帧属性](battle-enemy-frame-coordinate-attribute-0047d880.md)。
其它父入口属性、actor-ready八槽、共享状态影子、地址保护、操作/参数数组、
寄存器报告/轨迹/计数/Port及资源仍待迁移；完整父逐出口收敛未验收。
完整拥有对象验证不证明原部分地址/page fault等价；没有原版差分、Windows、
全量UT、实际资产或SDL游戏运行验收，Workpack316/B11状态不变。
[目标标记历史](battle-party-frame-target-selection-0047d8d0.md)、
[脚本坐标属性历史](battle-script-party-coordinate-suppression-0047d880.md)、
[队员父帧](battle-group-a-frame-00456680.md)、
[全项目清单](invoke-migration-progress.md)。
