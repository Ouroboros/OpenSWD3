# G08模式后退与分类重建 `0x00446420`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00446420..0x00446541`，145行，无FUNCTION CHUNK。3B480保存callback地址；455E0在模式2 hover分类变化后直接调用。

模式snapshot大于等于500时先预减运行时mode index并以signed小于0夹零，随后直接串联C9C0条目初始化、CC00 alias重建、CBD0页刷新、当前entry读取和CEF0消费，最后播放46。每个动态读取/消费点独立typed-stop并保留此前副作用。

模式2在selection31时只发布selection；其余路径写viewport480、清FC648，调用尚未关闭4482E0记录清理，按无符号预减分类并把旧值0回绕为6，再调用448230记录初始化、播放46，并由B9C0/B9E0更新文本。记录清理/初始化停止分别保留各自精确前缀。

模式5写动作0；模式10无符号预减后signed小于0夹零；模式11写列0；模式15直接复用BBC0后退二级cursor，不写transition标记。普通路径最终发布selection。

455E0 hover callee已回收为本typed helper，陈旧refresh_hover端口已删除。UT覆盖分类0回绕6、五helper模式2链、清理停止、模式10夹零、模式15cursor后退、高模式预减夹零重建及caller回归；独立ASan通过。

workpack双生成稳定为`130/227`，SHA256均为`209b5e0962ab0daaa496f56581fc591c0e76e4c4f1800f5abda77c0272a14214`；下一单元`0x00446550`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
