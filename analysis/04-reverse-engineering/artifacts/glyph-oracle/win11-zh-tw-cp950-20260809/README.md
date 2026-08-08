# Windows 11 繁体中文 CP950 字形 oracle

本目录只保留当前唯一字形基准：
`run-20260809-014152-8496/`。此前英文环境及较早的运行输出已经删除，不再
参与实现、测试或差分。

用户确认本次原版运行环境满足：

- Windows 11 已安装台湾繁体中文语言包；
- 非 Unicode 程序系统区域使用 Big5/CP950；
- Windows 首选语言为台湾繁体中文；
- 原版游戏中文字显示效果 100% 正确，与游戏原本显示一致。

`run.tsv` 同时记录了以下可机械复核的环境事实：

- Python 默认编码：`cp950`；
- 原版 EXE SHA-256：
  `78ddd0acf752dde32bbc4ea5a12256954878342899309c33516efd6dace0508a`；
- `mingliu.ttc` SHA-256：
  `d7857c403c7c79a4de93a11a22c8b0fedc077762e173a8b80b950c0b1e9caacc`；
- `mingliub.ttc` SHA-256：
  `8f8afdb3ec7047118f6dc51b29d395e697ee6770d0afd0cd407457e2ad6e93cb`。

本次共捕获 157 个 mask：`12x12=16`、`16x16=90`、`20x20=51`；所有字符
字节均可按 CP950 解码，三个字号的空格 mask 均为全零。校验命令：

```bash
python3 -B analysis/tools/verify_glyph_oracle_capture.py \
  analysis/04-reverse-engineering/artifacts/glyph-oracle/win11-zh-tw-cp950-20260809/run-20260809-014152-8496
```

本次使用的 agent 尚未生成 `font-selections.tsv`；字体选择的补充记录由独立
GDI 对照探针完成。该探针在 RGB555、RGB565 和 BGRA32 上均对
157 个 mask 达到逐字节零差异，随后生成了正式跨平台字形数据。

生成后的 atlas 还必须在开发机执行独立接收校验：

```bash
python3 -B analysis/tools/verify_legacy_glyph_atlas.py \
  --expect-canonical \
  assets/fonts/legacy-glyph-atlas.bin \
  analysis/04-reverse-engineering/artifacts/glyph-oracle/win11-zh-tw-cp950-20260809/run-20260809-014152-8496
```

正式 atlas 长 3,816,016 字节，SHA-256 为
`0a530284a3ff5fa5c426376571bd31acc8c4443f2237526273c5aefe10708df4`；
校验结果为 `oracle_exact=157/157`。这里的 157 是原版动态样本数；
atlas 本身在每个字号下覆盖 32,896 个原始字节 key。
