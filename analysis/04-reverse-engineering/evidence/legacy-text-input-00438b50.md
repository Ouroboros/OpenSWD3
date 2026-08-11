# `0x00438B50..0x00438F9F` DBCS/IME 文本编辑驱动

状态：B3.8 闭环；`assembly_exact`、`platform_adapted`、`blocked_runtime_oracle`

来源：`swd3.exe.lst` 的完整指令和调用点。IDA 伪码、FLIRT 名称及 Windows API 名称只用于定位，不作为行为依据。

## 1. 范围与状态

本单元覆盖七个完整函数：

- `0x00438B50..0x00438D66`：消息过滤与字符分发。
- `0x00438DC0..0x00438DC8`：写输入启用状态。
- `0x00438DD0..0x00438DD3`：读输入启用状态。
- `0x00438DE0..0x00438E02`：按 DBCS 边界向前移动光标。
- `0x00438E10..0x00438E40`：按 DBCS 边界向后移动光标。
- `0x00438E50..0x00438F5D`：在光标处插入字节串。
- `0x00438F60..0x00438F9F`：删除光标处的一个 DBCS 字符。

驱动借用 B2 已恢复的 `LegacyDbcsTextBuffer`。它只修改 `+0x10` 光标、`+0x14` 结果、`+0x18` IME 状态、`+0x1C` 输入启用状态和 `+0x00` 指向的字节缓冲，不取得对象生命周期。

`0x004FB7E8` 是独立的进程级 dword。它不是对象字段，也不随文本对象构造或销毁清零。`0x00438B50` 在 ANSI `WM_CHAR` 路径把它作为 DBCS 前导字节锁存使用；当前实现映射为 `LegacyTextInputDriverState::dbcs_lead_byte_latch`。

## 2. 窗口消息合同

窗口过程 `0x0040A0D0` 仅在内部位 `0x50` 非零时调用本驱动，并原样传入 `HWND`、消息、`wParam`、`lParam`。驱动不读取 `HWND`。

驱动只特殊处理四个消息值：

- `0x0051`，`WM_INPUTLANGCHANGE`：先把对象 `+0x18` 写零，再以 `lParam` 调用 `ImmIsIME`；真值写一。返回一。
- `0x0100`，`WM_KEYDOWN`：执行旧编辑键分发。返回一。
- `0x0102`，`WM_CHAR`：仅在 `+0x18 == 0` 时执行逐字节 ANSI 路径。返回一。
- `0x0286`，`WM_IME_CHAR`：把 `wParam` 的高字节、低字节按该顺序组成 NUL 结尾串并一次插入。返回零。

其他消息返回一。窗口过程只在驱动返回零时直接消费消息并返回一，因此 `WM_IME_CHAR` 是本函数唯一要求上层拦截的正常路径。

## 3. 编辑键与保留缺陷

`WM_KEYDOWN` 使用完整 `wParam` 值，不截成低字节。精确分支为：

- `8` Backspace：先调用前移 helper，再无条件进入 Delete；光标本来位于零时会删除第一个字符。
- `13` Enter：把结果字段写一。
- `27` Escape：把结果字段写二。
- `35` End：重复调用后移 helper，直到它返回零。
- `36` Home：重复调用前移 helper，直到它返回零。
- `37` Left、`39` Right：各调用一次对应 helper。
- `45` Insert：读取 `+0x1C`、计算逻辑反值并传给 setter；setter 无视实参，恒把字段写一，因此 Insert 永远不能关闭输入。
- `46` Delete：删除光标处一个字符。

前移 helper 调用 `CharPrevA(buffer, buffer + cursor)`，写回返回指针与缓冲起点的差，最后返回“新偏移是否非零”，不是“本次是否移动”。所以从第一个字符后方移到零时，光标确实改变但返回零。

后移 helper 调用 `CharNextA(buffer + cursor)`。只有返回指针不同于输入指针时才写回新字节偏移并返回一；位于 NUL 时保持原值并返回零。

## 4. 插入与删除

插入函数先以 `0x400` 为上限取得输入的完整 DBCS 字节长度，并分配“该长度 + 对象容量 + 1”字节临时区。随后严格按以下顺序执行：

1. 复制旧缓冲的 `[0, old_cursor)` 前缀。
2. 以 `capacity - old_cursor` 为上限复制完整输入字符，并得到 `new_cursor`。
3. 以 `capacity - new_cursor` 为上限复制旧光标后的完整字符后缀。
4. 把对象光标写成 `new_cursor`。
5. 清零原缓冲 `capacity + 1` 字节。
6. 从临时区复制 `new_cursor + suffix_length` 字节回原缓冲。

空间不足时旧后缀会在 DBCS 边界被截掉，不是拒绝整个插入。实现保持相同顺序和截断结果。

若 `+0x1C != 1`，原函数从入口 `push ecx` 留下的局部槽取出对象自身地址并传给 `operator delete`，然后返回一；对象由此悬空。这条路径不能由构造器、Insert 键或 setter 建立，当前调用图不可达。现代实现对外不提供关闭入口，并把被破坏状态隔离为确定性进程终止，不把悬空对象继续交回调用者。

删除函数取得 `next = CharNextA(buffer + cursor)`，调用一次有界长度函数但不使用其返回值，然后移动 `capacity - next_offset + 1` 字节到当前光标。移动区间发生重叠。

调用目标 `0x00489EB0` 虽被旧符号识别成 `memcpy`，其完整指令先比较源、目标区间；重叠且目标在源内部时切换为反向复制，其余使用正向复制。它实际实现 `memmove` 合同。当前代码使用 `std::memmove`，不是依赖 C++ 重叠 `memcpy` 的未定义行为。

## 5. ANSI 字符路径

`WM_CHAR` 只读取 `wParam` 低字节，并用位七等价复现原 `test al, al` 后的有符号跳转：

- 锁存值等于一、当前字节位七为一：忽略当前字节，锁存保持一。
- 锁存值等于一、当前字节位七为零：插入当前单字节并把锁存清零。
- 锁存值不等于一、当前字节位七为一：先插入当前单字节，再把锁存写一。
- 锁存值不等于一、当前字节小于 `0x1F`：忽略。
- 锁存值不等于一、当前字节等于 `0x25`：不插入，向音频端口请求效果 `0x8C`。
- 其他字节：插入并把锁存清零。

因此 CP950 的高位 trail byte 会在锁存为一时被忽略；这是原分支的可观察缺陷。若锁存为一，`0x25` 会作为 trail 插入而不会触发声音。两项都由 UT 固定，不能按合法 Big5 解码器的预期修正。

## 6. 平台隔离

原光标 API 依赖宿主 ACP。当前游戏文本是 CP950；继续使用现代宿主 ACP 会让字节边界随系统语言变化。B2 已按 `CharNextExA(950, ...)` 探针固定兼容规则，B3 的 previous helper从同一规则和缓冲起点扫描得到前一边界。该替换标记为 `platform_adapted`。

兼容核心不包含 Win32 头文件：IME 布局查询和声音请求经端口注入。SDL3 以后提交 UTF-8/UTF-16 文本时，必须先在配置编码边界转换成旧缓冲所需字节，再调用本单元；不得把 UTF-16 字符数代入这些字节容量和光标计算。实际菜单对象仍由后续 `special_modes` 拥有并接线。

## 7. 实现与验证映射

- `LegacyDbcsTextBuffer::borrow_edit_view` 只借出 B3 所需缓冲和四个可变字段；B2 继续拥有分配和释放。
- `legacy_cp950_next_character_offset`、`legacy_cp950_previous_character_offset` 映射两个字符导航 API。
- `legacy_text_input_enabled`、`legacy_set_text_input_enabled` 映射 `0x00438DD0`、`0x00438DC0`。
- `legacy_move_text_cursor_previous`、`legacy_move_text_cursor_next` 映射 `0x00438DE0`、`0x00438E10`。
- `legacy_insert_text_bytes`、`legacy_delete_text_at_cursor` 映射 `0x00438E50`、`0x00438F60`。
- `filter_legacy_text_input_message` 映射 `0x00438B50`。

UT 覆盖 CP950/ASCII 导航、移到零却返回零、Home/End/Left/Right、Enter/Escape、Insert 恒启用、Backspace 在零删除首字符、重叠删除、插入后缀截断、IME 布局门、`0x1F` 阈值、百分号声音、低位 trail 组装、高位 trail 丢弃、锁存跨消息状态以及 `WM_IME_CHAR` 高低字节顺序和唯一零返回。

Windows LLVM `core` 为 39/39 CTest；WSL 原生 Linux Clang 22.1.8 同样为 39/39 CTest。原程序尚无可运行的文本输入状态捕获后端，因此本单元继续标记 `blocked_runtime_oracle`，不能把静态复核和 UT 宣称为原程序动态差分。
