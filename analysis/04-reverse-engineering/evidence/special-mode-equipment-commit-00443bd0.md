# 装备物品模式提交状态机 `0x00443BD0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00443BD0..0x00444169`，678行，无FUNCTION CHUNK。F40有五个code callsite，3B480另绑定为动作callback；F40统一commit分派现已直接回收。B9C0、BE40、BE90、BFC0、442F10与44D6E0均已关闭并直接复用；装备端口只提供目标party的21-word属性记录，源贡献从已选装备176字节记录解析后直接应用。40DC50查询和476A80动作加载仍保持独立窄端口，4885A0释放由typed容器/cleanup端口表达，sample8B/8C保持平台端口。

## mode分派

入口保留`mode-1`残值。mode5写mode1及panel motion -128；mode17/18只写mode1；mode3/4、6..14、16与其他越界mode不写状态。mode1、mode2、mode15才索引当前记录。mode1的FFDC早退发生在party selector数组读取前；modern保持该顺序。

## mode1

先检查party装备gate非负、记录type低四位至少2，并按cost的bit15/bit14分别统计主/副资源要求。要求为0或任一不足均只播放sample8C。

- 记录0619：仅当物品4E不存在时调用BE40。找到MAPS值组后按组首三个u16调用BFC0，先按原`cost&7FFF`扣主资源，再直接调用442F10清理并清interaction block；物品已存在或值组未命中时进入mode18并播放sample8C。物品存在时不读取MAPS。
- 记录061E：物品4D存在或source gate不等于1时进入mode17并播放sample8C；否则先写mode15，再用BE90重建筛选表，清窗口offset/cursor，visible取`min(count,8)`。解析失败保留mode15及BE90已提交前缀。
- 普通记录：动作加载返回不等于1时直接返回。flags bit0为0时写mode2及transition word3，不扣资源；bit0为1时重新按原条件扣资源，先播放sample8B，再对物品1E..21逐项非零party执行装备复制。

## mode2

按bit15/bit14顺序检查并立即扣主/副资源。若主资源成功但副资源不足，主资源扣除不回滚，随后播放sample8C。全部要求成功且至少一项存在时，查询`selected_party_action+1E`；存在则复制到目标party后播放sample8B，不存在则保留资源扣除并直接返回。目标越界与复制端口停止均发生在资源扣除之后。

## mode15

先无条件按`cost&7FFF`扣当前party主资源，再按special offset+hover索引BE90记录；越界typed-stop保留扣除。筛选记录的first value低/高u16及second value依次传给BFC0。对话成功后释放全部typed筛选记录，使special offset结束在原记录数、count归零，再直接调用442F10并清interaction block。对话或cleanup停止保留此前副作用。

## typed边界与验证

state明确拥有四party装备gate、主副资源、动作/筛选/对话/转场owner。动作加载、装备复制、筛选查询、对话接口和清理端口只隔离尚未关闭的平台或业务callee，不隐藏已关闭helper。

UT覆盖六类mode、FFDC早退顺序、门禁/零需求、普通动作两类flags、四party复制、0619命中/存在短路/值组停止/对话/cleanup、061E阻断/筛选成功/筛选停止、mode15提交/索引/对话、mode2部分扣除/成功/目标与复制停止，以及selected/party/action-load边界。F40验证commit不再进入generic target，并直接传播typed-stop。

workpack双生成稳定为`107/227`，SHA256均为`1b0c466a94723454191aa1b0b7e3129f59d1d25644fe3aa1da926c323ce38ca9`；下一单元`0x004441A0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
