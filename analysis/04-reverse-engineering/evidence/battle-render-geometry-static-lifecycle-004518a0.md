# 战斗渲染几何静态生命周期注册 `0x004518A0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整范围与附件

权威LST主体为`0x004518A0..0x004518A9`，并声明外部`FUNCTION CHUNK AT 0x004518C0 SIZE 0x0C`。完整主函数为13行主体加10行chunk，共23行。

工作包未单列两个附件：

- `0x004518B0`共9行，加载固定owner `0x0053B0B8`到ECX并尾跳已关闭几何初始化`0x00433C40`；
- `0x004518D0`共9行，加载同一owner并尾跳已关闭资源清理`0x00433D70`。

本项把主函数、chunk与两个附件一起闭合，但工作包只计主函数一次。

## 2. 静态初始化顺序

函数无显式参数、无普通CODE XREF；唯一DATA XREF为`.data:0x0049E064`。

主体调用构造附件后无条件跳入chunk。modern直接对同一typed `LegacyBattleRenderGeometry`调用已关闭`initialize_legacy_battle_render_geometry`，保留初始化status、行表、矩形、方向向量与返回指针；不建立opaque构造端口。

即使初始化返回typed-stop状态，原静态包装器也无分支，仍继续注册退出函数。modern保持这一顺序。

## 3. 外部chunk与返回EAX

chunk压入退出附件`0x004518D0`，调用CRT `_atexit`，caller回收参数并返回。最终EAX为注册完整结果，不是几何初始化返回。

typed registration port显式接收退出token `0x004518D0`。测试以非零bit pattern证明最终返回直接来自注册。

## 4. 退出附件与closed清理

退出附件对同一owner尾跳`0x00433D70`。modern wrapper直接调用已关闭`release_legacy_battle_render_resources`，保留：

1. 附属缓冲释放；
2. surface行表释放；
3. primary行表释放。

测试在静态初始化实际建立两张行表后注入附属token，再执行退出wrapper，验证三个释放结果、回调token与owner字段归零。

owner地址以常量token `0x0053B0B8`记录，不把32位旧地址当主机指针。

## 5. 双向追溯

- `0x004518A0..0x004518A4`：调用固定owner构造附件；
- `0x004518A5..0x004518A9`：无条件跳入外部chunk；
- `0x004518B0..0x004518B9`：加载owner并尾跳closed初始化；
- `0x004518C0..0x004518CB`：注册退出附件并返回`_atexit` EAX；
- `0x004518D0..0x004518D9`：加载同一owner并尾跳closed清理。

C++到LST反向追溯覆盖主函数23行、两个各9行附件、固定owner、closed构造/清理和CRT边界。

## 6. 验证与动态差分

定向测试覆盖：

- 固定owner token；
- closed初始化只调用一次并成功发布两张行表；
- 退出注册收到精确cleanup token；
- 注册EAX成为主函数返回；
- closed cleanup只调用一次；
- 附属缓冲回调先行且token精确；
- surface与primary行表均释放；
- 三个owner字段最终为空；
- 既有几何初始化、失败与泄漏兼容测试未回归。

battle聚合目标零warning构建及定向测试通过。

当前没有原版静态owner内存、两张行表/附属缓冲与CRT注册联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。主函数、外部chunk及两个附件已完成typed闭环。
