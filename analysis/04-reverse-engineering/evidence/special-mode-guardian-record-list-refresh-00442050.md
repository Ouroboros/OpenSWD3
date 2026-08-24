# 护驾记录列表重建 `0x00442050`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。主范围`0x00442050..0x004420E4`，78行；尾跳FUNCTION CHUNK `0x00442020..0x00442043`同属本函数，已完整纳入。

## 精确流程

1. 直接复用已关闭41F70，以全局source owner、record head destination、guardian slot filter index和party.low16执行筛选排序。destination sort/reserved/reset字段在state显式保留。
2. 筛选完成后才按`party.low16*16+slot`读取party text index。
3. 当party text不是`0xFFDC`，或筛选结果head为空时，从head遍历到尾部并调用44D5D0创建missing节点；返回节点写到尾link并强制next=null。selected missing且已有非空链时不追加。
4. 直接复用43B980计数total；清list offset与local selection；visible head=head。
5. FUNCTION CHUNK先清visible count，再最多遍历10项。链长恰好10时最终EAX=null；超过10时EAX保留第11项，typed结果以`legacy_return_node`表达。

44D5D0保持为`create_missing_guardian_record`最窄生命周期边界；null结果只在原next写入点typed-stop。party record动态表只在筛选后原读取点检查，保留此前41F70链副作用。

## caller回收

- 40630初始化不再调用opaque `prepare_guardian_record_list`，而是从0xB0记录提取text-index表后直接调用本helper。
- B20/C20/D20/E10共享selection core及41060 party cycle中的`refresh_guardian_record` opaque target均已回收为直接helper调用；失败映射到既有guardian选择typed状态。
- 因共享core，F00/FB0和407F0间接路径同步使用已关闭owner。

UT覆盖筛选排序、missing追加/抑制、total与窗口清零、两项链、11项链的第11项返回、筛选后party表越界、44D5D0 null typed-stop、40630直接初始化及全部既有选择caller事件序列。定向测试通过。

workpack双生成稳定为`84/227`，SHA256均为`d7e41e8fa3bb0c6445741d88b31502200327aa85b42f32514b958b841a8552de`；下一单元`0x004420F0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
