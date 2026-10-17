# 配置模块验证（第 1 遍）

## 0x00423A10（已删除）

安装时新建 Env.dat 并调用 0x00423AF0 写入。按用户要求随安装命令行删除（偏差表第 75 行）；源码 src/config/config.cpp 补上说明注释。exact（删除已核实）。

## 0x00423AF0 Config::Update

原版：打开 Env.dat，旧格式时整体后移 4 字节并置 v12；写格式标记 0xFFFFFFFF，按键 16 字节（+4..+0x13，槽 4,6,3,5,1,12,9,0,10,2,16,17,18,19,7,8），+0x14 dword = 参数 6（unknown14），+0x18..+0x1D = 音量、音乐、缓存、关闭特效、文字速度、战斗速度；旧格式时 +0x1E..+0x2D 清 0（掩码、调试位）；+0x2E 起安装目录、光驱目录、安装类型 0x4C97F4、会话标记 0（0x00423D8C），文件长 = 两串长 + 0x32。调用方：0x00449050、0x0044B010（游戏内设置）与读入时的旧格式转换。重建按用户要求存入 [game]（偏差表第 75 行）：按键、unk14 与六个设置值一致，掩码与调试位保留。
- 差异（diff 轮）：重建保留 session_open，原版重写的文件以 0 结尾，保存设置即清除会话标记（之后崩溃，下次启动不清空地图缓存）。已改为同时清除 session_open，新增测试 config_test「saving the settings clears the session mark」。复核一致（exact 轮）。

## 0x00423E00 Config::SaveKeys

原版把 16 个按键从 Env.dat 偏移 0 写起，盖掉格式标记，下次启动按旧格式转成默认。重建按用户要求只写 [game.keys]（偏差表第 75 行登记）。exact（平台替换已核实）。

## 0x00423FB0 Config::ImportEnv

原版：清 0x4B7384 起 0x80 字节；格式标记不是 0xFFFFFFFF 时取安装类型、恢复默认按键（0x00424390），以 6、6、0x3C、0x64、1、2、0xA 写回（0x00423AF0）后重读；按键（同上槽序）、+0x14 dword、+0x18..+0x1D、掩码 +0x1E/+0x20/+0x22（0x49E0B4..B8）、调试位 +0x24 & 1、两串目录与安装类型。重建 ImportEnv 的偏移（KeysAt 4、Unknown14At 0x14、SettingsAt 0x18、MasksAt 0x1E、DebugAt 0x24、StringsAt 0x2E）、槽序 EnvKeyOrder 与读取的值一致；目录、安装类型不再使用，旧格式与缺失时取默认，会话标记取末字节——均为偏差表第 75 行登记的用户要求。exact（平台替换已核实）。

## 0x004242B0（已删除）

逐个盘符找光驱。按用户要求随光盘检查删除（偏差表第 73 行），src/game/game.cpp 留有注释。exact（删除已核实）。

## 0x00424390 Config::SetDefaultKeys

槽 0..10、12、16..19（0x4B7384 + 4 × 槽）= 1、0x39、0x36、0xCB、0xC8、0xCD、0xD0、0xC9、0xD1、0x9D、0xCF、0x1C、0x13、0x1E、0x22、0x3B。重建 Button 槽号（controls.h）与 Input 常量逐项相符。exact。

## 0x00424330 Game::SizeCaches

GlobalMemoryStatus 的物理内存 / 6（偏差表第 70 行：SDL_GetSystemRAM，上限 0xFFFFFFFF，商不超出 int），< 0x400000 取 0x400000，> 0x1000000 取 0x1000000，交给 0x004315C0；0x00432010(0x80000)。重建常量与顺序一致。exact。

## 0x00424440 Launcher（OnMouse／OnCommand／OnClose／Init）

Win32 对话框 0x6B 的过程（平台替换，偏差表第 67、69 行）。WM_MOUSEMOVE／WM_LBUTTONDOWN：先存旧高亮、清当前；纵坐标（HIWORD，u16）7..0xBB 内：[0x2B,0x52) 且旗标 0x12 → 第 0 项高亮 5，wParam == 1 时结束对话框 1；[0x57,0x7E) → 第 1 项，结束 2；[0x1E4,0x20B) → 第 2 项，打开 www.softstar.com.tw；[0x20E,0x235) → 第 3 项，打开 <安装目录>Readme.txt；[0x235,0x25C) → 第 4 项，结束 6；新亮起任一项时播放 WAVE 0x77；控件 0x40E..0x412 按高亮显示。WM_CLOSE 结束 6；WM_INITDIALOG 放置控件（0x30/0x5C/0x1E9/0x213/0x23A，4，0x27 × 0xB4）、清高亮；WM_COMMAND 0x3FE（先开对话框 0x70）→ 2、0x3FF 网址、0x400 Readme、0x401 → 6、0x402 → 3，对话框不含发出这些命令的控件。重建 Layout、HotTop/HotBottom、kLeftButton、命令值与 Choice 映射逐项相符。exact。

## 0x00424910 Game::StartGraphics

调用 0x00424B90，返回 1。一致。exact。

## 0x00424930 Launcher::RunBrowser、0x00424A60 Launcher::RunTextViewer

注册表取浏览器／文本程序后 WinExec（平台替换，偏差表第 68 行：交给系统默认处理程序）。exact（平台替换已核实）。

## 0x00424B90 Game::InitDrawKernel

窗口句柄；0x4BABA4 = 0、0x4A94A0 = 0；隐藏光标；0x00424EF0；鼠标灵敏度 2.0、位置 (0x1E0, 0x168)；音乐音量；0x00416D30 失败时 MessageBoxA(正文 "SWD3"，标题 "DrawKernel init Failed.") 并 WM_DESTROY；(0x00425570(1) + 0xFFFFF) >> 20 为已缓存 MB，缓存设定 − 已缓存 + 0x40 ≥ 剩余空间（0x00411D90）时 Sleep(0x400)、MessageBeep(0) 并 nullsub_1(「!!! 注意 !!!」, 0x004A0EFC, 该值)；0x004259B0；锁定取画面；三色掩码各转换一像素；清屏色 = 填充色对；三种字体（20/16/12，屏幕尺寸参数 0x0040F340 不用）；0x4A99F8 = 0xB；裁剪 0,0,640,480；16 组文字颜色 COLORREF → 555 → 显示格式成对；再锁定；0x0040DD20(0x23) 失败时 MessageBoxA("mmTimer init Failed.", "SWD3")、0x4BABA4 = 1、WM_DESTROY；0x0040F040（失败时其内部发 WM_DESTROY，返回值不用）；脚本缓冲 0x8000；maps.dat 自 0x200 起读入；0x00402030、0x0040E0B0、0x00439DE0、0x00425B50；菜单状态 0x80000003。重建逐项对应（ShowError 参数为 标题、正文，两处与原版相符）。
- 差异（diff 轮）：0x00424C79 的 nullsub_1 未转为调试日志。已补 DebugLog(u8"!!! 注意 !!! 遊戲過程中,磁碟空間不足\n請確認至少剩餘 %dM磁碟空間.", 值)（标题与正文以空格连接，同 0x0040D3C0 的写法，文字与 exe 中 Big5 转换后逐字相同）。复核一致（exact 轮）。

## 0x00424EF0 Game::InitDevices

0x4ACB90 = 0；DirectInput（0x00436FB0）失败时 |= 2、MessageBoxA(正文 "DirectInput 初始化錯誤%n" 0x4A0F5C，标题「軒轅劍三」0x4A0F78)、WM_DESTROY、返回 0；Miles 启动（0x00485360）、音效（0x004859B0）、MIDI（0x00484DD0，参数为数字驱动，重建 Open 忽略）、流（0x004865B0）；显示 0x00437570(0x4E22, 640, 480, 16) 失败时 0x4B7A9C |= 4、MessageBoxA(正文 "SWD3"，标题 "DDraw init Failed.")、WM_DESTROY、返回 0；成功时帧表面 0x00437B60(640, 480)、0x4B7CAC = 1、Bink 初始化（DirectDraw 对象与数字驱动，平台替换）。重建 ShowError 两处的标题／正文与原版相符。
- 差异（diff 轮）：DirectDraw 失败时重建没有置 systemFlags 位 4（QuitRequested，0x00424FBB..0x00424FD0 读改写 dword_4B7A9C | 4；关闭流程 0x0040A570 检查此位）。已补上。复核一致（exact 轮）。

## 0x00425040 Game::InitInstall

安装类型 2、音量 6、音乐 6、缓存 0x3C、文字速度 2、战斗速度 0xB；当前目录 + "\\" 为安装目录；光驱查找（已删除，偏差表第 73 行）；读入设置（0x00423FB0，重建为 [game]，偏差表第 75 行）；0x00424330；0x00425570(0)；建立 Save、Data、Music、ScrnShot、Video；置旗标 0x48，关闭特效为 0 时清除。没有 Env.dat 时 v1/v2 未初始化，见偏差表新增行。重建默认常量与顺序一致。exact。

## 0x00425150 Game::MakeDirectory

<安装目录><名>：SetCurrentDirectoryA 失败（不存在）时 CreateDirectoryA。存在时改变当前目录一事见偏差表 0x00425150 平台替换行。exact。

## 0x004251B0 Game::ReleaseGame

依次：三种字体（0x004351F0）、0x00406E00、0x0045EA30、0x00478110、链表 0x4A9A2C、0x0040F5E0、0x0040F500、0x0040F540、0x0040F570、0x0040DBC0、0x0040F5A0、0x0040F630、0x0040F670、链表 0x4AB2F4 与 0x4B89F4、0x004020C0、0x0040F3B0、0x4A9A04..0C、0x0040F410、0x00433010、0x00431960、0x00485710、0x00485740、0x4CAE78、调色板 0x4CD764、帧表面、0x00437A50、0x00437AD0、0x004374E0；八个文件句柄关闭失败时以 0x00411F90 记 "ReleaseGame : …"；释放 0x4B7404、0x4B794C、0x4B7948、0x4C9A10、0x4B8860、0x4ACCD0 与链表 0x4ACAC0；0x004258E0；ShowCursor(1)。重建各调用的地址注释与顺序逐项对应，八条错误日志文字相同；调色板二次释放见偏差表第 244 行。退出时的内存释放不可观察。exact。

## 0x00425470 NameProcedure

WM_CLOSE 结束 0；WM_INITDIALOG：控件 0x40A 发 EM_LIMITTEXT 8、0x46A(1, 0x42)，文字取 0x49E188（默认名字表第 0 项）；WM_COMMAND IDOK：取至多 8 字节，空时 MessageBoxA(「請輸入一個字以上」0x4A10E4，「姓名」0x4A10F8) 并返回 0，否则复制到 0x49E148（第 0 个角色名）并结束 1；0x40C 结束 0。重建（W 版 API，DWLP_USER 取对象）常量与流程一致。exact。

## 0x00425B40 Game::CheckStart

恒返回 1。exact。

## 0x00425B50 GameState::ClearWorld

释放地图图块与区块缓冲（0x4B7948、0x4B794C）、地图头 0x4B7930 起 0x64 字节清 0；角色数 0x49E0C4 = 0；256 个角色（0x4BABA8 起每个 0xD8 字节）释放其缓冲并整体清 0；队长 0x4B72A0 的 0xD8 字节填 0xFF；72 个移动记录（0x4AD490 起每个 0x21C 字节）各调用 0x0040DD40。重建 MapHeader{}、Role{}（成员初值全为 0，NoTalk = 0）、FillUnused、MoveRecord::Clear 与之一致。exact。
