# 护驾属性seed选择 `0x00442A40`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442A40..0x00442A94`，53行，无FUNCTION CHUNK；唯一caller为429B0，现已直接回收。

## mode分支

- `interaction_mode==0`：以`u16(party_selector)*16+guardian_slot`读取party record pointer。现代端由最窄`resolve_guardian_party_attribute_record`表边界返回typed `LegacyStandardModeForwardNode*`；只有表读取不可用时在原`dword_4C8AD0[eax*4]`处typed-stop。
- `interaction_mode==1`：按u32回绕计算`list_offset+local_selection`后解释为i32，直接复用已关闭的43B9A0等价链索引helper；返回节点允许为null，保持原seed值。
- 其他mode：返回null seed，不触发party表或链读取。

原函数返回裸record pointer；现代端在429B0→442B10→442CA0链内始终以`const LegacyStandardModeForwardNode*`传递，不降级为整数token，不暴露到用户可见结果。

429B0原`prepare_guardian_attribute_seed`整数端口已删除。mode0只保留party表解析边界；mode1与其他mode不进入该端口。A40表停止映射为429B0的`seed_preparation_stopped`，保留此前四个party cache填充。

UT覆盖mode0的party low16/slot参数和返回节点、mode1链中第二节点、其他mode null seed、mode0表不可用typed-stop，并重验429B0及全部owner路径。定向测试通过。

workpack双生成稳定为`90/227`，SHA256均为`1e73bce2145a20fc5a5f32f7f0f91aba24c85725828de578bd118ba174454b8a`；下一单元`0x00442AA0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
