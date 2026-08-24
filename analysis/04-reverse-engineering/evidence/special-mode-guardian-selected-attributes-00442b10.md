# 护驾选中记录属性合并 `0x00442B10`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442B10..0x00442BB3`，90行，无FUNCTION CHUNK；唯一caller为429B0，现已直接回收。

## 前置写入

1. 以`u16(party_selector)`读取并复制`0x38`字节模板到scratch。
2. 无条件把scratch `+0x26`与`+0x28`两个u16清零。
3. 无条件把输入destination的`0x50`字节清零。

目标越界只在原`memset(a3,0,0x50)`处typed-stop；此前模板复制及两个u16清零必须保留。

## 十六slot循环

输入party table基址为`party_index*16`。slot按`0..15`：

- 非选中slot：读取对应record `+0x0C`名称并调用44D6E0等价合并边界。
- 等于`guardian_slot`且seed非null：不读取party table该项，改用typed seed的`display_name`合并。
- 等于`guardian_slot`且seed为null：既不读取table也不调用44D6E0，直接进入下一slot。
- `guardian_slot>15`时没有相等项，十六项全部使用party table，保持原cmp行为。

循环后调用442BC0等价收尾边界并返回其EAX。seed全程为`const LegacyStandardModeForwardNode*`，不退化为整数。

442BC0后续已独立关闭并由B10直接调用；44D6E0仍由name merge窄端口表达。模板、destination、record/name与merge在原读取/调用点typed-stop，BC0目标越界则保留此前scratch、destination clear与成功合并。

429B0原`combine_guardian_selected_attributes`opaque端口已删除。其第六步直接调用B10；B10停止映射为`selected_combination_stopped`。同时移除了旧fixture伪造的`list_offset=77`副作用；LST无该写，40630初始化现在保持list offset为0。

UT覆盖非null seed替换slot3、slot3不读party table、null seed跳过、十六/十五次合并、scratch两个u16清零、`0x140`目标清零、17/16次helper与最终返回；另覆盖模板、目标、record5、merge3和finalize停止点。定向测试通过。

workpack双生成稳定为`92/227`，SHA256均为`57be1888cf62f087fcd7387123fa0ea9598b367c0397dc436ae2dfcd431bcde6`；下一单元`0x00442BC0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
