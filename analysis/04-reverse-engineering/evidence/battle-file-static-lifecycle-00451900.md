# 战斗文件对象静态生命周期注册 `0x00451900`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整范围与附件

权威LST主体为`0x00451900..0x00451909`，并声明外部`FUNCTION CHUNK AT 0x00451920 SIZE 0x0C`。完整主函数为13行主体加10行chunk，共23行。

工作包未单列两个附件：

- `0x00451910`共9行，加载固定文件owner `0x00521538`到ECX并尾跳已关闭文件构造`0x00438000`；
- `0x00451930`共9行，加载同一owner并尾跳已关闭文件析构`0x00438030`。

本项合并审计主函数、chunk和两个附件，但工作包只计主函数一次。

## 2. 构造与注册顺序

主体先调用构造附件，再无条件跳入chunk。chunk压入退出附件`0x00451930`，调用CRT `_atexit`，caller回收参数并返回。

构造返回不参与分支；最终完整EAX来自`_atexit`，覆盖文件构造返回的owner地址。

modern使用`LegacyBattleFileOwner`中的`std::optional<LegacyFile>`：构造附件在原时点`emplace`既有typed `LegacyFile`状态机，注册端口调用时对象已经存在。退出附件在原时点`reset`，真实调用typed文件析构。

## 3. closed文件callee

`0x00438000/0x00438030`已由`legacy-file-object-00438000.md`和`resource_io::LegacyFile`闭合：

- 构造建立无效文件句柄、空映射/视图/文件名与64字节错误域；
- 析构按视图、映射、文件与动态文件名顺序处理资源；
- Windows资源调用与POSIX适配均封装在resource_io，不在battle复制文件状态机。

原固定owner只以`0x00521538` token记录，不转换成主机指针。modern owner不暴露原80字节ABI。

## 4. 退出附件

`release_legacy_battle_file`只处理同一个typed owner：

- `cleanup_calls=1`；
- 对已构造对象执行一次析构；
- 清除optional占用；
- 不构造替代对象，不调用CRT注册端口。

析构附件本身是尾跳；主函数只注册其token，不在初始化期间提前析构。

## 5. 双向追溯

- `0x00451900..0x00451904`：调用固定owner构造附件；
- `0x00451905..0x00451909`：无条件跳入外部chunk；
- `0x00451910..0x00451919`：加载owner并尾跳closed文件构造；
- `0x00451920..0x0045192B`：注册退出附件并返回`_atexit`完整EAX；
- `0x00451930..0x00451939`：加载同一owner并尾跳closed文件析构。

C++到LST反向追溯覆盖主函数23行、两个各9行附件、固定owner、typed文件生命周期与CRT边界。

## 6. 验证与动态差分

定向测试覆盖：

- 固定owner token；
- 文件构造只执行一次；
- 注册发生时typed文件已存在；
- 注册收到精确退出token；
- 非平凡`_atexit` EAX `0x76543210`原样成为主函数返回；
- 退出wrapper只执行一次；
- typed文件真实析构并清除owner占用；
- 既有`LegacyFile`完整文件、映射、位置和错误测试不回归；
- battle聚合目标零warning构建、普通定向与独立ASan定向均`1/1`通过。

当前没有原版固定文件owner内存、Win32文件资源与CRT注册联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。静态协调器及两个附件已完成typed闭环。
