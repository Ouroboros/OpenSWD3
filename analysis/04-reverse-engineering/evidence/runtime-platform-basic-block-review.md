# B1 runtime/platform 逐基本块实现复核

状态：七个根地址已完成 B1 层复核；`module_closed_pending_oracle`

来源：`swd3.exe`

SHA-256：`0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c`

## 1. 复核规则

完整汇编是唯一行为真值。本次把原 EXE 的指令、分支目标、立即数、整数宽度、状态写入和调用顺序逐段对应到 `compat`、`app` 与 `platform_sdl3`；IDA 伪码没有用于裁定行为。

复核只确认 B1 负责的顶层合同。资源、输入、RNG、渲染、音频、视频、世界、剧情、特殊模式、战斗和存档的端口内部实现仍由冻结的后续模块负责，不能因顶层调用已经存在而标成已还原。

## 2. 根地址结论

| 根地址 | 汇编块 | B1 对应 | 结论 |
|---|---|---|---|
| `0x0048A740` | OS/CRT 建立、参数和环境初始化、调用 WinMain、把结果交给 `_exit` | C++ hosted runtime 与 SDL main 边界 | 原函数属于静态 CRT，不复制其私有实现；游戏可见入口合同已确认，现代 CRT 替换为平台适配 |
| `0x00409EC0` | 旧实例、命令行早退、窗口建立、同步初始化、两次播种、消息/空闲泵、退出值 | `run_process_startup_gates`、命令行/单实例适配、`seed_two_rng_streams`、`run_idle_iteration`、SDL main | 早退顺序、两次独立取秒和空闲优先级已对应；命令业务、两个 RNG 算法及真实 SDL3 工具链验证仍待所属模块或环境 |
| `0x0040A0D0` | 文本输入前缀、消息跳转表、显示切换、视频、暂停、截图、销毁、自定义启动 | `window_events`、`host_window_event`、`startup`、`screenshot`、SDL event translation | B1 可见的比较、门控、返回和调用顺序已对应；IME 内部、BMP 像素写入和宿主默认分派仍是明确端口 |
| `0x0040A570` | 入口门、时钟门、迁移、输入、高优先级/战斗/世界/模式互斥、公共尾 | `frame_preparation`、`frame_dispatch`、`frame_runtime`、`battle_transition` | 无符号回绕、接受时点、提前返回和互斥顺序已对应；各业务 handler 仍由其所有模块实现 |
| `0x0040AB50` | 显示对象存在门、停用、恢复、surface/字体重建、恢复循环、35 ms 重置 | `display_lifecycle` 与 SDL display ports | 复核时发现并补回 `0x00437DF0(0x2711)` 失败即返回的入口门；其余状态写入和端口顺序已对应，DirectDraw 恢复细节由 SDL 适配替换 |
| `0x00424B90` | 总初始化、被忽略的平台结果、绘图失败、时钟失败、后续资源状态 | `initialization`、`platform_backend_initialization` | B1 顶层顺序和两处停止点已对应；分配、资源、字体、地图和媒体内部动作保留为所属模块端口 |
| `0x004251B0` | 42 项无条件动作、四条链、八个可失败关闭、显示/输入释放、返回一 | `run_total_shutdown` | 50 个边界及失败后继续语义已按原顺序对应；各链和资源的内部释放实现仍归状态所有者 |

## 3. 本轮修正

原 `display_lifecycle` 直接执行停用或恢复。汇编在 `0x0040AB50` 入口先调用 `0x00437DF0(0x2711)`，结果为零时跳到函数尾，不改变帧间隔、显示值、过渡抑制，也不调用任何平台操作。

当前接口新增 `display_backend_available()`，两条路径都先执行该查询。UT 覆盖后端缺失时连续调用停用和恢复仍只产生两次查询，所有状态保持原值。

入口接线复核还发现 SDL smoke 在接受帧后直接呈现，绕过了已经恢复的 `run_accepted_frame`，并且启动结果 `1/2` 只写布尔值，没有进入 B1 总初始化。当前正式入口已经补回两条组合链：

```text
startup 1/2
→ run_initialization_dialog_wrapper
→ run_total_initialization
→ run_platform_backend_initialization

accepted idle frame
→ run_frame_preparation
→ run_accepted_frame
→ common-tail synchronous close when requested
```

后续模块 handler 仍为空端口；这里补的是 B1 调用位置和状态传递，不是提前实现业务模块。

## 4. 验证

- `app.display_lifecycle` 覆盖显示后端存在/缺失、停用、恢复和总销毁顺序。
- `app.runtime_platform_integration` 组合覆盖正常启动门、两次独立播种、活动空闲帧、宿主关闭、50 项总销毁、COM 反初始化和退出消息顺序。
- Windows LLVM/CMake/Ninja 已分别完成最新源码的 `core` 与 `app` 构建，两套配置的 CTest 均为 23/23。
- SDL3 边界使用真实 SDL 3.4.12 构建；RGB565 texture 创建已接入平台后端初始化的显示请求和失败销毁路径。实际 EXE 已创建标题为 `OpenSWD3` 的窗口，并在关闭请求后以退出码零结束。
- 原程序动态差分仍为 `blocked_runtime_oracle`，不能标为 `original_diff_verified`。

## 5. B1 状态

七个根地址的 B1.4 静态逐块复核已经完成；映射表显式包含 17 个游戏自有函数和 `0x0048A740` 外部 CRT 根边界。后续所属模块端口已逐项登记，不再作为 B1 顶层映射缺口。规定的 Windows LLVM 与真实 SDL3 验收已经通过；B1 只剩已登记的原程序动态差分阻塞，状态为 `module_closed_pending_oracle`，可以移交 B2。
