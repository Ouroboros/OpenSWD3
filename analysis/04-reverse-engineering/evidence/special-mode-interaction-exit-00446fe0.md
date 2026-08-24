# 特殊模式交互阶段退出 `0x00446FE0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。主物理范围`0x00446FE0..0x004470F4`，并包含外部FUNCTION CHUNK `0x0043C800..0x0043C819`；该chunk明确将46FE0列为additional parent，并在精确模式500时跳入C2F0完整运行时释放路径。455E0有五处code caller；46700模式4与模式11另有两处code caller；3B480保存本入口。

模式大于等于500时，仅精确500写回模式2并执行C2F0；其他高模式不改状态。C2F0现抽为`cleanup_legacy_standard_mode_runtime`，C3C0原退出路径与46FE0共同直接复用：释放scratch record token，依序释放scratch/status/slot表、16个长文本槽、64个短文本槽及entries，共85次storage release；随后动作0写232A/43。scratch token仅在非零时释放并清四字节。

低模式先按u16预减并立即发布新阶段。新阶段3（原4）回模式2并把FC648 owner写FFFFFF00；新阶段1（原2）直接调用455A0，随后以secondary1和当前selection调用B480重绑G08并写布局owner34；新阶段4（原5）写动作1并回2；新阶段9/16/17（原10/17/18）回2；新阶段10（原11）保持10；新阶段14（原15）按筛选记录数完成释放、保留释放计数游标、清记录与计数并回2；其他阶段发布零扩展selection。

关闭后，455E0五处退出与46700两处退出均直接调用typed helper；陈旧`exit_interaction`和`exit_current_interaction`端口已删除。退出typed-stop只可能来自原模式2的455A0记录清理，并在该点立即传播。46700函数签名补入运行时owner以完成两处caller回收。

UT覆盖精确500的record token、85次storage释放和动作重置；501高模式忽略；模式2清理重绑与flag49查询；模式4哨兵、模式5动作、模式10/11、模式15释放计数、模式17/18和默认预减发布；同时覆盖五处455E0及两处46700 caller。独立ASan通过。

workpack双生成稳定为`135/227`，SHA256均为`88460541bb05b3bbce55dd54437b8f51cda51324dfb7e7046eafd8af497a1c49`；下一单元`0x00447100`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
