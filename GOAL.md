# 逆向还原整体执行计划

版本：3

## 目标与范围

目标目录 `/mnt/e/Game/swd3`，主程序 `swd32.exe`（SHA-256 `4c4c2268…c03f2a`），实现语言 C++20；IDAT 路径见 AGENTS.md。范围为 `swd32.exe` 的函数全集（游戏代码、静态链接的 MSVC 调试版 CRT、全局初始化 thunk、EH funclet）及其依赖的常量表、全局数据与游戏数据文件。尽量还原原模块与代码结构，以正经的 C++ 类和结构体组织（见 reverse/analysis/architecture.md），原逻辑和逻辑BUG完整保留，越界读写类BUG不复现并逐条记录；平台层按 AGENTS.md 使用 SDL3 与 FFmpeg n9.0，窗口化显示（逻辑 640×480，窗口默认 1280×960、可拉伸，最近邻等比缩放加黑边，Alt+Enter 全屏）。Windows x86 为主平台，须可正常游玩；Linux 只要求编译、单元测试通过并用于代码验证。

## 不做的事项

不修复原程序BUG，不改造原业务逻辑，不扩展目标范围外的功能；不访问 OpenSWD3。

## 整体执行计划

由用户START GOAL后执行以下阶段。

1. 接续：读取Skill已完成的IDAT初步分析、已保存的工作数据库、结构概览和初始函数表，核对输入基线及范围缺口。
2. 还原：按模块逐函数实现，持续修正未知或错误的模块归属，直到全部代码还原。
3. 验证：全部还原后，从头逐函数对照汇编，修正差异直至bug-for-bug一致。

## 整体进度

- 当前阶段：第 3 阶段（验证），按版本 3 架构。
- 已识别函数数：1626（无 FLIRT 数据库导出；函数外代码头 0）。
- 函数总数：1626（范围缺口已闭合，见 reverse/analysis/scope-and-gaps.md 与 reverse/evidence/scope/）。
- 已实现／总数：1626／1626；静态链接的 CRT 与导入跳转按 AGENTS.md 映射到标准库或平台层；旧逐句转写实现（804 个）保存在标签 legacy-transliteration，仅作参考。
- 已验证／总数：1278／1626。
- 待实现／待验证：0／348。
- 模块归属：1626 个全部归属（reverse/functions.tsv 的 module 列）。

## 当前工作

第 2 阶段已完成：全部 1626 个函数已实现（含菜单各角色页、窗口生命周期与退出释放、动态初始化、异常处理小函数、CRT 映射、作弊与姓名对话框），范围缺口已闭合；重建版加了运行日志（exe 同目录 logs/，用户 2026-10-17 要求）。按 reverse/analysis/architecture.md 自底向上重建的模块：骨架与窗口 Display、文件、LZO、绘制核心 Painter、RLE 编解码、tsw/act 档案与动作脚本、Graphics、MemMgr、TextFont（自带细明体点阵字库 GlyphLibrary）、Input、音频（AudioDevice 与 Miles 封装）、Bink 影片（Movie 与 MediaDecoder）、随机数、游戏状态基础、文字输入框、配置（Env.dat）、按键状态、投影与粒子效果、A* 寻路、数据库（mon.dat／level.dat）、道具链表、LZO 压缩器（经原版存档逐字节验证）、名声与计数表（经原版存档逐字节验证）、队伍数据与升级、界面绘制、战斗界面绘制辅助（BattleHud）、战斗道具列表（BattleState）、战斗单位（BattleUnit 大部分方法与状态效果、BattleRole／BattleEnemy 构造与复位）、战斗讯息、启动器、游戏初始化链、新游戏起始状态、菜单核心与系统页（标题、设定、姓名输入）、存档预览、地图载入与绘制（Field、FieldView）、角色控制与路径（RoleControl、RolePath）、对话窗（Talk）、剧本解释器（0x427920 全部指令）与影片播放帧；各模块以旧实现生成的黄金数据逐项对照（reverse/analysis/golden/）。函数说明由 scripts/register.sh 写入函数表 notes 列，全局变量对照见 reverse/analysis/globals.tsv。

## 下一步

第 3 阶段：从函数表第一行开始逐函数重新依据汇编对照源码（分支、出口、位宽、溢出、调用顺序、随机数消费、状态副作用与时序），同时核对所依赖的常量表、全局数据与资源；修正差异后标记 assembly_exact，每轮对照结果记入 reverse/evidence/；verification_count 记通过的遍数（当前为第 1 遍，scripts/mark_verified.py --pass N）。并行事项：用户在 Windows 上运行 build/windows/swd3r.exe，按 logs/ 下的日志排查启动崩溃，直到可正常游玩。

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
- 代码架构：reverse/analysis/architecture.md；已知偏差：reverse/analysis/deviations.md
- 验证证据：reverse/evidence/（范围核对 scope/；逐函数验证按轮次记录）
- 临时目录：tmp/
