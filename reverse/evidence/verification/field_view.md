# 地图画面（FieldView）

## 0x004120B0 FieldView::Render

一致的部分：8 个动作槽（0x4B9F68..0x4BA390，每个 0x98 字节）从后往前执行；领队与镜头按 0x4AB2D0 × 步长移动；72 条移动记录（0x4AD490 起，每条 0x21C 字节）：角色号非 0xFFFF、步数 < 0x7FFF、角色可见、不是“领队且路径移动”、不是“面向说话者且等待中”时，延迟为 0 才移动（0xD4 为 0 或旗标 0x4000000 时步长加倍），方向字节非 0xFF 时设动作方向，到格点时重排行（行号 (y >> 4) − 1，shr）、到终点时清速度、改旗标（& 0xB9FFFFFF | 0x2000000）、设动作／招式／方向、按 +0x1B 位 7 继续移动、角色 1 停键并居中、清 0x40000；清格、按方向移格、标格、步数 + 1 并置 0x8000、格子旗标；没有领队时碰触角色（0x2000）按 ProbeCells(0x100) 成为领队；同伴（0x4B8E18 起）同样移动，重排行用第 0 行；PanCamera（0x00414570）、ShakeCamera（0x004148F0）；锁定离屏表面、Paint、DrawTimer(0x190, 8, 0)、调试模式时 DrawDebug；Blt 到主表面（DDBLT_WAIT）；领队到格点时重排行、清格、移格（4 个步长变量）、标格、清步长、足迹三表右移 31 项后记入；领队不在走路且有 0x1000 时执行动作；图层轮换（0x4CC2DC、0x49E0BC、0x49E0C0、0x4B7944、摆动 2）；晃动中且当前地图不是 0x16 时恢复镜头。

差异（diff 轮），均已按原版修正后复核一致（exact 轮）：
- 原版在动作执行失败（0x004321E0 返回 0）时记 4 条调试日志，重建缺：「Act Err(picPaint:HeadSgn)」0x4A03F0（act、qq、dir），「Act Err(picPaint:PathMove)」与「Act Err(picPaint:CompanyMove)」0x49F66C（GUID、act、qq、dir），「Act Err(PicPaint:Render:MRl)」0x49F66C。已补（格式中的换行按惯例改为空格）。
- 原版两次调用的是 AIL_serve（0x00412696、0x004126F0），重建调的是 AudioPump（多更新了音乐队列、音乐流与音效）。已改为 audioDevice.Serve()。
- 领队重排行的行号 0x0041272E 为 `shr eax, 4`（无符号），重建对 int 右移。已改为按 u32 右移。

越界（新增 deviations.md 0x004120B0 一行）：方向字节下标超出、方向值 8 以上读三张 8 项表之外、角色号或行号越界——重建不读或跳过。

## 0x00412930 FieldView::Paint

全屏裁剪；0x4C9A18 & 4 时探照灯裁剪；影片时 UpdateMovie；否则旗标 0xF 或 0x13 时清屏，0x4C9A18 & 1 时清屏并只画动画 B；其余：DrawOverlays，非 0x48 且 0x13 时探照灯裁剪，16 位地图按镜头是否对齐格子选 DrawTiles16／DrawTiles16Scrolled，8 位地图先设调色板（0x4B7404）再选 DrawTiles8／DrawTiles8Scrolled，清调色板；非 0xB 时 DrawReflections；DrawRoles、动画 A（0x4B7BD0）、DrawSprites；非 0x48 时 DrawLeaves、DrawLightning、DrawFireflies、DrawWeather、DrawHeat、DrawRipples，再 DrawSpotlight 与动画 B（0x4B88C8）；全屏裁剪、DrawLines、Talk::Draw、DrawRoleHeads；没有领队或领队 +0x20 < 8（u16）且非 0x51 时金钱计数（0x27C, 0x1CC, variables[0]）；非 0xA、非 9、非 0x51 或按住 C 时 RefreshCursor；FadeColors(1)；Notices::Draw。一致。exact。

## 0x00412BE0 FieldView::DrawTiles16（DrawTileGrid）

全屏 (0, 0)..(640, 480)；非 0x48 且 0x13 时为探照灯周围（(x + 15) 清低 4 位 ± 0xC0）；每行跳过 宽 − 40 格，左边不为 0 时再加 (左 − 右 + 640) >> 4；起始格 宽 × ((上 + 镜头上) >> 4) + ((左 + 镜头左) >> 4)（sar），图块加图层偏移 0x4B873C；隐藏位 0x8000000 跳过，透明位 0x4000000 用 0x00417530，否则 0x004174D0。一致。exact。

## 0x00412D30 FieldView::DrawTiles16Scrolled（DrawTileGridScrolled）

镜头对齐时转 DrawTiles16；行列 = 镜头 >> 4（sar），起点 −小数部分（0x4CC2E4／0x4CC2E8），负行负列时取 0、行列数减 1、起点 16 − 小数；有小数时行列数加 1 并截到地图宽高；没有探照灯时先用 DrawPicture 画四边（透明模式 4，否则 0），再从 (+1, +1) 起整块画内部到 右 − 16、下 − 16；有探照灯时只画探照灯范围的内部。一致。exact。

## 0x00413220 FieldView::DrawTiles8

与 0x00412BE0 相同的网格，图块源为 基址 + 0x100 × 图块号 + 0x200（调色板之后），透明用 0x00417650（DrawTile8Keyed），否则 0x004175B0（DrawTile8）。重建 DrawTileGrid(TileSet{true}) 一致。exact。

## 0x00413370 FieldView::DrawTiles8Scrolled

- 镜头对齐时调的是 0x00412BE0（16 位版，0x00413391），原版 BUG，重建同样调 DrawTiles16（Paint 只在未对齐时调用，实际到不了）。
- 四边与 16 位版一样用 0x004170E0 模式 0／4 画，即把 8 位图块当 16 位像素读 0x200 字节（原版 BUG，保留，边缘为错色）；内部用 DrawTile8Keyed／DrawTile8。
- 差异（diff 轮）：重建的 SetTileSource 只按 8 位图块的 0x100 字节检查边界，四边实际读 0x200 字节，最后的图块会读出缓存视图。已加 wideRead（按 0x200 字节检查，超出取视图开头；deviations.md 0x00413370 越界一行）。修正后复核一致（exact 轮）。

## 0x00413870 FieldView::DrawRoles

层顺序 0x4A9A0C、0x4A9A04、0x4A9A08（即 2、0、1）；起始行 镜头上 / 16 − 20（idiv 截断）；70 行；行号 < 高度 + 20（有符号）时逐个 DrawRole，角色 +0x2C 不为 0 时 UpdateRoleSound。重建一致；负行号原版读表外，重建跳过（越界）。exact。

## 0x00413910 FieldView::DrawRole

(旗标 & 0x8400) ≠ 0x8000 返回 0；x − 镜头左 在 −0x140..0x3C0 外返回 1；有动作时帧取 +0x8A，旗标 0xB 时跳过；+0x98 非 0 时 PlaySoundAt 并清零；GetPicture(帧, +0x8C)（第二参数高 16 位为残值，0x00431DF0、0x00431C50 都先 and 0FFFFh，不影响）；0x100 且天气 & 7 < 4 时 DrawShadow（0x4995D4 表）；波动 +0xC8、相位 +0xC9、透明 +0xCA，0x4CDBE0 清零，DrawPicture(x + +0x28 − +0x50 − 镜头左, y + +0x2A − +0x54 − 镜头上, 宽, 高, +0x58, 图 +4)；底色：0x100 时取地图色 0x4AB97C／0x4AB994／0x4ACB98，0xF00000 时各加 0x499594 表值，非 0 时用 (模式 & 0x80000003) | 0x10 再画一次；附着动作 +0x3C（下移 0x1C，模式取其 +0x18）；写回相位；0x200 且非 0x48 时气泡；没有领队且有标签时以 0x49E0C8 颜色、x − 11 × 长 / 2 − 镜头左 + 0x10 画标签（样式 4）。两张 16 项表与 exe 一致。原版取不到图片时读空指针，重建返回（越界）。exact。

## 0x00413CA0 FieldView::UpdateRoleSound

距离 = ftol(sqrt(dx² + dy²))，相对领队；计时器 +0x30：周期（高 16 位）不是 0xFFFF 时计数（低 16 位）减 1，不为 0 则写回且本帧不响；距离 > 0x200 或不可见时，有 0x1000000 则停止该音效并清旗标；否则 FindRole 取索引，未响过且到期时重新装填（周期 + 周期），周期为 0xFFFF 则置 0x1000000，Play(音效, 0, 0, 1)，记 0x4C97F8[索引] = (距离 << 7) / 0x200、0x4BA430[索引] = (i16)((y 差 & 0xFFFF) << 6) / 0x200；音量 = 音效音量 × (0x80 − (距离 << 7) / 0x200) / 11，声像 = (x 差 << 6) / 0x200（均为有符号除法）。
- 差异（diff 轮）：原版 dx、dy 与平方和为 32 位环绕（imul），重建用 int（溢出为未定义行为）。已改为 u32 环绕。修正后复核一致（exact 轮）。索引越界原版写表外，重建跳过（越界）。

## 0x00413EA0 FieldView::DrawReflections

第 0 层，起始行 镜头上 / 16 − 5，共 40 行（−10..29），行号 ≥ 高度时停止，< 0 时跳过；旗标 0x20000000 的角色 DrawReflection。一致。exact。

## 0x00413F00 FieldView::DrawReflection

(旗标 & 0x8400) ≠ 0x8000 返回；x − 镜头左 在 (−0x140, 0x3C0) 开区间内才画（不检查旗标 0xB）；图片同 DrawRole；透明度 4；DrawPicture(x + +0x28 − +0x50 − 镜头左, +0x2A − 镜头上 + y + 8, 宽, 高, (模式 & 0x80000001) | 0x16, 0)。一致。exact。

## 0x00413FE0 FieldView::DrawDebug

小字体背景 0xFFFE、无阴影；0x4CAEA0 为 1 时 MarkCells：0x40000000（红 + 绿 + 蓝掩码，内缩 0）、0x10000000（绿 + 蓝，2）、0xFF（红 + 绿，4）、0x20000000（红，6）、0x800000（红 + 蓝，8），掩码为 0x49E0B4 起的三个字；0x4CAE9C 为 1 时 7 行状态（y = 0、0x10 … 0x60，颜色 0x49E0C8[0]）：领队坐标 × 0.0625 与镜头格、指针格（u32 >> 4）与像素、1000 ÷ 帧时间等、地图号／0x49E0BC／步速、0x4A9920 等六项（0x4C9A20 & 0x7FFFFFFF）、领队 id 与事件等、音乐（四首曲目、曲目字节、劇情／場景、0x80000/0x40000 与 0x20000/0x10000 分别为 Rep1／Cyc2／Nil，0x4A0534 为 "Nil"）；指针下区域非 0 时 FindRegion，找到则在 0x1C0 画名称、0x1D0 画 劇情號碼（事件 & 0x7FFF、点选旗标及「可點選／不可點選」、通行旗标及「不可通過／可通過」），找不到在 0x1B0 画「錯誤的地圖劇情號碼」；角色 1..0x49E0C4−1 中有动作且指针（无符号比较 jbe／jnb）在 (x − 0x10, x + 0x30) × (y − 0x40, y + 0x10) 内的：第二个起在 0x190 画「角色重疊」，0x1A0 用 0x49E0D4 色画 GUID／act／qq／dir，0x1B0 画 GUID／Talk／Path／Argu／ArrIdx。重建一致。帧时间为 0 与指针格越界的处理登记于 deviations.md 0x00413FE0（越界）。exact。

## 0x00414570 FieldView::PanCamera

0x4A99F0（横向剩余）或 0x4A98C0（纵向剩余）非 0 时：镜头四边加步长 0x4A9480／0x4A93D8，先算 纵向剩余 − 纵向步长，再扣减剩余；横向剩余为 0 清横向步长，先算的纵向值为 0 清纵向步长。一致。exact。

## 0x004145F0 FieldView::DrawShadow

模式 (动作旗标 & 0x80000003) | 0xC；缩放高 = 高 >> 1，倾斜 = −(高 >> 2)，类型位 0 时取正；三色底色 = tint；类型位 3 时缩放高再 / 2、抬高 = 动作 y / 2、附加 4；类型 & 7 < 2 时纵移 (动作 y + 抬高 + 8) / 2 + 附加，否则加翻转（2）、纵移 附加 + 动作 y + 4；横移 类型位 0 时 −4，否则 4。一致。exact。
