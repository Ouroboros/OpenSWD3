# TextFont 核对记录

## 0x004350E0..0x00436EA0（第 1 遍，2026-10-18）

字形由 GDI 改为自带字库属已记录的平台替换（deviations.md 0x00435500、0x004352E0 一行）；构造时的桌面分辨率目标另记一行（0x004350E0）。

- 0x004350E0 构造：字形 16 × 16，无背景，目标表按屏幕。0x00435130 析构、0x00435160 Init（后台缓冲 pitch / 2 × 高，16 × 16，"細明體"（0x0049FB74），步进 = 宽，无背景，阴影 0，清 2000 个字码，计数 0）、0x004351F0 Shutdown：通过。
- 0x00435240 CreateGlyphSurface（离屏面清黑，记高与 pitch / 2）、0x004352C0 ReleaseGlyphSurface、0x004352E0 RenderString（清黑后在 (0, 0) 绘制，忽略 x、y）、0x004353C0 SetGlyphSize（0x40 × 0x40 字形面，像素缓冲，步进 = 宽）、0x00435410 FreeGlyphPixels、0x00435430 AllocGlyphCache（行字节 = 宽 / 8，有余数加一；2000 个位图）、0x004354D0 FreeGlyphCache、0x00435500 SetFont（最大 64）：通过。
- 0x004355B0 SetTarget（行表、字节数 2 × w × h、全裁剪）、0x00435620 SetClip、0x00435650 SetAdvance、0x00435660 SetBackground、0x00435670 SetShadow：通过。
- 0x00435680 DrawGlyph、0x004358C0 DrawGlyphBold、0x00435AF0 DrawGlyphShadow、0x00435D80 DrawGlyphBoldShadow、0x00436030 DrawGlyphOutline、0x00436410 DrawGlyphBoldOutline：字形序号可等于计数；位 k 的列条件 x ≥ left − k 且 x + k + 1（粗体 + 2）< right；阴影与描边要求下一行在裁剪内（描边还要求行 > top）；渐变 0x100/0x80 的步进以 movsx 把 0xFF 扩成 0xFFFF 加到颜色（高半为行计数或指针高位），写入只取低 16 位，即 −1/+1（0x0043587E、0x00435AAB、0x00435D41、0x00435FEA、0x004363C5、0x004367FD）；描边两种在画完轮廓后以已推进全部行的颜色调用普通或粗体绘制。通过。
- 0x00436840 CaptureGlyphSurface、0x004368D0 RasterizeGlyph（白色绘制，非 0 像素或入位图）、0x00436980 FindGlyph（二分）、0x004369C0 InsertGlyph（排序插入，空缓存时不清槽 0）：通过。
- 0x00436AD0 DrawText：单字节步进减半；未缓存时先插入并栅格化；有背景时最后一个字填 glyphWidth − half + 2，其余填步进宽；样式 1/2/4/8/0x10 依次检查；新字计数加一，达到 0x7CF 时计数不变并清末槽。通过。
- 0x00436EA0 FillRect：底边夹取用未夹取的 y 判断，首行逐像素填，其余行复制首行。通过。
