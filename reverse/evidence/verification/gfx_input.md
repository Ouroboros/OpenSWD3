# Input 与 Graphics 核对记录

## Input 0x00436F70..0x004374E0（第 1 遍，2026-10-18）

DirectInput 由 SDL 输入替换（平台层）；以下核对由原版计算得到的状态。

- 0x00436F70 构造（三个接口空，灵敏度 10）、0x00436FA0 析构（转 Release）、0x004374E0 Release：通过。
- 0x00436FB0 Init：键盘 c_dfDIKeyboard；鼠标自定义格式（dwFlags 1 绝对轴，dwDataSize 0x1C）；最后 GetDeviceState(mouse, 0x10, this) 的大小与格式不符而失败，原点取键盘缓冲前 8 字节（this[0]、this[1]）。重建同样取键盘缓冲。通过。
- 0x004372B0 PollKeyboard（0x100 字节）、0x004372D0 IsKeyDown（& 0x80）、0x004372E0 FirstKeyDown（最低的按下码，无则 0）、0x00437300 PressKey：通过。
- 0x00437310 ReadMouse：屏幕坐标 = 灵敏度 × (轴 − 原点) / 10（有符号）；左右按键为位 0、1；越过 0 或 639/479 时夹取并移动原点（6390 / 灵敏度、4790 / 灵敏度）。通过。
- 0x00437430 SetMousePosition（先夹取，原点 = 轴 − 10 × (坐标 / 灵敏度)）、0x004374C0 SetSensitivity（_ftol(值 × 10)）：通过。

## Graphics 0x00437560..0x00437E90（第 1 遍）

DirectDraw 由模拟表面替换（deviations.md 0x00437570 等一行；Lock/Unlock 见 0x00416F10 等一行）。

- 0x00437560 构造、0x00437570 Init（记宽高；0x4E21 为窗口：DDSCL_NORMAL 8，否则 0x13；任一步失败返回 0；成功后检测像素格式）、0x00437610 CreateDevice、0x00437680 SetCooperativeLevel（记旗标）、0x004376D0 SetDisplayMode、0x00437720 CreateClipper：通过。
- 0x004377D0 CreateSurfaces：先查显存；窗口模式建主表面、裁剪器与桌面大小的后台缓冲（重建为逻辑画面大小），记两者跨度与高度；全屏只建主表面，后台记录取主表面的值。通过。
- 0x00437A50 ReleaseSurfaces（设备为空时返回 0）、0x00437AD0 ReleaseDevice、0x00437AE0 Present（全屏等垂直同步后翻页，窗口 Blt 后台到主表面）：通过。
- 0x00437B60 CreateOffscreen（已用显存加 w × h × 位深 / 8）、0x00437C00 FillSurfaceColor、0x00437C40 ClearSurface（填清屏色）、0x00437C80 MatchSurfaceColor（GDI 写 (0, 0) 读回后恢复，位深 < 32 时取低位；CLR_INVALID 取当前左上像素）、0x00437D80 BitDepth、0x00437DC0 QueryVideoMemory、0x00437DF0 GetSurface（0x2711..0x2713）、0x00437E20 Device、0x00437E30 DetectPixelFormat（绿 0x7E0 → 2，蓝 0x3F → 1，红 0xFC00 → 3，依次覆盖，返回绿掩码）、0x00437E90 BackSize（后台跨度字节与高度）：通过。

## 0x00464DA0 Controls::PressConfirm

0x4B7CC0 = 1（Space 按下）、0x4B7CCC = 2（Space 持续帧）、0x4B7DAC = 2（左键持续帧）、0x4B7D7C = 1（Enter 持续帧），返回 2：一致，通过。
