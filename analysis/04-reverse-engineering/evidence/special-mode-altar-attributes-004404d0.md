# 炼妖左右祭坛属性汇总 `0x004404D0`

状态：`assembly_exact`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整函数范围`0x004404D0..0x004405B5`，101行；唯一caller为E3D0 phase1，无callee。

函数先把FCD14/FCD16/FCD18/FCD1A四个signed word清零，再依次处理FCA88与FCBA0两份runtime record。typed owner为`altar_spirit_values[2]`和`altar_body_values[2]`，分别对应左右祭坛；旧地址实际按交错word布局，但typed端保持单一语义owner。

每份record从`+0x60`读取u16等级。以下word非零时，将固定倍数乘等级并按每次word加法保留low16环绕：

- 灵力：`+0x72 ×2`、`+0x76 ×3`、`+0x7A ×5`、`+0x86 ×2`、`+0x8A ×4`。
- 体力：`+0x7E ×3`、`+0x82 ×5`。

最终四个word以signed i16供FA70文本显示。EAX严格为第二份runtime record指针。新增`calculate_legacy_standard_mode_altar_attributes`并由E3D0 phase1在F1E0成功后直接调用；删除原`prepare_database_phase_1`整块port，不混入FDE0 phase2边界。

UT覆盖四owner先清零、左右record、五项灵力、两项体力、`0xFFFF`等级的low16环绕及第二record指针返回；E3D0集成测试确认不再调用opaque边界。

定向测试通过。workpack双生成稳定为`69/227`，SHA256均为`3b5951973bed2ebd15feeb0ad949372d1d3d74e50e1a6961f2eebfeafe58a616`；下一单元`0x004405C0`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
