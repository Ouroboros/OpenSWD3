# 装备物品最终动作数量 `0x00444FB0`

状态：`assembly_exact`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00444FB0..0x00444FB5`，11行，无FUNCTION CHUNK。函数只有`mov eax,3; retn`，无参数、无状态读取、无callee。callers为已关闭442E40模式初始化和43A60 party循环，两处均已直接回收。

实现为无端口、`noexcept`的typed helper，恒定返回i32 3。原两个带selected party action参数、可返回nullopt的opaque finalize端口以及不可达停止状态全部删除。442E40仍在workspace分配后调用并把3发布到final owner、动作数和global mode；43A60在文本发布后调用并把3发布到动作数，再播放sample107。

UT直接验证恒定3，并通过两caller回归验证helper调用计数、发布顺序和sample顺序；原伪造失败测试已删除。

workpack双生成稳定为`115/227`，SHA256均为`23e890c6e31efa90806e2a47afa6e3833e6bf96b431709b6f2f6ecabd1cdf33f`；下一单元`0x00444FC0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
