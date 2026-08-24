# 装备物品记录原地抽取排序 `0x00444DB0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00444DB0..0x00444E45`，104行，无FUNCTION CHUNK。唯一caller为待审`0x00444E80`，返回寄存器不被caller观察。

函数接收一个dummy source root、一个输出头结构和filter index。入口先清输出head及offset8的u16，保留offset4的sentinel text index；source首节点为空时立即返回，不读取filter表。五项表严格为`001C,001B,001F,001D,001E`。

逐节点读取offset5E的`filter_category`。仅category等于表值或等于0时抽取；其他节点保留在source链且source link推进。抽取时先在destination链寻找插入点，条件严格为`scan.text_index >= current.text_index && predecessor.text_index < current.text_index`。首项的predecessor不是负无穷，而是输出结构中保留的sentinel；该比较BUG被保留。相同text index在前驱更小时插入到已有相同项之前。

随后按原顺序从source摘链、推进current、把节点插入destination link。公共ForwardNode继续保持const读链接口；仅本helper内部对调用者保证可变的原始intrusive节点做最窄`const_cast`，避免污染既有读链ABI。

原EAX残值可能是source root、最后一次插入位置的后继指针，或最后一个跳过节点的category低16位。跨平台不伪造地址低16位，result以`legacy_return_node/legacy_return_word/returned_pointer`表达逻辑形态；唯一caller不观察该值。

filter index越界只在首节点非空且即将读取原始五项表时typed-stop，保留此前输出清空且不改source。UT覆盖匹配值与零值抽取、非匹配保留、升序及equal前插、非零sentinel比较BUG、指针/word残值、越界停止前缀和空source早退。定向测试通过。

workpack双生成稳定为`110/227`，SHA256均为`628ca2ddf3b79e44b256b5c8fb3f10690ff40d543def6c3968ccf1f985a679fb`；下一单元`0x00444E50`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
