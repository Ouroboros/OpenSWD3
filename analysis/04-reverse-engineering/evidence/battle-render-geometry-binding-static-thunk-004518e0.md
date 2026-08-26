# 战斗渲染几何绑定静态转发 `0x004518E0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威LST函数为`0x004518E0..0x004518E4`，共9行，无外部FUNCTION CHUNK。

唯一指令为近跳转`jmp 0x004518F0`。函数没有栈帧、参数构造、条件分支、调用后处理或本地返回；DATA XREF为`.data:0x0049E068`。

## 2. 尾转发语义

入口所有可观察结果均来自相邻callee：

- 恰好进入`0x004518F0`一次；
- 不修改参数、寄存器snapshot或共享状态；
- 不建立额外返回层；
- callee完整EAX成为本thunk完整EAX。

`0x004518F0`已在下一独立工作包关闭为typed固定参数包装器。本thunk现直接调用该helper；只有更深层、仍待`audit_order=106`关闭的绑定对象initializer保留typed端口。

## 3. typed映射

`forward_legacy_battle_render_geometry_binding_static_initialization`直接调用`initialize_legacy_battle_render_geometry_binding`并返回其typed结果：

- 固定绑定对象token和几何owner均由callee helper准备；
- `initialization_calls=1`；
- callee完整32位返回值。

本thunk自身不准备token、不添加状态写。

## 4. 双向追溯

- LST到C++：`0x004518E0`唯一`jmp`映射为一次typed port调用；
- C++到LST：typed函数没有额外状态写、参数、条件或公共后缀；
- 输入输出：callee返回bit pattern原样传播。

## 5. 验证与动态差分

定向测试使用`0x89ABCDEF`返回snapshot，验证入口直连typed helper、结果调用计数为1、固定参数由callee准备且完整EAX不截断。battle聚合目标零warning构建、普通定向与独立ASan定向均`1/1`通过。

当前没有原版CRT静态初始化表与`0x004518F0`调用现场联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。本项纯尾转发已由完整LST和固定snapshot闭环。
