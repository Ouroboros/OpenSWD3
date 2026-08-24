# 护驾槽与列表分页后退 `0x00440E10`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整范围`0x00440E10..0x00440EF3`，106行；407F0一处caller。mode0先4420F0并直接写guardian slot=0，随后442050、独立dword文本索引、B9E0、4429B0与sample命令46，返回sample EAX且不改flags。

mode1直接复用BC60：local非零时只清local，否则offset减10并按signed负值归零；随后复用B9A0、BC90、B9C0、B9E0、4429B0和sample命令46。最终将mode flags低字节OR 3并返回完整dword。其他mode返回`mode-1`。

B20/C20/D20/E10共享typed核心。407F0第一动态strict区已直接调用E10并传播typed-stop。UT覆盖mode0固定槽0、mode1 local清零、链/文本/sample/flags3及407F0直接caller。定向测试通过。workpack双生成稳定为`76/227`，SHA256均为`87df23bec9fd4c358dad0af0af5eb72564ff16172cf03864d84f77524d7e72f7`；下一单元`0x00440F00`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
