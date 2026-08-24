# 护驾记录列表排空 `0x004420F0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004420F0..0x0044212A`，42行，无FUNCTION CHUNK。caller为B20/C20/D20/E10/F00/FB0、41060及41590。

函数从record head逐项弹出：

- 每轮先把head写成当前节点next。
- 节点offset4 text index不是`0xFFDC`时，将其next改为原filter-source head，再把filter-source head改为当前节点；普通节点因此按弹出顺序逐个头插，最终相对次序反转。
- text index等于`0xFFDC`时，不改next，按节点地址调用4885A0释放。
- 每轮尾部重新读取record head；空链及循环结束时EAX均为0。typed结果以null `legacy_return_node`及returned/released计数表达。

4885A0由`release_missing_guardian_record`最窄生命周期边界承载；普通节点归还完全在typed owner中执行。

所有已关闭caller中的`begin_slot_cycle` opaque target已回收：共享B20/C20/D20/E10移动核心、41060 party cycle，以及41590 mode0的两次连续排空均直接调用本helper；F00/FB0和407F0经共享owner同步复用。41590仍保留第一次有实效、第二次空链no-op的原始双调用。

UT覆盖普通→missing→普通混合链、普通节点逆序头插到既有source、missing单次释放、record head清空、最终null返回和空链no-op；既有caller事件序列全部更新。旧41590测试使用的伪地址record head在直连后由ASan准确定位，已改成真实typed节点。定向测试及独立ASan通过。

workpack双生成稳定为`85/227`，SHA256均为`29b15a3d8e47c4a4c6c7ee9623357f8e3798f60cb35a4d841b42e253edf00bd0`；下一单元`0x00442130`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
