# OpenSWD3 工作约束

- 非必要不使用 Windows CMD。搜索、读取、分析、文件操作和日常开发默认使用 Bash 工具。
- 只有 Bash 无法完成的 Windows 专属操作才使用 Windows CMD，例如执行 `build.bat`、启动 Windows EXE 或验证 Windows 窗口行为。
- 原版动态验证统一由 Codex 准备 Frida spawn 一键工具，再由用户执行；Codex 不自行启动原版，不再要求用户先启动后 attach。
- 创建 Git 提交时必须使用 `$commit` Skill，不得绕过 Skill 直接执行 `git commit`。
- 项目自有 C/C++ 源码、头文件和测试统一使用四空格缩进，不使用 Tab；格式以仓库根目录 `.clang-format` 和 `.editorconfig` 为准。
