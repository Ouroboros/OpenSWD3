# 原版 glyph-mask 捕获

本工具只捕获原版 `swd3.exe` 在 `sub_4368D0` 返回时已经生成的 1-bit
字形 mask，不修改磁盘上的 EXE，不替换游戏逻辑，也不自动操作游戏界面。

完整 LST 固定的捕获边界为：

```text
0x004368D0  入口：ECX=renderer，[ESP+4]=原始字符，[ESP+8]=目标 mask
0x00436974  返回：目标槽已有 height*row_bytes 字节的最终 mask
```

`capture.py` 会先校验原 EXE SHA-256，再通过 Frida spawn 挂起启动、装入
`agent.js`，最后恢复进程。没有显式的 `--confirm-run-original` 时，脚本拒绝
启动原版。

## 1. 安装 Frida

在 Windows CMD 中执行：

```bat
py -3 -m pip install frida==17.16.0
```

## 2. 建立一次全新的输出目录名

每次运行必须使用不同且不存在或为空的目录。例如：

```text
analysis\04-reverse-engineering\artifacts\glyph-oracle\win-modern-20260808-01
```

脚本不会清空或覆盖已有捕获目录。

## 3. 从仓库根目录运行

先进入 OpenSWD3 仓库：

```bat
cd /d E:\Game\swd3\OpenSWD3
```

再执行一条命令：

```bat
py -3 -B analysis\tools\glyph-oracle\capture.py --game-dir E:\Game\swd3 --output analysis\04-reverse-engineering\artifacts\glyph-oracle\win-modern-20260808-01 --confirm-run-original
```

终端出现 `[运行]` 后，由你正常操作原版。工具不会点击按钮或注入输入。

## 4. 首轮操作路径

首轮目标不是通关，而是让三个 renderer 和两类字符都发生 cache miss：

1. 在启动界面选择“开始游戏”。
2. 进入能显示中文和 ASCII/数字的普通游戏画面。
3. 打开主菜单、存档/读档页以及能进入的文字子页面。
4. 若操作成本可接受，再进入一次战斗画面。
5. 正常退出原版，等待终端打印 `[完成]`。

首轮产物至少应同时出现：

- `20x20`、`16x16`、`12x12`；
- 单字节 ASCII/数字；
- CP950 双字节中文。

如果不想退出原版，可以在捕获终端按 `Ctrl+C`。工具会 detach，不会强制结束
已经恢复运行的原版进程；之后需要你自行关闭游戏。

## 5. 回传内容

把整个本次输出目录保留并告知我路径。目录结构为：

```text
run.tsv
glyph-masks.tsv
masks/
  glyph-000001-20x20-key-XXXX.bin
  ...
```

`run.tsv` 保存 EXE、DLL、`Env.dat`、Frida agent 和候选細明體字体文件的哈希；
`glyph-masks.tsv` 保存每次 miss 的 renderer、原始字节、尺寸、mask 文件及
SHA-256。收到目录后，再做尺寸覆盖审计、mask 可视化和跨平台 provider 差分。
