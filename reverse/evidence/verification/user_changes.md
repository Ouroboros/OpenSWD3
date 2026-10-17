# 用户要求的改动后重新验证

这些函数此前已对照汇编验证过，因用户要求的平台替换（deviations.md 中“平台替换（用户要求）”各行）而退回待验证。本轮对照汇编确认：除已登记的偏差外，其余逻辑与原版一致；登记的偏差保持重建行为，不按原版改回。

## 0x00409EC0 Game::Run（WinMain）

- 原版：FindWindowExA 已有实例时返回；sub_411C90 处理安装命令行；ErrorLog 分隔线；建窗口（WM_CREATE 中的启动检查）；SendMessageA(0x404)；`sub_489B10(time(0))`（srand）；`sub_438FA0(time(0))`（游戏随机数）；dword_4CC2AC = 1；消息循环无消息时：4CC2AC 非 0 时，4B7A9C & 0x20 则 sub_484950 + sub_40CF10，4B7A9C & 1 或 4BABA4 则 Sleep(0)，否则 sub_40A570；4CC2AC 为 0 时 4B7CAC == 1 则 sub_411FA0，否则 Sleep(0)；GetMessageA 为 0 时返回 Msg.wParam（WM_DESTROY 调 PostQuitMessage(0)，即 0）。
- 重建：日志分隔线、Start（0x404）、crtRandom.Seed(time)、random.Seed(time)、running = 1 的顺序相同；LoopFrame 的分支与条件相同（running = 4CC2AC，systemFlags = 4B7A9C，inactive = 4BABA4，active = 4B7CAC）；返回 0。
- 已登记的偏差：不检查已运行实例；安装命令行去掉（用户要求，0x00411C90 删除）；停用时只重显上一画面；SDL 事件循环与 SDL_Delay(1)。

## 0x0040A0D0 Game::HandleEvent（窗口过程）

- 与上次 exact 验证（31b0a4e）相比，只改了 WM_SIZE／WM_ACTIVATEAPP 的处理。
- 原版 WM_SIZE（游戏旗标 1 时）：wParam 为 1、3、4 时 sub_40AB50(0)，为 0、2 时 sub_40AB50(1)；WM_ACTIVATEAPP：失去激活时 sub_40AB50(0)，激活时 sub_40AB50(1)。
- 重建：最小化（且仍在运行）时 Activate(0)；还原或最大化（且已停用）时 Activate(1)；不处理失去焦点。原版窗口固定 640×480 不会最大化；重建的窗口可最大化，最大化视为恢复。按 deviations.md 0x0040A0D0“平台替换（用户要求）”一行保留。

## 0x0040A570 Game::MainFrame

- 与上次 exact 验证（31b0a4e）相比，只删除了开头 `systemFlags & CheckingCd` 时返回的判断（光盘检查已去掉，deviations.md“不找光驱、不检查光盘”一行，用户要求）。其余不变。

## 0x0040C130 Game::LoadCity

- 与上次 exact 验证（7cbbcc8）相比，只删除了最小安装时检查光盘、改写光驱盘符的一段（同上，用户要求；地图一律从安装目录读取）。其余不变。
