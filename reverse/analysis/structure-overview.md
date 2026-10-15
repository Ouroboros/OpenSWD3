# 初步模块与代码结构概览

依据：`ida/functions_raw.tsv`（函数、尾块、调用关系）、`ida/func_refs.tsv`（每函数引用的导入 API、字符串、全局读写）、`ida/code_drefs.tsv`、`ida/text_pointer_scan.tsv`。模块初分脚本 `scripts/classify_modules.js`，统计见 `module_assignment.tsv`。以下模块名是当前工作名，原模块名有证据时优先采用（`manrole.cpp`、`org.cpp`、`Act Err(<模块>:...)` 前缀）。

## 主流程

```text
start 0x48A740 (CRT: GetVersion, heap init, ioinit, argv/envp, cinit)
 └─ _WinMain@16 0x409EC0  RegisterClassEx "SoftstarSwd3", CreateWindowEx, 消息循环
     ├─ WndProc sub_40A0D0（RegisterClassEx 结构中的函数指针 0x409F33）
     │    截图 ScrnShot\%05d.bmp、存读档、cdcheck、game_init/ReleaseGame、AIL_serve
     ├─ sub_40A570 主循环（剧情战斗切换、视图关闭重映射地图）
     └─ sub_411C90/sub_411FA0（CD 检查）、sub_484950（DirectDraw 错误框）
```

## 地址分区

| 地址范围 | 内容 | 依据 |
| --- | --- | --- |
| `0x401000`–`0x4014F0`、`0x4272C0`–`0x4272F0`、`0x4517A0`–`0x451940` | 全局对象动态初始化／atexit 注册／析构 thunk（static_init） | `call ctor; jmp reg` + `push dtor; call _atexit; pop ecx; retn` + `mov ecx, offset obj; jmp dtor` 模式，16 字节对齐分隔 |
| `0x402230`–`0x486A70` | 游戏代码 | 无 CRT 断言字符串，含游戏资源名与 `Act Err(...)` |
| `0x484550`–`0x484950` | Bink 视频封装（movie_bink） | binkw32 导入、`video\`、`.avi/.mpg` |
| `0x484DD0`–`0x486A70` | Miles 音频封装（audio_mss） | mss32 导入、`all.snd`、`SFX file`、WAV RIFF 头模板 |
| `0x486AB0`、`0x487AD8`、`0x487ADE` | DirectDrawCreate／DirectInputCreateA／ImmIsIME 导入 thunk | `jmp ds:__imp_*` |
| `0x486AC0`–`0x487AD8` | dinput.lib 静态数据 `c_dfDIKeyboard`（256×DIOBJECTDATAFORMAT + DIDATAFORMAT） | `0x4370C2` 引用 DIDATAFORMAT 头 |
| `0x487AF0`–`0x497FCC` | 静态链接 MSVC 调试版 CRT（crt） | `onexit.c`、`dbgheap.c`、`output.c`、`setenv.c` 等断言字符串；`RtlUnwind` thunk 结尾 |
| `0x497FD0`–`0x498422` | C++ EH funclet（unwind action + `SEH_<父函数>` handler） | `mov eax, offset FuncInfo; jmp __CxxFrameHandler(0x489740)` |

## 游戏代码模块（初分）

| 模块 | 代表函数／证据 |
| --- | --- |
| role_path | `0x402520` aStar、`0x405430`/`0x405500` `Act Err(Path:...)`、`0x406960` CompanyWalk |
| role_event | `0x402F80` `Act Err(KeyCole:Talk1)`、RoleEventBy.GUID |
| saveload | `0x4070A0`–`0x409DD0` `%ssave\%d.sav`、`%sFame.dat` |
| winmain | `0x409EC0` WinMain、`0x40A0D0` WndProc |
| dialog | `0x40AFF0` SetupDialog、`Act Err(Dialog:Frame)` |
| map | `0x40C130` LoadCity、`0x425570`–`0x427140` `HUGE.lmf`、`mcache.dat`、`data\%d.cm`（地图解压与缓存） |
| misc | `0x40E0B0` `Act Err(Misc:Init:Mouse...)`、`0x40F280` newRoleInit |
| game_init | `0x40F040` RoleDataBase/PathDataBase init（maps.dat、path.dat、talk*.dat）、`0x424B90` DrawKernel/mmTimer init、`0x424EF0` DInput/DDraw init、`0x425040` 目录（Save/Data/Music/ScrnShot/Video）、`0x4251B0` ReleaseGame |
| cheat_dialog | `0x40F890` DialogFunc（CheatDialog） |
| cdcheck | `0x4118B0` `swd3_dvd.dat` CD checker、`0x411D90` GetDiskFreeSpaceExA |
| pic_paint | `0x4120B0`–`0x4158C0` `Act Err(picPaint/PicPaint:...)`、FPS 调试串 |
| config_env | `0x423A10`–`0x423FB0` `%sEnv.dat` |
| launcher | `0x4242B0`–`0x424A60` 驱动器检测、内存状态、Readme/官网、注册表 shell open |
| talk | `0x427300`–`0x42E480` `%sTalk%d.dat`、`Act Err(Talk:...)` |
| dlg_message | `0x42ED40` `Act Err(DlgMessage:...)` |
| res_pack | `0x4315D0`–`0x4338F0` `all_*.tsw`／`all_*.act` 读取 |
| ddraw | `0x4311C0`、`0x437610`–`0x437C80`、`0x484950`（DDERR_* 文本、DirectDraw 初始化） |
| particle | `0x434DD0` particle_put |
| font_gdi | `0x4351F0`–`0x435500`（CreateFontIndirect、TextOut） |
| input_dinput | `0x436FB0` cDINPUT |
| file | `0x438030`–`0x438620` cFile（CreateFile/CreateFileMapping/MapViewOfFile） |
| memmgr | `0x438650`–`0x4389A0` MemMgr（HeapCreate、块越界检查） |
| text_dbcs | `0x438A80`–`0x438F60`（CharNext/CharPrev/ImmIsIME） |
| menu | `0x43AAA0`–`0x44FD30` `Act Err(Menu/Botl:...)`、`v1.02DVD` |
| battle | `0x451940`–`0x45F2A0` `battle.ffd`、`music\Battle_*.mp3`、`c:\snap\%d.bmp` |
| music | `0x40EB60` `Music\`、mp3 |
| figtalk | `0x46E0B0` `figtalk.dat` |
| manrole | `0x46E730`–`0x4710D0` `manrole.cpp` |
| org | `0x476A80`–`0x477F10` `mon.dat`、`level.dat`、`org.cpp`、fame 压缩 |

`0x45F2A0`–`0x46E0B0`、`0x471100`–`0x476A80`、`0x477F10`–`0x484550` 等区段证据不足，函数大多记为 unknown，待 GOAL 中按调用关系与共享数据细分。

## 共享数据与外部依赖

- 全局对象：static_init thunk 指向的对象地址（如 `0x4B6C78`、`0x4A9AC8`、`0x4C8450` 等）是各模块的全局单例，构造／析构函数分布在 `0x435130`、`0x4386E0`、`0x484ED0`、`0x4853C0`、`0x485440`、`0x485960`、`0x486610` 等。
- 数据文件：`maps.dat`、`path.dat`、`talk1-4.dat`、`figtalk.dat`、`mon.dat`、`level.dat`、`Env.dat`、`Fame.dat`、`swd3_dvd.dat`、`all_*.tsw/act`、`all.snd`、`HUGE.lmf`、`battle.ffd`、`data\%d.cm`、`data\mcache.dat`、`save\%d.sav`。
- 平台替换方向（见 AGENTS.md）：DirectDraw／DirectInput／GDI／WinMM／MSS → SDL3；Bink、MP3 → FFmpeg n9.0；游戏逻辑、资源解析、绘制规则、播放控制与时序按原汇编还原。
