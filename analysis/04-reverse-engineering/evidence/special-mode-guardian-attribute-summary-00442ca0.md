# 护驾属性附加摘要 `0x00442CA0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442CA0..0x00442D65`，99行，无FUNCTION CHUNK；唯一caller为429B0，现已直接回收。

目标为cache reference record的`+0x44/+0x48/+0x4C`三个dword。

## 无条件sentinel与slot分支

1. 先写`+0x44=FFFFFFFF`。
2. slot0：原地读取seed `text_index`；null seed在该读取点typed-stop。非`FFDC`时调用4779F0等价query，结果low16零扩展写`+0x44`。
3. 再写`+0x48=FFFFFFFF`。
4. slot7/8：读取seed `text_index`；null seed在此停止。非`FFDC`时调用477B40等价pair query，并按`low16(first) | low16(second)<<16`写`+0x48`。
5. 再写`+0x4C=FFFFFFFF`。
6. slot9/10：读取seed `text_index`；null seed在此停止。`FFDC`时不查询且返回`0xFFDC`；否则调用477800等价query，low16零扩展写`+0x4C`并作为返回。

其他slot不读取seed，三个字段均保持`FFFFFFFF`，返回完整`guardian_slot`。slot0最终返回0；slot7/8返回7/8；对应查询EAX会被后续slot reload覆盖。

## typed边界

- destination不足`0x50`：首个`+0x44`写前停止。
- slot0 null：只完成`+0x44` sentinel。
- slot7/8 null：完成`+0x44/+0x48` sentinel。
- slot9/10 null：完成三个sentinel。
- 三类query不可用：在原query调用返回点停止，保留此前sentinel。

4779F0、477B40、477800由slot0、pair、bonus三个typed query端口隔离；不再传seed pointer或destination给opaque summary。

429B0原`finalize_guardian_attribute_summary`端口已删除；至此429B0四个直接callee A40/AA0/B10/CA0全部闭环。测试同步揭示并保留原unsafe路径：407F0 mode2列表行变化若guardian slot0且A40返回null seed，会在CA0原seed读取点typed-stop，而不是旧opaque fixture伪造的成功。

UT覆盖slot0值、slot7 packed `5678:1234`、slot9 bonus、无关slot、FFDC residual、三种null seed的sentinel前缀、query失败和目标越界，并重验429B0全部owner。定向测试通过。

workpack双生成稳定为`94/227`，SHA256均为`96e7419ffa744049ceb039aa5f5acc76ac1bfce2fe9d18f32d90bf4cc47f3491`；下一单元`0x00442D70`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
