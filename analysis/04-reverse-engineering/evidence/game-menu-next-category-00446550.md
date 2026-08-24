# 切换到游戏内菜单的下一个分类并重建列表 `0x00446550`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00446550..0x00446665`，141行，无FUNCTION CHUNK。3B480保存callback地址，无code caller。

模式snapshot大于等于500时直接复用已关闭C760运行时mode推进。模式2在selection31时只发布selection；其余路径写viewport480、清FC648，调用4482E0记录清理，分类无符号递增并在精确等于7时回绕0，再调用448230重建、播放46，并由B9C0/B9E0更新当前文本。记录清理/初始化和链/文本停止均保留原始前缀。

模式5写动作1；模式10无符号递增后以signed边界夹到`outer_count-1`，count0保留FFFFFFFF；模式11写列1；模式15直接复用BB80推进二级cursor，不写transition。普通路径最终发布selection。

UT覆盖分类6回绕0、五helper模式2链、count0减一、模式15cursor推进与大于等于500委派；独立ASan通过。

workpack双生成稳定为`131/227`，SHA256均为`58e535b520c0624912bf2e02683c6756521a28428d9a7b62bd98adc019399eed`；下一单元`0x00446680`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
