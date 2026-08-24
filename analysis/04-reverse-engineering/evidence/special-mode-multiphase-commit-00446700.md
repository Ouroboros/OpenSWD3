# 特殊模式多阶段提交分派 `0x00446700`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00446700..0x00446F9B`，1040行、42个内部label、65个call，无外部FUNCTION CHUNK。455E0有七处code caller；3B480另把本入口写入G01第10槽。

函数按interaction mode的jump table分派2、3、4、5、10、11、15、17、18；大于等于500调用原nullsub后返回，其他mode不改状态。所有七处455E0 commit caller现均直接调用typed dispatcher，陈旧`commit_selection`端口已删除；提交typed-stop立即传播，不继续执行caller后续副作用。

模式2先按selection读取当前记录，selection31绕过记录门。selection31按4482E0清链、释放inventory root、回到30、播放45、448230重建并由BCC0刷新。selection32写动作1，按记录0x20 flag进入模式5或播放140，然后刷新。selection30保留记录type flag门及特殊ID分支：2D9请求世界转场；318进入500并初始化高模式；2B9执行77查询与模式15筛选表；2B8/2BA执行78查询、BE40分组、inventory移除、BFC0对话准备和455A0清理；301合并记录与inventory跨度进入模式10；2DB保留22/23门并请求战斗；普通记录加载40字节payload、按30..33存在性复制槽、移除物品并刷新。已关闭B9C0/BCC0/BE40/BE90/BFC0/455A0/46FE0均直接复用；尚未关闭的inventory、资源、战斗和动作owner收敛在独立InteractionCommitPorts。

模式3保留slot复制、0x80/0x20 flag、inventory返回值、139/184音效和刷新顺序。模式4委派46FE0。模式5按动作0执行移除、184和刷新后回模式2。模式10分配216字节资源、加载选择+71、读取+0x48 flag、进入11并释放；失败回2。模式11先回10，列非零直接返回；列零执行资源确认，成功回2并按flag决定46FE0。模式15先移除2B9，再读取筛选记录并BFC0准备对话；typed vector清理后把secondary offset保留为原释放数量，清计数、455A0并清全局owner。模式17/18回2。

UT覆盖特殊世界转场、318高模式、2B9受限阶段、301计数、普通装备副作用前缀、模式3返回值分支、模式4、模式10/11、模式15、17/18、selection31双45 caller链及七处455E0回归。存在性夹具扩为512项，修复测试自身对77索引的越界；独立ASan通过。

workpack双生成稳定为`134/227`，SHA256均为`06707b2e55251a843eb91e79aa9d22845dd968c2ac8c3961510cbdac48f99026`；下一单元`0x00446FE0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
