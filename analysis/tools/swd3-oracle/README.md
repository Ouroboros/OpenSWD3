# 原版统一动态采集工具

`swd3-oracle` 将原来的 `glyph-oracle` 与 `role-placement-oracle` 合并为一个
Frida 工具。它在原版恢复执行前一次性安装两个 hook，并在同一次运行中采集：

- `0x004368D0` 生成的 12×12、16×16、20×20 字形 mask 与实际 GDI 字体；
- `0x00413910` 收到的 GUID 248/249 角色位置、动作和相机状态。

工具只读取原版运行时数据，不修改磁盘上的 EXE、游戏内存、数据文件或输入。

## 一键运行

便携目录为 `build/vm/swd3-oracle/`，入口为 `swd3-oracle.exe`。把整个目录复制
到原版游戏目录下，确认原版尚未运行，再双击入口。目标 Windows 不需要安装
Python、Frida 或其他依赖。

工具会校验并启动 `swd32.exe`，输出到：

```text
swd3-oracle-output\run-YYYYMMDD-HHMMSS-PID\
  run.tsv
  font-selections.tsv
  glyph-masks.tsv
  role-placement.tsv
  masks\
    glyph-*.bin
```

进入目标画面并完成需要采集的操作后，可以正常退出原版；也可以在采集终端按
`Ctrl+C`。后者只会 detach，原版继续运行，需要手动关闭。

## 源码运行

从仓库根目录执行：

```bat
py -3 -B analysis\tools\swd3-oracle\capture.py --game-dir E:\Game\swd3 --output build\vm\swd3-oracle-output\manual-run-01
```

源码模式需要安装 `frida==16.5.1`。指定的输出目录必须不存在或为空，工具不会
清空或覆盖已有结果。

## 操作覆盖

为了覆盖三个字号以及中英文字符，依次经过启动界面、普通游戏画面、主菜单、
存读档页和文字子页面。需要角色位置样本时，在 GUID 248/249 可见的酒馆剧情
画面停留数秒。两类采集互不依赖：没有出现目标角色时，`role-placement.tsv`
只保留表头，不影响字形采集。

已确认的角色位置基准与既有 glyph 动态基准仍保留在逆向分析文档中；合并工具
只统一后续采集入口，不改写历史证据。
