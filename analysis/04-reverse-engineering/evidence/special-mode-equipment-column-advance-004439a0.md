# 装备物品模式行内列前移 `0x004439A0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004439A0..0x00443A5C`，98行，无FUNCTION CHUNK。唯一引用来自3B480数据callback槽G03/动作5；无code caller可回收。B9C0、B9E0、BB80已关闭并直接复用，sample46保留平台端口。

## mode1行内切换

mode1从`0x004439FC`起与`0x004438E0`对应序列一致：local偶数加1、奇数减1，立即写回；按signed i32与visible count比较，越界时写`visible_count-1`。以`local+list_offset`调用B9C0/B9E0，播放sample46，callee返回后才写`4FD080=0x30`。因此它与438E0都做奇偶列切换，但动作5成功终值是30而不是3。

B9C0 null与B9E0失败保留local写入，不播放sample、不写30。visible为0时仍写FFFFFFFF，并按B9C0的非正索引行为取head，不增加现代范围修复。

## mode2与mode15

mode2从party3向0读取每项首word，选择最高非FFFF项；四项全FFFF时在完整四项读取后typed-stop。

mode15直接复用BB80，并传入special total、offset、hover cursor及visible count；随后`or ah,30h`，即OR `0x3000`并返回完整EAX。其他mode返回`mode-15`。

UT覆盖偶列到奇列、无配对末项夹取、sample回调后写30、B9C0/B9E0停止、mode2最高party/全FFFF、mode15 BB80及`ABCD0001 -> ABCD3001`。

workpack双生成稳定为`104/227`，SHA256均为`37a8766ba566a28be23f960a72c5139a18476d5b197c44b5f0363bfda49518a0`；下一单元`0x00443A60`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
