# 护驾后退与重复刷新 `0x00440F00`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整范围`0x00440F00..0x00440FA2`，80行；B480绑定为callback。mode0与C20的槽减一、负值回10、文本、刷新及sample路径完全等价。

mode1先直接调用C20，因此完成BBC0、链重建、第一次B9E0/4429B0/sample46及flags OR3；返回后F00重新读取`offset+local`，再调用B9C0、B9E0、4429B0及sample46。最终EAX为第二次sample返回。其他mode返回`mode-1`。

实现直接组合已关闭C20 helper，仅重复原函数明确展开的第二组读取/刷新。短链和null在对应裸读取点typed-stop，保留C20已产生的全部副作用。UT锁定mode0单刷新，以及mode1两次attribute refresh、两次sample、helper count11和flags3。

定向测试通过。workpack双生成稳定为`77/227`，SHA256均为`7e22783593053596a4998739f3550287a27632e1a93a4f5027f7c171c164668b`；下一单元`0x00440FB0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
