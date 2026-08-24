# 装备物品模式退出/取消状态机 `0x004441A0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004441A0..0x0044427D`，118行，无FUNCTION CHUNK。code caller为F40一处，3B480另绑定为退出callback；F40现已直接回收。3B480与442F10已关闭并直接复用；4885A0筛选表释放由typed容器及窄释放端口表达。

入口保留`mode-1`残值。

- mode1：transition word按u16预减；结果为0时先清interaction block。随后以transition word为secondary、text resource word为primary直接调用3B480绑定callback，再尾调用442F10清理装备模式并返回其EAX。cleanup停止保留减法、interaction与callback副作用。
- mode2：transition word按u16预减，写mode1并清selected party action；`0 -> FFFF`不做饱和。
- mode5：写mode1及panel motion -128。
- mode15：按筛选记录数逐项释放并释放表，modern清typed vector，使special offset结束在原count、count归零；随后写mode1并返回释放端口EAX。
- mode17/18：只写mode1。
- 其他mode：仅返回入口`mode-1`残值，不写状态。

F40 buttons4 caller已直接调用本helper，传播cleanup停止；generic exit target不再被调用。

UT覆盖mode1 transition2到1的secondary callback绑定及完整cleanup、transition1到0后cleanup停止前缀、mode2 u16回绕、mode5、mode15两项释放、mode17/18、默认残值及F40 generic边界回收。

workpack双生成稳定为`108/227`，SHA256均为`7813c5f59f9657111758c41e228c58bada5c57dfdc5bcb16eef5ea6ed6f349dd`；下一单元`0x004442B0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
