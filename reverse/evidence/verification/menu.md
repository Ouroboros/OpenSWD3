# 菜单核对记录

## 0x00439DE0..0x0043B480（第 1 遍，2026-10-18）

- 0x00439DE0 InitActs：旗标清零，填页表，18 个动画复位；背景、标签面板、标题箭头（方向 2，旗标 0x49 时 3）与七条条形动画为 0x232A 的动作 0、1、4、5、6、0x18..0x1B，四个系统项为 0x232B 的 0x2C..0x2F，头像动作 3，战斗面板 0x233B。通过。
- 0x00439FD0 Run：进入时清指针、标签面板动作 1；菜单 1、2：模式 0、背景动作 0x34、四个标签 0x232A 的 8..0xB、帧 0x1E（商店装备页 0x24，Open(0x24, 2) 后帧 0x41，调用进入表，帧处理改为装备页），音效 0xBB；3、6：关档案，四项清零复位为 0x232B 的 0x2C..0x2F，背景 0x4E，模式 0 或 3；4、5：模式 1、2；帧 0x40，旗标清零，去掉位 30、31。然后帧加一、读鼠标、分派、绘制，菜单关闭时清旗标位 1。通过。
- 0x0043A2A0 Open：页 = (模式 − 0x1E) / 6 + 0xB，SetPage、BuildTabs(5)、清按键与色调，三种字体阴影 (6,4,3)、无背景。通过。
- 0x0043A380 BuildTabs：清四个标签，在队的角色依次编号（动作 8 + 序号，选中的升起）；最后把第 count 个标签 +0x10 复制到 +0xC。四人全在时这次复制落在 0x004A6AB4（格式串 "%4d/%4d" 的前两字节，复制的是 0x004A6AB8 相同的 "%4"），无实际效果，重建跳过。通过。
- 0x0043A470 Dispatch：帧前、帧处理；Escape（0x4B7CB0）、带 6 帧间隔的 RightControl、RightShift、End，Down/Up/PageDown/PageUp 重复，Left/Right 的重复按 PageUp 的按住帧数判奇偶，Space 或 Enter 选择。通过。
- 0x0043A610 Present：帧 0x41 时滑动量 0x320；不在“加倍”条件（页 0xF、代码 > 1 且旗标 0x49，或页 0xB 且代码 ≥ 0x1F4）时减半；锁定离屏面取像素；非系统页画背景（帧 < 0x4B 时按滑动量平移，交换页时页 0xF 不平移）；非主页调用绘制，菜单关闭则返回；模式 0 时画标题与标签（加倍条件下滑动量至少 1、加倍至 0x320）、主页金钱（0x4117F0 第三参数不用）、指针、色调、角落通知，显示画面；记上一页与指针。0x0043A82B 的 nullsub_1 没有参数。通过。
- 0x0043A880 DrawPageTitle：标题亮度在主页升到 0x10、其他页降到 8；箭头半透明，模式只留 0x80000003；亮满时画动作 页 + 5（交换页时 0x14/0x15 互换），否则动作 页 + 0x19（0x28/0x29 互换）以亮度 − 0x10 的色调画。通过。
- 0x0043AAA0 DrawRoleTabs：颜色 (0x1D,0x1B,0x15)，选中标签亮度 +3 到 0x10，其他 −1 到 6，无选中或主页时 0x10；三条条形按 0x4F × 值 / 最大值 + 0x64 裁剪；名字、等级、三组比值；位 15 的角色变灰。差异（diff 轮）：等级格式 0x004A6ABC 原文为 "%2d 級/%d"（等级与距下一级的经验），重建只有 "%2d"，经验未显示。已改正，修正后通过（exact 轮）。
- 0x0043AE40 DrawScrollBar：框、条形平铺、滑块（_ftol(高 × from/to)，0x0043AF29/0x0043AF38）、上下箭头与按下态。差异（diff 轮）：两条动画失败日志 "Act Err(Menu:DrawBar)"、"Act Err(Menu:Prop:Render:bar)"（格式 0x0049FD0C）缺失，原版记录后仍按旧图片绘制。已补，修正后通过（exact 轮）。
- 0x0043B080 ActDrawer::DrawGhost：失败时记录 "Act Err(Menu::DrawActGhost)"（0x0049FD0C）并返回，否则以 alpha 半透明、模式只留 0x80000003 | 0x14 绘制。差异（diff 轮）：日志缺失，已补。修正后通过（exact 轮）。
- 0x0043B110 FillBox：先按裁剪框截取；模式 0 逐行 0x00417DE0（给像素数，内部减半成对数）、1 色调、2 四分之一、3 灰阶、4 八分之一、5 上下对称渐变。通过。
- 0x0043B480 SetPage：系统页、主页与七种角色页的 12 个处理槽与原版逐一对应（函数地址与 functions.tsv 的名字对照），页不设的槽保留上一页的处理；0x0044A160 与 0x0044A1D0 都映射为 StatusPrevious（回绕判定写法不同，结果相同）。通过。

## 补正：0x00417DE0 的计数是像素数（第 1 遍）

0x00417DE0 以 a3 >> 1 为像素对数（do-while，至少一次；不足一对见 deviations.md）。核 0x0043B110 时发现两个已标 exact 的调用方把它当成对数：
- 0x0042ED40 Talk::Draw 名牌变暗：给 8 × 名字长度 + 0x1C，应混 4 × 长度 + 0xE 对；重建混了两倍宽。diff 轮后改正，exact 轮通过。
- 0x00414E50 FieldView::DrawLines：五处给线段的像素长度或 limit；重建同样两倍宽。diff 轮后改为传像素数、内部减半，越过画面末尾时停止，exact 轮通过。

## 0x0043B980..0x0043D470（第 1 遍，2026-10-18）

- 列表：0x0043B980 CountItems、0x0043B9A0 SkipItems、0x0043B9C0 ItemAfter（越过末尾时原版经空节点读，重建停在空）、0x0043B9E0 ObjectDescription（0xFFDC 为 "無"，否则 maps.dat 表 0x13 到 %Q）、0x0043BB40 FitListRow、0x0043BB80 ListDown、0x0043BBC0 ListUp、0x0043BBE0 ListPageDown、0x0043BC60 ListPageUp、0x0043BC90 CountShown、0x0043BCC0 FitMenuList（空表加 "無" 但计数仍为 0）：通过。
- 0x0043BA40 ReadMouse（左键单击 1、双击 2，右键 4、8）、0x0043BAB0 DrawBox（四周大 8 的填充框与 0x233B 的只画框，框厚 8）、0x0043BD70 DrawDescriptionBox（滑动量减半，停在 0x154 或 0x1E0；八分之一变暗框、框与五行文字）：通过。
- 0x0043BE40 FindExitWarp：表 0x16，按入口后的地图列表查找。差异（diff 轮）：原版以 u16 零扩展与 int 地图号比较，重建先把地图号截成 u16；改为按 int 比较。修正后通过（exact 轮）。
- 0x0043BE90 ListTowns（表 0x17，名字到 %Q、地图与坐标、任一旗标 0x1388 + n 打开）、0x0043BFC0 Warp（清屏显示、换图、地震表填 0xCF、领队事件与位置取受控角色）、0x0043C090 Controls::Repeating（首帧或 > 10 帧后的偶数帧）：通过。
- 图鉴：0x0043C0D0 BestiaryEnter（1..500 的类别与第一张名声表的击败数，类别 0 列出，背景 0x33）、0x0043C2F0 BestiaryLeave（背景 0x43）、0x0043C3C0 BestiaryFrame（行区、标签区（步长 (x − 0x6A) / 0x14，到 0xE 为止）、滚动条按住与右键）、0x0043C520/0x0043C590/0x0043C600/0x0043C670（列表移动后 ShowFrom、CountShown、Choose，箭头位或 0x30/3，音效 0x2E）、0x0043C6E0/0x0043C760（类别 0..0xB）、0x0043C7E0（Right 后再响一次音效）、0x0043C800（子页时回到角色页）、0x0043C820 BestiaryDraw（超过 15 个时滚动条、标签、行，选中行画图 0x44 与渐变高亮）、0x0043CBD0、0x0043CC00、0x0043CC20 DrawBestiaryRow（第三个物品 x = 0xF228 在屏幕外）、0x0043D050 BestiaryLines（"%4d級"、六项 KnownValue、三件物品去重）、0x0043D370 KnownValue（"%s 是 %4d"、差 1 "大概是"、差 2 "似乎是"，随机误差 owner.random，再早 " ???" 并在上限 > 1000 时改第 8 字节）、0x0043D470 DrawBestiaryTabs：通过。
- 0x0043C9C0 BestiaryList：类别表 {0..9, 0xD, 0xE}（之后为 0，类别经 Right 封顶 0xB 不会取到）；同类的怪物列入（原版不限 64 个），已击败的读名字与已知度。差异（diff 轮）：读物品失败的日志 "怪物集合 物品資料無法讀取(%d)"（0x004A6B48 + 0x004A6B54）缺失，已补；另一条 0x004A6B38 的条件 i > 0x40 永远不成立。修正后通过（exact 轮）。
- 0x0043CEF0 BestiaryChoose：清节点，载入记录，灵力 = 等级 × (物品 2/3/5、回复 2/4)，体力 = 等级 × (技能 3/5)，再取各行。差异（diff 轮）：载入失败的日志 "讀取道具(book) 沒有定義物品號碼(%d)"（0x004A6B90 + 0x004A6BA0）缺失，已补。修正后通过（exact 轮）。
