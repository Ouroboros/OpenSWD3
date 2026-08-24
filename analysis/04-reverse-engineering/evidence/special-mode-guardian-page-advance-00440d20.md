# 护驾槽与列表分页推进 `0x00440D20`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整范围`0x00440D20..0x00440E03`，106行；407F0一处caller。mode0先4420F0并直接写guardian slot=10，随后442050、独立dword文本索引、B9E0、4429B0与sample命令46，返回sample EAX且不改flags。

mode1直接复用BBE0，以total、offset、local、visible及step10推进/重建整页，再复用B9A0、BC90、B9C0、B9E0，随后4429B0与sample命令46；最终将mode flags低字节OR 0x30并返回完整dword。其他mode返回`mode-1`。

B20/C20/D20共享typed核心，动作枚举仅参数化mode0槽规则、mode1窗口helper和flags mask。407F0第二动态strict区已删除opaque边界并直接调用D20，传播typed-stop。

UT覆盖mode0固定槽10、mode1最终页重建、可见链/文本/sample/flags30及407F0直接caller。定向测试通过。workpack双生成稳定为`75/227`，SHA256均为`d38cb927e7579b7dae5305bcedceb6b70ef87ad9fad679d396503d000794f95c`；下一单元`0x00440E10`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
