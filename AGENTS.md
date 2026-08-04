# OpenSWD3 工作约束

- 非必要不使用 Windows CMD。搜索、读取、分析、文件操作和日常开发默认使用 Bash 工具。
- 只有 Bash 无法完成的 Windows 专属操作才使用 Windows CMD，例如执行 `build.bat`、启动 Windows EXE 或验证 Windows 窗口行为。
- 创建 Git 提交时必须使用 `$commit` Skill，不得绕过 Skill 直接执行 `git commit`。
