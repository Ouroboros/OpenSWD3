# 按下方向键时选择下一项或进入下一列 `0x00445C90`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00445C90..0x00445E5A`，216行，无FUNCTION CHUNK。3B480保存其callback地址；455E0在主、次两套下移Y域直接调用。

模式snapshot大于等于500时直接复用已关闭C520运行时游标推进并返回。模式2在selection31时只发布selection；其余路径先写viewport480并清FC648，再直接复用BB80推进window/local，B9A0发布可见链头，BC90按上限13重计可见数，低字节OR 0x30，发布local，播放46，最后由B9C0/B9E0更新当前文本。

B9A0与B9C0的动态链读取只在各自原`node->next`读取前typed-stop：BB80、viewport、FC648等此前副作用保留；停止后不提前写transition、音效、文本或最终selection发布。

模式3按四个party marker从当前索引预增并循环跳过FFFF，找到后播放46；完整四槽均为FFFF时，在原程序将继续无限扫描的位置typed-stop，不伪造成功索引。模式5恒写动作1；模式10先无符号递增外层行，再以signed边界夹到`count-1`，count为0时仍保留原FFFFFFFF；模式11恒写列1。

模式15直接复用BB80推进二级window/local，并把transition高字节OR 0x30，即整字OR 0x3000。所有非停止普通路径最终以zero-extended selection发布D14C。

455E0两处下移callee已从通用control端口回收为本typed helper。主/次控制仍只在LST明确位置重读pointer Y；本helper停止时caller立即传播，不继续后续Y域。

UT覆盖模式2六helper顺序、selection31跳过、短链停止、模式3稀疏四槽和全FFFF、模式5/10/11、模式15高字节标记及大于等于500委派；caller回归与独立ASan通过。

workpack双生成稳定为`126/227`，SHA256均为`b14b919987bcaea4c9798fad32abfc0483af66c3be021e10e2f9af97f3954922`；下一单元`0x00445E90`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
