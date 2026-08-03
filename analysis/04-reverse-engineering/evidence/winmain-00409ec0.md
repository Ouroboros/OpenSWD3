# WinMain 汇编证据：0x00409EC0

状态：已从原 EXE 字节逐基本块复核

来源：`swd3.exe`

SHA-256：`0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c`

范围：`0x00409EC0` 至 `0x0040A0C9`

提取命令：

```text
objdump -d -M intel --start-address=0x409ec0 --stop-address=0x40a0d0 swd3.exe
```

IDA 伪码没有用于确定本文分支。IDA 汇编只用于导入名、字符串名和交叉引用辅助；所有跳转、立即数、调用目标和全局地址均已回查原 EXE 字节。

## 1. 早期退出

### 0x00409EC6 · 旧窗口探测

实际调用：

```asm
push 0049FB30h       ; Big5 "軒轅劍參"
push 0049FB20h       ; "SoftstarSwd3"
push 0
push 0
call [00499218h]     ; FindWindowExA
test eax, eax
je   00409EE9h
```

找到匹配窗口时，函数直接返回零。

这里搜索的标题是“軒轅劍參”，而稍后创建窗口使用的标题是“軒轅劍參 DVD”。这是原字节中的实际差异，当前不擅自解释。

### 0x00409EE9 · 非空命令行路径

`lpCmdLine` 被传给 `0x00411C90`。

该函数的汇编行为是：

- 空指针或空串时返回零，继续正常启动。
- 非空时复制从第二个字节开始的字符串。
- 将第一个字节减去 ASCII `'0'`，保存到 `0x004C97F4`。
- 调用 `0x004242B0` 和 `0x00423A10` 后返回一。

`WinMain` 看到返回一便直接返回零。因此，非空命令行会进入一条独立处理路径，不会创建主窗口。该路径的业务用途尚未命名。

## 2. 窗口类

`0x00409F13` 调用 `CoInitialize(NULL)`，返回值未检查。

随后在栈上填写 48 字节 `WNDCLASSEXA`：

- `cbSize = 0x30`
- `style = 0`
- `lpfnWndProc = 0x0040A0D0`
- `cbClsExtra = 0`
- `cbWndExtra = 0`
- `hInstance = WinMain.hInstance`
- `hIcon = LoadIconA(hInstance, 0x65)`
- `hCursor = LoadCursorA(hInstance, 0x67)`
- `hbrBackground = GetStockObject(4)`
- `lpszMenuName = NULL`
- `lpszClassName = "SoftstarSwd3"`
- `hIconSm = LoadIconA(hInstance, 0x65)`

`RegisterClassExA` 返回的低 16 位为零时，`WinMain` 返回零。

## 3. 窗口创建

`CreateWindowExA` 的实际参数：

- `dwExStyle = 0x00040000`
- `lpClassName = "SoftstarSwd3"`
- `lpWindowName = Big5 "軒轅劍參 DVD"`
- `dwStyle = 0x86000000`
- `X = -1800`
- `Y = 0`
- `nWidth = 640`
- `nHeight = 480`
- `hWndParent = NULL`
- `hMenu = NULL`
- `hInstance = WinMain.hInstance`
- `lpParam = NULL`

`hInstance` 写入 `0x004C8BD0`，返回的窗口句柄写入 `0x004C9A1C`。创建失败时返回零。

创建成功后依次执行：

```text
ShowWindow(hWnd, nShowCmd)
UpdateWindow(hWnd)
SendMessageA(hWnd, 0x404, 0, 0)
```

`0x404` 是同步自定义消息。它返回后，程序才初始化两个随机序列并进入消息循环。其窗口过程分支属于 P1.3。

## 4. 随机序列初始化

第一条路径：

```text
time(NULL) → 0x00489B10
```

`0x00489B10` 只把种子写入 `0x004A833C`；紧随其后的 `0x00489B20` 是静态运行库 `_rand`。因此这条调用是该随机序列的种子设置入口。

第二条路径：

```text
time(NULL) → 0x00438FA0
```

`0x00438FA0` 初始化 `0x004A6610` 至 `0x004A69F7` 的 250 个 32 位状态字，并设置后续 xor 生成器状态。两次 `time(NULL)` 是两个独立调用，不能在重写时自动合并成一个种子读取。

随后 `0x004CC2AC` 被写为一。

## 5. 消息泵

循环头为 `0x0040A042`。

### 有待处理消息

先调用：

```text
PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE)
```

返回非零时，再调用：

```text
GetMessageA(&msg, NULL, 0, 0)
```

- 返回零时，从 `msg.wParam` 取得进程返回值并退出 `WinMain`。
- 返回非零时，调用 `TranslateMessage` 和 `DispatchMessageA`，然后回到循环头。

汇编只测试是否为零，没有单独处理 `GetMessageA == -1`。因此负一也会落入翻译和分发路径。这是原程序的实际边界行为。

### 没有待处理消息

分支完全由四个全局位置控制。

当 `[0x004CC2AC] != 0`：

- 若 `[0x004B7A9C] & 0x20 != 0`：调用 `0x00484950`，再调用 `0x0040CF10`。
- 否则若 `[0x004B7A9C] & 0x01 != 0`：调用 `Sleep(0)`。
- 否则若 `[0x004BABA4] != 0`：调用 `Sleep(0)`。
- 否则：调用单帧主循环 `0x0040A570`。

当 `[0x004CC2AC] == 0`：

- 若 `[0x004B7CAC] == 1`：调用 `0x00411FA0`。
- 否则：调用 `Sleep(0)`。

`0x00411FA0` 复制并绘制 Big5 文本“遊戲暫停  按F8繼續遊戲”，随后调用图形对象方法。因此这一分支可确认是暂停提示呈现路径。

P1.7 下游复核已经确认：`0x00484950` 是 Bink 视频解码、拷贝与 DirectDraw 呈现路径；`0x0040CF10` 随后维护 Miles 的音乐/流、stream、sequence 和 sample 对象。后者不调用 DirectDraw，不是最终呈现函数。

完整证据见 `presentation-lifecycle.md`。

## 6. 退出值与调用约定

仅 `GetMessageA` 返回零的正常消息循环出口会返回 `msg.wParam`。

重复窗口、特殊命令行、窗口类注册失败和窗口创建失败均返回零。

所有出口最终执行 `ret 10h`，确认 `__stdcall WinMain` 的四个 32 位参数由被调方清理。

## 7. OpenSWD3 平台接线

平台无关的两个早退顺序由 `run_process_startup_gates` 保留。现代窗口不再具有原 Win32 类名和 Big5 标题，所以跨平台端口用进程生命周期锁替代 `FindWindowExA`：Windows 使用 `Local\\OpenSWD3` 命名 mutex，POSIX 使用临时目录中的 advisory file lock。它只改变旧窗口探测机制，不改变“已有实例先于命令行处理并返回零”的调用者结果，登记为 `platform_adapted`。

Windows 后端直接从 `GetCommandLineA` 去掉首个可执行文件 token，保留后续原始字节尾；非 Windows 的 `argv` 已被宿主解析，无法恢复被引号和转义区分的原始字节，只能用单个空格连接 `argv[1..]`。后者是明确的平台信息损失，不作为 `assembly_exact`。

SDL 入口已在 `SDL_Init` 之前接入上述两门。非空命令行当前仍只保留早退、selector 和 payload 合同，`0x004242B0/0x00423A10` 的具体业务端口尚未实现；不能把 smoke 空端口解释为特殊命令已经完成。

启动对话框返回 `6` 时，SDL 路径在两次独立播种后结束进程；返回 `1/2` 时进入已初始化的 smoke 循环。返回 `3` 或其他非 `1/2/6` 值时不再因“未初始化”而提前退出，而是按汇编继续播种并进入消息循环，同时保持运行时初始化值和显示后端可用值为零。这样宿主关闭仍只执行 COM/退出边界，不错误触发总销毁；显示切换也会在后端查询处无副作用返回。
