# 模式2选择循环与高模式推进 `0x004466A0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。主物理范围`0x004466A0..0x004466F3`，并共享外部FUNCTION CHUNK `0x0043C7E0..0x0043C7F5`；该chunk明确将4466A0列为additional parent。3B480保存本入口，无code caller。

模式snapshot大于等于500时直接复用已关闭46680，从而保持C760推进以及相同sample handle的两次46音效。低模式仅当snapshot精确等于2时对u16 selection预增；结果大于32则写30，随后播放一次46。其他低模式不修改selection、不播放音效。所有低模式最后均复用46680发布零扩展selection，返回发布值而非音效返回值。

UT覆盖32递增后回绕30、模式2单音效、其他低模式不变无音效、发布返回值以及高模式双音效复用；独立ASan通过。

workpack双生成稳定为`133/227`，SHA256均为`426169f1064ced399ed8e350e29aa57543f93a3e9f971f3727ea759ea562b855`；下一单元`0x00446700`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
