# G08上移控制 `0x00445E90`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00445E90..0x0044605A`，213行，无FUNCTION CHUNK。3B480保存callback地址；455E0在主、次两套上移Y域直接调用。

模式snapshot大于等于500时直接复用C590运行时游标后退。模式2在selection31时只发布selection；其余路径先写viewport480并清FC648，直接复用BBC0后退window/local、B9A0发布可见链、BC90按13重计可见数，低字节OR 3，发布local，播放46，再由B9C0/B9E0更新当前文本。

B9A0与B9C0动态链越界均只在原`node->next`读取前typed-stop，保留BBC0及此前状态。模式3从当前索引预减、负值回绕3并跳过FFFF；完整四槽均为FFFF时typed-stop。模式5恒写动作0；模式10保留无符号预减后signed小于0夹零；模式11恒写列0。

模式15直接复用BBC0后退二级window/local，并把transition第二字节OR 3，即整字OR 0x0300。所有普通路径最终发布zero-extended selection。

455E0两处上移callee已回收为本typed helper；本helper停止时caller立即传播。UT覆盖模式2六helper顺序、反向四槽及全FFFF、模式5/10/11、模式15高字节bits0/1、大于等于500委派、caller动态回调Y重读及独立ASan。

workpack双生成稳定为`127/227`，SHA256均为`90bced146db81f4b8fa23dec458b8e1d63fd6ed19fa7bf3020fca31199d27e33`；下一单元`0x00446090`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
