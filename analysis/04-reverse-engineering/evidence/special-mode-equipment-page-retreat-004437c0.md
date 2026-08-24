# 装备物品模式分页后退 `0x004437C0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004437C0..0x004438D4`，144行，无FUNCTION CHUNK。code caller为F40两处，3B480另绑定为动作callback；F40现已直接回收。B9A0、B9C0、B9E0、BC60与444E50已关闭并直接复用；sample46保留平台端口。

## mode1分页

若local selection的signed值大于1，只写`local &= 1`并返回，不翻页、不刷新文本。

否则先无条件按u32写`list_offset -= 24`：

- 结果signed非负：先B9A0重建visible head、直接调用444E50刷新，返回后才写`local &= 1`。
- 结果signed为负：先写`local &= 1`、offset=0，再B9A0/444E50。

两路随后以`local+offset`调用B9C0/B9E0，写`4FD080=3`，再播放sample46并返回EAX。

B9C0 null与B9E0失败保留刷新及local/offset，不写3、不播放sample。

## mode2与mode15

mode2固定从party0向3扫描，选择最低非FFFF项；四项全FFFF时原函数继续向表后越界，modern完整读取四项后typed-stop。

mode15直接复用BC60，step=8；该helper只读取special offset、hover selection和step。返回后`or ah,3`即OR `0x0300`，不读取/改写special total或visible count。其他mode返回`mode-15`。

F40 mode1/mode15两处page retreat矩形直接调用本helper，传播selected record、shared text及party search状态。

UT覆盖页内归一、offset24退至0并重建24项visible/text/sample、零页负offset归零、B9C0/B9E0停止；mode2最低party/全FFFF；mode15 step8和`ABCD0001 -> ABCD0301`。F40对应scroll回归通过。

workpack双生成稳定为`102/227`，SHA256均为`6e11bcf5e9d99608de1e813ab2c614a4589018c3a773d69c2763902a3bae7013`；下一单元`0x004438E0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
