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

`0x004518F0`是下一独立工作包，当前以typed initialization entry port隔离，避免提前把callee计入本项。该callee关闭后必须删除此opaque边界，并由本thunk直接调用typed helper。

## 3. typed映射

`forward_legacy_battle_render_geometry_binding_static_initialization`只执行一次端口转发，并返回：

- `initialization_calls=1`；
- callee完整32位返回值。

函数没有owner token，因为LST中固定owner与绑定对象参数均由callee准备，而不是本thunk准备。

## 4. 双向追溯

- LST到C++：`0x004518E0`唯一`jmp`映射为一次typed port调用；
- C++到LST：typed函数没有额外状态写、参数、条件或公共后缀；
- 输入输出：callee返回bit pattern原样传播。

## 5. 验证与动态差分

定向测试使用`0x89ABCDEF`返回snapshot，验证入口调用一次、结果调用计数为1、完整EAX不截断。battle聚合目标零warning构建、普通定向与独立ASan定向均`1/1`通过。

当前没有原版CRT静态初始化表与`0x004518F0`调用现场联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。本项纯尾转发已由完整LST和固定snapshot闭环。
