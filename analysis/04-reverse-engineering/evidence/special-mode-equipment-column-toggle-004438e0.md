# 装备物品模式行内列切换 `0x004438E0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004438E0..0x0044399D`，99行，无FUNCTION CHUNK。唯一引用来自3B480数据callback槽G03/动作4；无code caller可回收。B9C0、B9E0、BBC0已关闭并直接复用，sample46保留平台端口。

## mode1行内切换

函数由local selection最低位计算步长：偶数加1，奇数减1。运算按u32回绕并立即写回。随后按signed i32比较local与visible count；若local不小于visible，写`visible_count - 1`。因此最后一个无配对偶数项会夹回自身；visible为0时会写FFFFFFFF，并按B9C0的非正索引行为取head，不增加现代范围修复。

以`local+list_offset`调用B9C0，再用B9E0刷新共享文本。成功后先播放sample46，callee返回才写`4FD080=3`。B9C0 null与B9E0失败保留local写入，不播放sample、不写3。

## mode2与mode15

mode2从party0向3读取每项首word，选择最低非FFFF项。四项全FFFF时原函数继续向表后越界；modern完整读取四项后typed-stop。

mode15直接复用BBC0，只把special offset与hover cursor传入：hover非零时减1且offset不变；hover从0减为负时归零并按需回退offset。随后`or ah,3`，即OR `0x0300`并返回完整EAX。其他mode返回`mode-15`。

UT覆盖偶列到奇列、奇列到偶列、无配对末项夹取、sample回调后写3、B9C0/B9E0停止、mode2最低party/全FFFF、mode15 BBC0及`ABCD0001 -> ABCD0301`。

workpack双生成稳定为`103/227`，SHA256均为`3c845e5101a99cf186a0e661976ab7f57134ea59aa6d0843c1712b8adc452d4b`；下一单元`0x004439A0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
