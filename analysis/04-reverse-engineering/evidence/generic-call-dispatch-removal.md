# 通用调用体系移除

## 目标与范围

全项目删除通用invoke、callee或操作编号、无语义参数数组、寄存器式请求/回复，
以及仅为这些结构服务的Port、Dispatch、Adapter、调用计数和汇总。
调用方直接使用实际操作、语义参数与结果、已有共享数据和真实资源所有权。

## 已发布的直接调用迁移

- MON文件、流与说明文字使用命名操作及真实分配/释放。
- 战斗画布锁定返回像素地址；变色刷新直接使用共享背景和画布数据。
- 单条效果帧直接更新动作记录、加载独立图像、播放音效并绘制，
  已删除`SingleEffectPortAdapter`、请求/回复、callee表、寄存器拼装和分派计数。

以上是已发布切片，不代表全项目通用调用体系已经清除。

## 全局分派计数清理

当前批次删除`src`、`include`、`tests`全部`port_calls`字段、递增、汇总、
日志输出和对应断言；目标选择中混合累计这些调用数的group-A/group-B计数也删除。

只用于携带这些计数的函数参数、成员引用及构造参数同步删除。
测试保留状态、数据、实际操作观察及失败后缀断言；不把计数改名后继续保留。
旧MON测试的通用调用集合引用改为检查实际文件和读取操作；定义读取失败时
检查未进入后续字段读取与释放，不再断言已取消传播的寄存器残留。

验证结果：

- `src`、`include`、`tests`内`port_calls`零匹配，`git diff --check`通过。
- core战斗及特殊模式测试18/18通过。
- AddressSanitizer同范围测试18/18通过。
- SDL应用目标`openswd3`编译链接通过，未启动游戏。
- 完整暂存差异复核：调用顺序、实参值和业务状态写入保留；
  删除计数专用参数与成员，测试保留其余状态、资源和实际操作断言。

命令分别为`./build.sh core --test --test-regex '^(battle\.|special_modes\.)'`、
`./build-asan.sh --test --test-regex '^(battle\.|special_modes\.)'`和
`./build.sh app --build-target openswd3`。
日志位于`build/tmp/runtime/invoke-counter-final-{core,asan,sdl}.log`。
ASan构建仍报告既有的outcome-resolution测试第133行整数窄化警告。
这些结果不代表实机游戏流程或原版动态差分已验收。

## 剩余工作

其他效果、动作、目标选择、脚本和平台路径仍有通用invoke、请求/回复及转接层。
后续继续按实际调用链直接接入操作，直到全项目残留审查通过。
SDL主文件拆分排在通用调用体系移除之后。
