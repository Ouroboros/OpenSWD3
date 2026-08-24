# G08阶段退出 `0x004453F0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004453F0..0x00445412`，24行，无FUNCTION CHUNK。code caller为已关闭450E0输入函数；3B480另把它绑定为G08退出callback。450E0已直接回收。

函数把lifecycle按u16预减并立即写回。结果为0时才把全局特殊模式请求清0；结果非0（包括0预减后的FFFF）保持原请求。随后以新lifecycle为secondary、共享横坐标为primary直接调用已关闭3B480，并把其EAX低16位原样返回。

为保留该可观察残值，`LegacyStandardModeCallbackBindingResult`新增机器返回owner：入口默认为secondary；secondary2进入G01..G07时按LST更新为primary或剧情查询结果；secondary1进入G08时更新为FC0剧情查询原值。453F0实际域因此为：1→0返回0且不改回调；2→1重装FC0/G08并返回flag49原值；0→FFFF返回FFFF且不改回调。

450E0原exit opaque端口已删除。fallback仍先写constant12，再直接调用本helper并累计G08嵌套查询。UT覆盖1→0清请求、2→1保留请求并重装三张表和13主槽、负剧情残值返回、0→FFFF回绕，以及450E0按钮12路径。

workpack双生成稳定为`121/227`，SHA256均为`c09511c942ef254e956c9789b8978a256d10ad24078ee940e00769474e2e9ced`；下一单元`0x00445420`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
