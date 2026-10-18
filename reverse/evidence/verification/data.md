# 常量表、全局数据与资源核对记录（第 1 遍）

## 方法

1. 引用收集（tmp/data_audit.py）：游戏代码 0x00402230–0x00486A70 的汇编中引用到 .idata／.rdata／.data 初始化部分（0x00499000–0x004AA000）的地址共 1250 个。源码、测试或 reverse/evidence/ 中按地址出现的 611 个视为已覆盖（这些地址的使用者均已逐函数对照为 assembly_exact，表的内容由下列测试逐字节比对或在注释中给出原地址）；其余 639 个逐项核对：导入表 146、.rdata 27、.data 466。
2. 分类（tmp/data_classify.py、tmp/string_audit.py、tmp/data_rest.py）：按 IDA 名称与字节内容分为字符串、导入、数值与表；字符串按 Big5 解码（相邻字面量拼接）后在源码中查找。
3. 逐项读取 exe 字节（`dd`／`od -tf4`／`od -tf8`／`od -tx4`），对照引用函数的汇编与重建常量。

逐字节比对 exe 的测试（swd3test::SameTable 或逐项比较）：game_state_test（BitMasks 0x004994EC）、talk_test（窗口位移 0x004994F4、0x00499514）、config_test（声道掩码 0x0049E0B4）、game_init_test（文字颜色 0x0049E0C8、0x0049E108）、battle_items_test（装备栏种类 0x0049E344）、menu_test（初始页签 0x004A6A38、按键名 0x0049E470、按键行 0x004A7230、行槽位 0x004A7210、菜单文字）、spline_test（基函数 0x004A6474）、random_test（R250 表 0x004A6610、LCG 0x004A69F8）、launcher_test（资源）、battle_message_test、input_test、painter 系列。

## 字符串（322 个，未匹配的 51 + 18 个逐项结论）

| 地址 | 内容 | 使用者 | 结论 |
|---|---|---|---|
| Fame.dat、mcache.dat 等 | 文件名 | 0x00477xxx、0x00424xxx | 由拼接或 PathFormat 生成，结果相同 |
| Born.avi | 影片名 | 0x00424330 的多余参数 | SizeCaches 不读取该参数，等价 |
| "overwrited!" 消息 | 调试文字 | 逐段拼接 | 拼接结果相同 |
| CD、注册表、DirectDraw、DirectInput、cDINPUT、particle_put 文字 | 错误与诊断 | 平台层 | 平台替换或不可达，见 deviations.md |
| 0x004A6B74 "  0%" | 图鉴进度 | 0x0043CC20 | 先复制后被 "%3d%%" 覆盖再绘制，等价 |
| 0x004A6C00..0x004A6C3C "敏捷  ????" 等 6 个 | 图鉴属性 | 0x0043D050（0x0043D082..0x0043D0C8） | 依次 lstrcpyA 到 CmdLine 并相互覆盖，之后 CmdLine 使用前都重写，无可见效果；重建省略 |
| 0x004A6D10 "鎖子甲" | 物件名 | 0x0043F7C0（0x0043F7CE lstrcmpA） | 结果在 0x0043F7DB `xor eax, eax` 前被丢弃，重建省略 |
| 0x0049E0D8、0x0049E118 | 文字颜色表第 5 项 | Talk::Draw 0x0042ED40（0x0042FFFA、0x0043000F） | 属于 TextColors::Primary／Secondary（16 项），game_init_test 比对 |
| 0x0049FB20 "SoftstarSwd3" | 窗口类名 | 0x00409EC0 | 单实例检查为平台替换（deviations.md 0x00409EC0） |
| 0x004A02B4、0x004A02C8、0x004A02DC | 放入光盘提示 | 0x004118B0 | 光盘检查按用户要求移除（deviations.md） |
| 0x004A0340 "GetDiskFreeSpaceExA"、0x004A0354 "KERNEL32" | 动态取函数 | 0x00411D90 | std::filesystem::space；原版结果 `(低32位>>20)+(高32位<<12)`（0x00411DE1..0x00411DEF）与 `available>>20` 截断为 u32 相同，失败均为 0xFFFFFFFF |
| 0x004A0E6C、0x004A0E70 | 像素转换函数指针，初值 nullsub_1 | 0x00423400 | 仅 RGB555 分支（0x004234EE）重设为 nullsub_1，未知格式保留旧值；重建 Conversion::None 初值、同样保留 |
| 0x004A6A48 | 初始页签第 2 项 | 0x0043A380 | 属于 Menu::InitialTabs，menu_test 比对 |
| 0x004A027C "2"、0x004A0280 "Cmpy"、0x004A0284 "1" | 变量名 | 0x00410730 | kCompanyA "Cmpy1"、kCompanyB "Cmpy2" |
| 0x004A0270 "%d%%" | 作弊对话框数值 | 0x004106BE、0x00410576 | 写入后跳到 0x004106EE／0x004105B7 被 "%d" 覆盖，CheatDialog::ValueText 同为 "%d"（cheat.md） |

## 数值与表（98 个）

| 地址 | exe 内容 | 使用者 | 重建 | 结论 |
|---|---|---|---|---|
| 0x00499378 | double 1.0 | PathFinder 0x00402690、0x004029C0 `fadd` | `parent.g + 1.0` | 一致 |
| 0x0049949C、0x004994A4..0x004994AB、0x0049969C | 位掩码 1,2,4..0x80 的副本 | 0x00405500、0x00409600、0x00427920 | GameState::BitMasks（0x004994EC 测试） | 一致 |
| 0x004994F8、0x00499518 | 窗口位移表内项 | 0x0040AFF0 | Talk::WindowShiftX／Y（talk_test） | 一致 |
| 0x00499614 | float 0.0625 | FieldView::DrawDebug 0x0041415B | `/ MapHeader::TileSize`（16，2 的幂，结果相同） | 一致 |
| 0x00499618 | float 0.0 | 0x004146F0、0x0045D2F0、0x00482DA0 `fcomp` | 与 0 比较 | 一致 |
| 0x004996A4、0x004996A8 | float −1000、double 0.017444444444444446 | Projector 0x00433C40 | kSlopeUnit 1000、kDegree | 一致 |
| 0x004996B0 | double 10 | Input::SetSensitivity 0x004374C0 | Input::Tenths 10 | 一致 |
| 0x004996B8 | double 2 | 样条 0x00437F90 | kSegmentBase 2.0 | 一致 |
| 0x004996C0 | float 1 | 名声 0x00477830 `fdivr` | `1.0 / maximum` | 一致 |
| 0x00499CF0 | float 7 | BattleHud 0x00450C50 | kVisibleRows 7 | 一致 |
| 0x00499D2C | float 12 | BattleUnit 0x0046F812 | kOrbStep 0xC | 一致 |
| 0x00499D30 | float 62 | GaugeLength 0x00478358 | kGaugePixels 62.0 | 一致 |
| 0x00499D88、0x00499DC8 | IID_IDirectDraw4、IID_IDirectDrawSurface4 | 0x00437610、0x004377D0 | DirectDraw 仿真 | 平台替换 |
| 0x0049A718、0x0049A728、0x0049A788、0x0049A7C8、0x0049A7D8 | GUID_XAxis、GUID_YAxis、GUID_Button、GUID_SysMouse、GUID_SysKeyboard | Input::Init 0x00436FB0 | DirectInput 仿真 | 平台替换 |
| 0x0049E0B6、0x0049E0B8 | 声道掩码 0x3E0、0x1F | 0x00413FE0 等 | Config::DefaultChannelMasks（config_test） | 一致 |
| 0x0049E334、0x0049E340 | 道具类别种类 0x08000040、0x20 | ItemsTake 0x00448020 | kCategoryKinds {0x8000,0x1003,0x1C,0x08000040,0x700,0x800,0x20} | 一致 |
| 0x0049E360、0x0049E368 | 装备栏种类第 8、10 项 0x40、0x800 | 作弊对话框 0x0040F890、0x00410730 | Party::SlotKinds（battle_items_test） | 一致 |
| 0x0049E36C、0x0049E384 | 循环边界（第 11 项、第 16 项之后） | CompareEquipment 0x0044FBB5、0x0044FBFA | 前 11 栏换装、16 栏比较 | 一致 |
| 0x0049FBE8 | 动作→值的字节对表（1→0x1E … 0x1F→0x32） | LoadCity 0x0040CBE5 | 结果只写入 var_C（0x0040CC09），之后无读取 | 无效计算，省略 |
| 0x004A0698..0x004A06A4 | 缩放 0x28、0x48、0x600、0x400 | 0x00422C70 等 | Painter zoomAnchorX/Y、zoomX/Y | 一致 |
| 0x004A0E64、0x004A0E68 | 0x294、0x210 | BlitCopy 0x00418350 等 | 使用前先写入（0x0041836A、0x00418384） | 初值无影响 |
| 0x004A135E | 0x80008000 的高半 | 0x00427920 | Talk::NoOrigin 0x80008000 | 一致 |
| 0x004A6020、0x004A6098 | 0x600000、0x100000 | tsw／act 档案 | TswArchive／ActArchive cacheLimit | 一致 |
| 0x004A661C、0x004A68DC | R250 表内项 | 0x00438FA0 | random_test 比对 0x004A6610 起 250 项 | 一致 |
| 0x004A6A1A | 循环边界（13 个 u16） | ItemsCountShown 0x00447FE4 | kItemRows 13 | 一致 |
| 0x004A6D0A、0x004A6D0E | "西"、"東" 的结尾 NUL | AltarDraw 0x0043E800 | kWest、kEast | 一致 |
| 0x004A6F20..0x004A6F60 | 装备栏名（含结尾 NUL） | 0x004425C0，栈上 var_58 起每 8 字节：武器、頭部、身體、手部、足部、飾品、空格、法寶、空格、護駕、空格 | kSlotNames 同序 | 一致 |
| 0x004A6F84..0x004A6FA4 | 法术页签名 | 0x004442B0，栈上依次 0x004A6FA0、0x004A6F98、0x004A6F90… | kTabNames {魔法,仙術,絕技,武術,醫術} | 一致 |
| 0x004A7024..0x004A7034 | 使用、整理、丟棄 | ItemsDraw 0x00447100 | kModeLabels | 一致 |
| 0x004A7218 | 按键行槽位内项 | 0x0044C160 | Menu::KeyRowSlots（menu_test） | 一致 |
| 0x004A739C..0x004A73BC | 设定页签名 | 0x0044C160 | menu_options.cpp 注释所列 5 项 | 一致 |
| 0x004A74D6 | 循环边界（3 个 u16 之后） | DrawTimingBar 0x004511FA | BattleHud::TimingMarks {20,40,55} | 一致 |
| 0x004A9120..0x004A9AC8 | 全 0 | NewGame 0x0040E0B0、ShowLoading 0x0040ED60、DrawTimer 0x004308C0 等 | Scene 的 ActState 数组成员、Party::skills（globals.tsv），成员默认值全为 0 | 一致 |

## 导入表（146 个，tmp/iat_audit.py）

游戏代码直接 `call ds:` 的导入共 147 个（data_audit 未计 `_AIL_stream_user_data`），每个调用点所在的函数都已逐函数对照为 assembly_exact；映射如下。

| 导入 | 重建 |
|---|---|
| RegOpenKeyExA、RegQueryValueExA、RegCloseKey、WinExec | 启动器从 HKEY_CLASSES_ROOT 取浏览器或文本程序并 WinExec：平台替换（deviations.md 0x00424930、0x00424A60） |
| CreateFontIndirectA、SelectObject、SetTextColor、SetBkColor、SetBkMode、TextOutA、DeleteObject、GetStockObject、GetPixel、SetPixel | TextFont 的 GDI 文字：自带细明体点阵字库 GlyphLibrary（GlyphRasterizer） |
| CreateFileA、ReadFile、WriteFile、SetFilePointer、GetFileSize、GetFileTime、SetEndOfFile、SetFileAttributesA、DeleteFileA、CopyFileA、CreateFileMappingA、MapViewOfFile、UnmapViewOfFile、CloseHandle、CreateDirectoryA、GetCurrentDirectoryA、SetCurrentDirectoryA、GetDriveTypeA | cFile 与 file_system（UTF-8 路径、W API、共享方式打开，用户要求） |
| HeapCreate、HeapDestroy、GetSystemInfo、GlobalMemoryStatus、LoadLibraryA、GetProcAddress、FreeLibrary、GetLastError、FormatMessageA | MemoryManager、system_info、日志 |
| lstrcpyA、lstrcatA、lstrlenA、lstrcmpA、wsprintfA、CharNextA、CharPrevA | std::string 与 WinFormat（wsprintf 语义）、text/dbcs.h |
| GetLocalTime、Sleep、timeGetTime | system_info（Windows 上启动时 timeBeginPeriod(1)，deviations.md 0x0040A570） |
| RegisterClassExA、CreateWindowExA、ShowWindow、UpdateWindow、SetWindowPos、GetWindowRect、SetRect、IntersectRect、FindWindowExA、DefWindowProcA、PeekMessageA、GetMessageA、TranslateMessage、DispatchMessageA、PostQuitMessage、SendMessageA、ShowCursor、SetCursorPos、SetFocus、GetSystemMetrics、LoadIconA、LoadCursorA、CoInitialize、CoUninitialize | Display（SDL3 窗口与事件，逻辑 640×480）；图标 0x65 与光标 0x67 见下节 |
| DialogBoxParamA、EndDialog、GetDlgItem、GetDlgItemTextA、SetDlgItemTextA、EnableWindow | Windows 上载入 swd32.exe 的同一对话框模板（deviations.md 0x0040F890、0x00425470），启动器 0x6B 绘制在游戏窗口中 |
| MessageBoxA、MessageBeep、PlaySoundA、LoadStringA | platform/message_box、system_sound；LoadStringA 读的是 MSS32.DLL 的字符串，结果未使用 |
| CreateEventA | DirectInput 事件：仿真 |
| _Bink* 14 个 | Movie 与 MediaDecoder（FFmpeg） |
| _AIL_* 49 个 | Miles 封装仿真（SDL 音频，include/swd3/mss.h） |

## 资源（.rsrc，scripts/dump_resources.py）

| 资源 | 原版使用者 | 重建 | 结论 |
|---|---|---|---|
| DIALOG 0x6B (107) | Game::HandleEvent 0x0040A4AD | Launcher（Launcher::Dialog），launcher_test 比对模板大小 274 | 一致 |
| DIALOG 0x6A (106) | ControlPlayer 0x00403386（F4 作弊） | Windows 上从 exe 载入同一模板 | 一致 |
| DIALOG 0x70 (112) | Launcher::OnClose 0x00424605 | 同上（姓名对话框） | 一致 |
| BITMAP 113..118 | 启动器背景与 5 张高亮图 | DecodeBitmap，launcher_test 比对尺寸 | 一致 |
| WAVE 0x77 (119) | 启动器悬停 0x004248DF | Launcher::HoverSound，launcher_test 比对大小 | 一致 |
| WAVE 0x78 (120) | 开始音效 0x0040A46B | Game::StartSound | 一致 |
| WAVE 121 | 无使用者 | — | 无需处理 |
| GROUP_ICON 0x65 (101)：ICON 1..4（32×32，1/4/8/24 位） | 窗口类 hIcon、hIconSm（0x00409F47、0x00409F50） | 本轮补上：ExeResources::FindIcon 取位深最高的 ICON 4，Display::SetIcon 设为窗口图标（launcher_test 检查掩码与像素） | 已修正 |
| GROUP_CURSOR 0x67 (103)：CURSOR 5（32×32 单色，热点 0,0，AND 掩码全 1、XOR 全 0，即全透明） | 窗口类 hCursor（0x00409F59）：游戏窗口上系统光标即使显示（F4 时 ShowCursor）也看不见；启动器 0x6B 与对话框用对话框类的箭头 | 本轮补上：启动器结束后 Display::SetCursor 设为该空白光标，启动器期间仍为箭头（launcher_test 检查全透明） | 已修正 |

## 游戏数据文件

数据文件的格式由读取它们的函数决定，已随这些函数逐函数对照；另有测试用原版文件逐字节验证：lzo_test（原版存档的压缩与解压）、fame_test（名声与计数表）、tsw_test／act_test（tsw、act 档案与动作脚本）、field_test（maps.dat 与地图）、file_test、font_test、game_state_test（存档状态）。
