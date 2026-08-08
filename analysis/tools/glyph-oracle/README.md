# 原版 glyph-mask 捕获

本工具只捕获原版 `swd3.exe` 在 `sub_4368D0` 返回时已经生成的 1-bit
字形 mask，不修改磁盘上的 EXE，不替换游戏逻辑，也不自动操作游戏界面。

完整 LST 固定的捕获边界为：

```text
0x004368D0  入口：ECX=renderer，[ESP+4]=原始字符，[ESP+8]=目标 mask
0x00436974  返回：目标槽已有 height*row_bytes 字节的最终 mask
```

`capture.py` 会先校验原 EXE SHA-256，再 attach 到你已经启动的原版进程并
装入 `agent.js`。host 不包含 spawn、resume、kill 或自动点击逻辑。

## 1. 安装 Frida

在 Windows CMD 中执行：

```bat
py -3 -m pip install frida==17.16.0
```

## 2. 手动启动原版并取得 PID

手动运行原版，停留在启动界面，不要先进入游戏。然后在 Windows CMD 查询：

```bat
tasklist /FI "IMAGENAME eq swd3.exe" /FO LIST
```

记下输出中的 PID。下面用 `12345` 作为示例。

## 3. 建立一次全新的输出目录名

每次运行必须使用不同且不存在或为空的目录。例如：

```text
analysis\04-reverse-engineering\artifacts\glyph-oracle\win-modern-20260808-01
```

脚本不会清空或覆盖已有捕获目录。

## 4. 从仓库根目录运行

先进入 OpenSWD3 仓库：

```bat
cd /d E:\Game\swd3\OpenSWD3
```

再执行一条命令：

```bat
py -3 -B analysis\tools\glyph-oracle\capture.py --pid 12345 --game-dir E:\Game\swd3 --output analysis\04-reverse-engineering\artifacts\glyph-oracle\win-modern-20260808-01
```

终端出现 `[已附加]` 后，再回到原版启动界面继续操作。工具不会启动、结束、
点击或注入输入。

## 5. 首轮操作路径

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

如果不想退出原版，可以在捕获终端按 `Ctrl+C`。工具只会 detach，原版继续
运行；之后需要你自行关闭游戏。

## 6. 回传内容

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
