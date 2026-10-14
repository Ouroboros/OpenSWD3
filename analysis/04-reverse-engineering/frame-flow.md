# 顶层帧流程

最后更新：2026-08-02

当前覆盖：P1.1 至 P1.7

## 已确认流程

```text
PE 入口 0x0048A740
  → 建立 SEH / CRT 启动栈帧
  → 读取 Windows 版本
  → 初始化 CRT 堆和 I/O
  → 获取命令行与环境
  → 初始化 argv / envp / C 运行库
  → 获取 STARTUPINFOA
  → 计算 nCmdShow
  → GetModuleHandleA(NULL)
  → 调用 0x00409EC0 WinMain
      → 拒绝已存在的匹配窗口
      → 处理非空命令行特殊路径
      → CoInitialize(NULL)
      → 注册 SoftstarSwd3 窗口类
      → 创建 640×480 主窗口
      → SendMessageA(hWnd, 0x404, 0, 0) 同步初始化
      → 分别用 time(NULL) 初始化两个随机序列
      → 进入消息泵
  → 将 WinMain 返回值交给 CRT _exit
```

## WinMain 实参

在 `0x0048A83B` 至 `0x0048A84E`，实际压栈顺序为：

1. `nCmdShow`
2. `lpCmdLine`
3. 零 `hPrevInstance`
4. `GetModuleHandleA(NULL)` 返回的 `hInstance`

随后调用 `0x00409EC0`。`WinMain` 在 `0x0040A0C7` 以 `ret 10h` 返回，确认四个参数合计 16 字节。

## WinMain 消息泵

```text
0x0040A042
  → PeekMessageA(..., PM_NOREMOVE)
      ├─ 有消息
      │   → GetMessageA
      │       ├─ 返回 0：返回 msg.wParam
      │       └─ 非 0：TranslateMessage → DispatchMessageA → 循环
      └─ 无消息
          → [0x004CC2AC] 是否非零
              ├─ 是
              │   ├─ flags & 0x20：视频解码/呈现 0x00484950 → 音频维护 0x0040CF10
              │   ├─ flags & 0x01：Sleep(0)
              │   ├─ [0x004BABA4] != 0：Sleep(0)
              │   └─ 其他：0x0040A570 单帧主循环
              └─ 否
                  ├─ [0x004B7CAC] == 1：绘制并呈现暂停提示
                  └─ 其他：Sleep(0)
```

## 窗口过程

窗口过程 `0x0040A0D0` 已确认：

- `WM_SIZE` / `WM_ACTIVATEAPP` 通过 `0x0040AB50` 停用或恢复显示。
- F8 切换全局帧执行门控；剧情 VM 和调试对话框也有写入路径。
- P 在条件允许时保存 640×480 BMP 截图。
- 视频活动期间 Esc 释放视频对象并清除状态位 `0x20`。
- `WM_DESTROY` 执行条件化资源释放，随后 `CoUninitialize` 和 `PostQuitMessage(0)`。
- 自定义 `0x404` 建立目录、检查存档、打开启动对话框，并按返回值进入主初始化或退出。

## 单帧主循环

`0x0040A570` 已确认的顶层顺序：

```text
入口/显示门控
  → timeGetTime 帧间隔判断
  → 倒计时与内部位迁移
  → 设备状态采样
  → 输入边沿与重复状态更新
  → 高优先级模式检查
  → 战斗请求检查
      ├─ 战斗活动：0x00469D20 → 分支内呈现 → 音频维护 → 按返回值恢复/切换 → 本帧返回
      └─ 无战斗
          → 声音/流状态更新
          → [0x004B8740] 模式分支
              ├─ 普通世界：0x00427300 → 0x00402F80 → 0x00427920 → 分支内呈现
              └─ 特殊模式：0x00439FD0 或 0x0044EA60 → 分支内呈现
  → 0x0040CF10 Miles 音频对象维护
  → 关闭请求位转成 WM_CLOSE
```

`0x0040CF10` 不调用 DirectDraw。原程序的高优先级、普通世界、战斗、特殊模式、暂停和视频路径分别在自己的分支内调用 `Blt`，不存在一个统一的顶层最终呈现函数。

战斗活动时不会继续执行同帧的世界和剧情路径。

战斗核心 `0x00469D20` 从 `[0x0053CE84]` 读取当前指令指针，并按有符号 16 位操作码 `-1..83` 分派。

它对主循环只返回四个值：

```text
1 → 保持战斗活动，直接结束本帧
0 → 清除战斗活动，恢复/重载地图，直接结束本帧
2 → 清除战斗活动，设置特殊模式 0x80000004，直接结束本帧
3 → 清除战斗活动，重映射/恢复地图，直接结束本帧
```

零、二、三的最终游戏业务名称尚未确认。这里仅记录汇编中的状态写入和控制流。

剧情核心 `0x00427920` 在每个 opcode 后根据两个局部继续量决定：

```text
任一继续量非零 → 同帧继续取指
两个继续量都为零 → AIL_serve → 返回 1 → 下一帧重入
```

指令偏移是否推进由各 opcode handler 自己决定。

## 暂停、失焦和退出

```text
F8 WM_KEYUP
  → 切换 [0x004CC2AC]
  → 零时停止正常单帧主循环
  → 窗口已恢复则 0x00411FA0 绘制并 Blt 暂停提示

WM_SIZE / WM_ACTIVATEAPP / WM_SYSKEYDOWN Enter 路径
  → 0x0040AB50(0)：停用音频/字体 renderer/战斗显示，隐藏窗口，抑制正常帧
  → 0x0040AB50(1)：恢复 surface、字体 renderer 和战斗显示，解除帧抑制
  → 完整函数不调用 DirectInput wrapper；输入只因正常帧门控而停止采样

[0x004B7A9C] & 0x04
  → 单帧尾同步 WM_CLOSE
  → DefWindowProcA 销毁窗口
  → WndProc 处理 WM_DESTROY
  → 条件化完整释放 → CoUninitialize → PostQuitMessage(0)
  → GetMessageA 返回 0 → WinMain 返回 msg.wParam → CRT _exit
```

原始入口证据见：`evidence/entry-point-0048a740.md`。

原始 `WinMain` 证据见：`evidence/winmain-00409ec0.md`。

原始窗口过程证据见：`evidence/wndproc-0040a0d0.md`。

原始单帧主循环证据见：`evidence/main-frame-0040a570.md`。

剧情入口与让出规则见：`story-vm-entry.md`。

战斗入口与返回值契约见：`battle-entry.md`。

呈现、暂停、显示恢复与退出证据见：`evidence/presentation-lifecycle.md`。
