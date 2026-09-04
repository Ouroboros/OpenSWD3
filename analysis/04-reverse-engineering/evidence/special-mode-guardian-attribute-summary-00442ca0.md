# 护驾属性附加摘要 `0x00442CA0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442CA0..0x00442D65`，99行，无FUNCTION CHUNK；唯一caller为429B0，现已直接回收。

目标为cache reference record的`+0x44/+0x48/+0x4C`三个dword。

## 无条件sentinel与slot分支

1. 先写`+0x44=FFFFFFFF`。
2. slot0：原地读取seed `text_index`；null seed在该读取点typed-stop。非`FFDC`时在原位置直接组合已关闭固定键曲线查询`0x004779F0`，结果low16零扩展写`+0x44`。查询前缀保留429B0发布的`ECX=attribute_cache_token+0x140`及442B10/442BC0留下的`EDX=0x004FCD4C`。
3. 再写`+0x48=FFFFFFFF`。
4. slot7/8：读取seed `text_index`；null seed在此停止。非`FFDC`时调用477B40等价pair query，并按`low16(first) | low16(second)<<16`写`+0x48`。
5. 再写`+0x4C=FFFFFFFF`。
6. slot9/10：读取seed `text_index`；null seed在此停止。`FFDC`时不查询且返回`0xFFDC`；否则在原call位置直接组合已关闭固定键计数查询`0x00477800`，以seed低word为键，返回low16零扩展写`+0x4C`并作为返回。

其他slot不读取seed，三个字段均保持`FFFFFFFF`，返回完整`guardian_slot`。slot0最终返回0；slot7/8返回7/8；对应查询EAX会被后续slot reload覆盖。

## typed边界

- destination不足`0x50`：首个`+0x44`写前停止。
- slot0 null：只完成`+0x44` sentinel。
- slot7/8 null：完成`+0x44/+0x48` sentinel。
- slot9/10 null：完成三个sentinel。
- slot0固定曲线、slot7/8 pair query或slot9/10固定计数链在真实访问点停止：在原调用位置停止，保留此前sentinel。

slot0与slot9/10分别直连已关闭`0x004779F0`和`0x00477800`，共同复用唯一`LegacyBattleFixedObjectStatePort`；仅`0x00477B40`继续由slot7/8的pair query端口隔离。不再传seed pointer或destination给opaque summary。

429B0原`finalize_guardian_attribute_summary`端口已删除；至此429B0四个直接callee A40/AA0/B10/CA0全部闭环。测试同步揭示并保留原unsafe路径：407F0 mode2列表行变化若guardian slot0且A40返回null seed，会在CA0原seed读取点typed-stop，而不是旧opaque fixture伪造的成功。

UT覆盖slot0固定曲线根命中及caller ECX高字/EDX token、slot7 packed `5678:1234`、slot9固定数量链根命中、无关slot、FFDC residual、三种null seed的sentinel前缀、pair query失败、两类固定链typed-stop和目标越界，并重验429B0全部owner及旧opaque边界零调用。定向测试通过。

workpack双生成稳定为`94/227`，SHA256均为`96e7419ffa744049ceb039aa5f5acc76ac1bfce2fe9d18f32d90bf4cc47f3491`；下一单元`0x00442D70`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
