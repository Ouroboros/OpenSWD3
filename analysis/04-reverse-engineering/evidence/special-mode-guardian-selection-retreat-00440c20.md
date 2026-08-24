# 护驾槽与列表后退 `0x00440C20`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整范围`0x00440C20..0x00440D10`，99行；caller为407F0与40F00各一处。callee集合与440B20相同，仅窗口helper由BB80改为BBC0。

mode0先调用4420F0，把guardian slot按32位环绕减一；signed结果为负时写10。随后442050、按`party_selector.low16*16+slot`读取独立dword文本索引、B9E0、4429B0及sample命令46，最终返回sample EAX且不改flags。

mode1直接复用BBC0后退`list_offset/local_selection`，再按新offset复用B9A0、BC90、B9C0、B9E0，随后4429B0和sample命令46。最终仅将mode flags低字节OR 3，并返回完整写回dword。其他mode返回`mode-1`。

B20/C20共享一个typed核心，仅把槽方向、窗口helper和flags mask参数化；各自公开入口保持独立。407F0的103..115快捷区已删除opaque cycle边界并直接调用C20，传播typed-stop。

UT覆盖mode0槽0回绕到10、mode1跨页后退、可见链/文本/sample/flags3、B20回归，以及407F0直接caller。定向测试通过。workpack双生成稳定为`74/227`，SHA256均为`2046a6f16832350e23a378efa733cf94311b232902e571d54264fb3825662384`；下一单元`0x00440D20`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
