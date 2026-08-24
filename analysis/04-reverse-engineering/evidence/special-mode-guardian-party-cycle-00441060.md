# 护驾角色循环切换 `0x00441060`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整范围`0x00441060..0x0044115D`，120行；407F0角色点击唯一caller。interaction mode非0时直接返回原mode。

mode0把party selector low16加一并按`&3`归一，保留high16；以四份28字节party记录首word为可用标记，跳过FFFF。第一候选在检查前即写入selector；后续仅在找到可用候选时写回。四项全FFFF的原无限循环在第四次原表读取后typed-stop，并保留第一候选写入。

候选确定后依次调用4420F0、442050，直接复用BB40调整窗口、B9A0重建可见链、BC90计数，再按`party.low16*16+guardian_slot`从独立dword文本表调用B9E0，随后4429B0与sample命令263。最终EAX为sample返回。

407F0角色区先写`row-1`后直接调用本helper，因此最终selector为目标row；caller已删除opaque switch边界。UT覆盖跳过FFFF、high16保留、全不可用typed-stop、mode非0、sample263及407F0直接caller。定向测试通过。workpack双生成稳定为`79/227`，SHA256均为`5ed86fe075c1a59d2effa61cd53cbe138b84e53302c41a1598ce2daa048a3265`；下一单元`0x00441160`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
