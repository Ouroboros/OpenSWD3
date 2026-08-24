# 装备物品记录列表重建 `0x00444E80`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00444E80..0x00444EED`，52行，无FUNCTION CHUNK。callers为已关闭E40初始化、43A60 party循环和43B70列表类别循环，三处均已直接回收。

入口取party selector低16位，按原四party池解析dummy source root，并把list kind传给DB0。party索引越界只在原池表读取点typed-stop；平台池解析为null只在DB0即将解引用dummy root时停止，均不清当前输出。

DB0直接完成五值/零category抽取和哨兵排序，随后把输出head及offset8 cleared word回写state。filter index越界时，保留DB0已经清空输出的前缀并停止。

输出为空时调用44D5D0的最窄typed生命周期端口；成功后只把新节点next写null。分配失败停在原返回指针解引用前，保留空head且不写总数/窗口。公共端口通过`LegacyStandardModeEquipmentRecordListPorts`虚基在初始化与输入多继承路径共享，不复制生命周期接口。

列表非空后直接调用B980计数，依次写list offset 0、local selection 0、visible head=head，并尾调用已关闭E50发布最多24项visible count；result保留E50停止节点。空筛选因此一定产生一个缺省记录，B9C0 selected-record-missing路径在三个正常caller中不再伪造为可达。

E40、43A60、43B70原`initialize_*record_list` opaque端口已删除。caller仍把一次E80视为一次helper call；E80内部result另记录DB0、可选分配、B980和E50调用数。equal text按DB0规则前插，三项相等窗口会逆序，旧opaque测试预期已按LST校正。

UT覆盖party池抽取、非匹配保留、text排序、总数/窗口重置、空筛选缺省节点、party索引、source root、filter表和分配停止点；E40、43A60、43B70全部旧caller回归通过。

workpack双生成稳定为`112/227`，SHA256均为`3f4d3536cdeae920f14142991024012d9d54f801c73b8b190fbeaedf4960e6b6`；下一单元`0x00444F00`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
