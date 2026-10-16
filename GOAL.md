# 逆向还原整体执行计划

版本：2

## 目标与范围

目标目录 `/mnt/e/Game/swd3`，主程序 `swd32.exe`（SHA-256 `4c4c2268…c03f2a`），实现语言 C++20；IDAT 路径见 AGENTS.md。范围为 `swd32.exe` 的函数全集（游戏代码、静态链接的 MSVC 调试版 CRT、全局初始化 thunk、EH funclet）及其依赖的常量表、全局数据与游戏数据文件。尽量还原原模块与代码结构，原逻辑和BUG完整保留；平台层按 AGENTS.md 使用 SDL3 与 FFmpeg n9.0。Windows x86 为主平台，须可正常游玩；Linux 只要求编译、单元测试通过并用于代码验证。

## 不做的事项

不修复原程序BUG，不改造原业务逻辑，不扩展目标范围外的功能；不访问 OpenSWD3。

## 整体执行计划

由用户START GOAL后执行以下阶段。

1. 接续：读取Skill已完成的IDAT初步分析、已保存的工作数据库、结构概览和初始函数表，核对输入基线及范围缺口。
2. 还原：按模块逐函数实现，持续修正未知或错误的模块归属，直到全部代码还原。
3. 验证：全部还原后，从头逐函数对照汇编，修正差异直至bug-for-bug一致。

## 整体进度

- 当前阶段：第 2 阶段（还原）。
- 已识别函数数：1626（无 FLIRT 数据库导出；函数外代码头 0）。
- 函数总数：待确定（间接调用目标与函数指针表尚未全部核对，见 reverse/analysis/scope-and-gaps.md）。
- 已实现／总数：647／待确定（已识别 1626）。
- 已验证／总数：0／待确定（已识别 1626）。
- 待实现／待验证：979／1626（按已识别函数计）。
- 模块初分：已归属 884，unknown 742（reverse/analysis/module_assignment.tsv）。

## 当前工作

按 reverse/analysis/plan.md 自底向上还原。已完成：file、memmgr、crt 映射、LZO、资源包（tsw／act）、像素格式、绘图核心与全部 blitter、像素操作与缩放、Graphics、Input、静态初始化、TextFont、随机数、文本输入、音频（MSS 模拟）、A* 寻路、ACT 动画、角色结构与游戏状态、BattleUnit 大部分方法、战斗消息与结果面板、窗框与填色、Bink 播放模拟与 Movie、投影辅助与碎裂粒子特效、BattleHero 部分方法。Linux（i386）与 Windows x86 构建及 58 个单元测试通过。

## 下一步

BattleHero 剩余方法（0x0046E730 起）与 BattleEnemy（0x00475560 起），随后 figtalk、菜单与地图模块。

## GOAL完成条件

范围缺口已闭合，函数全集均有实现映射并通过汇编对照，范围内数据与资源均已核对且有证据，包含原有BUG，无剩余行为差异；Linux构建与单元测试通过；Windows构建通过且可正常游玩。

## 工作入口

路径相对工作区根目录。

- 规则：AGENTS.md
- 函数表：reverse/functions.tsv
- 源码：src/、include/；单元测试：tests/
- 构建：CMakeLists.txt、CMakePresets.json（linux／windows）、cmake/、scripts/deps/（SDL3 release-3.4.18、FFmpeg n9.0 获取与构建）
- 工作数据库：reverse/analysis/swd32.i64（输入 reverse/analysis/work/swd32.exe）
- 输入基线：reverse/analysis/input-baseline.md
- 结构概览：reverse/analysis/structure-overview.md
- 范围与缺口：reverse/analysis/scope-and-gaps.md
- 分析入口与 IDAT 用法：reverse/analysis/README.md
- 验证证据：reverse/evidence/（尚无）
- 临时目录：tmp/
