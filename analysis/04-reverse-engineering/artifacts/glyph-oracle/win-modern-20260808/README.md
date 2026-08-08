# Windows glyph-mask 动态样本

本目录原样保存 2026-08-08 在 Windows `10.0.26200` 环境中由
`analysis/tools/glyph-oracle/` 捕获的两次 SWD3 字形 mask。逻辑解释仍以完整
LST 为准；动态样本只固定该环境中 GDI 产生的基础轮廓。

运行环境的关键输入为：

- SWD3 EXE SHA-256：
  `78ddd0acf752dde32bbc4ea5a12256954878342899309c33516efd6dace0508a`
- `mingliub.ttc` SHA-256：
  `8f8afdb3ec7047118f6dc51b29d395e697ee6770d0afd0cd407457e2ad6e93cb`
- Frida：`16.5.1`
- Python：`3.8.10`
- agent SHA-256：
  `fa045bc4b6621fcd6ae792d1cb89c72a134d59d5c9bdff17572a1db28db1a1f6`

`run-20260808-230349-8960/` 包含 39 个字形：16 个 `20x20`、23 个
`16x16`。`run-20260808-231134-8540/` 是当前规范样本，包含 180 个字形：
17 个 `12x12`、95 个 `16x16`、68 个 `20x20`。

两次运行共有 39 个相同字号、相同 cache key 的字形；对应 mask 全部逐字节
一致。规范样本内三个字号的空格 mask 均为全零，所有行尾 padding bit 为零，
全部原始字符字节均可按 CP950 解码。

从仓库根目录执行完整性与重复性校验：

```bash
python3 analysis/tools/verify_glyph_oracle_capture.py \
  --expect-canonical \
  analysis/04-reverse-engineering/artifacts/glyph-oracle/win-modern-20260808/run-20260808-230349-8960 \
  analysis/04-reverse-engineering/artifacts/glyph-oracle/win-modern-20260808/run-20260808-231134-8540
```

校验内容包括索引、renderer 与字号映射、原始字节/cache key、文件集合、长度、
SHA-256、MSB-first 行尾 padding、空格 mask 以及跨运行重复字形。
