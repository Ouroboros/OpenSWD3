# 选择发布与高模式推进 `0x00446680`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。主物理范围`0x00446680..0x0044669D`，并包含外部FUNCTION CHUNK `0x0043C7E0..0x0043C7F5`；该chunk明确将446680列为additional parent。3B480把本入口写入G01第8槽，无code caller。

模式snapshot小于500时，零扩展selection并发布到共享选择owner后返回，不播放音效。模式snapshot大于等于500时进入外部chunk：直接复用已关闭C760运行时mode推进；仅在C760完整完成后，再播放一次46并返回第二次音效结果。因此正常高模式路径严格保留C760内部一次46与所属chunk额外一次46的双音效顺序；C760 typed-stop时不伪造后续成功。

UT覆盖低模式零helper发布、无音效，以及高模式推进、相同handle的两次46和第二次返回值；独立ASan通过。

workpack双生成稳定为`132/227`，SHA256均为`3b902da1619f9249cde86bec83e44d228b1f5e652e7e1eb5d033054111ea4311`；下一单元`0x004466A0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
