# 战斗角色资源选择与资料应用 `0x00470AC0`

状态：`platform_adapted`；4b163e92迁移选择接口及三个真实调用点，已推送并核对远端main。
角色资源列表整链、选择诊断转发及完整菜单父协议仍未验收。
旧文档的“三处已完整回收”结论被本次实物审计纠正，历史全量门禁不证明本批。

## 1. 权威范围

LST为`0x00470AC0..0x00470E15`，原行213874..214274，共401行、
244条指令、5个call、41个跳转、25个局部标签、4个返回点，无外部chunk。
LST SHA256为
`701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b`。
三个静态caller均位于462740：4631F6、4636CF、46377C。

## 2. 语义接口与共享输出

删除选择Request、actor/profile伪地址、入口及返回寄存器、调用与写入计数。
参数为实际list、configuration、final、item、workspace，以及完整DWORD类别和序号。
结果只保留状态与业务成功；输出直接借caller DWORD和WORD，不返回输出副本。

入口清40字节profile、16字节pre-effect和workspace的2F14 WORD。
不预清两个caller输出、selected及容量required字段。
序号零立即返回业务失败，保留原根及输出。非零先发布根，再逐节点推进current。
类别0..5映射为10/0C/1001/0800/2000/08000000，其余DWORD原样使用。
普通资格要求mode05、category相交、signed tertiary减unsigned primary加signed
secondary严格大于零。最终mask2000且capacity bit13时独立再计数一次；
两次增量后才比较序号，因此同节点可跳过序号1而命中2。没有扫描上限。

## 3. 命中、资料与失败副作用

470BAA先置item mode掩码0x40，发布selected，再清caller WORD。
mask2000且当前capacity bit13时，以4A资料号加载，重读current，将30 WORD写入
item derived slot0后成功返回。该分支保留caller DWORD，不增加数量。
普通路径优先使用54非零资料号，否则使用4A；普通文件打开失败不构成业务失败门。

加载后重读current。capacity bit15先发布低15位required，与配置6偏移的signed
WORD比较；通过后再读capacity，bit14发布低14位required，与配置8偏移比较。
任一超限均保留已发生的清零、选择、资料加载、required和输出WORD清零；
caller DWORD保留旧值，未到达的required字段不改。

48 WORD为零时复制40/42/44三WORD；随后清profile copy latch，48的0200位可再次
复制并置latch。原flags_49重复保存48高字节，现已删除，统一读取实际WORD。

## 4. 输出与数量

470D3B读取刚加载的profile_buffer[5]低WORD，零扩展写caller DWORD。
旧action.copied_runtime_word影子不再作为该输出来源。
重新读取profile WORD，零时调用诊断；返回后继续原后缀。
资料I/O、诊断及可能改变current的输出写入之后，按LST时点重取节点，
不保留跨callback的vector元素指针。

当前category掩码0x0800置item掩码0x10并发布5C到workspace 2F14。
mask及category共同含08时输出mode1，mask含10也输出mode1；
旧实现误测0800已纠正，类别3不能因此被当作mode1。
精确mask08000000先置item掩码0x20并输出2，再重读ID；ID非0300时输出3并清0x20。
最后重读current：category80且不含08000000，或mask精确0800时不加数量；
其余primary WORD加一回绕。

## 5. 实际调用方

- 4631F6：message4按live类别及序号选择，直接写startup十槽DWORD和本地WORD。
  原resolve_action_target通用调用删除；失败按原类别、WORD、说明和声音分支退出。
- 4636CF：message27固定类别4；特殊成功保留原startup DWORD。
- 46377C：message30固定类别5；失败声音及成功后重读角色顺序保持。

三个caller删除脚本选择兼容开关、子寄存器回复及调用计数。
本地DWORD只替换低WORD，高WORD保留；业务失败和既有宿主访问停止分别传播。

同时纠正相邻462FEB：原call为4708C0，之前非脚本分支错接470AC0。
现调用实际动作4完成函数，置mode掩码0x02与completion，按非零replacement写动作，
清76个early workspace DWORD；不选择资源或重载资料。
apply_special_actor_action误名操作一并删除。

## 6. 测试与审查

独立向量覆盖六类及raw mask、完整DWORD序号、零入口、signed数量正/零/负、
双重计数、普通/替代/特殊资料、signed容量边界及两个门的失败副作用。
还覆盖profile来源、输出宽度、旧输出保留、诊断继续、输出别名、callback改变current、
WORD数量回绕和同址48/49字段。

菜单夹具改用实际资源链和MON资料。资料重载后的字节预期按476A80的tag写集合推导；
未被tag写入的字节在选择入口清零后不能要求保留旧哨兵。
目标模式向量保留真实类别，并使用该类别实际可能产生的模式。
旧脚本可强制成功的耗尽资源不能用来证明装备选择后释放；装备路径改为验证实际可用
资源的“选择增加、随后释放”顺序。另两个直接释放路径继续覆盖耗尽、删链和失败边界。
动作4独立caller向量覆盖十个角色、零及非零replacement，核对资料和资源保持。

本轮四项定向门及SDL构建已通过，日志为`build/tmp/runtime/`下
`resource-selection-final-{setup-core,actor-core,setup-asan,actor-asan,sdl}.log`。
补充动作4断言后，菜单core/ASan再次各1/1通过，分别11.00/19.59秒，日志为
`resource-selection-caller-{core,asan}.log`；角色帧core/ASan各1/1为25.81/27.11秒。
两轮源码SHA清单仅菜单测试文件变化；五份生产源码及另一测试文件未变。
最终七文件身份检查通过，见`resource-selection-caller-identities.log`。
退休请求、操作、兼容开关及选择计数在include/src/tests全域扫描无命中。

## 7. 未验收边界

后续诊断批次已删除ResourceSelectionAdapter及resource_missing_word_diagnostic，
核心调用方显式借同一窄诊断接口，SDL窗口owner提供维护错误对话框实现。
完整SDL帧接线与实际交互未验收，Port尚不勾选合法保留；
见[诊断证据](battle-resource-selection-diagnostic-00431150.md)及[invoke Port清单](invoke-removal-ports.md)。

同族状态、资料、资源生产，以及菜单父操作/参数包/寄存器/计数/Port仍pending。
无效实际list、节点、配置和资料访问的typed-stop是宿主访问边界，不能证明原版非法
地址故障等价；不标assembly_exact。
本批未执行全量UT、Windows验证、真实游戏、原版动态差分或新增真实资产验收。
