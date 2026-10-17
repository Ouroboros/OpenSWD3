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

## 0x004146F0 FieldView::FadeColors

三个步长（0x4A9A00、0x4A94B8、0x4C97F0，浮点）都为 0 时返回；`step` 非 0 时帧数 0x4A9934 < 0 则恢复步长（0x4A93DC、0x4B6C74、0x4C9A04），否则减 1；帧数 ≥ 0 时三色级（0x4A93E0、0x4B6C70、0x4C9A00）加步长；最后按 ftol 后的级给 0x4B000 个像素上色（0x00420490）。一致。exact。

## 0x004147E0 FieldView::DrawAnimations

逐个执行动作；GetPicture(+0x52, +0x54)；透明度 +0x92；DrawPicture(x − +0xC, y − +0xE, 宽, 高, +0x20, 0)；+0x60 非 0 时 PlaySoundAt(…, 镜头左 + x, 镜头上 + y) 并清零；+0x94 为 1 时摘除并释放。
差异（diff 轮），均已修正后复核一致（exact 轮）：
- 动作执行失败时原版记「Act Err(PicPaint:PlayAct)」0x49FD0C（act、qq、dir），重建缺。已补。
- 坐标字 +0、+2 原版零扩展（0x0041486B `xor ecx, ecx` / `mov cx, [esi]`，0x00414898 同），重建按 i16 符号扩展。已改为按 u16 读取。

## 0x004148F0 FieldView::ShakeCamera

0x4ACE70[0] 不是 0xCFCF 且地图不是 0x16 时：索引 0x4B7EB0 处为 0xCFCF 则归 0；取一对 i16 位移；0x4A94A4 减 1 后 ≤ 0 则重置为 0x4AD0D0、索引 + 2；记下 0x4A93E4／0x4A992C 为原镜头后四边加位移。一致。exact。

## 0x004149B0 Game::RefreshCursor

0x2E 键按下时指针动作 = 0xF；不在菜单时：有步长则计数 0x4ACBC8 加 1 并封顶 0，否则空闲计数 0x4ACBCC 超过 0x10 后 0x4ACBC8 减 1（下限 −0x20），点击且指针 x > 0x262、y < 0x18（无符号）且没有领队时菜单状态 = 0x80000001；没有领队或领队 +0x20 < 8 时在 (0x4ACBC8 + 0x282, 0) 画系统按钮（0x0040EBF0）；动作变化时清延迟；执行指针动作；GetPicture(帧, 招式)；透明度；DrawPicture(指针 − 动作偏移, 宽, 高, 旗标, 图 +4)。
差异（diff 轮），均已修正后复核一致（exact 轮）：
- 动作执行失败时原版记「Act Err(RefrashCursor)」0x49F66C（GUID 0x4ACCFC、act、qq、dir），重建缺。已补。
- 原版 DrawPicture 第 6 参数传图片 +4（调色板指针），重建缺省为空。已传 PaletteArgument。

## 0x00414B60 FieldView::DrawSprites

屏幕坐标 = ftol(x) − 镜头左、ftol(y) − 镜头上；执行动作；在 (−0x48, 0x2C8) × (−0x48, 0x228) 内时画（透明度 +0x8A，参数图 +4）；延迟 +0x44 非 0 时跳过移动；x 用 fst 存回后对扩展精度和取整（重建 double），y 先 fst 到栈上 float 再取整（重建 float）；离目标 ±0x20 以内（各边开区间外即保留）时释放。
- 差异（diff 轮）：动作执行失败时原版记「Act Err(PicPaint:DrawTObj)」0x49FD0C（act、qq、dir），重建缺。已补；复核一致（exact 轮）。

## 0x00414CE0 FieldView::DrawRoleHeads

执行动作；画在 (+0x98 − 动作 x, +0x9E − 动作 y)，参数 0；速度 +0x9A & 0x7FFF 非 0 时 x += 速度、速度 × 3，x ≤ −0x78（0x00414DED `cmp 0FF88h` + jle，有符号）或 ≥ 0x2F8（jge）时释放；否则 x += 2 × (目标 − x) / 3，步长在 −1..1 时直接到目标。
- 差异（diff 轮）：动作执行失败时原版记「Act Err(PicPaint:drawRoleHead)」0x49FD0C，重建缺。已补；复核一致（exact 轮）。

## 0x00414E50 FieldView::DrawLines

每行像素 = 0x4A0E74 >> 1；模式高字节 0x8 静止：逐行混色 limit 对；0x80 下落：从最后一列起长度 += 2 × Below(6) + 0x30，≥ limit 夹住并计数；0x40 上升：偏移 −= 0x30 + 2 × Below(0x18)，≤ 0 夹住，长度 = limit − 偏移；以上全部完成时改为静止；0x20 展开：偏移 += 2 × Below(0x30) + 0x30，≥ limit 夹住，长度 = limit − 偏移，> 1（jg）否则为 2，全部完成时释放；0x10 收起：长度 −= 0x30 + 2 × Below(0x30)，> 2（jg，有符号）否则为 2，全部完成时释放；混色地址 = x + 偏移 + 每行像素 × (列 + y)，颜色 0x49E0C8[色号]。一致。exact。

## 0x004151F0 FieldView::DrawOverlays

层级 0..30：取第一个层级相符且区域（+0xA、+0xC、+0xE + 16、+0x10 + 16）与镜头矩形相交（IntersectRect）的叠层；按交集相对镜头的矩形 SetClip（0x00416F80）；视差 +0x16／+0x18 为 8 时偏移 = 镜头 ≥ 区域 ? 镜头 − 区域 : 0，否则 = 视差 × (镜头右／下 − 区域左／上) / 16（cdq/and/sar，有符号），镜头在区域之前时加裁剪起点；DrawPicture(裁剪左 − 偏移 x, 裁剪上 − 偏移 y, +6, +8, 0, 0)；SetClipClamped(0, 0, 640, 480)。一致。exact。

## 0x004153D0 Notices::Draw

背景 0xFFFE、无阴影（两遍）；y 从 8 起每条 + 0x18；旗标 0xE 未设时以 0x49E0D4 色、样式 0x10 画在 0x280 − 11 × 长度；帧数减 1，原值为 1 时释放。一致。exact。

## 0x004154A0 FieldView::UpdateMovie

未播放返回 0；对话框、线条、头像存在或暂停（0x10）时首次保存画面、之后恢复；保存过则恢复一次并清旗标；帧号 ≤ 0 时画帧并在上下各清 (4 × 帧号 + 0x34) 行，帧号 + 1；帧号 ≥ 10000 时淡出：变暗（选项 1）为 TintPixels 2 × (10000 − 帧号)，否则清黑屏旗标重画地图再上下清 2 × (0x272E − 帧号) 行；帧号 + 1，变暗超过 0x271F、收拢超过 0x272E 时释放全部、关文件、清 0x4B7A9C 位 1、帧间隔 0x23；其余帧读帧（到末尾则帧号 = 10000）或画帧后帧号 + 1。一致。exact。

## 0x004157D0 FieldView::DrawMovieFrame

起点 = 画面 + 每行 × ((480 − 高) >> 1)；有调色板时每行像素逐个查表写入，否则先原地 ConvertPixels（(长 − 8) >> 1 个）再整行拷贝（同一帧重画会再转换一次，原版 BUG，保留）。一致。exact。

## 0x004158C0 FieldView::ReadMovieFrame

已读过的块：SeekBegin(偏移 + 0xC)，非空时读并整块解压（不扣尾部 0xC，原版如此）；否则从最后一块接着读：超过最后一帧时把画面存为帧并返回 1；每块新建记录、读入，当前帧号正好是调色板帧时 SeekEnd(−(0x300 × (余数) + 0x400)) 读 0x300 字节换算 RGB555 并 ConvertPixels（之后不回到原位置，原版如此）；取尾部三个 dword 作为下一块；到当前帧时解压 (大小 − 0xC)。
- 差异（diff 轮）：原版三处调用 AIL_serve（只服务 Miles），重建为 AudioPump（多更新音乐队列、音乐流与音效）。已改为 audioDevice.Serve()；复核一致（exact 轮）。

## 0x00415B70 FieldView::DrawWeather

旗标 5 时处理两个粒子：每个先取 Below(1000)；在 x ≤ −0x280、x ≥ 16 × (宽 + 0x28)、y ≤ −0x140、y ≥ 16 × (高 + 0x14) 之外时，随机数 ≥ 0x3DE 才重生：目标色调 0、目标速度 = 随机数 / 200 + 1、动作 = 0x4ACEF0 + 随机数 × 0x4C97F6 / 1000，按 0x4B7EB4 取 0：x = Below(16 宽)、y = 16 高 + 0x76、步长 Below(1) − 2 两次；1：同位置、步长 Below(2) 两次；2：y = −0x12C、步长 Below(2) − 2 两次；3：y = −0x12C、步长 Below(2) 两次；其他不重生。范围内时计数 + 1 超过速度则归零、移动（步长 + (步速 × 0x4B7518/0x4B7920) >> 2）、随机数 < 0x19 时目标速度 = Below(5) + 1、速度逼近目标、随机数 < 5 时目标色调 = Below(4) − 5、色调逼近目标；动作 0x232B 以该动作执行，三色底色 = 色调，透明度取动作，模式 0x4C97F6 为 4 时 4（加亮）否则 0x2C（减暗），参数 0。一致。exact。

## 0x00415EE0 FieldView::DrawBubbles

在镜头 ±0x40 内时：摆动 = Below(5) − 3；动作 0x232B 取 0x3B 执行；4 个标记（0x4CACE0 起每个 16 字节）中角色相符的：取角色（0x0040C0D0）坐标，清发射位，Below(1000) > 250 置位；计数 < 0x18 且发射时新建气泡（x = 16 × (x − 8 + Below(16))，寿命 地图 0x32 时 Below(16) + 16 否则 Below(30) + 30，横步 Below(16) − 8，纵步 −1 − Below(1)）；逐个气泡取 Below(1000)，移动（横向加摆动），> 50 时寿命 − 1；寿命 > 0 时按地图 0x32（寿命 − 0x1E/0x26/0x32）或（(寿命 >> 1) − 0x1E）上色，画在 (x / 16 − 动作 x − 镜头左, y − 动作 y − 镜头上)，模式取动作旗标；否则用下一个覆盖当前并在同一轮再处理，没有下一个时释放并结束。
- 差异（diff 轮）：0x00415F7D 以 movsx 取标记的角色号（符号扩展）与零扩展的 id 比较，id ≥ 0x8000 永不相符；重建按 u16 比较。已改为同样的比较；复核一致（exact 轮）。

## 0x004161C0 FieldView::DrawLeaves

旗标 6 时 4 片叶子（0x4B86F8 起；动作 0x4AD0D8 起每个 0x98 字节）：在地图 ±0x78 外时标为未放置；未放置时 x = −0x40、y = Below(16 高)、纵步 Below(5) − 2、横步 Below(7) − 3（0 改为 2，负时 x = 16 宽 + 0x40）；否则 Below(1000) > 250 时转向 t = Below(3) − 1，纵步 += Below(3) − 1 夹在 ±3，横步 += t 夹在 ±5，为 0 时 = −t；动作 = 栈上表 {'4','4','4','6','8','8','8','6'}[纵步 + 3]（0x00416311 以 var_5 为基址，反编译误作 5 字节无偏移），横步 > 0 时 + 1；执行动作并画，模式取动作旗标；最后 x、y 各加步长。一致。exact。

## 0x004163C0 FieldView::DrawHeat

旗标 7 时：计数 0x4CAE90 为 0 时按列（0..0x280 步 0x50）、行（0..0x4B000 步 0xC800）给 48 组依次取长度 2 × Below(0x26) + 0x4C、偏移 行 + Below(3) + 列 + 0x27F、行数 Below(0x4E) + 1；否则计数 & 3 为 0 时按 (计数 >> 2) & 3 选 0：64 个长度重取，1：48 个偏移重取，2：64 个行数重取（0x4AB2F8..0x4AB378、0x4AB8FC..0x4AB97C 各 64 个 i16）。计数 + 1。每组按行数把 dword_4CD76C + 2 × 偏移 处的长度字节从下一行（+0x500）向前复制（rep movsd/movsb），长度与行数以 movsx 读出（取值恒正）。越界复制按偏差表跳过。一致。exact。

## 0x00416590 FieldView::DrawLightning

Below(1000) > 900 时：旗标 8 置位则计数 0x4AAEC8 + 1，超过 8 时夹到 8；否则 − 1，小于 0 时夹到 0。之后计数或显示数 0x4B88BC 非 0 才画：显示数清 0；64 道（0x4B7520 起 16 字节）中未激活的在已启动数 < 计数时启动（x = 16 × Below(0x280)、y = 0、斜率 Below(3)，x > 0x1400 时取负、速度 Below(0x1E) + 0x10、长度 Below(0x10) + 8、寿命速度 > 0xF 时 0x20 否则 0x10、相位 0、激活）；激活的沿线逐段：地址 > 画面起点时，≥ 画面起点 + 0x96000 则停用，否则以亮度 v5 调亮 1 个像素（0x004239D0 返回值未用，0x00420490）；x 加斜率、下一行、亮度 + 1（寿命 0x20 时再 + 1），段数超过长度结束；之后 x += 速度 × 斜率、y += 速度，寿命 − 1 后非 0 则显示数 + 1，否则停用。
- 差异（diff 轮）：0x00416610 的画面末端固定为 0x96000 字节，重建用 跨度 × 高。重建画面固定 640 宽，数值相同；已改为固定像素数；复核一致（exact 轮）。

## 0x004167B0 FieldView::DrawFireflies

计数 0x4BAB80（上限 1）与显示数 0x4B737C 的增减和闪电同构（Below(1000) > 900、旗标 0x16）。96 个（0x4C8C80 起 16 字节）中未激活的在已启动数 < 计数时启动：x = 16 × Below(0x280)、y = 0、斜率 Below(3)（x > 0x1400 取负）、速度 Below(3) + 1、长度 1、寿命 0xA0 × 速度 − Below(0xA0)、相位 0、激活。激活的：亮度 = 0x1F − (0x1E0 − 寿命) / 0x18，以 jbe（无符号）> 0x3E8 时为 0；摆动 = 栈表 {0,1,1,1,0,−1,−1,−1}[相位 / 4]；逐段：地址 > 起点 + 0x500 时，≥ 起点 + 0x95B00 则停用，否则中心亮度、上下左右 >> 1、四角 >> 2 各调亮一个像素；x 加斜率、下一行、亮度 + 1（寿命 0x20 再 + 1），段数 ≥ 长度结束。之后 x += 速度 × 斜率、y += 速度、相位 + 1 超过 0x1C 归零、寿命 −= 速度，> 0 时显示数 + 1 否则停用。越界像素按偏差表跳过。一致。exact。

## 0x00416B30 FieldView::DrawSpotlight

旗标 0x13 时：动作 0x232B 取 0x4E 执行，裁剪区 (x − 宽/2, y − 宽/2, x + 高/2, 高/2 + y)（原版混用宽高），在 (x − 宽/2, y − 高/2) 以模式 0 画；再取 0x4F 执行，裁剪区 x、y ± 0xC0，以模式 0x2C 画；位置与目标 0x4A94A8/0x4CAE80 不同时加步长 0x4ACE58/0x4ACE5C，到达的轴步长清 0。图片缺失按偏差表跳过。一致。exact。

## 0x00416CC0 FieldView::DrawRipples

链表 0x4AC9B8（next 在 +0x28，以 0x4AC990 为前驱哨兵）：每个依次复制画面（0x00430D50）、折射（0x00430D80）、推进（0x00430EB0），推进返回非 0 时摘下并析构（0x00430CF0）释放，否则前进。一致。exact。

## 0x00430180..0x00430FF0（第 1 遍，2026-10-18）

- 0x00430180 FillCellMark：上下两行写 (a3 >> 1) 个 dword 像素对，值为 (u16(color) << 16) + color（0x0043018F..0x0043019E）；左右两列各 a4 行写 u16，行距 4 × (pitch >> 2) 字节。差异（diff 轮）：重建两个像素都写低 16 位，颜色超过 16 位时第二个像素不同；宽度按除法而非 sar。已按原式改正，修正后通过（exact 轮）。
- 0x00430230 MarkCells：从镜头 +16 的格起 28 行 × 38 列，命中掩码时 FillCellMark(inset + x − (offX & 0xF), inset − (offY & 0xF) + 16 + 16 × 行, 0xF − inset, 0xF − inset)。通过。
- 0x004302F0 DrawDebugText：vsprintf 到 0x100 字节缓冲，小字体（0x004A9ED0）样式 0x10。通过。
- 0x004308C0 DrawTimer：which 0/1 分别需旗标 0x10/0x4A，0x4C 隐藏；帧数 /30（有符号）夹到 0；动作 0x232C，十分钟位非 0 才画，分、冒号（0xA）、十秒、秒依次画并右移图宽。通过。
- 0x00430C60/0x00430CF0 ScriptObject 构造与析构：两页高度清零，阻尼 4，页 0；屏幕副本原版不清零，所有调用方在 Refract 前先 CopyScreen，重建清零无可见差别。通过。
- 0x00430D10 MoveTo、0x00430D50 CopyScreen（2 × pitch × lines 字节）、0x00430D80 Refract（只读第一页，每次两个像素，斜率差 >> 3）、0x00430EB0 Step（翻页，四邻与左右的和 >> 2 减旧值，再减去 >> 阻尼，新页全 0 时为真）、0x00430FF0 Splash（负坐标用 CRT rand 取格内位置，半径内加 _ftol((r − √d) × depth)）。通过。
