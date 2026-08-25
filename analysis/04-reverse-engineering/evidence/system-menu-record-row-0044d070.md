# 绘制记载页单条文字及可选的左侧图标 `0x0044D070`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044D070..0x0044D0E0`，57行，无外部FUNCTION CHUNK。唯一直接caller为`0x0044C160`；callee为`0x0040EBF0`和`0x004306C0`。

函数按原顺序执行：

1. 以完整signed索引读取`list_owner + 2 * index`处的u16记载ID。
2. 从原字符串数据库读取偏移表基址，以记载ID选择32位偏移，再加数据库基址得到文字首地址。
3. 读取文字首字节；若为`0x25`（`%`），把共享动作写成ID`0x232A`、变体`0x17`，在`x - 0x1C, y`绘制图标，然后把文字地址前移1字节。
4. 以文字地址、原x/y、模式2、宽`0x154`和样式4绘制正文，并返回该次绘制EAX。

文字不以`%`开头时不绘图标，正文地址不偏移。`x - 0x1C`保持32位回绕。

## 2. typed边界和caller回收

记载目录为128个u16；null owner和signed索引越界只在原目录词读取点停止。字符串数据库查找由`resolve_system_menu_record_text`表达；资源或首字节不可用时，在原首字节读取点停止，已读取的记载ID保留。

未关闭的动作和文字绘制分别表达为`draw_record_marker`和`draw_record_text`命令；正文命令携带资源token、0/1字节偏移、完整x/y、2、`0x154`、4。

`0x0044C160`已删除`draw_record_entry` opaque命令，直接调用typed入口并合并命令数、helper数和返回值。成功后仍按原程序以visible count覆盖EAX；typed-stop时立即返回，不执行覆盖。

## 3. 验证

UT覆盖null owner、负索引、文字资源不可用、纯文字和`%`图标文字。纯文字验证完整七参正文；图标路径验证`x=0`回绕到`0xFFFFFFE4`、动作ID/变体、正文偏移1和两次helper顺序。C160用例覆盖两条可见记载直连及文字资源停止传播。

workpack双生成稳定为`181/227`，SHA256为`311b42efa53d69da23ca8a3c2f4f398f0ec31f676021a7d4bcec575352aeec9a`；下一项为`0x0044D0F0`。
