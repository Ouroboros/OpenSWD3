# 护驾单party属性填充 `0x00442AA0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442AA0..0x00442B02`，55行，无FUNCTION CHUNK；唯一caller为429B0，共四处，现已全部回收为直接typed helper。

## 行为顺序

1. 以`u16(party_selector)`读取`4AB790 + index*0x38`模板，并完整覆盖scratch前`0x38`字节；scratch其余字节保持原值。
2. 指定输入party的record table基址为`party_index*16`。
3. 严格循环record `0..15`：在原table pointer及`+0x0C`名称读取点解析name，再调用44D6E0等价合并边界；前一项成功后才进入下一项。
4. 十六项全部成功后调用442BC0等价收尾边界，把scratch结果写入输入destination offset。
5. 返回442BC0的EAX。

模板索引使用当前selected party，而16项名称使用函数输入party，二者不得混淆。modern state以固定scratch/cache数组和destination offset替代裸pointer。

44D6E0与442BC0尚未独立关闭，分别以`merge_guardian_attribute_record_name`和`finalize_guardian_party_attribute_record`窄端口保留。record table只暴露typed name，不向业务层传递`record+0x0C`裸地址。

## typed-stop

- 模板越界：复制前停止。
- 第N项record/name不可用：保留模板及前N项成功合并。
- 第N次44D6E0失败：保留该次调用前的全部副作用，不增加成功合并数。
- 442BC0失败：保留16项合并及其调用副作用。

429B0四个`populate_guardian_party_attributes`opaque调用已删除。parent仍将每个AA0计作一个helper；AA0结果独立记录最多16次merge、一次finalize及最终EAX。40630真实初始化现在还保留AA0最后一次模板在scratch，修正了旧opaque测试错误的“scratch全零”假设。

UT覆盖selected-party模板与输入party名称分离、十六项`P2R0..P2R15`顺序、scratch仅前0x38覆盖、destination `0x50`、17次helper、最终返回，以及模板、record5、merge3、finalize四个停止点的0/5/4/17次调用和0/5/3/16项成功合并。定向测试通过。

workpack双生成稳定为`91/227`，SHA256均为`3301ecc91d53f1c850507952ddc3412a8a80eb7093989074a8f4df7a5fc03cfb`；下一单元`0x00442B10`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
