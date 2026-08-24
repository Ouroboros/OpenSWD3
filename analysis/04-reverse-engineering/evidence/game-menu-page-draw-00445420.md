# 绘制游戏内菜单当前页面 `0x00445420`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00445420..0x00445428`，13行（含proc标记），无FUNCTION CHUNK。无code caller；3B480把它绑定为G08主绘制callback。

函数把selection按u16零扩展，直接以其为索引尾跳到`dword_4FBF54`表。FC0安装的7个绘制目标位于该表索引11..17，因此typed owner以`draw_callbacks[selection-11]`表达共享表，而不是复制新表。

selection越界只在原表读取点typed-stop；目标为0或typed调用拒绝时在原尾跳点停止。成功时返回目标原始i32结果；失败前残值保持selection。result区分索引越界与目标缺失，并记录一次尾调用。

UT覆盖selection11的首目标、目标返回值、selection10越界、selection17空目标及目标拒绝。3B480 G08主槽仍保存45420目标，实际绘制目标由该typed尾分派解析。

workpack双生成稳定为`122/227`，SHA256均为`43b969d4808c19cc7a208fc8f4e50e6ba7b33668f0978a80fb61361067af0491`；下一单元`0x00445430`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
