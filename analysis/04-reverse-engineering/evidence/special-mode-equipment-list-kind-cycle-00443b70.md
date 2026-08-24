# 装备物品模式列表类别循环 `0x00443B70`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00443B70..0x00443BCB`，51行，无FUNCTION CHUNK。code caller为F40一处，3B480另绑定为动作callback；F40现已直接回收。B9C0、B9E0已关闭并直接复用；444F00、444E80保持最窄typed端口，sample46保留平台端口。

入口先计算`mode-1`作为legacy EAX；只有mode1继续。成功路径先调用444F00等价清理，再按u32递增list kind；仅当递增结果精确等于3时写0，原值3递增为4时不做额外范围修复。随后调用444E80等价记录列表重建，callee可改record head、offset及local，后续全部重读。

以`offset+local`直接调用B9C0，再调用B9E0发布共享文本；最后播放sample46并返回callee EAX。444F00/444E80不可用时在各原call site停止；B9C0 null与B9E0失败保留已提交的list kind及列表重建副作用，不播放sample。

F40在overlay返回后重读input snapshot和mode，再计算目标类别并先写`target-1`；对应caller已直接调用本helper，使原递增得到目标类别并传播sample返回值。通用list-kind callback边界已移除；typed-stop状态直接返回F40。

UT覆盖`2 -> 0`精确回绕、`3 -> 4`原始行为、非mode1返回、清理/记录列表/B9C0/B9E0停止、文本与sample顺序，以及F40 overlay同时改写坐标后重读并循环类别。

workpack双生成稳定为`106/227`，SHA256均为`5129b38a31fcb38a1bdc2fb1023c61b5d82bb062a47b804aabed12f92c3236f1`；下一单元`0x00443BD0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
