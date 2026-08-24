# 装备物品模式推进 `0x00443450`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00443450..0x00443568`，140行，无FUNCTION CHUNK。code caller为F40两处，3B480另绑定为动作callback；F40现已直接回收。直接callee B9A0、BC90、B9C0、B9E0、BB80均已关闭并直接复用；仅sample46保留平台端口。

## mode1两列列表

local selection先按u32加2，再以signed i32和visible count比较。越界时，若`visible_count + list_offset < total_count`的signed比较成立则list offset加2；无论是否滚动，local再减2。

随后严格执行：

1. B9A0以调用后list offset重建visible head。
2. BC90从visible head最多计24项并回写visible count。
3. 重读local；若仍以signed比较大于等于新visible count，再减2。
4. B9C0以`local + list_offset`的u32回绕值索引原head。
5. B9E0发布选中记录文本。
6. 写`4FD080=0x30`，播放sample46并返回其EAX。

B9C0 null与B9E0失败在原读取点typed-stop；此前offset/local/visible head/count完整保留，且不写0x30、不播放sample。

## mode2 party推进

从current party action开始递增；signed值达到4即归零，跳过首word为FFFF的party，找到后才发布selection并返回其索引。四项全FFFF时原函数无限循环；modern在完整四项均检查后typed-stop，不伪造selection。

## mode15特殊列表

直接复用BB80：以special total、window offset、hover selection、visible count推进。返回后重读`4FD080`并执行`or ah,0x30`，即只OR bit `0x3000`，不等价于赋值0x30；发布并返回完整EAX。

其他mode返回原dispatch算术`mode-15`。

F40 mode1和mode15的advance rectangle不再调用opaque target；直接调用本helper，传播selected record、shared text和party-cycle typed status。可变state均在callee返回后重读。

UT覆盖mode1跨两列滚动、B9A0/BC90结果、FFDC文本/sample、B9C0 null、B9E0失败；mode2跳过FFFF及全空停止；mode15八行window推进与`ABCD0001 -> ABCD3001`。F40四类mode1/mode15 scrollbar回归通过。

workpack双生成稳定为`99/227`，SHA256均为`b749e5b310dcbd7856d03a6069c9b31c632d107b13108ceef95366673d6a8470`；下一单元`0x00443570`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
