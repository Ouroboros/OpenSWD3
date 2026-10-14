# 战斗绘图绑定静态转发 `004518E0`

权威范围为`004518E0..004518E4`，唯一指令尾跳`004518F0`。
初始化表项位于`0049E068`，紧随几何静态初始化表项`0049E064`。
它不写状态、不准备参数，也不处理返回值。

现代代码已删除该转发函数及所转发的寄存器结果、计数结构。
SDL直接在几何初始化之后初始化现有绑定对象，保留两项的先后顺序，
不另建包装层或复制状态。

实际初始化、固定参数来源与测试见
[绑定对象初始化](battle-render-geometry-binding-object-initialization-0045f0f0.md)
和[固定参数入口](battle-render-geometry-binding-initialization-004518f0.md)。
不使用旧返回snapshot证明迁移完成；原版动态差分状态仍为
`blocked_runtime_oracle`。
