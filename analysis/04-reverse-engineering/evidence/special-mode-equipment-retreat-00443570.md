# 装备物品模式后退 `0x00443570`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00443570..0x0044366D`，128行，无FUNCTION CHUNK。code caller为F40两处，3B480另绑定为动作callback；F40现已直接回收。直接callee B9A0、BC90、B9C0、B9E0、BBC0均已关闭并直接复用；仅sample46保留平台端口。

## mode1两列列表

先按u32把local selection减2，并以signed i32检查负值。若为负，local恢复原值；仅当list offset的signed值大于0时offset减2。随后严格执行B9A0重建visible head、BC90最多计24项、B9C0以`local+offset`回绕值索引原head、B9E0发布文本，最后写`4FD080=3`、播放sample46并返回其EAX。

与推进不同，后退在BC90后不再次按visible count夹local。B9C0 null与B9E0失败在原读取点typed-stop，保留local/offset、visible head/count及已有文本，不写3、不播放sample。

## mode2 party后退

从current party action递减；signed值小于0即回绕3，跳过首word为FFFF的party，找到后才发布selection并返回索引。四项全FFFF时原函数无限循环；modern完整检查四项后typed-stop。

## mode15特殊列表

直接复用BBC0，以special window offset和hover selection后退；返回后重读`4FD080`并执行`or ah,3`，即OR `0x0300`而非赋值3，发布并返回完整EAX。其他mode返回dispatch算术`mode-15`。

F40 mode1/mode15两处retreat rectangle直接调用本helper并传播selected record、shared text和party-cycle typed status。可变state均在callee返回后重读。

UT覆盖mode1 local负值恢复、正offset减2、B9A0/BC90/B9C0/B9E0文本链、sample与final3，另覆盖B9C0 null和B9E0失败；mode2从0回绕3、全FFFF停止；mode15 `ABCD0001 -> ABCD0301`及window后退。F40四类mode1/mode15 scrollbar回归通过。

workpack双生成稳定为`100/227`，SHA256均为`f7fcf6c5bc919e9541f52682934c2eb105c1c3a5b2c21758c1c1a545ab381f8a`；下一单元`0x00443670`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
