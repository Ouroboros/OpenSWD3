# 将当前菜单列表翻到下一页 `0x00446090`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00446090..0x00446227`，199行，无FUNCTION CHUNK。3B480保存callback地址；455E0在主、次第二动态Y域直接调用。

模式snapshot大于等于500时按LST直接串联BBE0十五步分页、CC00 alias重建、CBD0可见页刷新、当前entry读取与CEF0消费，随后低字节OR 0x30并播放46。page刷新、entry索引与消费分别在原位置typed-stop，保留此前分页/alias/刷新副作用。

模式2在selection31时只发布selection；其余路径以步长13直接复用BBE0，随后B9A0发布可见链、BC90按13重计，低字节OR 0x30，发布local并播放46，再由B9C0/B9E0更新文本。链越界停止规则与上下移控制一致。

模式3从槽3向前寻找最后一个非FFFF marker；完整四槽均为FFFF时typed-stop。模式5写动作1；模式10无条件写`outer_count-1`，count0保留FFFFFFFF；模式11写列1。模式15以步长8复用BBE0并OR 0x3000。

455E0两处第二动态callee已回收为本typed helper，停止立即传播。UT覆盖模式2十五记录末页重建、可见链/文本、末个party槽、count0减一、模式15八步分页及caller动态Y重读；独立ASan通过。

workpack双生成稳定为`128/227`，SHA256均为`d1b3f7f1fd074051f1de387493a98ccd531720b52eb943b63a8ef1ef2f9a40fb`；下一单元`0x00446260`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
