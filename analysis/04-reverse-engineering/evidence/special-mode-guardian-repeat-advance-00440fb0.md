# 护驾推进与重复刷新 `0x00440FB0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整范围`0x00440FB0..0x00441055`，84行；B480绑定为callback。mode0与B20的槽加一、11回0、文本、刷新及sample路径完全等价。

mode1先直接调用B20，完成BB80、链重建、第一次B9E0/4429B0/sample46及flags OR30；随后重新读取`offset+local`，重复B9C0、B9E0、4429B0及sample46，最终返回第二次sample EAX。其他mode返回`mode-1`。

F00/FB0共享方向化组合核心。UT锁定mode0单刷新，以及mode1两次attribute refresh、两次sample、helper count11、local推进及flags30。定向测试通过。workpack双生成稳定为`78/227`，SHA256均为`b4f61dc6ccabbf5be5a50b5c451112aeda151fdea6dc5739565bb4bf5dff6c94`；下一单元`0x00441060`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
