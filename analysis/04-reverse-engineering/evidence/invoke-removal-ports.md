# Port、Dispatch、Adapter 定义与引用清单

对应[业务执行清单与覆盖索引](invoke-removal-list.md)。定义和引用行号来自
建表快照；条目内的迁移记录用于说明后续变化，不宣称旧定义仍存在。

每项列出定义、直接残留特征、继承和符号引用文件/行。
无直接特征的类型仍待检查继承协议及纯转发；不自动判定应删除或可保留。

## 与函数执行项的关系

- 按用户最新要求，以函数为执行粒度。本表按类型检查删除或合法保留，
  不作为函数完成率的第二个分母。
- 同一Port承载多个函数时，逐函数记录地址、实际调用方、剩余协议与验证。
  一个函数验收通过即可发布，不等待整个Port消失；部分函数删除不能
  作为整个Port已删除的证据。
- 纯转发Adapter随所属行为的全部生产调用方一起回收，不另算一个业务成果。
- 平台接口保留须说明实际能力、真实实现、全部调用方、所有权和失败行为；
  只改方法名或参数形式不能作为保留依据。
- 本条方框只表示该类型本身已完成当前源码复核。函数完成以总清单的
  逐函数验收及证据为准；全仓完成仍要求所有覆盖项和本表均验收。
- 随函数迁移补充映射，不等待全量建表。历史业务ID保留用于查找旧证据。

首族关联：`PORT-e7050ee9a7d7`对应`BEHAVIOR-actor-list-availability`；
`PORT-395776ccb4e9`与`PORT-afd331603c89`对应
`BEHAVIOR-resource-diagnostic`。后续按各类型的实际调用逐项补齐映射。

## PORT-187e0eae1881

- [ ] `AuxiliaryTextDialogPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/auxiliary_text_dialog.hpp:37](../../../include/openswd3/app/auxiliary_text_dialog.hpp:37)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/auxiliary_text_dialog.hpp](../../../include/openswd3/app/auxiliary_text_dialog.hpp)：37, 39, 46
  - [src/app/auxiliary_text_dialog.cpp](../../../src/app/auxiliary_text_dialog.cpp)：50
  - [tests/unit/app/auxiliary_text_dialog_test.cpp](../../../tests/unit/app/auxiliary_text_dialog_test.cpp)：119

## PORT-b6ef63f2e8e5

- [ ] `BattleTransitionPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/battle_transition.hpp:20](../../../include/openswd3/app/battle_transition.hpp:20)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/battle_transition.hpp](../../../include/openswd3/app/battle_transition.hpp)：20, 22, 53, 57
  - [include/openswd3/app/frame_runtime.hpp](../../../include/openswd3/app/frame_runtime.hpp)：16
  - [src/app/battle_transition.cpp](../../../src/app/battle_transition.cpp)：8, 34
  - [tests/unit/app/battle_transition_test.cpp](../../../tests/unit/app/battle_transition_test.cpp)：43

## PORT-a211718f6100

- [ ] `CommandLinePorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/command_line.hpp:10](../../../include/openswd3/app/command_line.hpp:10)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/command_line.hpp](../../../include/openswd3/app/command_line.hpp)：10, 12, 20
  - [include/openswd3/app/process_startup.hpp](../../../include/openswd3/app/process_startup.hpp)：26
  - [src/app/command_line.cpp](../../../src/app/command_line.cpp)：6
  - [src/app/process_startup.cpp](../../../src/app/process_startup.cpp)：8
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：612
  - [tests/unit/app/command_line_test.cpp](../../../tests/unit/app/command_line_test.cpp)：26
  - [tests/unit/app/process_startup_test.cpp](../../../tests/unit/app/process_startup_test.cpp)：33
  - [tests/unit/app/runtime_platform_integration_test.cpp](../../../tests/unit/app/runtime_platform_integration_test.cpp)：27

## PORT-579677afd1a8

- [ ] `RngSeedPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/command_line.hpp:23](../../../include/openswd3/app/command_line.hpp:23)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/command_line.hpp](../../../include/openswd3/app/command_line.hpp)：23, 25, 32
  - [src/app/command_line.cpp](../../../src/app/command_line.cpp)：21
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：631
  - [tests/unit/app/command_line_test.cpp](../../../tests/unit/app/command_line_test.cpp)：42
  - [tests/unit/app/runtime_platform_integration_test.cpp](../../../tests/unit/app/runtime_platform_integration_test.cpp)：28

## PORT-637969f4ac5a

- [ ] `DisplayLifecyclePorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/display_lifecycle.hpp:12](../../../include/openswd3/app/display_lifecycle.hpp:12)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/display_lifecycle.hpp](../../../include/openswd3/app/display_lifecycle.hpp)：12, 14, 35, 39
  - [include/openswd3/app/host_window_event.hpp](../../../include/openswd3/app/host_window_event.hpp)：33
  - [include/openswd3/app/window_events.hpp](../../../include/openswd3/app/window_events.hpp)：30
  - [src/app/display_lifecycle.cpp](../../../src/app/display_lifecycle.cpp)：15, 38
  - [src/app/host_window_event.cpp](../../../src/app/host_window_event.cpp)：10
  - [src/app/window_events.cpp](../../../src/app/window_events.cpp)：71
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：976
  - [tests/unit/app/display_lifecycle_test.cpp](../../../tests/unit/app/display_lifecycle_test.cpp)：39
  - [tests/unit/app/host_window_event_test.cpp](../../../tests/unit/app/host_window_event_test.cpp)：26
  - [tests/unit/app/runtime_platform_integration_test.cpp](../../../tests/unit/app/runtime_platform_integration_test.cpp)：31

## PORT-b7e9d84e2af1

- [ ] `ShutdownPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/display_lifecycle.hpp:98](../../../include/openswd3/app/display_lifecycle.hpp:98)。
- 继承：`无`。
- 直接残留：operation。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/display_lifecycle.hpp](../../../include/openswd3/app/display_lifecycle.hpp)：98, 100, 110
  - [include/openswd3/app/window_events.hpp](../../../include/openswd3/app/window_events.hpp)：93
  - [src/app/display_lifecycle.cpp](../../../src/app/display_lifecycle.cpp)：61
  - [src/app/window_events.cpp](../../../src/app/window_events.cpp)：146
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1152, 2088, 9360
  - [tests/unit/app/display_lifecycle_test.cpp](../../../tests/unit/app/display_lifecycle_test.cpp)：130
  - [tests/unit/app/runtime_platform_integration_test.cpp](../../../tests/unit/app/runtime_platform_integration_test.cpp)：32
  - [tests/unit/app/window_events_test.cpp](../../../tests/unit/app/window_events_test.cpp)：318

## PORT-900aed6aeea4

- [ ] `ExternalLaunchPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/external_launch.hpp:9](../../../include/openswd3/app/external_launch.hpp:9)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/external_launch.hpp](../../../include/openswd3/app/external_launch.hpp)：9, 11, 18, 22
  - [src/app/external_launch.cpp](../../../src/app/external_launch.cpp)：6, 13
  - [src/platform/sdl3/external_launch_sdl3.hpp](../../../src/platform/sdl3/external_launch_sdl3.hpp)：7
  - [src/platform/sdl3/startup_dialog_sdl3.cpp](../../../src/platform/sdl3/startup_dialog_sdl3.cpp)：42
  - [src/platform/sdl3/startup_dialog_sdl3.hpp](../../../src/platform/sdl3/startup_dialog_sdl3.hpp)：20, 43
  - [tests/unit/app/external_launch_test.cpp](../../../tests/unit/app/external_launch_test.cpp)：23

## PORT-3e47878555c7

- [ ] `FramePreparationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/frame_preparation.hpp:46](../../../include/openswd3/app/frame_preparation.hpp:46)。
- 继承：`无`。
- 直接残留：operation。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/frame_preparation.hpp](../../../include/openswd3/app/frame_preparation.hpp)：46, 48, 75
  - [src/app/frame_preparation.cpp](../../../src/app/frame_preparation.cpp)：16, 57, 85
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1703
  - [tests/unit/app/frame_preparation_test.cpp](../../../tests/unit/app/frame_preparation_test.cpp)：31

## PORT-45b0eb6aaab0

- [ ] `FrameRuntimePorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/frame_runtime.hpp:16](../../../include/openswd3/app/frame_runtime.hpp:16)。
- 继承：`public BattleTransitionPorts,
                          public LegacyNewGameTransitionPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/frame_runtime.hpp](../../../include/openswd3/app/frame_runtime.hpp)：16, 46
  - [src/app/frame_runtime.cpp](../../../src/app/frame_runtime.cpp)：9, 29
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1704
  - [tests/unit/app/frame_runtime_test.cpp](../../../tests/unit/app/frame_runtime_test.cpp)：54

## PORT-95e765f3bcaf

- [ ] `IdleRuntimePorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/idle_runtime.hpp:7](../../../include/openswd3/app/idle_runtime.hpp:7)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/idle_runtime.hpp](../../../include/openswd3/app/idle_runtime.hpp)：7, 9, 19
  - [src/app/idle_runtime.cpp](../../../src/app/idle_runtime.cpp)：5
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1702
  - [tests/unit/app/idle_runtime_test.cpp](../../../tests/unit/app/idle_runtime_test.cpp)：18
  - [tests/unit/app/runtime_platform_integration_test.cpp](../../../tests/unit/app/runtime_platform_integration_test.cpp)：29

## PORT-34f546f6ea95

- [ ] `InitializationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/initialization.hpp:15](../../../include/openswd3/app/initialization.hpp:15)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/initialization.hpp](../../../include/openswd3/app/initialization.hpp)：15, 17, 34, 38
  - [src/app/initialization.cpp](../../../src/app/initialization.cpp)：6, 39
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：799, 920, 968, 6823, 6827, 6844
  - [tests/unit/app/initialization_test.cpp](../../../tests/unit/app/initialization_test.cpp)：24

## PORT-db41a19a4f4f

- [ ] `LegacyNewGameTransitionPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/legacy_new_game_transition.hpp:14](../../../include/openswd3/app/legacy_new_game_transition.hpp:14)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/frame_runtime.hpp](../../../include/openswd3/app/frame_runtime.hpp)：17, 42
  - [include/openswd3/app/legacy_new_game_transition.hpp](../../../include/openswd3/app/legacy_new_game_transition.hpp)：14, 16, 46
  - [src/app/frame_runtime.cpp](../../../src/app/frame_runtime.cpp)：19
  - [src/app/legacy_new_game_transition.cpp](../../../src/app/legacy_new_game_transition.cpp)：6
  - [tests/unit/app/legacy_new_game_transition_test.cpp](../../../tests/unit/app/legacy_new_game_transition_test.cpp)：10, 27

## PORT-e6ec14094bc4

- [ ] `PlatformBackendInitializationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/platform_backend_initialization.hpp:38](../../../include/openswd3/app/platform_backend_initialization.hpp:38)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/platform_backend_initialization.hpp](../../../include/openswd3/app/platform_backend_initialization.hpp)：38, 40, 70
  - [src/app/platform_backend_initialization.cpp](../../../src/app/platform_backend_initialization.cpp)：8
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：657, 803, 902
  - [tests/unit/app/platform_backend_initialization_test.cpp](../../../tests/unit/app/platform_backend_initialization_test.cpp)：29

## PORT-94b597ca4098

- [ ] `ExistingInstancePorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/process_startup.hpp:10](../../../include/openswd3/app/process_startup.hpp:10)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/process_startup.hpp](../../../include/openswd3/app/process_startup.hpp)：10, 12, 25
  - [src/app/process_startup.cpp](../../../src/app/process_startup.cpp)：7
  - [src/platform/sdl3/single_instance.hpp](../../../src/platform/sdl3/single_instance.hpp)：9
  - [tests/unit/app/process_startup_test.cpp](../../../tests/unit/app/process_startup_test.cpp)：18
  - [tests/unit/app/runtime_platform_integration_test.cpp](../../../tests/unit/app/runtime_platform_integration_test.cpp)：26

## PORT-331ae1e7a096

- [ ] `ScreenshotPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/screenshot.hpp:13](../../../include/openswd3/app/screenshot.hpp:13)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/screenshot.hpp](../../../include/openswd3/app/screenshot.hpp)：13, 15, 25
  - [src/app/screenshot.cpp](../../../src/app/screenshot.cpp)：16
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：449
  - [tests/unit/app/screenshot_test.cpp](../../../tests/unit/app/screenshot_test.cpp)：26

## PORT-d99e02f3609c

- [ ] `StartupPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/startup.hpp:11](../../../include/openswd3/app/startup.hpp:11)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/startup.hpp](../../../include/openswd3/app/startup.hpp)：11, 13, 29
  - [src/app/startup.cpp](../../../src/app/startup.cpp)：6
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：915
  - [tests/unit/app/startup_test.cpp](../../../tests/unit/app/startup_test.cpp)：22
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：173, 233, 533, 594, 641, 712, 905, 975, 1687, 1802, 1892, 1937, 1970, 2174, 2218, 2302, 2373, 2414, 2448, 2571, 2727, 3126, 3209, 3317, 3353, 3399, 3439, 3483, 3513, 3547, 3594, 3630, 3665

## PORT-0271e5d485cf

- [ ] `StartupDialogPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/startup_dialog.hpp:72](../../../include/openswd3/app/startup_dialog.hpp:72)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/startup_dialog.hpp](../../../include/openswd3/app/startup_dialog.hpp)：72, 74, 83
  - [src/app/startup_dialog.cpp](../../../src/app/startup_dialog.cpp)：87
  - [src/platform/sdl3/startup_dialog_sdl3.hpp](../../../src/platform/sdl3/startup_dialog_sdl3.hpp)：16
  - [tests/unit/app/startup_dialog_test.cpp](../../../tests/unit/app/startup_dialog_test.cpp)：201

## PORT-e229428027e4

- [ ] `TextInputMessagePorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/window_events.hpp:41](../../../include/openswd3/app/window_events.hpp:41)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/window_events.hpp](../../../include/openswd3/app/window_events.hpp)：41, 43, 57
  - [src/app/window_events.cpp](../../../src/app/window_events.cpp)：95
  - [tests/unit/app/window_events_test.cpp](../../../tests/unit/app/window_events_test.cpp)：104

## PORT-7bbd2a19d876

- [ ] `WindowEventPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/window_events.hpp:66](../../../include/openswd3/app/window_events.hpp:66)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/host_window_event.hpp](../../../include/openswd3/app/host_window_event.hpp)：31
  - [include/openswd3/app/window_events.hpp](../../../include/openswd3/app/window_events.hpp)：66, 68, 76, 80
  - [src/app/host_window_event.cpp](../../../src/app/host_window_event.cpp)：8
  - [src/app/window_events.cpp](../../../src/app/window_events.cpp)：20, 111, 136
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：448
  - [tests/unit/app/host_window_event_test.cpp](../../../tests/unit/app/host_window_event_test.cpp)：9
  - [tests/unit/app/runtime_platform_integration_test.cpp](../../../tests/unit/app/runtime_platform_integration_test.cpp)：30
  - [tests/unit/app/window_events_test.cpp](../../../tests/unit/app/window_events_test.cpp)：169

## PORT-98ae3f027e5d

- [ ] `ProcessExitPorts`：抽象接口；待复核。
- 定义：[include/openswd3/app/window_events.hpp:83](../../../include/openswd3/app/window_events.hpp:83)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/window_events.hpp](../../../include/openswd3/app/window_events.hpp)：83, 85, 94
  - [src/app/window_events.cpp](../../../src/app/window_events.cpp)：147
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1336, 2089, 9361
  - [tests/unit/app/runtime_platform_integration_test.cpp](../../../tests/unit/app/runtime_platform_integration_test.cpp)：33
  - [tests/unit/app/window_events_test.cpp](../../../tests/unit/app/window_events_test.cpp)：353

## PORT-a736bd231a94

- [ ] `LegacyActionDrawPorts`：抽象接口；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_action_draw_bridge.hpp:15](../../../include/openswd3/asset_runtime/legacy_action_draw_bridge.hpp:15)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_action_draw_bridge.hpp](../../../include/openswd3/asset_runtime/legacy_action_draw_bridge.hpp)：15, 17, 55, 113, 122, 132
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：1914
  - [include/openswd3/special_modes/legacy_initial_menu.hpp](../../../include/openswd3/special_modes/legacy_initial_menu.hpp)：87
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：7780
  - [include/openswd3/world_map/legacy_moving_actions.hpp](../../../include/openswd3/world_map/legacy_moving_actions.hpp)：69
  - [include/openswd3/world_map/legacy_picture_actions.hpp](../../../include/openswd3/world_map/legacy_picture_actions.hpp)：77
  - [include/openswd3/world_map/legacy_role_head_actions.hpp](../../../include/openswd3/world_map/legacy_role_head_actions.hpp)：64
  - [include/openswd3/world_map/legacy_world_cursor.hpp](../../../include/openswd3/world_map/legacy_world_cursor.hpp)：57, 68
  - [include/openswd3/world_map/legacy_world_dialog_runtime.hpp](../../../include/openswd3/world_map/legacy_world_dialog_runtime.hpp)：48, 65, 123
  - [include/openswd3/world_map/legacy_world_flagged_roles.hpp](../../../include/openswd3/world_map/legacy_world_flagged_roles.hpp)：52, 80
  - [include/openswd3/world_map/legacy_world_frame_runtime.hpp](../../../include/openswd3/world_map/legacy_world_frame_runtime.hpp)：90
  - [include/openswd3/world_map/legacy_world_head_sign_actions.hpp](../../../include/openswd3/world_map/legacy_world_head_sign_actions.hpp)：58
  - [include/openswd3/world_map/legacy_world_load_progress.hpp](../../../include/openswd3/world_map/legacy_world_load_progress.hpp)：69
  - [include/openswd3/world_map/legacy_world_map_role_paths.hpp](../../../include/openswd3/world_map/legacy_world_map_role_paths.hpp)：90
  - [include/openswd3/world_map/legacy_world_party_role_actions.hpp](../../../include/openswd3/world_map/legacy_world_party_role_actions.hpp)：56
  - [include/openswd3/world_map/legacy_world_player_post_frame.hpp](../../../include/openswd3/world_map/legacy_world_player_post_frame.hpp)：73
  - [include/openswd3/world_map/legacy_world_role_lifecycle.hpp](../../../include/openswd3/world_map/legacy_world_role_lifecycle.hpp)：104
  - [src/asset_runtime/legacy_action_draw_bridge.cpp](../../../src/asset_runtime/legacy_action_draw_bridge.cpp)：48, 73, 214, 224, 243
  - [src/special_modes/legacy_initial_menu.cpp](../../../src/special_modes/legacy_initial_menu.cpp)：256, 308, 402
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：21761
  - [src/world_map/legacy_moving_actions.cpp](../../../src/world_map/legacy_moving_actions.cpp)：66
  - [src/world_map/legacy_picture_actions.cpp](../../../src/world_map/legacy_picture_actions.cpp)：56
  - [src/world_map/legacy_role_head_actions.cpp](../../../src/world_map/legacy_role_head_actions.cpp)：60
  - [src/world_map/legacy_world_cursor.cpp](../../../src/world_map/legacy_world_cursor.cpp)：46, 55
  - [src/world_map/legacy_world_dialog_runtime.cpp](../../../src/world_map/legacy_world_dialog_runtime.cpp)：121, 146, 151, 181, 229
  - [src/world_map/legacy_world_flagged_roles.cpp](../../../src/world_map/legacy_world_flagged_roles.cpp)：53, 123
  - [src/world_map/legacy_world_frame_coordinator.cpp](../../../src/world_map/legacy_world_frame_coordinator.cpp)：28, 54
  - [src/world_map/legacy_world_head_sign_actions.cpp](../../../src/world_map/legacy_world_head_sign_actions.cpp)：37
  - [src/world_map/legacy_world_load_progress.cpp](../../../src/world_map/legacy_world_load_progress.cpp)：77
  - [src/world_map/legacy_world_map_role_paths.cpp](../../../src/world_map/legacy_world_map_role_paths.cpp)：308
  - [src/world_map/legacy_world_party_role_actions.cpp](../../../src/world_map/legacy_world_party_role_actions.cpp)：144, 161
  - [src/world_map/legacy_world_player_post_frame.cpp](../../../src/world_map/legacy_world_player_post_frame.cpp)：57
  - [src/world_map/legacy_world_role_lifecycle.cpp](../../../src/world_map/legacy_world_role_lifecycle.cpp)：156
  - [tests/unit/asset_runtime/legacy_action_draw_bridge_test.cpp](../../../tests/unit/asset_runtime/legacy_action_draw_bridge_test.cpp)：20, 48
  - [tests/unit/battle/legacy_battle_frame_coordinator_test.cpp](../../../tests/unit/battle/legacy_battle_frame_coordinator_test.cpp)：449
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：26, 222
  - [tests/unit/world_map/legacy_moving_actions_test.cpp](../../../tests/unit/world_map/legacy_moving_actions_test.cpp)：13, 34
  - [tests/unit/world_map/legacy_picture_actions_test.cpp](../../../tests/unit/world_map/legacy_picture_actions_test.cpp)：14, 36
  - [tests/unit/world_map/legacy_role_head_actions_test.cpp](../../../tests/unit/world_map/legacy_role_head_actions_test.cpp)：13, 34
  - [tests/unit/world_map/legacy_world_cursor_test.cpp](../../../tests/unit/world_map/legacy_world_cursor_test.cpp)：11, 35
  - [tests/unit/world_map/legacy_world_dialog_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_dialog_runtime_test.cpp)：33
  - [tests/unit/world_map/legacy_world_flagged_roles_test.cpp](../../../tests/unit/world_map/legacy_world_flagged_roles_test.cpp)：21, 55
  - [tests/unit/world_map/legacy_world_frame_coordinator_test.cpp](../../../tests/unit/world_map/legacy_world_frame_coordinator_test.cpp)：19, 185
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：29, 199
  - [tests/unit/world_map/legacy_world_head_sign_actions_test.cpp](../../../tests/unit/world_map/legacy_world_head_sign_actions_test.cpp)：10, 27
  - [tests/unit/world_map/legacy_world_load_progress_test.cpp](../../../tests/unit/world_map/legacy_world_load_progress_test.cpp)：14, 68
  - [tests/unit/world_map/legacy_world_map_role_paths_test.cpp](../../../tests/unit/world_map/legacy_world_map_role_paths_test.cpp)：14, 87
  - [tests/unit/world_map/legacy_world_party_role_actions_test.cpp](../../../tests/unit/world_map/legacy_world_party_role_actions_test.cpp)：12, 71
  - [tests/unit/world_map/legacy_world_player_post_frame_test.cpp](../../../tests/unit/world_map/legacy_world_player_post_frame_test.cpp)：11, 33
  - [tests/unit/world_map/legacy_world_role_lifecycle_test.cpp](../../../tests/unit/world_map/legacy_world_role_lifecycle_test.cpp)：54
  - [tests/unit/world_map/legacy_world_story_vm_test_support.hpp](../../../tests/unit/world_map/legacy_world_story_vm_test_support.hpp)：998

## PORT-7bc64e8c1dc4

- [ ] `LegacyActionDrawRuntimePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_action_draw_bridge.hpp:55](../../../include/openswd3/asset_runtime/legacy_action_draw_bridge.hpp:55)。
- 继承：`public LegacyActionDrawPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_action_draw_bridge.hpp](../../../include/openswd3/asset_runtime/legacy_action_draw_bridge.hpp)：55, 57
  - [src/asset_runtime/legacy_action_draw_bridge.cpp](../../../src/asset_runtime/legacy_action_draw_bridge.cpp)：141, 154, 158, 184
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：2169, 2682, 3512, 5252, 6631, 7315, 7458, 7515, 7576, 7622, 7730, 8066, 8865
  - [tests/unit/asset_runtime/legacy_action_draw_bridge_test.cpp](../../../tests/unit/asset_runtime/legacy_action_draw_bridge_test.cpp)：21, 308
  - [tests/unit/world_map/legacy_world_flagged_roles_test.cpp](../../../tests/unit/world_map/legacy_world_flagged_roles_test.cpp)：22, 504
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：30, 1108
  - [tests/unit/world_map/legacy_world_runtime_session_test.cpp](../../../tests/unit/world_map/legacy_world_runtime_session_test.cpp)：951

## PORT-f054803d8402

- [ ] `LegacyAniActivityPorts`：抽象接口；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_activity.hpp:41](../../../include/openswd3/asset_runtime/legacy_ani_activity.hpp:41)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_activity.hpp](../../../include/openswd3/asset_runtime/legacy_ani_activity.hpp)：41, 43, 126, 143
  - [src/asset_runtime/legacy_ani_activity.cpp](../../../src/asset_runtime/legacy_ani_activity.cpp)：120, 242
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1352
  - [tests/unit/asset_runtime/legacy_ani_activity_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_activity_test.cpp)：27, 128

## PORT-dc01c6327a93

- [ ] `LegacyAniDirectionalServicePort`：抽象接口；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp:100](../../../include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp:100)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp)：100, 102, 211
  - [src/asset_runtime/legacy_ani_directional_effect.cpp](../../../src/asset_runtime/legacy_ani_directional_effect.cpp)：261
  - [src/world_map/legacy_world_frame_runtime.cpp](../../../src/world_map/legacy_world_frame_runtime.cpp)：40
  - [tests/unit/asset_runtime/legacy_ani_directional_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_directional_effect_test.cpp)：38, 79

## PORT-596e8c6acc5d

- [ ] `LegacyAniDirectionalPorts`：抽象接口；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp:107](../../../include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp:107)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp)：107, 109, 129, 213
  - [include/openswd3/world_map/legacy_world_frame_runtime.hpp](../../../include/openswd3/world_map/legacy_world_frame_runtime.hpp)：87
  - [src/asset_runtime/legacy_ani_directional_effect.cpp](../../../src/asset_runtime/legacy_ani_directional_effect.cpp)：263
  - [tests/unit/asset_runtime/legacy_ani_directional_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_directional_effect_test.cpp)：36, 92
  - [tests/unit/world_map/legacy_world_frame_coordinator_test.cpp](../../../tests/unit/world_map/legacy_world_frame_coordinator_test.cpp)：187
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：36, 297

## PORT-e923f0c8070f

- [ ] `LegacyAniDirectionalRuntimePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp:128](../../../include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp:128)。
- 继承：`public LegacyAniDirectionalPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_directional_effect.hpp)：128, 131
  - [src/asset_runtime/legacy_ani_directional_effect.cpp](../../../src/asset_runtime/legacy_ani_directional_effect.cpp)：111, 123, 129, 155
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6668
  - [tests/unit/asset_runtime/legacy_ani_directional_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_directional_effect_test.cpp)：37, 897
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：1119
  - [tests/unit/world_map/legacy_world_runtime_session_test.cpp](../../../tests/unit/world_map/legacy_world_runtime_session_test.cpp)：1018

## PORT-67ef5828086f

- [ ] `LegacyAniDriftServicePort`：抽象接口；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp:37](../../../include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp:37)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp)：37, 39, 138
  - [src/asset_runtime/legacy_ani_drift_effect.cpp](../../../src/asset_runtime/legacy_ani_drift_effect.cpp)：162
  - [src/world_map/legacy_world_frame_runtime.cpp](../../../src/world_map/legacy_world_frame_runtime.cpp)：37
  - [tests/unit/asset_runtime/legacy_ani_drift_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_drift_effect_test.cpp)：29, 54

## PORT-09c4355bd91b

- [ ] `LegacyAniDriftPorts`：抽象接口；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp:44](../../../include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp:44)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp)：44, 46, 63, 139
  - [include/openswd3/world_map/legacy_world_frame_runtime.hpp](../../../include/openswd3/world_map/legacy_world_frame_runtime.hpp)：86
  - [src/asset_runtime/legacy_ani_drift_effect.cpp](../../../src/asset_runtime/legacy_ani_drift_effect.cpp)：163
  - [tests/unit/asset_runtime/legacy_ani_drift_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_drift_effect_test.cpp)：27, 67
  - [tests/unit/world_map/legacy_world_frame_coordinator_test.cpp](../../../tests/unit/world_map/legacy_world_frame_coordinator_test.cpp)：186
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：37, 296

## PORT-abb3faee5b8d

- [ ] `LegacyAniDriftRuntimePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp:63](../../../include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp:63)。
- 继承：`public LegacyAniDriftPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_drift_effect.hpp)：63, 65
  - [src/asset_runtime/legacy_ani_drift_effect.cpp](../../../src/asset_runtime/legacy_ani_drift_effect.cpp)：70, 83, 87, 113
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6660
  - [tests/unit/asset_runtime/legacy_ani_drift_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_drift_effect_test.cpp)：28, 655
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：1116
  - [tests/unit/world_map/legacy_world_runtime_session_test.cpp](../../../tests/unit/world_map/legacy_world_runtime_session_test.cpp)：1010

## PORT-e8fbbb8c7e03

- [ ] `LegacyAniFollowerPorts`：抽象接口；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_follower_effect.hpp:26](../../../include/openswd3/asset_runtime/legacy_ani_follower_effect.hpp:26)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_follower_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_follower_effect.hpp)：26, 28, 48, 108
  - [include/openswd3/world_map/legacy_world_frame_runtime.hpp](../../../include/openswd3/world_map/legacy_world_frame_runtime.hpp)：88
  - [src/asset_runtime/legacy_ani_follower_effect.cpp](../../../src/asset_runtime/legacy_ani_follower_effect.cpp)：129
  - [tests/unit/asset_runtime/legacy_ani_follower_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_follower_effect_test.cpp)：22, 54
  - [tests/unit/world_map/legacy_world_frame_coordinator_test.cpp](../../../tests/unit/world_map/legacy_world_frame_coordinator_test.cpp)：188
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：38, 298

## PORT-e8bc0053f21d

- [ ] `LegacyAniFollowerRuntimePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_follower_effect.hpp:48](../../../include/openswd3/asset_runtime/legacy_ani_follower_effect.hpp:48)。
- 继承：`public LegacyAniFollowerPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_follower_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_follower_effect.hpp)：48, 50
  - [src/asset_runtime/legacy_ani_follower_effect.cpp](../../../src/asset_runtime/legacy_ani_follower_effect.cpp)：48, 60, 66, 91, 101
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6677
  - [tests/unit/asset_runtime/legacy_ani_follower_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_follower_effect_test.cpp)：23, 478
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：1122
  - [tests/unit/world_map/legacy_world_runtime_session_test.cpp](../../../tests/unit/world_map/legacy_world_runtime_session_test.cpp)：1027

## PORT-50014e7449e8

- [ ] `LegacyAniRoleParticlePositionPort`：抽象接口；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp:86](../../../include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp:86)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp)：86, 88, 203
  - [src/asset_runtime/legacy_ani_role_particle_effect.cpp](../../../src/asset_runtime/legacy_ani_role_particle_effect.cpp)：296
  - [src/platform/sdl3/world_role_runtime_adapter.cpp](../../../src/platform/sdl3/world_role_runtime_adapter.cpp)：23
  - [tests/unit/app/world_role_runtime_adapter_test.cpp](../../../tests/unit/app/world_role_runtime_adapter_test.cpp)：37, 223
  - [tests/unit/asset_runtime/legacy_ani_role_particle_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_role_particle_effect_test.cpp)：33, 68

## PORT-4ef90f0541e5

- [ ] `LegacyAniRoleParticlePorts`：抽象接口；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp:95](../../../include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp:95)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp)：95, 97, 118, 205
  - [src/asset_runtime/legacy_ani_role_particle_effect.cpp](../../../src/asset_runtime/legacy_ani_role_particle_effect.cpp)：298
  - [src/platform/sdl3/world_role_runtime_adapter.cpp](../../../src/platform/sdl3/world_role_runtime_adapter.cpp)：70
  - [src/platform/sdl3/world_role_runtime_adapter.hpp](../../../src/platform/sdl3/world_role_runtime_adapter.hpp)：9, 58, 108
  - [tests/unit/app/world_role_runtime_adapter_test.cpp](../../../tests/unit/app/world_role_runtime_adapter_test.cpp)：36, 197
  - [tests/unit/asset_runtime/legacy_ani_role_particle_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_role_particle_effect_test.cpp)：32, 90

## PORT-9a093152f449

- [ ] `LegacyAniRoleParticleRuntimePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp:117](../../../include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp:117)。
- 继承：`public LegacyAniRoleParticlePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp)：117, 120
  - [src/asset_runtime/legacy_ani_role_particle_effect.cpp](../../../src/asset_runtime/legacy_ani_role_particle_effect.cpp)：185, 198, 204, 230
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6555
  - [tests/unit/asset_runtime/legacy_ani_role_particle_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_role_particle_effect_test.cpp)：34, 876

## PORT-ccd99c810ed4

- [ ] `LegacyAniSparkServicePort`：抽象接口；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_spark_effect.hpp:42](../../../include/openswd3/asset_runtime/legacy_ani_spark_effect.hpp:42)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_spark_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_spark_effect.hpp)：42, 44, 78
  - [src/asset_runtime/legacy_ani_spark_effect.cpp](../../../src/asset_runtime/legacy_ani_spark_effect.cpp)：145
  - [src/world_map/legacy_world_frame_runtime.cpp](../../../src/world_map/legacy_world_frame_runtime.cpp)：39
  - [tests/unit/asset_runtime/legacy_ani_spark_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_spark_effect_test.cpp)：19, 31

## PORT-73b0991c193b

- [ ] `LegacyAniStreakServicePort`：抽象接口；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_ani_streak_effect.hpp:43](../../../include/openswd3/asset_runtime/legacy_ani_streak_effect.hpp:43)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_ani_streak_effect.hpp](../../../include/openswd3/asset_runtime/legacy_ani_streak_effect.hpp)：43, 45, 76
  - [src/asset_runtime/legacy_ani_streak_effect.cpp](../../../src/asset_runtime/legacy_ani_streak_effect.cpp)：98
  - [src/world_map/legacy_world_frame_runtime.cpp](../../../src/world_map/legacy_world_frame_runtime.cpp)：38
  - [tests/unit/asset_runtime/legacy_ani_streak_effect_test.cpp](../../../tests/unit/asset_runtime/legacy_ani_streak_effect_test.cpp)：19, 31

## PORT-5ec7908b0939

- [ ] `LegacyAssetCacheLimitPorts`：抽象接口；待复核。
- 定义：[include/openswd3/asset_runtime/legacy_asset_cache_limits.hpp:18](../../../include/openswd3/asset_runtime/legacy_asset_cache_limits.hpp:18)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/asset_runtime/legacy_asset_cache_limits.hpp](../../../include/openswd3/asset_runtime/legacy_asset_cache_limits.hpp)：18, 20, 32
  - [src/asset_runtime/legacy_asset_cache_limits.cpp](../../../src/asset_runtime/legacy_asset_cache_limits.cpp)：19
  - [tests/unit/asset_runtime/legacy_asset_cache_limits_test.cpp](../../../tests/unit/asset_runtime/legacy_asset_cache_limits_test.cpp)：10, 29

## PORT-7f9d00e1572c

- [ ] `LegacyAudioQueuePorts`：抽象接口；待复核。
- 定义：[include/openswd3/audio_video/legacy_audio_coordination.hpp:36](../../../include/openswd3/audio_video/legacy_audio_coordination.hpp:36)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/audio_video/legacy_audio_coordination.hpp](../../../include/openswd3/audio_video/legacy_audio_coordination.hpp)：36, 38, 59, 76
  - [src/audio_video/legacy_audio_coordination.cpp](../../../src/audio_video/legacy_audio_coordination.cpp)：10
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：387
  - [tests/support/legacy_audio_fixture.hpp](../../../tests/support/legacy_audio_fixture.hpp)：14, 29
  - [tests/unit/audio_video/legacy_audio_coordination_test.cpp](../../../tests/unit/audio_video/legacy_audio_coordination_test.cpp)：20

## PORT-cbc1b7b01790

- [ ] `LegacyVideoFramePorts`：抽象接口；待复核。
- 定义：[include/openswd3/audio_video/legacy_video.hpp:91](../../../include/openswd3/audio_video/legacy_video.hpp:91)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/audio_video/legacy_video.hpp](../../../include/openswd3/audio_video/legacy_video.hpp)：91, 93, 136
  - [src/audio_video/legacy_video.cpp](../../../src/audio_video/legacy_video.cpp)：97
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1730
  - [tests/unit/audio_video/legacy_sample_backend_sdl3_test.cpp](../../../tests/unit/audio_video/legacy_sample_backend_sdl3_test.cpp)：232
  - [tests/unit/audio_video/legacy_video_test.cpp](../../../tests/unit/audio_video/legacy_video_test.cpp)：17, 130

## PORT-77b3544a479a

- [ ] `LegacyBattleSummonFramePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_action_dispatch.hpp:160](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp:160)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：160, 162, 171, 1976, 1987
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：150
  - [src/battle/legacy_battle_action_dispatch.cpp](../../../src/battle/legacy_battle_action_dispatch.cpp)：547
  - [src/battle/legacy_battle_action_dispatch_effects.cpp](../../../src/battle/legacy_battle_action_dispatch_effects.cpp)：1655

## PORT-e3865412f2ac

- [ ] `LegacyBattleActionDispatchPort`：抽象接口；待复核。
- 首项行动消耗直接借party数据，删除该子链寄存器请求/回包和调用计数；未新增或保留行动转发Port。core/ASan定向、SDL及6源身份通过，源码df52c985已推送并核对远端；其余父协议仍pending。见[行动消耗证据](battle-actor-list-action-00470820.md#6-行动消耗父协议迁移)。
- 定义：[include/openswd3/battle/legacy_battle_action_dispatch.hpp:168](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp:168)。
- 继承：`public virtual LegacyBattleMonDatabasePort,
      public virtual LegacyBattleFixedObjectState,
      public virtual LegacyBattleSummonFramePort,
      public virtual LegacyBattleRetreatCommitPort,
      public virtual LegacyBattleActorMetricStatePort,
      public virtual LegacyBattlePairTransitionPort,
      public virtual LegacyBattleSharedPhaseStatePort,
      public virtual LegacyBattleOutcomeResolutionStatePort,
      public virtual LegacyBattleFrameRefreshStatePort,
      public virtual LegacyBattleFrameInputResolutionStatePort,
      public virtual LegacyBattleColorAccumulationStatePort,
      public virtual LegacyBattleEffectShiftStatePort,
      public virtual LegacyBattleFrameEffectControlStatePort,
      public virtual LegacyBattleGroupASummonMaterializationPort,
      public virtual world_map::LegacyWorldItemListStatePort`。
- 直接残留：invoke, register, callee, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：168, 185, 1771, 1781, 1790, 1808, 1817, 1827, 1837, 1847, 1857, 1876, 1886, 1897, 1908, 1918, 1928, 1939, 1950, 1960, 2023
  - [include/openswd3/battle/legacy_battle_actor_action_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_presentation.hpp)：26, 49
  - [include/openswd3/battle/legacy_battle_actor_frame_sequence.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_sequence.hpp)：11
  - [include/openswd3/battle/legacy_battle_actor_metrics.hpp](../../../include/openswd3/battle/legacy_battle_actor_metrics.hpp)：13, 195
  - [include/openswd3/battle/legacy_battle_final_actor_step.hpp](../../../include/openswd3/battle/legacy_battle_final_actor_step.hpp)：47
  - [include/openswd3/battle/legacy_battle_group_a_action_execution.hpp](../../../include/openswd3/battle/legacy_battle_group_a_action_execution.hpp)：13, 63
  - [include/openswd3/battle/legacy_battle_group_a_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_a_frame.hpp)：78, 89
  - [include/openswd3/battle/legacy_battle_group_a_reward_profile_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_reward_profile_application.hpp)：53
  - [include/openswd3/battle/legacy_battle_group_b_action_execution.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_execution.hpp)：9, 67
  - [include/openswd3/battle/legacy_battle_group_b_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_b_frame.hpp)：56
  - [include/openswd3/battle/legacy_battle_opponent_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_opponent_action_dispatch.hpp)：13, 23
  - [include/openswd3/battle/legacy_battle_outcome_finalization.hpp](../../../include/openswd3/battle/legacy_battle_outcome_finalization.hpp)：39
  - [include/openswd3/battle/legacy_battle_post_action.hpp](../../../include/openswd3/battle/legacy_battle_post_action.hpp)：26
  - [include/openswd3/battle/legacy_battle_target_ready.hpp](../../../include/openswd3/battle/legacy_battle_target_ready.hpp)：10, 68
  - [include/openswd3/battle/legacy_battle_victory_rewards.hpp](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp)：126
  - [src/battle/legacy_battle_action_dispatch.cpp](../../../src/battle/legacy_battle_action_dispatch.cpp)：7, 136, 220, 711, 886, 1196, 1490, 1700, 1797, 1978, 2296
  - [src/battle/legacy_battle_action_dispatch_effects.cpp](../../../src/battle/legacy_battle_action_dispatch_effects.cpp)：8, 336, 837, 977
  - [src/battle/legacy_battle_action_dispatch_internal.hpp](../../../src/battle/legacy_battle_action_dispatch_internal.hpp)：325, 388, 444, 471, 503, 522, 537, 560, 758
  - [src/battle/legacy_battle_action_dispatch_special.cpp](../../../src/battle/legacy_battle_action_dispatch_special.cpp)：10, 314, 483, 888, 1309
  - [src/battle/legacy_battle_actor_metrics.cpp](../../../src/battle/legacy_battle_actor_metrics.cpp)：283
  - [src/battle/legacy_battle_final_actor_step.cpp](../../../src/battle/legacy_battle_final_actor_step.cpp)：53, 63, 151, 174, 235, 435, 679
  - [src/battle/legacy_battle_group_a_action_execution.cpp](../../../src/battle/legacy_battle_group_a_action_execution.cpp)：69
  - [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：124, 152, 158, 215, 221, 265, 402, 538, 616, 769, 791, 871, 1163
  - [src/battle/legacy_battle_group_a_reward_profile_application.cpp](../../../src/battle/legacy_battle_group_a_reward_profile_application.cpp)：121
  - [src/battle/legacy_battle_group_b_action_execution.cpp](../../../src/battle/legacy_battle_group_b_action_execution.cpp)：103
  - [src/battle/legacy_battle_group_b_frame.cpp](../../../src/battle/legacy_battle_group_b_frame.cpp)：143, 277, 309, 460
  - [src/battle/legacy_battle_opponent_action_dispatch.cpp](../../../src/battle/legacy_battle_opponent_action_dispatch.cpp)：100, 136, 149, 202, 211, 262, 298
  - [src/battle/legacy_battle_post_action.cpp](../../../src/battle/legacy_battle_post_action.cpp)：65, 78
  - [src/battle/legacy_battle_target_ready.cpp](../../../src/battle/legacy_battle_target_ready.cpp)：67
  - [tests/unit/battle/legacy_battle_action_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_action_dispatch_test.cpp)：40, 82, 92, 102, 116, 127, 137, 149, 172, 182, 196, 212, 225
  - [tests/unit/battle/legacy_battle_actor_action_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_action_presentation_test.cpp)：28
  - [tests/unit/battle/legacy_battle_actor_metrics_test.cpp](../../../tests/unit/battle/legacy_battle_actor_metrics_test.cpp)：14, 26, 66, 376, 442
  - [tests/unit/battle/legacy_battle_final_actor_step_test.cpp](../../../tests/unit/battle/legacy_battle_final_actor_step_test.cpp)：18, 31
  - [tests/unit/battle/legacy_battle_frame_refresh_test.cpp](../../../tests/unit/battle/legacy_battle_frame_refresh_test.cpp)：16, 51
  - [tests/unit/battle/legacy_battle_group_a_action_execution_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_action_execution_test.cpp)：17, 20
  - [tests/unit/battle/legacy_battle_group_a_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_frame_test.cpp)：34
  - [tests/unit/battle/legacy_battle_group_a_reward_profile_application_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_reward_profile_application_test.cpp)：16, 22
  - [tests/unit/battle/legacy_battle_group_b_action_execution_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_action_execution_test.cpp)：22
  - [tests/unit/battle/legacy_battle_group_b_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_frame_test.cpp)：37
  - [tests/unit/battle/legacy_battle_opponent_action_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_opponent_action_dispatch_test.cpp)：30
  - [tests/unit/battle/legacy_battle_post_action_test.cpp](../../../tests/unit/battle/legacy_battle_post_action_test.cpp)：15, 19
  - [tests/unit/battle/legacy_battle_target_ready_test.cpp](../../../tests/unit/battle/legacy_battle_target_ready_test.cpp)：122, 742, 748
  - [tests/unit/battle/legacy_battle_transition_test.cpp](../../../tests/unit/battle/legacy_battle_transition_test.cpp)：102

## PORT-1cc3568ae19a

- [ ] `LegacyBattleOffsetActionFrameDrawStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_action_frame_draw.hpp:117](../../../include/openswd3/battle/legacy_battle_action_frame_draw.hpp:117)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_frame_draw.hpp](../../../include/openswd3/battle/legacy_battle_action_frame_draw.hpp)：117, 125
  - [include/openswd3/battle/legacy_battle_context_prompt.hpp](../../../include/openswd3/battle/legacy_battle_context_prompt.hpp)：16
  - [include/openswd3/battle/legacy_battle_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_grid_frame.hpp)：56
  - [include/openswd3/battle/legacy_battle_list_frame.hpp](../../../include/openswd3/battle/legacy_battle_list_frame.hpp)：34

## PORT-4d636c2d3ddb

- [ ] `LegacyBattleActionRotationUpdatePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_action_rotation_cache.hpp:23](../../../include/openswd3/battle/legacy_battle_action_rotation_cache.hpp:23)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_rotation_cache.hpp](../../../include/openswd3/battle/legacy_battle_action_rotation_cache.hpp)：23, 25, 34, 195, 208, 227
  - [include/openswd3/battle/legacy_battle_background_initialization.hpp](../../../include/openswd3/battle/legacy_battle_background_initialization.hpp)：120, 131
  - [include/openswd3/battle/legacy_battle_frame_effect.hpp](../../../include/openswd3/battle/legacy_battle_frame_effect.hpp)：69
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：636
  - [src/battle/legacy_battle_action_rotation_cache.cpp](../../../src/battle/legacy_battle_action_rotation_cache.cpp)：103, 327, 401
  - [src/battle/legacy_battle_background_initialization.cpp](../../../src/battle/legacy_battle_background_initialization.cpp)：131, 334
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：1168
  - [tests/unit/battle/legacy_battle_action_rotation_cache_test.cpp](../../../tests/unit/battle/legacy_battle_action_rotation_cache_test.cpp)：88
  - [tests/unit/battle/legacy_battle_background_initialization_test.cpp](../../../tests/unit/battle/legacy_battle_background_initialization_test.cpp)：16, 105
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：181

## PORT-2b5d57e26962

- [ ] `LegacyBattleActionUpdaterRotationPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_action_rotation_cache.hpp:33](../../../include/openswd3/battle/legacy_battle_action_rotation_cache.hpp:33)。
- 继承：`public LegacyBattleActionRotationUpdatePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_rotation_cache.hpp](../../../include/openswd3/battle/legacy_battle_action_rotation_cache.hpp)：33, 36
  - [src/battle/legacy_battle_action_rotation_cache.cpp](../../../src/battle/legacy_battle_action_rotation_cache.cpp)：77, 84
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：2980
  - [tests/unit/battle/legacy_battle_action_rotation_cache_test.cpp](../../../tests/unit/battle/legacy_battle_action_rotation_cache_test.cpp)：278, 301, 325, 367
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：2458

## PORT-646f55206409

- [ ] `LegacyBattleMutableFrameImagePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_action_rotation_cache.hpp:59](../../../include/openswd3/battle/legacy_battle_action_rotation_cache.hpp:59)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_rotation_cache.hpp](../../../include/openswd3/battle/legacy_battle_action_rotation_cache.hpp)：59, 61, 196
  - [include/openswd3/battle/legacy_battle_action_rotation_resources.hpp](../../../include/openswd3/battle/legacy_battle_action_rotation_resources.hpp)：16
  - [include/openswd3/battle/legacy_battle_background_initialization.hpp](../../../include/openswd3/battle/legacy_battle_background_initialization.hpp)：121, 132
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：637
  - [src/battle/legacy_battle_action_rotation_cache.cpp](../../../src/battle/legacy_battle_action_rotation_cache.cpp)：104
  - [src/battle/legacy_battle_background_initialization.cpp](../../../src/battle/legacy_battle_background_initialization.cpp)：132, 335
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：1169
  - [tests/unit/battle/legacy_battle_action_rotation_cache_test.cpp](../../../tests/unit/battle/legacy_battle_action_rotation_cache_test.cpp)：126
  - [tests/unit/battle/legacy_battle_background_initialization_test.cpp](../../../tests/unit/battle/legacy_battle_background_initialization_test.cpp)：24, 124
  - [tests/unit/battle/legacy_battle_frame_effect_test.cpp](../../../tests/unit/battle/legacy_battle_frame_effect_test.cpp)：73
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：182

## PORT-8bbb503d4e27

- [ ] `LegacyBattleActionRotationReleasePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_action_rotation_cache.hpp:67](../../../include/openswd3/battle/legacy_battle_action_rotation_cache.hpp:67)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_rotation_cache.hpp](../../../include/openswd3/battle/legacy_battle_action_rotation_cache.hpp)：67, 69, 220
  - [include/openswd3/battle/legacy_battle_action_rotation_resources.hpp](../../../include/openswd3/battle/legacy_battle_action_rotation_resources.hpp)：17
  - [include/openswd3/battle/legacy_battle_background_initialization.hpp](../../../include/openswd3/battle/legacy_battle_background_initialization.hpp)：119, 130
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：60
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：635
  - [src/battle/legacy_battle_action_rotation_cache.cpp](../../../src/battle/legacy_battle_action_rotation_cache.cpp)：297
  - [src/battle/legacy_battle_background_initialization.cpp](../../../src/battle/legacy_battle_background_initialization.cpp)：130, 333
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：1167
  - [tests/unit/battle/legacy_battle_action_rotation_cache_test.cpp](../../../tests/unit/battle/legacy_battle_action_rotation_cache_test.cpp)：170
  - [tests/unit/battle/legacy_battle_background_initialization_test.cpp](../../../tests/unit/battle/legacy_battle_background_initialization_test.cpp)：15, 83
  - [tests/unit/battle/legacy_battle_frame_effect_test.cpp](../../../tests/unit/battle/legacy_battle_frame_effect_test.cpp)：74
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：180

## PORT-aeaaca3cd801

- [ ] `LegacyBattleActionSummaryPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_action_summary.hpp:38](../../../include/openswd3/battle/legacy_battle_action_summary.hpp:38)。
- 继承：`public virtual LegacyBattleDebugHotkeyStatePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_summary.hpp](../../../include/openswd3/battle/legacy_battle_action_summary.hpp)：38, 41, 94
  - [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：132
  - [src/battle/legacy_battle_action_summary.cpp](../../../src/battle/legacy_battle_action_summary.cpp)：88, 118, 125
  - [tests/unit/battle/legacy_battle_action_summary_test.cpp](../../../tests/unit/battle/legacy_battle_action_summary_test.cpp)：19

## PORT-4020b11af967

- [ ] `LegacyBattleActorFrameHostHeapPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_host_heap.hpp:10](../../../include/openswd3/battle/legacy_battle_actor_frame_host_heap.hpp:10)。
- 继承：`public LegacyBattleActorFrameWin32AllocationPort`。
- 直接残留：register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_host_heap.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_host_heap.hpp)：10, 13
  - [src/battle/legacy_battle_actor_frame_host_heap.cpp](../../../src/battle/legacy_battle_actor_frame_host_heap.cpp)：12, 18
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：19000

## PORT-44e623b4acb6

- [ ] `LegacyBattleActorFrameHeapRetryPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:545](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:545)。
- 继承：`无`。
- 直接残留：register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：545, 547, 701
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：691

## PORT-7a299cc4c0e6

- [ ] `LegacyBattleActorFrameWin32AllocationPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:568](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:568)。
- 继承：`无`。
- 直接残留：register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_host_heap.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_host_heap.hpp)：11
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：568, 570, 682
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：649

## PORT-bce4a6a79a59

- [ ] `LegacyBattleActorFrameDecodePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:764](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:764)。
- 继承：`无`。
- 直接残留：register, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：764, 766, 1036, 1426, 1751
  - [src/battle/legacy_battle_actor_frame_presentation_decoder.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder.cpp)：19
  - [src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp)：750
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：610

## PORT-72f3184c4566

- [ ] `LegacyBattleActorFrameRectanglePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:787](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:787)。
- 继承：`无`。
- 直接残留：register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：787, 789, 1038, 1495
  - [src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp)：1042
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：757

## PORT-0711ed9a27e9

- [ ] `LegacyBattleActorFrameParticlePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:804](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:804)。
- 继承：`无`。
- 直接残留：register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：804, 806, 1039, 1528
  - [src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp)：1365
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：796

## PORT-116407424685

- [ ] `LegacyBattleActorFrameMetricsPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:822](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:822)。
- 继承：`无`。
- 直接残留：register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：822, 824, 1037, 1478, 2256
  - [src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp)：853
  - [src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp)：1365
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：724

## PORT-2580ef150bce

- [ ] `LegacyBattleActorFrameSoundPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:834](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:834)。
- 继承：`无`。
- 直接残留：register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：834, 836, 1034, 1306, 1511, 1563, 1681, 1765, 2008, 2173, 2305, 2346, 2429, 2541, 2612, 2628, 2642
  - [src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp)：1251, 1642
  - [src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp)：1001, 1505, 1609, 1641
  - [src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp)：314
  - [src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp)：1533
  - [src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp)：872, 1847, 2002, 2024, 2046, 2068
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：442

## PORT-6a1f12d149d4

- [ ] `LegacyBattleActorFrameLinkedNodeResolverPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:855](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:855)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：855, 857, 1032
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：594

## PORT-393f36387a26

- [ ] `LegacyBattleActorFrameReleasePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:864](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:864)。
- 继承：`无`。
- 直接残留：register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：864, 866, 1033, 1194, 1390, 2284
  - [src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_decoder.cpp)：1601
  - [src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp)：24, 2397
  - [tests/unit/battle/legacy_battle_actor_frame_free_validation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_free_validation_test.cpp)：15
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：555

## PORT-8a3cdbb661bb

- [ ] `LegacyBattleActorFrameDrawPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:881](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:881)。
- 继承：`无`。
- 直接残留：register, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：881, 883, 1035, 1342, 1588, 1702, 1869, 2368, 2396, 2451, 2479, 2663, 2684, 2716, 2739, 2773
  - [src/battle/legacy_battle_actor_frame_presentation_case_five.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_case_five.cpp)：429, 534, 949, 1039
  - [src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_decoder_continuations.cpp)：2042
  - [src/battle/legacy_battle_actor_frame_presentation_draw_arguments.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_draw_arguments.cpp)：312
  - [src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp)：2309, 2450, 2466, 2483, 2500, 2517
  - [src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp)：2040
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：478

## PORT-0e88be68f775

- [ ] `LegacyBattleActorFrameScaledRlePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:893](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:893)。
- 继承：`无`。
- 直接残留：register, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：893, 895, 1040, 2572
  - [src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_late_cases.cpp)：523
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：513

## PORT-1fa5cc29946f

- [ ] `LegacyBattleActorFrameUpdatePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:912](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:912)。
- 继承：`无`。
- 直接残留：register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：912, 914, 1031, 1205, 1216
  - [include/openswd3/battle/legacy_battle_actor_frame_tsw_lookup.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_tsw_lookup.hpp)：15, 19, 44
  - [src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp)：190, 451
  - [src/battle/legacy_battle_actor_frame_tsw_lookup.cpp](../../../src/battle/legacy_battle_actor_frame_tsw_lookup.cpp)：30
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：384
  - [tests/unit/battle/legacy_battle_actor_frame_tsw_declared_count_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_tsw_declared_count_test.cpp)：76

## PORT-323464d4f75c

- [ ] `LegacyBattleActorFrameEntryRoutePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:1029](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp:1029)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：1029, 1126, 1150, 1162
  - [src/battle/legacy_battle_action_dispatch_cases_low.cpp](../../../src/battle/legacy_battle_action_dispatch_cases_low.cpp)：778
  - [src/battle/legacy_battle_actor_frame_presentation.cpp](../../../src/battle/legacy_battle_actor_frame_presentation.cpp)：469, 609
  - [src/battle/legacy_battle_actor_frame_route_internal.hpp](../../../src/battle/legacy_battle_actor_frame_route_internal.hpp)：12, 19, 26, 33, 40, 47
  - [src/battle/legacy_battle_actor_frame_route_phase_1.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_1.cpp)：25
  - [src/battle/legacy_battle_actor_frame_route_phase_2.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_2.cpp)：25
  - [src/battle/legacy_battle_actor_frame_route_phase_3.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_3.cpp)：25
  - [src/battle/legacy_battle_actor_frame_route_phase_4.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_4.cpp)：25
  - [src/battle/legacy_battle_actor_frame_route_phase_5.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_5.cpp)：25
  - [src/battle/legacy_battle_actor_frame_route_phase_6.cpp](../../../src/battle/legacy_battle_actor_frame_route_phase_6.cpp)：25
  - [src/battle/legacy_battle_final_actor_step.cpp](../../../src/battle/legacy_battle_final_actor_step.cpp)：80
  - [src/battle/legacy_battle_opponent_action_dispatch.cpp](../../../src/battle/legacy_battle_opponent_action_dispatch.cpp)：743
  - [tests/unit/battle/legacy_battle_action_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_action_dispatch_test.cpp)：26, 596, 5419, 5454, 5565, 5649, 5681, 8267
  - [tests/unit/battle/legacy_battle_final_actor_step_test.cpp](../../../tests/unit/battle/legacy_battle_final_actor_step_test.cpp)：555, 612, 686, 768, 849, 917
  - [tests/unit/battle/legacy_battle_opponent_action_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_opponent_action_dispatch_test.cpp)：24, 389, 1360, 1387, 1489, 2278

## PORT-777564b100e8

- [ ] `LegacyBattleActorFrameTswUpdatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_frame_tsw_lookup.hpp:14](../../../include/openswd3/battle/legacy_battle_actor_frame_tsw_lookup.hpp:14)。
- 继承：`public LegacyBattleActorFrameUpdatePort`。
- 直接残留：register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_frame_tsw_lookup.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_tsw_lookup.hpp)：14, 17
  - [src/battle/legacy_battle_actor_frame_tsw_lookup.cpp](../../../src/battle/legacy_battle_actor_frame_tsw_lookup.cpp)：28, 34, 72
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：955, 1058, 1113, 1173, 1196, 1228, 1302, 3249, 5521, 5557, 5635, 5706, 5769, 5830, 5891, 5981
  - [tests/unit/battle/legacy_battle_actor_frame_tsw_declared_count_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_tsw_declared_count_test.cpp)：152, 193, 227

## PORT-e7050ee9a7d7

- [ ] `LegacyBattleActorListStatePort`：本批已删除；所属整链未验收。
- 本批证据：[状态处理及唯一菜单调用方](battle-actor-list-state-processing-00470380.md)。消息、音频改用实际函数与共享对象，旧状态Request/Reply及计数同时删除；core/ASan、SDL与21源身份已通过，源码bc6b14e7已推送并核对远端main一致。以下定义和引用为历史定位。
- 定义：[include/openswd3/battle/legacy_battle_actor_list_query.hpp:181](../../../include/openswd3/battle/legacy_battle_actor_list_query.hpp:181)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_list_query.hpp](../../../include/openswd3/battle/legacy_battle_actor_list_query.hpp)：181, 183, 426
  - [src/battle/legacy_battle_actor_list_query.cpp](../../../src/battle/legacy_battle_actor_list_query.cpp)：1418
  - [tests/unit/battle/legacy_battle_actor_list_query_test.cpp](../../../tests/unit/battle/legacy_battle_actor_list_query_test.cpp)：52

## PORT-395776ccb4e9

- [ ] `LegacyBattleActorResourceSelectionPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_list_query.hpp:199](../../../include/openswd3/battle/legacy_battle_actor_list_query.hpp:199)。
- 继承：`无`。
- 直接残留：保留只接收实际资源ID的同步平台诊断接口；旧Adapter及通用诊断操作已删除。
- 处理与验收记录：核心输入/消息两路显式借同一诊断对象，SDL窗口owner实现原维护错误对话框。平台保留候选依据及未决项见[诊断证据](battle-resource-selection-diagnostic-00431150.md)。完整SDL帧接线与实际交互未验收，暂不勾选合法保留。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_list_query.hpp](../../../include/openswd3/battle/legacy_battle_actor_list_query.hpp)：199, 201, 356
  - [src/battle/legacy_battle_actor_list_query.cpp](../../../src/battle/legacy_battle_actor_list_query.cpp)：1109
  - [src/battle/legacy_battle_target_selection_refresh.cpp](../../../src/battle/legacy_battle_target_selection_refresh.cpp)：36
  - [tests/unit/battle/legacy_battle_actor_list_query_test.cpp](../../../tests/unit/battle/legacy_battle_actor_list_query_test.cpp)：19

## PORT-a356b5049ace

- [ ] `LegacyBattleActorPublicationStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_metrics.hpp:56](../../../include/openswd3/battle/legacy_battle_actor_metrics.hpp:56)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_metrics.hpp](../../../include/openswd3/battle/legacy_battle_actor_metrics.hpp)：56, 69, 70
  - [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：52
  - [include/openswd3/battle/legacy_battle_pending_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_pending_action_commit.hpp)：48
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：148
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1718

## PORT-4b8940cc4813

- [ ] `LegacyBattleActorMetricStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_metrics.hpp:76](../../../include/openswd3/battle/legacy_battle_actor_metrics.hpp:76)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：173
  - [include/openswd3/battle/legacy_battle_actor_metrics.hpp](../../../include/openswd3/battle/legacy_battle_actor_metrics.hpp)：76, 114, 116, 121
  - [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：51
  - [include/openswd3/battle/legacy_battle_pending_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_pending_action_commit.hpp)：47
  - [include/openswd3/battle/legacy_battle_pre_frame.hpp](../../../include/openswd3/battle/legacy_battle_pre_frame.hpp)：53
  - [include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)：75
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：147
  - [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：116
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1717, 2093
  - [tests/unit/battle/legacy_battle_actor_metrics_test.cpp](../../../tests/unit/battle/legacy_battle_actor_metrics_test.cpp)：21, 38

## PORT-f13d46e6d951

- [ ] `LegacyBattleActorProfilePreparationPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_profile_preparation.hpp:18](../../../include/openswd3/battle/legacy_battle_actor_profile_preparation.hpp:18)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_profile_preparation.hpp](../../../include/openswd3/battle/legacy_battle_actor_profile_preparation.hpp)：18, 20, 66
  - [src/battle/legacy_battle_actor_profile_preparation.cpp](../../../src/battle/legacy_battle_actor_profile_preparation.cpp)：37
  - [tests/unit/battle/legacy_battle_actor_profile_preparation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_profile_preparation_test.cpp)：9

## PORT-f36643bb9d23

- [ ] `LegacyBattleActorReadyPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_ready.hpp:19](../../../include/openswd3/battle/legacy_battle_actor_ready.hpp:19)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_ready.hpp](../../../include/openswd3/battle/legacy_battle_actor_ready.hpp)：19, 21, 45
  - [include/openswd3/battle/legacy_battle_pending_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_pending_action_commit.hpp)：49
  - [src/battle/legacy_battle_actor_ready.cpp](../../../src/battle/legacy_battle_actor_ready.cpp)：9
  - [tests/unit/battle/legacy_battle_actor_ready_test.cpp](../../../tests/unit/battle/legacy_battle_actor_ready_test.cpp)：9

## PORT-78f64a54c45e

- [ ] `LegacyBattleActorStartupResetHeapPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_startup_reset.hpp:10](../../../include/openswd3/battle/legacy_battle_actor_startup_reset.hpp:10)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_startup_reset.hpp](../../../include/openswd3/battle/legacy_battle_actor_startup_reset.hpp)：10, 12, 58
  - [include/openswd3/battle/legacy_battle_group_a_startup_reset.hpp](../../../include/openswd3/battle/legacy_battle_group_a_startup_reset.hpp)：46
  - [include/openswd3/battle/legacy_battle_group_a_storage.hpp](../../../include/openswd3/battle/legacy_battle_group_a_storage.hpp)：29
  - [include/openswd3/battle/legacy_battle_group_b_startup_reset.hpp](../../../include/openswd3/battle/legacy_battle_group_b_startup_reset.hpp)：20
  - [include/openswd3/battle/legacy_battle_group_b_storage.hpp](../../../include/openswd3/battle/legacy_battle_group_b_storage.hpp)：35
  - [src/battle/legacy_battle_actor_startup_reset.cpp](../../../src/battle/legacy_battle_actor_startup_reset.cpp)：126
  - [src/battle/legacy_battle_group_a_startup_reset.cpp](../../../src/battle/legacy_battle_group_a_startup_reset.cpp)：213
  - [src/battle/legacy_battle_group_b_startup_reset.cpp](../../../src/battle/legacy_battle_group_b_startup_reset.cpp)：214
  - [tests/unit/battle/legacy_battle_actor_startup_reset_test.cpp](../../../tests/unit/battle/legacy_battle_actor_startup_reset_test.cpp)：19
  - [tests/unit/battle/legacy_battle_group_a_startup_reset_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_startup_reset_test.cpp)：22
  - [tests/unit/battle/legacy_battle_group_b_startup_reset_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_startup_reset_test.cpp)：29

## PORT-97d222967168

- [ ] `LegacyBattleActorStartupResetPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_startup_reset.hpp:22](../../../include/openswd3/battle/legacy_battle_actor_startup_reset.hpp:22)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_startup_reset.hpp](../../../include/openswd3/battle/legacy_battle_actor_startup_reset.hpp)：22, 24, 57
  - [src/battle/legacy_battle_actor_startup_reset.cpp](../../../src/battle/legacy_battle_actor_startup_reset.cpp)：125
  - [src/battle/legacy_battle_group_a_startup_reset.cpp](../../../src/battle/legacy_battle_group_a_startup_reset.cpp)：76
  - [src/battle/legacy_battle_group_b_startup_reset.cpp](../../../src/battle/legacy_battle_group_b_startup_reset.cpp)：78
  - [tests/unit/battle/legacy_battle_actor_startup_reset_test.cpp](../../../tests/unit/battle/legacy_battle_actor_startup_reset_test.cpp)：18

## PORT-e6008042ae5e

- [ ] `LegacyBattleActorTargetPreparationPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_actor_target_preparation.hpp:37](../../../include/openswd3/battle/legacy_battle_actor_target_preparation.hpp:37)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_target_preparation.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_preparation.hpp)：37, 39, 94
  - [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：131
  - [src/battle/legacy_battle_actor_target_preparation.cpp](../../../src/battle/legacy_battle_actor_target_preparation.cpp)：29
  - [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：121
  - [tests/unit/battle/legacy_battle_actor_target_preparation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_target_preparation_test.cpp)：16

## PORT-57b6e1832ed1

- [ ] `LegacyBattleBackgroundImageLoadPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_background_initialization.hpp:32](../../../include/openswd3/battle/legacy_battle_background_initialization.hpp:32)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_background_initialization.hpp](../../../include/openswd3/battle/legacy_battle_background_initialization.hpp)：32, 34, 44, 118
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：634
  - [src/battle/legacy_battle_background_initialization.cpp](../../../src/battle/legacy_battle_background_initialization.cpp)：129
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：1166
  - [tests/unit/battle/legacy_battle_background_initialization_test.cpp](../../../tests/unit/battle/legacy_battle_background_initialization_test.cpp)：18, 53
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：179

## PORT-a5f5ee600cd5

- [ ] `LegacyBattleArchiveBackgroundImageLoadPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_background_initialization.hpp:43](../../../include/openswd3/battle/legacy_battle_background_initialization.hpp:43)。
- 继承：`public LegacyBattleBackgroundImageLoadPort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_background_initialization.hpp](../../../include/openswd3/battle/legacy_battle_background_initialization.hpp)：43
  - [src/battle/legacy_battle_background_initialization.cpp](../../../src/battle/legacy_battle_background_initialization.cpp)：30, 339
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：2450

## PORT-16725e22a897

- [ ] `LegacyBattleColorAccumulationStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_color_accumulation.hpp:11](../../../include/openswd3/battle/legacy_battle_color_accumulation.hpp:11)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：179
  - [include/openswd3/battle/legacy_battle_color_accumulation.hpp](../../../include/openswd3/battle/legacy_battle_color_accumulation.hpp)：11, 34, 35
  - [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：53
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：43
  - [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：113
  - [tests/unit/battle/legacy_battle_script_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_script_dispatch_test.cpp)：119

## PORT-9c69fd704d9f

- [ ] `LegacyBattleContextPromptPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_context_prompt.hpp:15](../../../include/openswd3/battle/legacy_battle_context_prompt.hpp:15)。
- 继承：`public virtual LegacyBattleOffsetActionFrameDrawStatePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_context_prompt.hpp](../../../include/openswd3/battle/legacy_battle_context_prompt.hpp)：15, 18, 74
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：389
  - [src/battle/legacy_battle_context_prompt.cpp](../../../src/battle/legacy_battle_context_prompt.cpp)：47
  - [tests/unit/battle/legacy_battle_context_prompt_test.cpp](../../../tests/unit/battle/legacy_battle_context_prompt_test.cpp)：137, 159, 187, 212, 256, 276, 297, 323, 350

## PORT-3af2858d579f

- [ ] `LegacyBattleControlPanelFramePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_control_panel_frame.hpp:65](../../../include/openswd3/battle/legacy_battle_control_panel_frame.hpp:65)。
- 继承：`public virtual LegacyBattleGroupBActionItemOptionPort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_control_panel_frame.hpp](../../../include/openswd3/battle/legacy_battle_control_panel_frame.hpp)：65, 68, 167
  - [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：137
  - [src/battle/legacy_battle_control_panel_frame.cpp](../../../src/battle/legacy_battle_control_panel_frame.cpp)：44, 553, 567
  - [tests/unit/battle/legacy_battle_control_panel_frame_test.cpp](../../../tests/unit/battle/legacy_battle_control_panel_frame_test.cpp)：77

## PORT-f52d60a7b4cd

- [ ] `LegacyBattleDatabaseShutdownPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_database_shutdown.hpp:21](../../../include/openswd3/battle/legacy_battle_database_shutdown.hpp:21)。
- 继承：`public virtual LegacyBattleMonDatabasePort,
      public virtual LegacyBattleLevelDatabasePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_database_shutdown.hpp](../../../include/openswd3/battle/legacy_battle_database_shutdown.hpp)：21, 25, 55
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：40
  - [src/battle/legacy_battle_database_shutdown.cpp](../../../src/battle/legacy_battle_database_shutdown.cpp)：6
  - [tests/unit/battle/legacy_battle_database_shutdown_test.cpp](../../../tests/unit/battle/legacy_battle_database_shutdown_test.cpp)：10, 14

## PORT-55bac89def47

- [ ] `LegacyBattleDebugHotkeyPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_debug_hotkeys.hpp:59](../../../include/openswd3/battle/legacy_battle_debug_hotkeys.hpp:59)。
- 继承：`public virtual LegacyBattleDebugHotkeyStatePort,
      public virtual LegacyBattleOutcomeResolutionStatePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_debug_hotkeys.hpp](../../../include/openswd3/battle/legacy_battle_debug_hotkeys.hpp)：59, 63, 184
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：386
  - [src/battle/legacy_battle_debug_hotkeys.cpp](../../../src/battle/legacy_battle_debug_hotkeys.cpp)：101, 135, 143, 221, 460
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1713
  - [tests/unit/battle/legacy_battle_debug_hotkeys_test.cpp](../../../tests/unit/battle/legacy_battle_debug_hotkeys_test.cpp)：23, 40, 153, 1013

## PORT-3d40ca273a58

- [ ] `LegacyBattleDebugRecordPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_debug_hotkeys.hpp:75](../../../include/openswd3/battle/legacy_battle_debug_hotkeys.hpp:75)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_debug_hotkeys.hpp](../../../include/openswd3/battle/legacy_battle_debug_hotkeys.hpp)：75, 77, 93, 103
  - [src/battle/legacy_battle_debug_actor_value.cpp](../../../src/battle/legacy_battle_debug_actor_value.cpp)：24, 113, 122
  - [src/battle/legacy_battle_debug_hotkeys.cpp](../../../src/battle/legacy_battle_debug_hotkeys.cpp)：307
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1714
  - [tests/unit/battle/legacy_battle_debug_hotkeys_test.cpp](../../../tests/unit/battle/legacy_battle_debug_hotkeys_test.cpp)：154

## PORT-1054cd126300

- [ ] `LegacyBattleDebugOverlayPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_debug_overlay.hpp:57](../../../include/openswd3/battle/legacy_battle_debug_overlay.hpp:57)。
- 继承：`public virtual LegacyBattleDebugOverlayGateStatePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_debug_overlay.hpp](../../../include/openswd3/battle/legacy_battle_debug_overlay.hpp)：57, 60, 159
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：387
  - [src/battle/legacy_battle_debug_overlay.cpp](../../../src/battle/legacy_battle_debug_overlay.cpp)：118, 161, 184
  - [tests/unit/battle/legacy_battle_debug_overlay_test.cpp](../../../tests/unit/battle/legacy_battle_debug_overlay_test.cpp)：21, 34

## PORT-b505b28f9b0d

- [ ] `LegacyBattleDebugHotkeyStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_debug_state.hpp:25](../../../include/openswd3/battle/legacy_battle_debug_state.hpp:25)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_summary.hpp](../../../include/openswd3/battle/legacy_battle_action_summary.hpp)：39
  - [include/openswd3/battle/legacy_battle_debug_hotkeys.hpp](../../../include/openswd3/battle/legacy_battle_debug_hotkeys.hpp)：60
  - [include/openswd3/battle/legacy_battle_debug_state.hpp](../../../include/openswd3/battle/legacy_battle_debug_state.hpp)：25, 38, 39
  - [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：50
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：50
  - [include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)：76
  - [include/openswd3/battle/legacy_battle_script_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp)：291
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：143, 389
  - [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：109
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：1090, 1096, 1101

## PORT-1b5a6c2e0ec3

- [ ] `LegacyBattleDebugOverlayGateStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_debug_state.hpp:45](../../../include/openswd3/battle/legacy_battle_debug_state.hpp:45)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_debug_overlay.hpp](../../../include/openswd3/battle/legacy_battle_debug_overlay.hpp)：58
  - [include/openswd3/battle/legacy_battle_debug_state.hpp](../../../include/openswd3/battle/legacy_battle_debug_state.hpp)：45, 57, 58
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：51
  - [include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)：77

## PORT-75dcd66e85f6

- [ ] `LegacyBattleDefeatPanelPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_defeat_panel.hpp:58](../../../include/openswd3/battle/legacy_battle_defeat_panel.hpp:58)。
- 继承：`无`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_defeat_panel.hpp](../../../include/openswd3/battle/legacy_battle_defeat_panel.hpp)：58, 60, 120
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：160
  - [src/battle/legacy_battle_defeat_panel.cpp](../../../src/battle/legacy_battle_defeat_panel.cpp)：40, 268, 280
  - [tests/unit/battle/legacy_battle_defeat_panel_test.cpp](../../../tests/unit/battle/legacy_battle_defeat_panel_test.cpp)：93, 102
  - [tests/unit/battle/legacy_battle_message_phase_test.cpp](../../../tests/unit/battle/legacy_battle_message_phase_test.cpp)：260

## PORT-04aa251baab4

- [ ] `LegacyBattleDialogFormatPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_dialog_text.hpp:47](../../../include/openswd3/battle/legacy_battle_dialog_text.hpp:47)。
- 继承：`public virtual LegacyBattleMonDatabasePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_dialog_text.hpp](../../../include/openswd3/battle/legacy_battle_dialog_text.hpp)：47, 98
  - [src/battle/legacy_battle_dialog_text.cpp](../../../src/battle/legacy_battle_dialog_text.cpp)：235
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1706
  - [tests/unit/battle/legacy_battle_script_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_script_dispatch_test.cpp)：7831

## PORT-80b3c5932ebf

- [ ] `LegacyBattleDisplaySurfacePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_display_surfaces.hpp:12](../../../include/openswd3/battle/legacy_battle_display_surfaces.hpp:12)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_display_surface_runtime.hpp](../../../include/openswd3/battle/legacy_battle_display_surface_runtime.hpp)：22
  - [include/openswd3/battle/legacy_battle_display_surfaces.hpp](../../../include/openswd3/battle/legacy_battle_display_surfaces.hpp)：12, 14, 42, 48
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：142
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：652, 674

## PORT-6d055401c423

- [ ] `LegacyBattleEffectCoordinatorStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_effect_coordinator.hpp:69](../../../include/openswd3/battle/legacy_battle_effect_coordinator.hpp:69)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_effect_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_effect_coordinator.hpp)：69, 82, 83
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：397
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：49
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1715
  - [tests/unit/battle/legacy_battle_group_b_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_frame_test.cpp)：40

## PORT-9244ea9fae5b

- [ ] `LegacyBattleEffectCallPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_effect_frame.hpp:49](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp:49)。
- 继承：`public virtual LegacyBattleDebugHotkeyStatePort,
      public virtual LegacyBattleActorMetricStatePort,
      public virtual LegacyBattleActorPublicationStatePort,
      public virtual LegacyBattleColorAccumulationStatePort,
      public virtual LegacyBattleFrameEffectControlStatePort,
      public virtual LegacyBattlePairTransitionPort,
      public virtual LegacyBattleGroupARewardProfileStatePort,
      public virtual LegacyBattleSharedPhaseStatePort,
      public virtual LegacyBattleFrameRefreshStatePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_effect_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_effect_coordinator.hpp)：147
  - [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：49, 60, 233, 246
  - [include/openswd3/battle/legacy_battle_effect_shift.hpp](../../../include/openswd3/battle/legacy_battle_effect_shift.hpp)：9, 91
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：384, 399
  - [include/openswd3/battle/legacy_battle_group_a_effect_reward_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_effect_reward_application.hpp)：58
  - [include/openswd3/battle/legacy_battle_group_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_effect_frame.hpp)：93
  - [include/openswd3/battle/legacy_battle_reward_scale.hpp](../../../include/openswd3/battle/legacy_battle_reward_scale.hpp)：42
  - [src/battle/legacy_battle_effect_coordinator.cpp](../../../src/battle/legacy_battle_effect_coordinator.cpp)：113, 560, 585
  - [src/battle/legacy_battle_effect_frame.cpp](../../../src/battle/legacy_battle_effect_frame.cpp)：142, 263
  - [src/battle/legacy_battle_effect_shift.cpp](../../../src/battle/legacy_battle_effect_shift.cpp)：73, 213, 247
  - [src/battle/legacy_battle_group_a_effect_reward_application.cpp](../../../src/battle/legacy_battle_group_a_effect_reward_application.cpp)：80
  - [src/battle/legacy_battle_group_effect_frame.cpp](../../../src/battle/legacy_battle_group_effect_frame.cpp)：165
  - [src/battle/legacy_battle_intensity_effect_frame.cpp](../../../src/battle/legacy_battle_intensity_effect_frame.cpp)：65
  - [src/battle/legacy_battle_reward_scale.cpp](../../../src/battle/legacy_battle_reward_scale.cpp)：16
  - [tests/unit/battle/legacy_battle_effect_coordinator_test.cpp](../../../tests/unit/battle/legacy_battle_effect_coordinator_test.cpp)：29
  - [tests/unit/battle/legacy_battle_effect_frame_test.cpp](../../../tests/unit/battle/legacy_battle_effect_frame_test.cpp)：31
  - [tests/unit/battle/legacy_battle_effect_shift_test.cpp](../../../tests/unit/battle/legacy_battle_effect_shift_test.cpp)：17, 25
  - [tests/unit/battle/legacy_battle_frame_refresh_test.cpp](../../../tests/unit/battle/legacy_battle_frame_refresh_test.cpp)：17, 52
  - [tests/unit/battle/legacy_battle_group_a_effect_reward_application_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_effect_reward_application_test.cpp)：14, 20
  - [tests/unit/battle/legacy_battle_group_b_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_frame_test.cpp)：305
  - [tests/unit/battle/legacy_battle_group_effect_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_effect_frame_test.cpp)：50
  - [tests/unit/battle/legacy_battle_intensity_effect_frame_test.cpp](../../../tests/unit/battle/legacy_battle_intensity_effect_frame_test.cpp)：18

## PORT-f22ae89410fc

- [ ] `LegacyBattleEffectShiftStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_effect_shift.hpp:31](../../../include/openswd3/battle/legacy_battle_effect_shift.hpp:31)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：180
  - [include/openswd3/battle/legacy_battle_effect_shift.hpp](../../../include/openswd3/battle/legacy_battle_effect_shift.hpp)：31, 44, 45
  - [include/openswd3/battle/legacy_battle_pair_transition.hpp](../../../include/openswd3/battle/legacy_battle_pair_transition.hpp)：45
  - [include/openswd3/battle/legacy_battle_script_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp)：292
  - [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：110
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1716

## PORT-e59fcf9dbbd4

- [ ] `LegacyBattleFrameCoordinatorPort`：抽象接口；迁移中，未验收。
- 计数批次2de716db已验证推送：删除两种列表初始化操作、行上限回包及纯转发；最终core/ASan、SDL及12源身份通过。见[计数证据](battle-actor-list-count-004702e0.md)。
- 行查询批次95a3d125已验证推送：删除列表/窄网格查询与复位操作、值/文本回包及纯转发分支；其余通用操作与继承接口仍pending。见[行查询证据](battle-actor-list-query-00470180.md)。
- 定义：[include/openswd3/battle/legacy_battle_frame_coordinator.hpp:368](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp:368)。
- 继承：`public LegacyBattleHudCallPort,
      public LegacyBattleEffectCallPort,
      public LegacyBattlePreFramePort,
      public LegacyBattleDebugHotkeyPort,
      public LegacyBattleDebugOverlayPort,
      public LegacyBattleOutcomeResolutionPort,
      public LegacyBattleContextPromptPort,
      public LegacyBattleVerticalShiftPort,
      public LegacyBattlePendingActionPort,
      public LegacyBattleFrameInputResolutionPort,
      public virtual LegacyBattleFrameSurfacePort,
      public LegacyBattleSelectionFramePort,
      public LegacyBattleMessagePhasePort,
      public LegacyBattleTextMessageFramePort,
      public virtual LegacyBattleEffectCoordinatorStatePort`。
- 直接残留：invoke, register, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_metrics.hpp](../../../include/openswd3/battle/legacy_battle_actor_metrics.hpp)：14, 212
  - [include/openswd3/battle/legacy_battle_actor_priority.hpp](../../../include/openswd3/battle/legacy_battle_actor_priority.hpp)：10, 49
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：382, 402, 2068
  - [src/battle/legacy_battle_actor_metrics.cpp](../../../src/battle/legacy_battle_actor_metrics.cpp)：331
  - [src/battle/legacy_battle_actor_priority.cpp](../../../src/battle/legacy_battle_actor_priority.cpp)：144
  - [src/battle/legacy_battle_frame_coordinator.cpp](../../../src/battle/legacy_battle_frame_coordinator.cpp)：118
  - [tests/unit/battle/legacy_battle_actor_priority_test.cpp](../../../tests/unit/battle/legacy_battle_actor_priority_test.cpp)：16, 35
  - [tests/unit/battle/legacy_battle_frame_coordinator_test.cpp](../../../tests/unit/battle/legacy_battle_frame_coordinator_test.cpp)：115

## PORT-0bc8fbaacbda

- [ ] `LegacyBattleFrameEffectImagePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_frame_effect.hpp:27](../../../include/openswd3/battle/legacy_battle_frame_effect.hpp:27)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_effect.hpp](../../../include/openswd3/battle/legacy_battle_frame_effect.hpp)：27, 29, 36, 53
  - [tests/unit/battle/legacy_battle_frame_effect_test.cpp](../../../tests/unit/battle/legacy_battle_frame_effect_test.cpp)：103

## PORT-0fbfe71d8507

- [ ] `LegacyBattleBackgroundFrameEffectImagePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_frame_effect.hpp:35](../../../include/openswd3/battle/legacy_battle_frame_effect.hpp:35)。
- 继承：`public LegacyBattleFrameEffectImagePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_effect.hpp](../../../include/openswd3/battle/legacy_battle_frame_effect.hpp)：35, 38
  - [src/battle/legacy_battle_frame_coordinator.cpp](../../../src/battle/legacy_battle_frame_coordinator.cpp)：348
  - [src/battle/legacy_battle_frame_effect.cpp](../../../src/battle/legacy_battle_frame_effect.cpp)：349, 350, 357
  - [src/battle/legacy_battle_transition.cpp](../../../src/battle/legacy_battle_transition.cpp)：295
  - [tests/unit/battle/legacy_battle_frame_effect_test.cpp](../../../tests/unit/battle/legacy_battle_frame_effect_test.cpp)：125
  - [tests/unit/battle/legacy_battle_frame_refresh_test.cpp](../../../tests/unit/battle/legacy_battle_frame_refresh_test.cpp)：306
  - [tests/unit/battle/legacy_battle_script_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_script_dispatch_test.cpp)：6517, 6625, 6743

## PORT-6707630e445a

- [ ] `LegacyBattleFrameEffectPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_frame_effect.hpp:68](../../../include/openswd3/battle/legacy_battle_frame_effect.hpp:68)。
- 继承：`public LegacyBattleActionRotationUpdatePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：1905
  - [include/openswd3/battle/legacy_battle_frame_effect.hpp](../../../include/openswd3/battle/legacy_battle_frame_effect.hpp)：68, 71, 149
  - [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：114
  - [src/battle/legacy_battle_frame_effect.cpp](../../../src/battle/legacy_battle_frame_effect.cpp)：184, 202, 311, 383
  - [tests/unit/battle/legacy_battle_frame_coordinator_test.cpp](../../../tests/unit/battle/legacy_battle_frame_coordinator_test.cpp)：323
  - [tests/unit/battle/legacy_battle_frame_effect_test.cpp](../../../tests/unit/battle/legacy_battle_frame_effect_test.cpp)：18, 26
  - [tests/unit/battle/legacy_battle_frame_refresh_test.cpp](../../../tests/unit/battle/legacy_battle_frame_refresh_test.cpp)：18
  - [tests/unit/battle/legacy_battle_script_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_script_dispatch_test.cpp)：6441, 6558, 6672

## PORT-5e1aa8e7b00a

- [ ] `LegacyBattleFrameEffectControlStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_frame_effect_control.hpp:15](../../../include/openswd3/battle/legacy_battle_frame_effect_control.hpp:15)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：181
  - [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：54
  - [include/openswd3/battle/legacy_battle_frame_effect_control.hpp](../../../include/openswd3/battle/legacy_battle_frame_effect_control.hpp)：15, 28, 29
  - [include/openswd3/battle/legacy_battle_frame_refresh.hpp](../../../include/openswd3/battle/legacy_battle_frame_refresh.hpp)：32
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：45
  - [include/openswd3/battle/legacy_battle_input_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp)：155
  - [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：112
  - [tests/unit/battle/legacy_battle_script_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_script_dispatch_test.cpp)：121

## PORT-168328af62c0

- [ ] `LegacyBattleFrameInputResolutionPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_frame_input_resolution.hpp:57](../../../include/openswd3/battle/legacy_battle_frame_input_resolution.hpp:57)。
- 继承：`public virtual LegacyBattleFrameInputResolutionStatePort,
      public virtual LegacyBattleInputDispatchPort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：392
  - [include/openswd3/battle/legacy_battle_frame_input_resolution.hpp](../../../include/openswd3/battle/legacy_battle_frame_input_resolution.hpp)：57, 61, 245
  - [src/battle/legacy_battle_frame_input_resolution.cpp](../../../src/battle/legacy_battle_frame_input_resolution.cpp)：253
  - [tests/unit/battle/legacy_battle_frame_input_resolution_test.cpp](../../../tests/unit/battle/legacy_battle_frame_input_resolution_test.cpp)：60

## PORT-75c4b984e77a

- [ ] `LegacyBattleFrameInputResolutionStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_frame_input_resolution_state.hpp:54](../../../include/openswd3/battle/legacy_battle_frame_input_resolution_state.hpp:54)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：178
  - [include/openswd3/battle/legacy_battle_frame_input_resolution.hpp](../../../include/openswd3/battle/legacy_battle_frame_input_resolution.hpp)：58
  - [include/openswd3/battle/legacy_battle_frame_input_resolution_state.hpp](../../../include/openswd3/battle/legacy_battle_frame_input_resolution_state.hpp)：54, 67, 68
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：55
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：145
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1726

## PORT-6ea02dfb62aa

- [ ] `LegacyBattleFrameRefreshStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_frame_refresh.hpp:30](../../../include/openswd3/battle/legacy_battle_frame_refresh.hpp:30)。
- 继承：`public virtual LegacyBattleFrameSurfacePort,
      public virtual LegacyBattleFrameEffectControlStatePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：177
  - [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：58
  - [include/openswd3/battle/legacy_battle_frame_refresh.hpp](../../../include/openswd3/battle/legacy_battle_frame_refresh.hpp)：30, 80, 81, 109
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：44
  - [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：111
  - [src/battle/legacy_battle_frame_coordinator.cpp](../../../src/battle/legacy_battle_frame_coordinator.cpp)：130
  - [src/battle/legacy_battle_frame_refresh.cpp](../../../src/battle/legacy_battle_frame_refresh.cpp)：17, 21, 27, 33, 39, 46
  - [tests/support/legacy_battle_frame_refresh_fixture.hpp](../../../tests/support/legacy_battle_frame_refresh_fixture.hpp)：28
  - [tests/unit/battle/legacy_battle_script_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_script_dispatch_test.cpp)：120

## PORT-d4ae5c34c93e

- [ ] `LegacyBattleFrameSurfacePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_frame_surface.hpp:14](../../../include/openswd3/battle/legacy_battle_frame_surface.hpp:14)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：393
  - [include/openswd3/battle/legacy_battle_frame_refresh.hpp](../../../include/openswd3/battle/legacy_battle_frame_refresh.hpp)：31
  - [include/openswd3/battle/legacy_battle_frame_surface.hpp](../../../include/openswd3/battle/legacy_battle_frame_surface.hpp)：14, 16, 43, 50
  - [src/battle/legacy_battle_frame_surface.cpp](../../../src/battle/legacy_battle_frame_surface.cpp)：11, 15, 22
  - [tests/unit/battle/legacy_battle_frame_surface_test.cpp](../../../tests/unit/battle/legacy_battle_frame_surface_test.cpp)：16

## PORT-157307fdc89c

- [ ] `LegacyBattleGlobalResetRuntimePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_global_reset.hpp:39](../../../include/openswd3/battle/legacy_battle_global_reset.hpp:39)。
- 继承：`public LegacyBattleDatabaseShutdownPort,
      public LegacyBattleStartupPort,
      public virtual LegacyBattleRetreatCommitStatePort,
      public virtual LegacyBattleColorAccumulationStatePort,
      public virtual LegacyBattleFrameRefreshStatePort,
      public virtual LegacyBattleFrameEffectControlStatePort,
      public virtual LegacyBattlePairTransitionStatePort,
      public virtual LegacyBattleOutcomeResolutionStatePort,
      public virtual LegacyBattleOutcomeFinalizationStatePort,
      public virtual LegacyBattleEffectCoordinatorStatePort,
      public virtual LegacyBattleDebugHotkeyStatePort,
      public virtual LegacyBattleDebugOverlayGateStatePort,
      public virtual LegacyBattleVerticalShiftStatePort,
      public virtual LegacyBattleInputDispatchStatePort,
      public virtual LegacyBattleTargetSelectionRuntimeStatePort,
      public virtual LegacyBattleFrameInputResolutionStatePort,
      public virtual LegacyBattleSelectionFrameStatePort,
      public virtual LegacyBattleMessagePhaseStatePort,
      public virtual LegacyBattleVictoryRewardStatePort,
      public virtual LegacyBattleLevelAdvancementStatePort,
      public LegacyBattleActionRotationReleasePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：39, 62, 120
  - [src/battle/legacy_battle_global_reset.cpp](../../../src/battle/legacy_battle_global_reset.cpp)：590
  - [tests/unit/battle/legacy_battle_global_reset_test.cpp](../../../tests/unit/battle/legacy_battle_global_reset_test.cpp)：24, 78, 97

## PORT-da997d756887

- [ ] `LegacyBattleGridFramePort`：抽象接口；迁移中，未验收。
- 计数批次2de716db已验证推送：删除initialize_narrow_rows及行上限回包，实际窄列表直接累计共享BYTE；最终core/ASan、SDL及12源身份通过。见[计数证据](battle-actor-list-count-004702e0.md)。
- 行查询批次95a3d125已验证推送：删除query_narrow_row、refresh_actor和对应输出字段；窄网格直接查询共享列表及MON，字体和绘制协议仍pending。见[行查询证据](battle-actor-list-query-00470180.md)。
- 本批f21aa0b1已删除普通行查询操作与转发；core/ASan及SDL构建通过，远端main已核对。
- 剩余字体、绘制及其他网格操作仍使用invoke；不能将该Port作为合法边界勾选保留。
- 证据：[普通资源行查询](battle-actor-resource-list-query-00470910.md)。
- 定义：[include/openswd3/battle/legacy_battle_grid_frame.hpp:46](../../../include/openswd3/battle/legacy_battle_grid_frame.hpp:46)。
- 继承：`public virtual LegacyBattleOffsetActionFrameDrawStatePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_alternate_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_alternate_grid_frame.hpp)：112
  - [include/openswd3/battle/legacy_battle_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_grid_frame.hpp)：55, 58, 185
  - [include/openswd3/battle/legacy_battle_guard_panel_frame.hpp](../../../include/openswd3/battle/legacy_battle_guard_panel_frame.hpp)：91
  - [include/openswd3/battle/legacy_battle_mode_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_mode_grid_frame.hpp)：114
  - [include/openswd3/battle/legacy_battle_narrow_grid_frame.hpp](../../../include/openswd3/battle/legacy_battle_narrow_grid_frame.hpp)：107
  - [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：135
  - [src/battle/legacy_battle_alternate_grid_frame.cpp](../../../src/battle/legacy_battle_alternate_grid_frame.cpp)：78, 460, 473
  - [src/battle/legacy_battle_grid_frame.cpp](../../../src/battle/legacy_battle_grid_frame.cpp)：95, 619, 632
  - [src/battle/legacy_battle_guard_panel_frame.cpp](../../../src/battle/legacy_battle_guard_panel_frame.cpp)：66, 342, 354
  - [src/battle/legacy_battle_mode_grid_frame.cpp](../../../src/battle/legacy_battle_mode_grid_frame.cpp)：56, 469, 482
  - [src/battle/legacy_battle_narrow_grid_frame.cpp](../../../src/battle/legacy_battle_narrow_grid_frame.cpp)：78, 463, 476
  - [tests/unit/battle/legacy_battle_alternate_grid_frame_test.cpp](../../../tests/unit/battle/legacy_battle_alternate_grid_frame_test.cpp)：95
  - [tests/unit/battle/legacy_battle_grid_frame_test.cpp](../../../tests/unit/battle/legacy_battle_grid_frame_test.cpp)：110
  - [tests/unit/battle/legacy_battle_guard_panel_frame_test.cpp](../../../tests/unit/battle/legacy_battle_guard_panel_frame_test.cpp)：87
  - [tests/unit/battle/legacy_battle_mode_grid_frame_test.cpp](../../../tests/unit/battle/legacy_battle_mode_grid_frame_test.cpp)：94
  - [tests/unit/battle/legacy_battle_narrow_grid_frame_test.cpp](../../../tests/unit/battle/legacy_battle_narrow_grid_frame_test.cpp)：91

## PORT-54e9aba5bfc3

- [ ] `LegacyBattleGroupAAttributeAggregationPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp:78](../../../include/openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp:78)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp](../../../include/openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp)：78, 80, 131
  - [src/battle/legacy_battle_group_a_attribute_aggregation.cpp](../../../src/battle/legacy_battle_group_a_attribute_aggregation.cpp)：211
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：191
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1708
  - [tests/unit/battle/legacy_battle_group_a_attribute_aggregation_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_attribute_aggregation_test.cpp)：14, 21

## PORT-554bddc7c40e

- [ ] `LegacyBattleGroupAAttributeEffectPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_group_a_attribute_effect.hpp:43](../../../include/openswd3/battle/legacy_battle_group_a_attribute_effect.hpp:43)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_group_a_attribute_effect.hpp](../../../include/openswd3/battle/legacy_battle_group_a_attribute_effect.hpp)：43, 45, 92, 104
  - [src/battle/legacy_battle_group_a_attribute_effect.cpp](../../../src/battle/legacy_battle_group_a_attribute_effect.cpp)：59, 231
  - [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：219
  - [tests/unit/battle/legacy_battle_group_a_attribute_effect_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_attribute_effect_test.cpp)：15, 22

## PORT-ee048797c4f3

- [ ] `LegacyBattleGroupAConfigurationDiagnosticPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_group_a_configuration.hpp:74](../../../include/openswd3/battle/legacy_battle_group_a_configuration.hpp:74)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_group_a_configuration.hpp](../../../include/openswd3/battle/legacy_battle_group_a_configuration.hpp)：74, 76, 120, 137
  - [include/openswd3/battle/legacy_battle_group_a_storage.hpp](../../../include/openswd3/battle/legacy_battle_group_a_storage.hpp)：15, 44
  - [src/battle/legacy_battle_group_a_configuration.cpp](../../../src/battle/legacy_battle_group_a_configuration.cpp)：83, 207
  - [src/battle/legacy_battle_group_a_storage.cpp](../../../src/battle/legacy_battle_group_a_storage.cpp)：125
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：252
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1707
  - [tests/unit/battle/legacy_battle_group_a_configuration_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_configuration_test.cpp)：10, 38, 58
  - [tests/unit/battle/legacy_battle_group_a_startup_reset_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_startup_reset_test.cpp)：74

## PORT-e8a07778a7da

- [ ] `LegacyBattleGroupAFinalProcessingPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_group_a_final_processing.hpp:12](../../../include/openswd3/battle/legacy_battle_group_a_final_processing.hpp:12)。
- 继承：`public LegacyBattleGroupAItemEffectApplicationPort,
      public LegacyBattleGroupAProfileModeSelectionPort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_group_a_final_processing.hpp](../../../include/openswd3/battle/legacy_battle_group_a_final_processing.hpp)：12, 16, 97
  - [src/battle/legacy_battle_group_a_final_processing.cpp](../../../src/battle/legacy_battle_group_a_final_processing.cpp)：78
  - [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：156
  - [tests/unit/battle/legacy_battle_group_a_final_processing_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_final_processing_test.cpp)：10, 17

## PORT-7075003e8754

- [ ] `LegacyBattleGroupAItemEffectApplicationPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_group_a_item_effect_application.hpp:53](../../../include/openswd3/battle/legacy_battle_group_a_item_effect_application.hpp:53)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_group_a_final_processing.hpp](../../../include/openswd3/battle/legacy_battle_group_a_final_processing.hpp)：13
  - [include/openswd3/battle/legacy_battle_group_a_item_effect_application.hpp](../../../include/openswd3/battle/legacy_battle_group_a_item_effect_application.hpp)：53, 55, 103
  - [src/battle/legacy_battle_group_a_item_effect_application.cpp](../../../src/battle/legacy_battle_group_a_item_effect_application.cpp)：74
  - [tests/unit/battle/legacy_battle_group_a_item_effect_application_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_item_effect_application_test.cpp)：16, 23

## PORT-73eb1218ea44

- [ ] `LegacyBattleGroupAProfileModeSelectionPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp:16](../../../include/openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp:16)。
- 继承：`无`。
- 直接残留：register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_group_a_final_processing.hpp](../../../include/openswd3/battle/legacy_battle_group_a_final_processing.hpp)：14
  - [include/openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp)：16, 18, 58
  - [src/battle/legacy_battle_group_a_profile_mode_selection.cpp](../../../src/battle/legacy_battle_group_a_profile_mode_selection.cpp)：17
  - [tests/unit/battle/legacy_battle_group_a_profile_mode_selection_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_profile_mode_selection_test.cpp)：11, 14

## PORT-164486c15a77

- [ ] `LegacyBattleGroupARewardProfileStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_group_a_reward_profile_state.hpp:30](../../../include/openswd3/battle/legacy_battle_group_a_reward_profile_state.hpp:30)。
- 继承：`public virtual LegacyBattleFixedObjectState`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：56
  - [include/openswd3/battle/legacy_battle_group_a_reward_profile_state.hpp](../../../include/openswd3/battle/legacy_battle_group_a_reward_profile_state.hpp)：30, 44, 45
  - [include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp)：72
  - [include/openswd3/battle/legacy_battle_victory_rewards.hpp](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp)：124

## PORT-f12f61c2ae44

- [ ] `LegacyBattleGroupASummonMaterializationPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp:55](../../../include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp:55)。
- 继承：`public virtual LegacyBattleMonDatabasePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：182
  - [include/openswd3/battle/legacy_battle_group_a_npc_materialization.hpp](../../../include/openswd3/battle/legacy_battle_group_a_npc_materialization.hpp)：65, 79
  - [include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp](../../../include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp)：55, 58, 101
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：461
  - [src/battle/legacy_battle_group_a_npc_materialization.cpp](../../../src/battle/legacy_battle_group_a_npc_materialization.cpp)：105, 122, 310
  - [src/battle/legacy_battle_group_a_summon_materialization.cpp](../../../src/battle/legacy_battle_group_a_summon_materialization.cpp)：73, 88
  - [tests/unit/battle/legacy_battle_group_a_npc_materialization_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_npc_materialization_test.cpp)：15, 74
  - [tests/unit/battle/legacy_battle_group_a_summon_materialization_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_summon_materialization_test.cpp)：15, 56

## PORT-8cc7663143f8

- [ ] `LegacyBattleGroupBActionCompositionPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_group_b_action_composition.hpp:41](../../../include/openswd3/battle/legacy_battle_group_b_action_composition.hpp:41)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_group_b_action_composition.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_composition.hpp)：41, 43, 88
  - [src/battle/legacy_battle_action_dispatch_internal.hpp](../../../src/battle/legacy_battle_action_dispatch_internal.hpp)：386
  - [src/battle/legacy_battle_group_b_action_composition.cpp](../../../src/battle/legacy_battle_group_b_action_composition.cpp)：35
  - [src/battle/legacy_battle_script_dispatch_internal.hpp](../../../src/battle/legacy_battle_script_dispatch_internal.hpp)：867
  - [tests/unit/battle/legacy_battle_group_b_action_composition_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_action_composition_test.cpp)：17, 29

## PORT-911a059381af

- [ ] `LegacyBattleGroupBActionItemOptionPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_group_b_action_item_option.hpp:24](../../../include/openswd3/battle/legacy_battle_group_b_action_item_option.hpp:24)。
- 继承：`public virtual LegacyBattleGroupBActionItemSelectionPort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_control_panel_frame.hpp](../../../include/openswd3/battle/legacy_battle_control_panel_frame.hpp)：66
  - [include/openswd3/battle/legacy_battle_group_b_action_item_option.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_option.hpp)：24, 27, 79
  - [include/openswd3/battle/legacy_battle_group_b_action_item_special_option.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_special_option.hpp)：47
  - [include/openswd3/battle/legacy_battle_input_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp)：157
  - [src/battle/legacy_battle_group_b_action_item_option.cpp](../../../src/battle/legacy_battle_group_b_action_item_option.cpp)：29
  - [src/battle/legacy_battle_group_b_action_item_special_option.cpp](../../../src/battle/legacy_battle_group_b_action_item_special_option.cpp)：28
  - [tests/unit/battle/legacy_battle_group_b_action_item_option_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_action_item_option_test.cpp)：44
  - [tests/unit/battle/legacy_battle_group_b_action_item_special_option_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_action_item_special_option_test.cpp)：16, 22

## PORT-efe72baa43ee

- [ ] `LegacyBattleGroupBActionItemSelectionPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp:9](../../../include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp:9)。
- 继承：`public virtual LegacyBattleMonDatabasePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_group_b_action_item_option.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_option.hpp)：25
  - [include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp)：9, 12, 60
  - [src/battle/legacy_battle_group_b_action_item_selection.cpp](../../../src/battle/legacy_battle_group_b_action_item_selection.cpp)：72
  - [src/battle/legacy_battle_message_phase.cpp](../../../src/battle/legacy_battle_message_phase.cpp)：51
  - [tests/unit/battle/legacy_battle_group_b_action_item_selection_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_action_item_selection_test.cpp)：44

## PORT-5c656e4a608b

- [ ] `LegacyBattleGroupBActionSeventeenFramePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp:47](../../../include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp:47)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp)：47, 49, 125
  - [src/battle/legacy_battle_group_b_action_seventeen_frame.cpp](../../../src/battle/legacy_battle_group_b_action_seventeen_frame.cpp)：135
  - [src/battle/legacy_battle_opponent_action_dispatch.cpp](../../../src/battle/legacy_battle_opponent_action_dispatch.cpp)：133
  - [tests/unit/battle/legacy_battle_group_b_action_seventeen_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_action_seventeen_frame_test.cpp)：95

## PORT-b407b49d5c4a

- [ ] `LegacyBattleGrowthActorSelectionStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_growth_actor_selection.hpp:42](../../../include/openswd3/battle/legacy_battle_growth_actor_selection.hpp:42)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_growth_actor_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_actor_selection.hpp)：42, 55, 56, 101
  - [include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp)：71

## PORT-9eb33b05d668

- [ ] `LegacyBattleGrowthActorSelectionPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_growth_actor_selection.hpp:99](../../../include/openswd3/battle/legacy_battle_growth_actor_selection.hpp:99)。
- 继承：`public virtual LegacyBattleMonDatabasePort,
      public virtual LegacyBattleGrowthActorSelectionStatePort,
      public virtual world_map::LegacyWorldItemListStatePort`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_growth_actor_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_actor_selection.hpp)：99, 104, 169
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：155
  - [src/battle/legacy_battle_growth_actor_selection.cpp](../../../src/battle/legacy_battle_growth_actor_selection.cpp)：43, 515, 531
  - [tests/unit/battle/legacy_battle_growth_actor_selection_test.cpp](../../../tests/unit/battle/legacy_battle_growth_actor_selection_test.cpp)：53, 62
  - [tests/unit/battle/legacy_battle_message_phase_test.cpp](../../../tests/unit/battle/legacy_battle_message_phase_test.cpp)：187

## PORT-c72a604940f2

- [ ] `LegacyBattleGrowthCaptionPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_growth_caption.hpp:68](../../../include/openswd3/battle/legacy_battle_growth_caption.hpp:68)。
- 继承：`无`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_growth_caption.hpp](../../../include/openswd3/battle/legacy_battle_growth_caption.hpp)：68, 70, 156
  - [include/openswd3/battle/legacy_battle_growth_completion_caption.hpp](../../../include/openswd3/battle/legacy_battle_growth_completion_caption.hpp)：13
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：156
  - [src/battle/legacy_battle_growth_caption.cpp](../../../src/battle/legacy_battle_growth_caption.cpp)：54, 430, 446
  - [src/battle/legacy_battle_growth_completion_caption.cpp](../../../src/battle/legacy_battle_growth_completion_caption.cpp)：7
  - [tests/unit/battle/legacy_battle_growth_caption_test.cpp](../../../tests/unit/battle/legacy_battle_growth_caption_test.cpp)：96, 107
  - [tests/unit/battle/legacy_battle_message_phase_test.cpp](../../../tests/unit/battle/legacy_battle_message_phase_test.cpp)：339

## PORT-7d32b439f489

- [ ] `LegacyBattleGrowthItemCompletionPanelPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp:65](../../../include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp:65)。
- 继承：`无`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_completion_panel.hpp)：65, 67, 150
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：157
  - [src/battle/legacy_battle_growth_item_completion_panel.cpp](../../../src/battle/legacy_battle_growth_item_completion_panel.cpp)：51, 322, 339
  - [tests/unit/battle/legacy_battle_growth_item_completion_panel_test.cpp](../../../tests/unit/battle/legacy_battle_growth_item_completion_panel_test.cpp)：63, 75
  - [tests/unit/battle/legacy_battle_message_phase_test.cpp](../../../tests/unit/battle/legacy_battle_message_phase_test.cpp)：321

## PORT-fd5db5506d60

- [ ] `LegacyBattleGrowthItemResultSelectionPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp:69](../../../include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp:69)。
- 继承：`public virtual LegacyBattleMonDatabasePort,
      public virtual LegacyBattleGrowthActorSelectionStatePort,
      public virtual LegacyBattleGroupARewardProfileStatePort`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp)：69, 74, 132
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：158
  - [src/battle/legacy_battle_growth_item_result_selection.cpp](../../../src/battle/legacy_battle_growth_item_result_selection.cpp)：37, 293, 306
  - [tests/unit/battle/legacy_battle_growth_item_result_selection_test.cpp](../../../tests/unit/battle/legacy_battle_growth_item_result_selection_test.cpp)：23, 37
  - [tests/unit/battle/legacy_battle_message_phase_test.cpp](../../../tests/unit/battle/legacy_battle_message_phase_test.cpp)：213

## PORT-37047085d1fd

- [ ] `LegacyBattleHudCallPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_hud_frame.hpp:30](../../../include/openswd3/battle/legacy_battle_hud_frame.hpp:30)。
- 继承：`public LegacyBattleTextPanelPort,
      public virtual LegacyBattleVictoryRewardStatePort`。
- 直接残留：invoke, register, callee, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：383
  - [include/openswd3/battle/legacy_battle_hud_frame.hpp](../../../include/openswd3/battle/legacy_battle_hud_frame.hpp)：30, 34, 152
  - [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：115
  - [src/battle/legacy_battle_hud_frame.cpp](../../../src/battle/legacy_battle_hud_frame.cpp)：145
  - [tests/unit/battle/legacy_battle_hud_frame_test.cpp](../../../tests/unit/battle/legacy_battle_hud_frame_test.cpp)：17

## PORT-61523c9cd1bf

- [ ] `LegacyBattleInputDispatchStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_input_dispatch.hpp:76](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp:76)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：53
  - [include/openswd3/battle/legacy_battle_input_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp)：76, 89, 90, 154
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1719

## PORT-390b7e9ca4de

- [ ] `LegacyBattleInputDispatchPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_input_dispatch.hpp:152](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp:152)。
- 继承：`public virtual LegacyBattleMonDatabasePort,
      public virtual LegacyBattleInputDispatchStatePort,
      public virtual LegacyBattleFrameEffectControlStatePort,
      public virtual LegacyBattleTargetSelectionRuntimePort,
      public virtual LegacyBattleGroupBActionItemOptionPort`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_mode_refresh.hpp](../../../include/openswd3/battle/legacy_battle_action_mode_refresh.hpp)：68
  - [include/openswd3/battle/legacy_battle_actor_action_candidate_availability.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_candidate_availability.hpp)：40
  - [include/openswd3/battle/legacy_battle_actor_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_commit.hpp)：44
  - [include/openswd3/battle/legacy_battle_actor_action_cycle.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_cycle.hpp)：43
  - [include/openswd3/battle/legacy_battle_actor_action_reverse_cycle.hpp](../../../include/openswd3/battle/legacy_battle_actor_action_reverse_cycle.hpp)：43
  - [include/openswd3/battle/legacy_battle_available_actor_cycle.hpp](../../../include/openswd3/battle/legacy_battle_available_actor_cycle.hpp)：55
  - [include/openswd3/battle/legacy_battle_available_actor_reverse_cycle.hpp](../../../include/openswd3/battle/legacy_battle_available_actor_reverse_cycle.hpp)：12
  - [include/openswd3/battle/legacy_battle_frame_input_resolution.hpp](../../../include/openswd3/battle/legacy_battle_frame_input_resolution.hpp)：59
  - [include/openswd3/battle/legacy_battle_input_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp)：152, 159, 406
  - [include/openswd3/battle/legacy_battle_menu_context_advance.hpp](../../../include/openswd3/battle/legacy_battle_menu_context_advance.hpp)：46
  - [include/openswd3/battle/legacy_battle_menu_context_retreat.hpp](../../../include/openswd3/battle/legacy_battle_menu_context_retreat.hpp)：46
  - [include/openswd3/battle/legacy_battle_menu_input_finalize.hpp](../../../include/openswd3/battle/legacy_battle_menu_input_finalize.hpp)：50
  - [include/openswd3/battle/legacy_battle_menu_page_advance.hpp](../../../include/openswd3/battle/legacy_battle_menu_page_advance.hpp)：40
  - [include/openswd3/battle/legacy_battle_menu_page_retreat.hpp](../../../include/openswd3/battle/legacy_battle_menu_page_retreat.hpp)：40
  - [include/openswd3/battle/legacy_battle_menu_selection_advance.hpp](../../../include/openswd3/battle/legacy_battle_menu_selection_advance.hpp)：63
  - [include/openswd3/battle/legacy_battle_menu_selection_retreat.hpp](../../../include/openswd3/battle/legacy_battle_menu_selection_retreat.hpp)：63
  - [include/openswd3/battle/legacy_battle_target_selection_entry.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_entry.hpp)：80
  - [include/openswd3/battle/legacy_battle_target_selection_refresh.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_refresh.hpp)：156
  - [include/openswd3/battle/legacy_battle_victory_rewards.hpp](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp)：125
  - [src/battle/legacy_battle_action_mode_refresh.cpp](../../../src/battle/legacy_battle_action_mode_refresh.cpp)：110
  - [src/battle/legacy_battle_action_summary.cpp](../../../src/battle/legacy_battle_action_summary.cpp)：86
  - [src/battle/legacy_battle_actor_action_candidate_availability.cpp](../../../src/battle/legacy_battle_actor_action_candidate_availability.cpp)：19
  - [src/battle/legacy_battle_actor_action_commit.cpp](../../../src/battle/legacy_battle_actor_action_commit.cpp)：16
  - [src/battle/legacy_battle_actor_action_cycle.cpp](../../../src/battle/legacy_battle_actor_action_cycle.cpp)：18
  - [src/battle/legacy_battle_actor_action_reverse_cycle.cpp](../../../src/battle/legacy_battle_actor_action_reverse_cycle.cpp)：19
  - [src/battle/legacy_battle_available_actor_cycle.cpp](../../../src/battle/legacy_battle_available_actor_cycle.cpp)：12
  - [src/battle/legacy_battle_available_actor_reverse_cycle.cpp](../../../src/battle/legacy_battle_available_actor_reverse_cycle.cpp)：14
  - [src/battle/legacy_battle_input_dispatch.cpp](../../../src/battle/legacy_battle_input_dispatch.cpp)：31, 50, 246
  - [src/battle/legacy_battle_menu_context_advance.cpp](../../../src/battle/legacy_battle_menu_context_advance.cpp)：38
  - [src/battle/legacy_battle_menu_context_retreat.cpp](../../../src/battle/legacy_battle_menu_context_retreat.cpp)：38
  - [src/battle/legacy_battle_menu_input_finalize.cpp](../../../src/battle/legacy_battle_menu_input_finalize.cpp)：35
  - [src/battle/legacy_battle_menu_page_advance.cpp](../../../src/battle/legacy_battle_menu_page_advance.cpp)：26
  - [src/battle/legacy_battle_menu_page_retreat.cpp](../../../src/battle/legacy_battle_menu_page_retreat.cpp)：21
  - [src/battle/legacy_battle_menu_selection_advance.cpp](../../../src/battle/legacy_battle_menu_selection_advance.cpp)：57
  - [src/battle/legacy_battle_menu_selection_retreat.cpp](../../../src/battle/legacy_battle_menu_selection_retreat.cpp)：57
  - [src/battle/legacy_battle_target_selection_entry.cpp](../../../src/battle/legacy_battle_target_selection_entry.cpp)：87
  - [src/battle/legacy_battle_target_selection_refresh.cpp](../../../src/battle/legacy_battle_target_selection_refresh.cpp)：38, 50, 55, 74, 95, 2283, 2350
  - [tests/unit/battle/legacy_battle_action_mode_refresh_test.cpp](../../../tests/unit/battle/legacy_battle_action_mode_refresh_test.cpp)：16
  - [tests/unit/battle/legacy_battle_actor_action_candidate_availability_test.cpp](../../../tests/unit/battle/legacy_battle_actor_action_candidate_availability_test.cpp)：16
  - [tests/unit/battle/legacy_battle_actor_action_commit_test.cpp](../../../tests/unit/battle/legacy_battle_actor_action_commit_test.cpp)：17
  - [tests/unit/battle/legacy_battle_actor_action_cycle_test.cpp](../../../tests/unit/battle/legacy_battle_actor_action_cycle_test.cpp)：17
  - [tests/unit/battle/legacy_battle_actor_action_reverse_cycle_test.cpp](../../../tests/unit/battle/legacy_battle_actor_action_reverse_cycle_test.cpp)：17
  - [tests/unit/battle/legacy_battle_available_actor_cycle_test.cpp](../../../tests/unit/battle/legacy_battle_available_actor_cycle_test.cpp)：15
  - [tests/unit/battle/legacy_battle_available_actor_reverse_cycle_test.cpp](../../../tests/unit/battle/legacy_battle_available_actor_reverse_cycle_test.cpp)：15
  - [tests/unit/battle/legacy_battle_group_a_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_frame_test.cpp)：3356
  - [tests/unit/battle/legacy_battle_input_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_input_dispatch_test.cpp)：47
  - [tests/unit/battle/legacy_battle_menu_context_advance_test.cpp](../../../tests/unit/battle/legacy_battle_menu_context_advance_test.cpp)：23
  - [tests/unit/battle/legacy_battle_menu_context_retreat_test.cpp](../../../tests/unit/battle/legacy_battle_menu_context_retreat_test.cpp)：23
  - [tests/unit/battle/legacy_battle_menu_input_finalize_test.cpp](../../../tests/unit/battle/legacy_battle_menu_input_finalize_test.cpp)：18
  - [tests/unit/battle/legacy_battle_menu_page_advance_test.cpp](../../../tests/unit/battle/legacy_battle_menu_page_advance_test.cpp)：15
  - [tests/unit/battle/legacy_battle_menu_page_retreat_test.cpp](../../../tests/unit/battle/legacy_battle_menu_page_retreat_test.cpp)：15
  - [tests/unit/battle/legacy_battle_menu_selection_advance_test.cpp](../../../tests/unit/battle/legacy_battle_menu_selection_advance_test.cpp)：49
  - [tests/unit/battle/legacy_battle_menu_selection_retreat_test.cpp](../../../tests/unit/battle/legacy_battle_menu_selection_retreat_test.cpp)：49
  - [tests/unit/battle/legacy_battle_target_selection_entry_test.cpp](../../../tests/unit/battle/legacy_battle_target_selection_entry_test.cpp)：49
  - [tests/unit/battle/legacy_battle_target_selection_refresh_test.cpp](../../../tests/unit/battle/legacy_battle_target_selection_refresh_test.cpp)：50

## PORT-9bdb1bd31ce7

- [ ] `LegacyBattleLevelAdvancementStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_level_advancement.hpp:35](../../../include/openswd3/battle/legacy_battle_level_advancement.hpp:35)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：59
  - [include/openswd3/battle/legacy_battle_level_advancement.hpp](../../../include/openswd3/battle/legacy_battle_level_advancement.hpp)：35, 48, 49, 91
  - [include/openswd3/battle/legacy_battle_script_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp)：294

## PORT-587465dbdb06

- [ ] `LegacyBattleLevelAdvancementPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_level_advancement.hpp:90](../../../include/openswd3/battle/legacy_battle_level_advancement.hpp:90)。
- 继承：`public virtual LegacyBattleLevelAdvancementStatePort,
      public virtual LegacyBattleLevelProfilePort`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_level_advancement.hpp](../../../include/openswd3/battle/legacy_battle_level_advancement.hpp)：90, 94, 195
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：153
  - [src/battle/legacy_battle_level_advancement.cpp](../../../src/battle/legacy_battle_level_advancement.cpp)：48, 379, 393
  - [tests/unit/battle/legacy_battle_level_advancement_test.cpp](../../../tests/unit/battle/legacy_battle_level_advancement_test.cpp)：22

## PORT-4da57ddbeb5f

- [ ] `LegacyBattleLevelGrowthPanelPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_level_growth_panel.hpp:72](../../../include/openswd3/battle/legacy_battle_level_growth_panel.hpp:72)。
- 继承：`无`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_level_growth_panel.hpp](../../../include/openswd3/battle/legacy_battle_level_growth_panel.hpp)：72, 74, 155
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：154
  - [src/battle/legacy_battle_level_growth_panel.cpp](../../../src/battle/legacy_battle_level_growth_panel.cpp)：119, 704, 719
  - [tests/unit/battle/legacy_battle_level_growth_panel_test.cpp](../../../tests/unit/battle/legacy_battle_level_growth_panel_test.cpp)：96, 108
  - [tests/unit/battle/legacy_battle_message_phase_test.cpp](../../../tests/unit/battle/legacy_battle_message_phase_test.cpp)：162

## PORT-f7ecb1eddb59

- [ ] `LegacyBattleLevelProfilePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_level_profile.hpp:56](../../../include/openswd3/battle/legacy_battle_level_profile.hpp:56)。
- 继承：`public virtual LegacyBattleLevelDatabasePort,
      public virtual LegacyBattleMonDatabasePort`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_level_advancement.hpp](../../../include/openswd3/battle/legacy_battle_level_advancement.hpp)：92
  - [include/openswd3/battle/legacy_battle_level_profile.hpp](../../../include/openswd3/battle/legacy_battle_level_profile.hpp)：56, 60, 186
  - [src/battle/legacy_battle_level_profile.cpp](../../../src/battle/legacy_battle_level_profile.cpp)：57, 508, 524
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1727
  - [tests/unit/battle/legacy_battle_level_profile_test.cpp](../../../tests/unit/battle/legacy_battle_level_profile_test.cpp)：32, 60, 70

## PORT-c7528372200b

- [ ] `LegacyBattleLevelDatabasePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_level_requirement.hpp:58](../../../include/openswd3/battle/legacy_battle_level_requirement.hpp:58)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_database_shutdown.hpp](../../../include/openswd3/battle/legacy_battle_database_shutdown.hpp)：23
  - [include/openswd3/battle/legacy_battle_level_profile.hpp](../../../include/openswd3/battle/legacy_battle_level_profile.hpp)：57
  - [include/openswd3/battle/legacy_battle_level_requirement.hpp](../../../include/openswd3/battle/legacy_battle_level_requirement.hpp)：58, 60, 139
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：824, 1259
  - [include/openswd3/world_map/legacy_world_story_vm.hpp](../../../include/openswd3/world_map/legacy_world_story_vm.hpp)：253
  - [src/battle/legacy_battle_level_requirement.cpp](../../../src/battle/legacy_battle_level_requirement.cpp)：168, 173, 186
  - [tests/support/legacy_battle_level_database_fixture.hpp](../../../tests/support/legacy_battle_level_database_fixture.hpp)：16
  - [tests/unit/battle/legacy_battle_level_requirement_test.cpp](../../../tests/unit/battle/legacy_battle_level_requirement_test.cpp)：15, 30

## PORT-5273e66ab0e8

- [ ] `LegacyBattleListContentsPort`：抽象接口；迁移中，未验收。
- 计数批次2de716db已验证推送：删除initialize_rows、行上限回包和初始化次数，实际列表直接累计共享BYTE；最终core/ASan、SDL及12源身份通过。见[计数证据](battle-actor-list-count-004702e0.md)。
- 行查询批次95a3d125已验证推送：删除query_row、refresh_actor及值回包；初始化、行解析、字体和绘制协议仍pending。见[行查询证据](battle-actor-list-query-00470180.md)。
- 定义：[include/openswd3/battle/legacy_battle_list_contents.hpp:48](../../../include/openswd3/battle/legacy_battle_list_contents.hpp:48)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_list_contents.hpp](../../../include/openswd3/battle/legacy_battle_list_contents.hpp)：51, 53, 170
  - [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：134
  - [src/battle/legacy_battle_list_contents.cpp](../../../src/battle/legacy_battle_list_contents.cpp)：102, 575, 588
  - [tests/unit/battle/legacy_battle_list_contents_test.cpp](../../../tests/unit/battle/legacy_battle_list_contents_test.cpp)：21

## PORT-7fd8a5281cc0

- [ ] `LegacyBattleListFramePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_list_frame.hpp:33](../../../include/openswd3/battle/legacy_battle_list_frame.hpp:33)。
- 继承：`public virtual LegacyBattleOffsetActionFrameDrawStatePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_list_frame.hpp](../../../include/openswd3/battle/legacy_battle_list_frame.hpp)：33, 36, 102
  - [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：133
  - [src/battle/legacy_battle_list_frame.cpp](../../../src/battle/legacy_battle_list_frame.cpp)：35
  - [tests/unit/battle/legacy_battle_list_frame_test.cpp](../../../tests/unit/battle/legacy_battle_list_frame_test.cpp)：103

## PORT-13fadaf7ce03

- [ ] `LegacyBattleMessagePhaseStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_message_phase.hpp:50](../../../include/openswd3/battle/legacy_battle_message_phase.hpp:50)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：57
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：50, 63, 64, 151
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1721

## PORT-d4160261067a

- [ ] `LegacyBattleMessagePhasePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_message_phase.hpp:148](../../../include/openswd3/battle/legacy_battle_message_phase.hpp:148)。
- 继承：`public virtual LegacyBattleMonDatabasePort,
      public virtual LegacyBattleSummonFramePort,
      public virtual LegacyBattleMessagePhaseStatePort,
      public virtual LegacyBattleVictoryRewardPort,
      public virtual LegacyBattleLevelAdvancementPort,
      public virtual LegacyBattleLevelGrowthPanelPort,
      public virtual LegacyBattleGrowthActorSelectionPort,
      public virtual LegacyBattleGrowthCaptionPort,
      public virtual LegacyBattleGrowthItemCompletionPanelPort,
      public virtual LegacyBattleGrowthItemResultSelectionPort,
      public virtual LegacyBattleVictoryItemListPanelPort,
      public virtual LegacyBattleDefeatPanelPort,
      public virtual LegacyBattleTalismanResultPanelPort`。
- 直接残留：invoke, register, callee, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：395
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：148, 163, 327
  - [src/battle/legacy_battle_message_phase.cpp](../../../src/battle/legacy_battle_message_phase.cpp)：54, 123, 130, 1398, 1410
  - [tests/unit/battle/legacy_battle_message_phase_test.cpp](../../../tests/unit/battle/legacy_battle_message_phase_test.cpp)：98

## PORT-4fbbe55430e0

- [ ] `LegacyBattleMonDatabasePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_mon_profile.hpp:40](../../../include/openswd3/battle/legacy_battle_mon_profile.hpp:40)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：169
  - [include/openswd3/battle/legacy_battle_actor_list_query.hpp](../../../include/openswd3/battle/legacy_battle_actor_list_query.hpp)：301, 312, 357
  - [include/openswd3/battle/legacy_battle_actor_profile_preparation.hpp](../../../include/openswd3/battle/legacy_battle_actor_profile_preparation.hpp)：67
  - [include/openswd3/battle/legacy_battle_database_shutdown.hpp](../../../include/openswd3/battle/legacy_battle_database_shutdown.hpp)：22
  - [include/openswd3/battle/legacy_battle_dialog_text.hpp](../../../include/openswd3/battle/legacy_battle_dialog_text.hpp)：44, 48
  - [include/openswd3/battle/legacy_battle_fixed_count_chain.hpp](../../../include/openswd3/battle/legacy_battle_fixed_count_chain.hpp)：184, 216
  - [include/openswd3/battle/legacy_battle_group_a_final_processing.hpp](../../../include/openswd3/battle/legacy_battle_group_a_final_processing.hpp)：98
  - [include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp](../../../include/openswd3/battle/legacy_battle_group_a_summon_materialization.hpp)：56
  - [include/openswd3/battle/legacy_battle_group_b_action_composition.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_composition.hpp)：89
  - [include/openswd3/battle/legacy_battle_group_b_action_configuration.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_configuration.hpp)：46, 56
  - [include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp)：10
  - [include/openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_profile_mode.hpp)：47
  - [include/openswd3/battle/legacy_battle_group_b_action_profile_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_profile_selection.hpp)：51, 59
  - [include/openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp)：36
  - [include/openswd3/battle/legacy_battle_group_b_storage.hpp](../../../include/openswd3/battle/legacy_battle_group_b_storage.hpp)：15, 54
  - [include/openswd3/battle/legacy_battle_growth_actor_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_actor_selection.hpp)：100
  - [include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_item_result_selection.hpp)：70
  - [include/openswd3/battle/legacy_battle_input_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp)：153
  - [include/openswd3/battle/legacy_battle_level_profile.hpp](../../../include/openswd3/battle/legacy_battle_level_profile.hpp)：58
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：149
  - [include/openswd3/battle/legacy_battle_mon_definition.hpp](../../../include/openswd3/battle/legacy_battle_mon_definition.hpp)：86
  - [include/openswd3/battle/legacy_battle_mon_definition_text_release.hpp](../../../include/openswd3/battle/legacy_battle_mon_definition_text_release.hpp)：42
  - [include/openswd3/battle/legacy_battle_mon_profile.hpp](../../../include/openswd3/battle/legacy_battle_mon_profile.hpp)：40, 42, 138
  - [include/openswd3/battle/legacy_battle_party_item_definition.hpp](../../../include/openswd3/battle/legacy_battle_party_item_definition.hpp)：153
  - [include/openswd3/battle/legacy_battle_player_item_quantity.hpp](../../../include/openswd3/battle/legacy_battle_player_item_quantity.hpp)：8, 32
  - [include/openswd3/battle/legacy_battle_script_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp)：293
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：146
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：823, 2267, 2554, 3781, 4594, 4691, 5122, 5813, 6389, 6473
  - [src/battle/legacy_battle_actor_list_query.cpp](../../../src/battle/legacy_battle_actor_list_query.cpp)：59, 193, 1110
  - [src/battle/legacy_battle_actor_profile_preparation.cpp](../../../src/battle/legacy_battle_actor_profile_preparation.cpp)：38
  - [src/battle/legacy_battle_dialog_text.cpp](../../../src/battle/legacy_battle_dialog_text.cpp)：34, 62
  - [src/battle/legacy_battle_fixed_count_chain.cpp](../../../src/battle/legacy_battle_fixed_count_chain.cpp)：990, 1269
  - [src/battle/legacy_battle_group_a_final_processing.cpp](../../../src/battle/legacy_battle_group_a_final_processing.cpp)：79
  - [src/battle/legacy_battle_group_b_action_composition.cpp](../../../src/battle/legacy_battle_group_b_action_composition.cpp)：36
  - [src/battle/legacy_battle_group_b_action_configuration.cpp](../../../src/battle/legacy_battle_group_b_action_configuration.cpp)：49, 81
  - [src/battle/legacy_battle_group_b_action_profile_mode.cpp](../../../src/battle/legacy_battle_group_b_action_profile_mode.cpp)：31
  - [src/battle/legacy_battle_group_b_action_profile_selection.cpp](../../../src/battle/legacy_battle_group_b_action_profile_selection.cpp)：32
  - [src/battle/legacy_battle_group_b_action_reconfiguration.cpp](../../../src/battle/legacy_battle_group_b_action_reconfiguration.cpp)：38
  - [src/battle/legacy_battle_group_b_storage.cpp](../../../src/battle/legacy_battle_group_b_storage.cpp)：87
  - [src/battle/legacy_battle_mon_definition.cpp](../../../src/battle/legacy_battle_mon_definition.cpp)：38, 521, 533
  - [src/battle/legacy_battle_mon_definition_text_release.cpp](../../../src/battle/legacy_battle_mon_definition_text_release.cpp)：26
  - [src/battle/legacy_battle_mon_profile.cpp](../../../src/battle/legacy_battle_mon_profile.cpp)：428, 433, 438, 442, 448, 452, 458, 465, 469, 473, 478, 482, 488
  - [src/battle/legacy_battle_party_item_definition.cpp](../../../src/battle/legacy_battle_party_item_definition.cpp)：141
  - [src/battle/legacy_battle_player_item_quantity.cpp](../../../src/battle/legacy_battle_player_item_quantity.cpp)：69
  - [tests/support/legacy_battle_mon_database_fixture.hpp](../../../tests/support/legacy_battle_mon_database_fixture.hpp)：19
  - [tests/unit/battle/legacy_battle_group_b_startup_reset_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_startup_reset_test.cpp)：85
  - [tests/unit/battle/legacy_battle_mon_definition_test.cpp](../../../tests/unit/battle/legacy_battle_mon_definition_test.cpp)：16, 69, 156
  - [tests/unit/battle/legacy_battle_mon_definition_text_release_test.cpp](../../../tests/unit/battle/legacy_battle_mon_definition_text_release_test.cpp)：7, 29
  - [tests/unit/battle/legacy_battle_mon_file_runtime_test.cpp](../../../tests/unit/battle/legacy_battle_mon_file_runtime_test.cpp)：22
  - [tests/unit/battle/legacy_battle_mon_profile_test.cpp](../../../tests/unit/battle/legacy_battle_mon_profile_test.cpp)：15, 57, 160
  - [tests/unit/battle/legacy_battle_script_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_script_dispatch_test.cpp)：7735

## PORT-70edce35e328

- [ ] `LegacyBattleActorObjectResetPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_object_reset.hpp:30](../../../include/openswd3/battle/legacy_battle_object_reset.hpp:30)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_object_reset.hpp](../../../include/openswd3/battle/legacy_battle_object_reset.hpp)：30, 32, 53
  - [src/battle/legacy_battle_object_reset.cpp](../../../src/battle/legacy_battle_object_reset.cpp)：19
  - [tests/unit/battle/legacy_battle_object_reset_test.cpp](../../../tests/unit/battle/legacy_battle_object_reset_test.cpp)：18

## PORT-8993a2dfa5cd

- [ ] `LegacyBattleOutcomeFinalizationStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_outcome_finalization.hpp:19](../../../include/openswd3/battle/legacy_battle_outcome_finalization.hpp:19)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：48
  - [include/openswd3/battle/legacy_battle_outcome_finalization.hpp](../../../include/openswd3/battle/legacy_battle_outcome_finalization.hpp)：19, 31, 32, 40

## PORT-b64661945895

- [ ] `LegacyBattleOutcomeFinalizationPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_outcome_finalization.hpp:38](../../../include/openswd3/battle/legacy_battle_outcome_finalization.hpp:38)。
- 继承：`public virtual LegacyBattleActionDispatchPort,
      public virtual LegacyBattleOutcomeFinalizationStatePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：400
  - [include/openswd3/battle/legacy_battle_outcome_finalization.hpp](../../../include/openswd3/battle/legacy_battle_outcome_finalization.hpp)：38, 42, 75
  - [include/openswd3/battle/legacy_battle_outcome_resolution.hpp](../../../include/openswd3/battle/legacy_battle_outcome_resolution.hpp)：25
  - [src/battle/legacy_battle_outcome_finalization.cpp](../../../src/battle/legacy_battle_outcome_finalization.cpp)：18, 50
  - [tests/unit/battle/legacy_battle_outcome_finalization_test.cpp](../../../tests/unit/battle/legacy_battle_outcome_finalization_test.cpp)：17, 21

## PORT-d418321a615f

- [ ] `LegacyBattleOutcomeResolutionPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_outcome_resolution.hpp:24](../../../include/openswd3/battle/legacy_battle_outcome_resolution.hpp:24)。
- 继承：`public virtual LegacyBattleOutcomeFinalizationPort,
      public virtual LegacyBattleOutcomeResolutionStatePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：388
  - [include/openswd3/battle/legacy_battle_outcome_resolution.hpp](../../../include/openswd3/battle/legacy_battle_outcome_resolution.hpp)：24, 28, 75
  - [src/battle/legacy_battle_outcome_resolution.cpp](../../../src/battle/legacy_battle_outcome_resolution.cpp)：38
  - [tests/unit/battle/legacy_battle_outcome_resolution_test.cpp](../../../tests/unit/battle/legacy_battle_outcome_resolution_test.cpp)：20, 33

## PORT-d2d6d7766342

- [ ] `LegacyBattleOutcomeResolutionStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_outcome_state.hpp:14](../../../include/openswd3/battle/legacy_battle_outcome_state.hpp:14)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：176
  - [include/openswd3/battle/legacy_battle_debug_hotkeys.hpp](../../../include/openswd3/battle/legacy_battle_debug_hotkeys.hpp)：61
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：47
  - [include/openswd3/battle/legacy_battle_outcome_resolution.hpp](../../../include/openswd3/battle/legacy_battle_outcome_resolution.hpp)：26
  - [include/openswd3/battle/legacy_battle_outcome_state.hpp](../../../include/openswd3/battle/legacy_battle_outcome_state.hpp)：14, 27, 28
  - [include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)：78

## PORT-297df14b6866

- [ ] `LegacyBattlePairTransitionStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_pair_transition.hpp:44](../../../include/openswd3/battle/legacy_battle_pair_transition.hpp:44)。
- 继承：`public virtual LegacyBattleEffectShiftStatePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：46
  - [include/openswd3/battle/legacy_battle_pair_transition.hpp](../../../include/openswd3/battle/legacy_battle_pair_transition.hpp)：44, 66, 67, 75

## PORT-a0ff1255e5e7

- [ ] `LegacyBattlePairTransitionPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_pair_transition.hpp:74](../../../include/openswd3/battle/legacy_battle_pair_transition.hpp:74)。
- 继承：`public virtual LegacyBattlePairTransitionStatePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：174
  - [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：55
  - [include/openswd3/battle/legacy_battle_pair_transition.hpp](../../../include/openswd3/battle/legacy_battle_pair_transition.hpp)：74, 77, 127
  - [src/battle/legacy_battle_pair_transition.cpp](../../../src/battle/legacy_battle_pair_transition.cpp)：37, 92, 99
  - [tests/unit/battle/legacy_battle_pair_transition_test.cpp](../../../tests/unit/battle/legacy_battle_pair_transition_test.cpp)：15, 18

## PORT-770177f05785

- [ ] `LegacyBattlePartyItemDefinitionPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_party_item_definition.hpp:59](../../../include/openswd3/battle/legacy_battle_party_item_definition.hpp:59)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_party_item_definition.hpp](../../../include/openswd3/battle/legacy_battle_party_item_definition.hpp)：59, 61, 152
  - [src/battle/legacy_battle_party_item_definition.cpp](../../../src/battle/legacy_battle_party_item_definition.cpp)：140
  - [src/battle/legacy_battle_script_dispatch_internal.hpp](../../../src/battle/legacy_battle_script_dispatch_internal.hpp)：946
  - [tests/unit/battle/legacy_battle_party_item_definition_test.cpp](../../../tests/unit/battle/legacy_battle_party_item_definition_test.cpp)：15, 23

## PORT-4c2f8535575a

- [ ] `LegacyBattlePendingActionPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_pending_action_commit.hpp:46](../../../include/openswd3/battle/legacy_battle_pending_action_commit.hpp:46)。
- 继承：`public virtual LegacyBattleActorMetricStatePort,
      public virtual LegacyBattleActorPublicationStatePort,
      public LegacyBattleActorReadyPort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：391
  - [include/openswd3/battle/legacy_battle_pending_action_commit.hpp](../../../include/openswd3/battle/legacy_battle_pending_action_commit.hpp)：46, 51, 97
  - [src/battle/legacy_battle_pending_action_commit.cpp](../../../src/battle/legacy_battle_pending_action_commit.cpp)：21, 36
  - [tests/unit/battle/legacy_battle_pending_action_commit_test.cpp](../../../tests/unit/battle/legacy_battle_pending_action_commit_test.cpp)：21

## PORT-d98289cbb7b2

- [ ] `LegacyBattlePreFramePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_pre_frame.hpp:52](../../../include/openswd3/battle/legacy_battle_pre_frame.hpp:52)。
- 继承：`public virtual LegacyBattleActorMetricStatePort,
      public virtual LegacyBattleSharedPhaseStatePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：385
  - [include/openswd3/battle/legacy_battle_pre_frame.hpp](../../../include/openswd3/battle/legacy_battle_pre_frame.hpp)：52, 56, 106
  - [src/battle/legacy_battle_pre_frame.cpp](../../../src/battle/legacy_battle_pre_frame.cpp)：121
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1712
  - [tests/unit/battle/legacy_battle_pre_frame_test.cpp](../../../tests/unit/battle/legacy_battle_pre_frame_test.cpp)：19, 48

## PORT-ac77a2188211

- [ ] `LegacyBattleRetreatCommitStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_retreat_commit.hpp:28](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp:28)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：42
  - [include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)：28, 40, 41, 74

## PORT-01cef6293059

- [ ] `LegacyBattleRetreatCommitPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_retreat_commit.hpp:73](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp:73)。
- 继承：`public virtual LegacyBattleRetreatCommitStatePort,
      public virtual LegacyBattleActorMetricStatePort,
      public virtual LegacyBattleDebugHotkeyStatePort,
      public virtual LegacyBattleDebugOverlayGateStatePort,
      public virtual LegacyBattleOutcomeResolutionStatePort,
      public virtual LegacyBattleSharedPhaseStatePort,
      public virtual LegacyBattleTextMessagePort`。
- 直接残留：invoke, register, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：172
  - [include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)：73, 82, 154
  - [src/battle/legacy_battle_retreat_commit.cpp](../../../src/battle/legacy_battle_retreat_commit.cpp)：17
  - [tests/unit/battle/legacy_battle_retreat_commit_test.cpp](../../../tests/unit/battle/legacy_battle_retreat_commit_test.cpp)：15, 19
  - [tests/unit/battle/legacy_battle_script_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_script_dispatch_test.cpp)：122, 4318

## PORT-4139fa9e1174

- [ ] `LegacyBattleScriptDispatchPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_script_dispatch.hpp:290](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp:290)。
- 继承：`public virtual LegacyBattleDebugHotkeyStatePort,
      public virtual LegacyBattleEffectShiftStatePort,
      public virtual LegacyBattleMonDatabasePort,
      public virtual LegacyBattleLevelAdvancementStatePort,
      public virtual world_map::LegacyWorldItemListStatePort`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_script_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp)：290, 297, 455
  - [src/battle/legacy_battle_script_dispatch.cpp](../../../src/battle/legacy_battle_script_dispatch.cpp)：62
  - [src/battle/legacy_battle_script_dispatch_internal.hpp](../../../src/battle/legacy_battle_script_dispatch_internal.hpp)：199, 1779
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1705
  - [tests/unit/battle/legacy_battle_opponent_action_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_opponent_action_dispatch_test.cpp)：31
  - [tests/unit/battle/legacy_battle_script_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_script_dispatch_test.cpp)：40, 118, 4316

## PORT-9607f06747d3

- [ ] `LegacyBattleScriptFilePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_script_file.hpp:30](../../../include/openswd3/battle/legacy_battle_script_file.hpp:30)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_script_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp)：166
  - [include/openswd3/battle/legacy_battle_script_file.hpp](../../../include/openswd3/battle/legacy_battle_script_file.hpp)：30, 32, 47, 93, 102, 108
  - [src/battle/legacy_battle_script_file.cpp](../../../src/battle/legacy_battle_script_file.cpp)：87, 161, 193
  - [tests/unit/battle/legacy_battle_script_file_test.cpp](../../../tests/unit/battle/legacy_battle_script_file_test.cpp)：29

## PORT-62023efc7570

- [ ] `LegacyBattleSelectionFrameStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_selection_frame.hpp:54](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp:54)。
- 继承：`public virtual input_time_rng::LegacyMouseFrameStatePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：56
  - [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：54, 68, 69, 130

## PORT-982d98cdcbdd

- [ ] `LegacyBattleSelectionFramePort`：抽象接口；迁移中，未验收。
- 行查询批次95a3d125已验证推送：向两个实际caller借出MON数据库及同一音乐路径owner；新增MON数据库虚基类用于现有文件边界，未增加行查询转发。其余通用业务接口仍pending。见[行查询证据](battle-actor-list-query-00470180.md)。
- 定义：[include/openswd3/battle/legacy_battle_selection_frame.hpp:129](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp:129)。
- 继承：`public virtual LegacyBattleSelectionFrameStatePort,
      public virtual LegacyBattleActorTargetPreparationPort,
      public virtual LegacyBattleActionSummaryPort,
      public virtual LegacyBattleListFramePort,
      public virtual LegacyBattleListContentsPort,
      public virtual LegacyBattleGridFramePort,
      public virtual LegacyBattleSelectionHintFramePort,
      public virtual LegacyBattleControlPanelFramePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：394
  - [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：129, 139, 278
  - [src/battle/legacy_battle_selection_frame.cpp](../../../src/battle/legacy_battle_selection_frame.cpp)：75, 1664, 1688
  - [tests/unit/battle/legacy_battle_selection_frame_test.cpp](../../../tests/unit/battle/legacy_battle_selection_frame_test.cpp)：42

## PORT-19075230a180

- [ ] `LegacyBattleSelectionHintFramePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_selection_hint_frame.hpp:65](../../../include/openswd3/battle/legacy_battle_selection_hint_frame.hpp:65)。
- 继承：`public virtual LegacyBattleFixedObjectState`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：136
  - [include/openswd3/battle/legacy_battle_selection_hint_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_hint_frame.hpp)：65, 68, 175
  - [src/battle/legacy_battle_selection_hint_frame.cpp](../../../src/battle/legacy_battle_selection_hint_frame.cpp)：67, 490, 502
  - [tests/unit/battle/legacy_battle_selection_hint_frame_test.cpp](../../../tests/unit/battle/legacy_battle_selection_hint_frame_test.cpp)：90

## PORT-d0741dd9f3ec

- [ ] `LegacyBattleSharedPhaseStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_shared_phase.hpp:7](../../../include/openswd3/battle/legacy_battle_shared_phase.hpp:7)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：175
  - [include/openswd3/battle/legacy_battle_effect_frame.hpp](../../../include/openswd3/battle/legacy_battle_effect_frame.hpp)：57
  - [include/openswd3/battle/legacy_battle_pre_frame.hpp](../../../include/openswd3/battle/legacy_battle_pre_frame.hpp)：54
  - [include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)：79
  - [include/openswd3/battle/legacy_battle_shared_phase.hpp](../../../include/openswd3/battle/legacy_battle_shared_phase.hpp)：7, 28, 29
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：150, 502
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：810
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1720
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：1180

## PORT-a844c6f3c5c1

- [ ] `LegacyBattleStartupPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_startup.hpp:141](../../../include/openswd3/battle/legacy_battle_startup.hpp:141)。
- 继承：`public LegacyBattleDisplaySurfacePort,
      public virtual LegacyBattleDebugHotkeyStatePort,
      public virtual input_time_rng::LegacyMouseFrameStatePort,
      public virtual LegacyBattleFrameInputResolutionStatePort,
      public virtual LegacyBattleMonDatabasePort,
      public virtual LegacyBattleActorMetricStatePort,
      public virtual LegacyBattleActorPublicationStatePort,
      public virtual LegacyBattleFixedObjectState,
      public virtual LegacyBattleSharedPhaseStatePort,
      public virtual LegacyBattleTargetSelectionRuntimeStatePort,
      public virtual world_map::LegacyWorldItemListStatePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_actor_metrics.hpp](../../../include/openswd3/battle/legacy_battle_actor_metrics.hpp)：15, 207
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：41
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：141, 154, 632
  - [src/battle/legacy_battle_actor_metrics.cpp](../../../src/battle/legacy_battle_actor_metrics.cpp)：305
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：28, 41, 55, 62, 187, 194, 248, 255, 283, 619, 627, 632, 637, 1164
  - [tests/unit/battle/legacy_battle_actor_metrics_test.cpp](../../../tests/unit/battle/legacy_battle_actor_metrics_test.cpp)：17, 27, 379, 443
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：176, 217

## PORT-d9653246e88e

- [ ] `LegacyBattleStartupSupplementalPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_startup.hpp:460](../../../include/openswd3/battle/legacy_battle_startup.hpp:460)。
- 继承：`public LegacyBattleGroupASummonMaterializationPort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：460, 487
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：59, 494, 715
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1709
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：107

## PORT-3e60c2b3f4f5

- [ ] `LegacyBattleBoundedRandomPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_status_indicator.hpp:9](../../../include/openswd3/battle/legacy_battle_status_indicator.hpp:9)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：1431, 1798
  - [include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp](../../../include/openswd3/battle/legacy_battle_actor_frame_presentation.hpp)：1030, 1171, 1279, 1820
  - [include/openswd3/battle/legacy_battle_actor_runtime_reset.hpp](../../../include/openswd3/battle/legacy_battle_actor_runtime_reset.hpp)：13, 283, 289
  - [include/openswd3/battle/legacy_battle_actor_target_preparation.hpp](../../../include/openswd3/battle/legacy_battle_actor_target_preparation.hpp)：93
  - [include/openswd3/battle/legacy_battle_debug_hotkeys.hpp](../../../include/openswd3/battle/legacy_battle_debug_hotkeys.hpp)：102, 111
  - [include/openswd3/battle/legacy_battle_group_a_frame.hpp](../../../include/openswd3/battle/legacy_battle_group_a_frame.hpp)：69
  - [include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_action_item_selection.hpp)：59
  - [include/openswd3/battle/legacy_battle_group_b_opponent_mode.hpp](../../../include/openswd3/battle/legacy_battle_group_b_opponent_mode.hpp)：32
  - [include/openswd3/battle/legacy_battle_group_b_reward_item_selection.hpp](../../../include/openswd3/battle/legacy_battle_group_b_reward_item_selection.hpp)：37
  - [include/openswd3/battle/legacy_battle_group_b_status_action.hpp](../../../include/openswd3/battle/legacy_battle_group_b_status_action.hpp)：45
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：194
  - [include/openswd3/battle/legacy_battle_post_action.hpp](../../../include/openswd3/battle/legacy_battle_post_action.hpp)：27
  - [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：169
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：519
  - [include/openswd3/battle/legacy_battle_status_indicator.hpp](../../../include/openswd3/battle/legacy_battle_status_indicator.hpp)：9, 11, 77
  - [include/openswd3/battle/legacy_battle_victory_rewards.hpp](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp)：165
  - [src/battle/legacy_battle_action_dispatch_effects.cpp](../../../src/battle/legacy_battle_action_dispatch_effects.cpp)：312
  - [src/battle/legacy_battle_actor_frame_presentation.cpp](../../../src/battle/legacy_battle_actor_frame_presentation.cpp)：680
  - [src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_resource_pipeline.cpp)：1162
  - [src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp](../../../src/battle/legacy_battle_actor_frame_presentation_selector_audio.cpp)：1436
  - [src/battle/legacy_battle_actor_runtime_reset.cpp](../../../src/battle/legacy_battle_actor_runtime_reset.cpp)：1238, 1720
  - [src/battle/legacy_battle_actor_target_preparation.cpp](../../../src/battle/legacy_battle_actor_target_preparation.cpp)：28
  - [src/battle/legacy_battle_debug_actor_value.cpp](../../../src/battle/legacy_battle_debug_actor_value.cpp)：121
  - [src/battle/legacy_battle_frame_coordinator.cpp](../../../src/battle/legacy_battle_frame_coordinator.cpp)：17
  - [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：827
  - [src/battle/legacy_battle_group_b_action_item_selection.cpp](../../../src/battle/legacy_battle_group_b_action_item_selection.cpp)：33, 71
  - [src/battle/legacy_battle_group_b_opponent_mode.cpp](../../../src/battle/legacy_battle_group_b_opponent_mode.cpp)：36
  - [src/battle/legacy_battle_group_b_reward_item_selection.cpp](../../../src/battle/legacy_battle_group_b_reward_item_selection.cpp)：21
  - [src/battle/legacy_battle_group_b_status_action.cpp](../../../src/battle/legacy_battle_group_b_status_action.cpp)：25, 47
  - [src/battle/legacy_battle_post_action.cpp](../../../src/battle/legacy_battle_post_action.cpp)：79
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：38, 826
  - [src/battle/legacy_battle_status_indicator.cpp](../../../src/battle/legacy_battle_status_indicator.cpp)：46
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1710
  - [tests/unit/battle/legacy_battle_action_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_action_dispatch_test.cpp)：435
  - [tests/unit/battle/legacy_battle_actor_frame_oracle_reset_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_oracle_reset_test.cpp)：41
  - [tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_frame_presentation_test.cpp)：371
  - [tests/unit/battle/legacy_battle_actor_runtime_reset_test.cpp](../../../tests/unit/battle/legacy_battle_actor_runtime_reset_test.cpp)：26
  - [tests/unit/battle/legacy_battle_actor_target_preparation_test.cpp](../../../tests/unit/battle/legacy_battle_actor_target_preparation_test.cpp)：37
  - [tests/unit/battle/legacy_battle_debug_hotkeys_test.cpp](../../../tests/unit/battle/legacy_battle_debug_hotkeys_test.cpp)：28
  - [tests/unit/battle/legacy_battle_final_actor_step_test.cpp](../../../tests/unit/battle/legacy_battle_final_actor_step_test.cpp)：24
  - [tests/unit/battle/legacy_battle_frame_coordinator_test.cpp](../../../tests/unit/battle/legacy_battle_frame_coordinator_test.cpp)：534
  - [tests/unit/battle/legacy_battle_group_a_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_frame_test.cpp)：172
  - [tests/unit/battle/legacy_battle_group_b_action_execution_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_action_execution_test.cpp)：171
  - [tests/unit/battle/legacy_battle_group_b_action_item_selection_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_action_item_selection_test.cpp)：17
  - [tests/unit/battle/legacy_battle_group_b_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_frame_test.cpp)：146
  - [tests/unit/battle/legacy_battle_group_b_opponent_mode_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_opponent_mode_test.cpp)：15
  - [tests/unit/battle/legacy_battle_group_b_reward_item_selection_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_reward_item_selection_test.cpp)：14
  - [tests/unit/battle/legacy_battle_group_b_status_action_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_status_action_test.cpp)：15
  - [tests/unit/battle/legacy_battle_growth_caption_test.cpp](../../../tests/unit/battle/legacy_battle_growth_caption_test.cpp)：25
  - [tests/unit/battle/legacy_battle_level_growth_panel_test.cpp](../../../tests/unit/battle/legacy_battle_level_growth_panel_test.cpp)：25
  - [tests/unit/battle/legacy_battle_message_phase_test.cpp](../../../tests/unit/battle/legacy_battle_message_phase_test.cpp)：24
  - [tests/unit/battle/legacy_battle_opponent_action_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_opponent_action_dispatch_test.cpp)：168
  - [tests/unit/battle/legacy_battle_post_action_test.cpp](../../../tests/unit/battle/legacy_battle_post_action_test.cpp)：20
  - [tests/unit/battle/legacy_battle_selection_frame_test.cpp](../../../tests/unit/battle/legacy_battle_selection_frame_test.cpp)：208
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：84
  - [tests/unit/battle/legacy_battle_status_indicator_test.cpp](../../../tests/unit/battle/legacy_battle_status_indicator_test.cpp)：93
  - [tests/unit/battle/legacy_battle_target_ready_test.cpp](../../../tests/unit/battle/legacy_battle_target_ready_test.cpp)：99
  - [tests/unit/battle/legacy_battle_transition_test.cpp](../../../tests/unit/battle/legacy_battle_transition_test.cpp)：424
  - [tests/unit/battle/legacy_battle_victory_rewards_test.cpp](../../../tests/unit/battle/legacy_battle_victory_rewards_test.cpp)：95

## PORT-dcddf6f7982f

- [ ] `LegacyBattleIndicatorSoundPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_status_indicator.hpp:15](../../../include/openswd3/battle/legacy_battle_status_indicator.hpp:15)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：1432
  - [include/openswd3/battle/legacy_battle_status_indicator.hpp](../../../include/openswd3/battle/legacy_battle_status_indicator.hpp)：15, 17, 78
  - [src/battle/legacy_battle_status_indicator.cpp](../../../src/battle/legacy_battle_status_indicator.cpp)：47
  - [tests/unit/battle/legacy_battle_action_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_action_dispatch_test.cpp)：444
  - [tests/unit/battle/legacy_battle_frame_coordinator_test.cpp](../../../tests/unit/battle/legacy_battle_frame_coordinator_test.cpp)：542
  - [tests/unit/battle/legacy_battle_group_a_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_frame_test.cpp)：184
  - [tests/unit/battle/legacy_battle_group_b_action_execution_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_action_execution_test.cpp)：179
  - [tests/unit/battle/legacy_battle_group_b_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_frame_test.cpp)：169
  - [tests/unit/battle/legacy_battle_opponent_action_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_opponent_action_dispatch_test.cpp)：176
  - [tests/unit/battle/legacy_battle_status_indicator_test.cpp](../../../tests/unit/battle/legacy_battle_status_indicator_test.cpp)：107
  - [tests/unit/battle/legacy_battle_target_ready_test.cpp](../../../tests/unit/battle/legacy_battle_target_ready_test.cpp)：107
  - [tests/unit/battle/legacy_battle_transition_test.cpp](../../../tests/unit/battle/legacy_battle_transition_test.cpp)：432

## PORT-5a1ba6935260

- [ ] `LegacyBattleSurfaceBlendPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_surface_blend.hpp:46](../../../include/openswd3/battle/legacy_battle_surface_blend.hpp:46)。
- 继承：`无`。
- 直接残留：operation。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_surface_blend.hpp](../../../include/openswd3/battle/legacy_battle_surface_blend.hpp)：46, 48, 105
  - [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：251
  - [src/battle/legacy_battle_surface_blend.cpp](../../../src/battle/legacy_battle_surface_blend.cpp)：26, 35, 45
  - [src/battle/legacy_battle_transition.cpp](../../../src/battle/legacy_battle_transition.cpp)：370
  - [tests/unit/battle/legacy_battle_surface_blend_test.cpp](../../../tests/unit/battle/legacy_battle_surface_blend_test.cpp)：13
  - [tests/unit/battle/legacy_battle_transition_test.cpp](../../../tests/unit/battle/legacy_battle_transition_test.cpp)：105

## PORT-7932e6ddad59

- [ ] `LegacyBattleTalismanResultPanelPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_talisman_result_panel.hpp:76](../../../include/openswd3/battle/legacy_battle_talisman_result_panel.hpp:76)。
- 继承：`无`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：161
  - [include/openswd3/battle/legacy_battle_talisman_result_panel.hpp](../../../include/openswd3/battle/legacy_battle_talisman_result_panel.hpp)：76, 78, 157
  - [src/battle/legacy_battle_talisman_result_panel.cpp](../../../src/battle/legacy_battle_talisman_result_panel.cpp)：67, 380, 394
  - [tests/unit/battle/legacy_battle_message_phase_test.cpp](../../../tests/unit/battle/legacy_battle_message_phase_test.cpp)：244
  - [tests/unit/battle/legacy_battle_talisman_result_panel_test.cpp](../../../tests/unit/battle/legacy_battle_talisman_result_panel_test.cpp)：94, 104

## PORT-129eeaecd845

- [ ] `LegacyBattleTargetSelectionRuntimeStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_target_selection_runtime.hpp:44](../../../include/openswd3/battle/legacy_battle_target_selection_runtime.hpp:44)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：54
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：151
  - [include/openswd3/battle/legacy_battle_target_selection_runtime.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_runtime.hpp)：44, 57, 58, 110
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1724
  - [tests/unit/battle/legacy_battle_startup_test.cpp](../../../tests/unit/battle/legacy_battle_startup_test.cpp)：178

## PORT-886bcc672f06

- [ ] `LegacyBattleTargetSelectionRuntimePort`：实现或转发类；待复核。
- 资料批次5e487db5已验证推送：删除query_primary_target操作和两处菜单参数包调用；其余协议仍pending，Port未完成。见[资料应用证据](battle-actor-list-profile-application-004705c0.md)。
- 定义：[include/openswd3/battle/legacy_battle_target_selection_runtime.hpp:109](../../../include/openswd3/battle/legacy_battle_target_selection_runtime.hpp:109)。
- 继承：`public virtual LegacyBattleTargetSelectionRuntimeStatePort`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_group_b_target_cycle.hpp](../../../include/openswd3/battle/legacy_battle_group_b_target_cycle.hpp)：47
  - [include/openswd3/battle/legacy_battle_input_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_input_dispatch.hpp)：156
  - [include/openswd3/battle/legacy_battle_target_selection_runtime.hpp](../../../include/openswd3/battle/legacy_battle_target_selection_runtime.hpp)：109, 112
  - [src/battle/legacy_battle_group_b_target_cycle.cpp](../../../src/battle/legacy_battle_group_b_target_cycle.cpp)：29, 148, 160
  - [tests/unit/battle/legacy_battle_group_b_target_cycle_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_target_cycle_test.cpp)：24

## PORT-215eeb64554b

- [ ] `LegacyBattleTextMessagePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_text_message.hpp:68](../../../include/openswd3/battle/legacy_battle_text_message.hpp:68)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_retreat_commit.hpp](../../../include/openswd3/battle/legacy_battle_retreat_commit.hpp)：80
  - [include/openswd3/battle/legacy_battle_text_message.hpp](../../../include/openswd3/battle/legacy_battle_text_message.hpp)：68, 70, 112
  - [src/battle/legacy_battle_debug_hotkeys.cpp](../../../src/battle/legacy_battle_debug_hotkeys.cpp)：98
  - [src/battle/legacy_battle_input_dispatch.cpp](../../../src/battle/legacy_battle_input_dispatch.cpp)：29
  - [src/battle/legacy_battle_target_selection_refresh.cpp](../../../src/battle/legacy_battle_target_selection_refresh.cpp)：53
  - [src/battle/legacy_battle_text_message.cpp](../../../src/battle/legacy_battle_text_message.cpp)：22
  - [src/battle/legacy_battle_transition.cpp](../../../src/battle/legacy_battle_transition.cpp)：31
  - [tests/unit/battle/legacy_battle_text_message_test.cpp](../../../tests/unit/battle/legacy_battle_text_message_test.cpp)：138

## PORT-99c11857a4d1

- [ ] `LegacyBattleTextMessageFramePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_text_message_frame.hpp:43](../../../include/openswd3/battle/legacy_battle_text_message_frame.hpp:43)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：396
  - [include/openswd3/battle/legacy_battle_text_message_frame.hpp](../../../include/openswd3/battle/legacy_battle_text_message_frame.hpp)：43, 45, 109
  - [src/battle/legacy_battle_text_message_frame.cpp](../../../src/battle/legacy_battle_text_message_frame.cpp)：80
  - [tests/unit/battle/legacy_battle_text_message_test.cpp](../../../tests/unit/battle/legacy_battle_text_message_test.cpp)：62

## PORT-2781b9333f3e

- [ ] `LegacyBattleTextPanelPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_text_panel.hpp:46](../../../include/openswd3/battle/legacy_battle_text_panel.hpp:46)。
- 继承：`无`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_debug_status_panel.hpp](../../../include/openswd3/battle/legacy_battle_debug_status_panel.hpp)：122
  - [include/openswd3/battle/legacy_battle_hud_frame.hpp](../../../include/openswd3/battle/legacy_battle_hud_frame.hpp)：31
  - [include/openswd3/battle/legacy_battle_text_panel.hpp](../../../include/openswd3/battle/legacy_battle_text_panel.hpp)：46, 48, 93
  - [src/battle/legacy_battle_debug_status_panel.cpp](../../../src/battle/legacy_battle_debug_status_panel.cpp)：106
  - [src/battle/legacy_battle_text_panel.cpp](../../../src/battle/legacy_battle_text_panel.cpp)：32
  - [tests/unit/battle/legacy_battle_debug_status_panel_test.cpp](../../../tests/unit/battle/legacy_battle_debug_status_panel_test.cpp)：61
  - [tests/unit/battle/legacy_battle_text_panel_test.cpp](../../../tests/unit/battle/legacy_battle_text_panel_test.cpp)：14

## PORT-4240acb37dfc

- [ ] `LegacyBattleTransitionBufferPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_transition.hpp:43](../../../include/openswd3/battle/legacy_battle_transition.hpp:43)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：43, 45, 249
  - [src/battle/legacy_battle_transition.cpp](../../../src/battle/legacy_battle_transition.cpp)：245, 368
  - [tests/unit/battle/legacy_battle_transition_test.cpp](../../../tests/unit/battle/legacy_battle_transition_test.cpp)：103

## PORT-9aab8c7b9bdf

- [ ] `LegacyBattleTransitionSurfacePort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_transition.hpp:65](../../../include/openswd3/battle/legacy_battle_transition.hpp:65)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：65, 67, 250
  - [src/battle/legacy_battle_transition.cpp](../../../src/battle/legacy_battle_transition.cpp)：110, 369
  - [tests/unit/battle/legacy_battle_transition_test.cpp](../../../tests/unit/battle/legacy_battle_transition_test.cpp)：104

## PORT-190abae1c145

- [ ] `LegacyBattleTransitionPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_transition.hpp:108](../../../include/openswd3/battle/legacy_battle_transition.hpp:108)。
- 继承：`public virtual LegacyBattleDebugHotkeyStatePort,
      public virtual LegacyBattleEffectShiftStatePort,
      public virtual LegacyBattleFrameRefreshStatePort,
      public virtual LegacyBattleFrameEffectControlStatePort,
      public virtual LegacyBattleColorAccumulationStatePort,
      public LegacyBattleFrameEffectPort,
      public LegacyBattleHudCallPort,
      public virtual LegacyBattleActorMetricStatePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_transition.hpp](../../../include/openswd3/battle/legacy_battle_transition.hpp)：108, 118, 248
  - [src/battle/legacy_battle_transition.cpp](../../../src/battle/legacy_battle_transition.cpp)：22, 33, 49, 54, 155, 172, 186, 206, 275, 314, 333, 367
  - [tests/unit/battle/legacy_battle_transition_test.cpp](../../../tests/unit/battle/legacy_battle_transition_test.cpp)：101

## PORT-b990ae590803

- [ ] `LegacyBattleVerticalShiftStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_vertical_shift.hpp:26](../../../include/openswd3/battle/legacy_battle_vertical_shift.hpp:26)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：52
  - [include/openswd3/battle/legacy_battle_vertical_shift.hpp](../../../include/openswd3/battle/legacy_battle_vertical_shift.hpp)：26, 34, 41

## PORT-c85bfdf9142a

- [ ] `LegacyBattleVerticalShiftPort`：抽象接口；待复核。
- 定义：[include/openswd3/battle/legacy_battle_vertical_shift.hpp:40](../../../include/openswd3/battle/legacy_battle_vertical_shift.hpp:40)。
- 继承：`public virtual LegacyBattleVerticalShiftStatePort`。
- 直接残留：operation。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：390
  - [include/openswd3/battle/legacy_battle_vertical_shift.hpp](../../../include/openswd3/battle/legacy_battle_vertical_shift.hpp)：40, 43, 74
  - [src/battle/legacy_battle_vertical_shift.cpp](../../../src/battle/legacy_battle_vertical_shift.cpp)：62, 120
  - [tests/unit/battle/legacy_battle_vertical_shift_test.cpp](../../../tests/unit/battle/legacy_battle_vertical_shift_test.cpp)：15

## PORT-e99bc2bc4419

- [ ] `LegacyBattleVictoryItemListPanelPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp:69](../../../include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp:69)。
- 继承：`无`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：159
  - [include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp](../../../include/openswd3/battle/legacy_battle_victory_item_list_panel.hpp)：69, 71, 156
  - [src/battle/legacy_battle_victory_item_list_panel.cpp](../../../src/battle/legacy_battle_victory_item_list_panel.cpp)：41, 379, 394
  - [tests/unit/battle/legacy_battle_message_phase_test.cpp](../../../tests/unit/battle/legacy_battle_message_phase_test.cpp)：278
  - [tests/unit/battle/legacy_battle_victory_item_list_panel_test.cpp](../../../tests/unit/battle/legacy_battle_victory_item_list_panel_test.cpp)：97, 109

## PORT-83b19a82edf3

- [ ] `LegacyBattleVictoryRewardStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_victory_rewards.hpp:52](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp:52)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_global_reset.hpp](../../../include/openswd3/battle/legacy_battle_global_reset.hpp)：58
  - [include/openswd3/battle/legacy_battle_hud_frame.hpp](../../../include/openswd3/battle/legacy_battle_hud_frame.hpp)：32
  - [include/openswd3/battle/legacy_battle_victory_rewards.hpp](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp)：52, 65, 66, 123
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1722

## PORT-4172aa8df321

- [ ] `LegacyBattleVictoryRewardPort`：实现或转发类；待复核。
- 定义：[include/openswd3/battle/legacy_battle_victory_rewards.hpp:122](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp:122)。
- 继承：`public virtual LegacyBattleVictoryRewardStatePort,
      public virtual LegacyBattleGroupARewardProfileStatePort,
      public virtual LegacyBattleInputDispatchPort,
      public virtual LegacyBattleActionDispatchPort`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_level_up_panel.hpp](../../../include/openswd3/battle/legacy_battle_level_up_panel.hpp)：90
  - [include/openswd3/battle/legacy_battle_message_phase.hpp](../../../include/openswd3/battle/legacy_battle_message_phase.hpp)：152
  - [include/openswd3/battle/legacy_battle_victory_rewards.hpp](../../../include/openswd3/battle/legacy_battle_victory_rewards.hpp)：122, 128, 233
  - [src/battle/legacy_battle_level_up_panel.cpp](../../../src/battle/legacy_battle_level_up_panel.cpp)：31, 302, 315
  - [src/battle/legacy_battle_victory_rewards.cpp](../../../src/battle/legacy_battle_victory_rewards.cpp)：120, 659, 672
  - [tests/unit/battle/legacy_battle_level_up_panel_test.cpp](../../../tests/unit/battle/legacy_battle_level_up_panel_test.cpp)：94
  - [tests/unit/battle/legacy_battle_victory_rewards_test.cpp](../../../tests/unit/battle/legacy_battle_victory_rewards_test.cpp)：132

## PORT-7cb2fa0d8bbe

- [ ] `LegacyMouseFrameStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/input_time_rng/legacy_mouse_frame_state.hpp:9](../../../include/openswd3/input_time_rng/legacy_mouse_frame_state.hpp:9)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_selection_frame.hpp](../../../include/openswd3/battle/legacy_battle_selection_frame.hpp)：55
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：144
  - [include/openswd3/input_time_rng/legacy_mouse_frame_state.hpp](../../../include/openswd3/input_time_rng/legacy_mouse_frame_state.hpp)：9, 21, 22
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1711

## PORT-f4bce2b0680e

- [ ] `LegacyTextInputPorts`：抽象接口；待复核。
- 定义：[include/openswd3/input_time_rng/legacy_text_input.hpp:23](../../../include/openswd3/input_time_rng/legacy_text_input.hpp:23)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/input_time_rng/legacy_text_input.hpp](../../../include/openswd3/input_time_rng/legacy_text_input.hpp)：23, 25, 61
  - [src/input_time_rng/legacy_text_input.cpp](../../../src/input_time_rng/legacy_text_input.cpp)：97, 267
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：185
  - [tests/unit/input_time_rng/legacy_text_input_test.cpp](../../../tests/unit/input_time_rng/legacy_text_input_test.cpp)：18

## PORT-2a8734c66051

- [ ] `LegacyPackedRowDrawPorts`：抽象接口；待复核。
- 定义：[include/openswd3/rendering/legacy_action_renderers.hpp:24](../../../include/openswd3/rendering/legacy_action_renderers.hpp:24)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：1912
  - [include/openswd3/rendering/legacy_action_renderers.hpp](../../../include/openswd3/rendering/legacy_action_renderers.hpp)：24, 26, 37, 91
  - [src/rendering/legacy_action_renderers.cpp](../../../src/rendering/legacy_action_renderers.cpp)：54, 133
  - [tests/unit/battle/legacy_battle_frame_coordinator_test.cpp](../../../tests/unit/battle/legacy_battle_frame_coordinator_test.cpp)：437
  - [tests/unit/rendering/legacy_action_renderers_test.cpp](../../../tests/unit/rendering/legacy_action_renderers_test.cpp)：18, 31

## PORT-08f8d487dd54

- [ ] `LegacyFramebufferPackedRowDrawPorts`：实现或转发类；待复核。
- 定义：[include/openswd3/rendering/legacy_action_renderers.hpp:36](../../../include/openswd3/rendering/legacy_action_renderers.hpp:36)。
- 继承：`public LegacyPackedRowDrawPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/rendering/legacy_action_renderers.hpp](../../../include/openswd3/rendering/legacy_action_renderers.hpp)：36, 39
  - [src/rendering/legacy_action_renderers.cpp](../../../src/rendering/legacy_action_renderers.cpp)：72, 78
  - [src/world_map/legacy_world_frame_runtime.cpp](../../../src/world_map/legacy_world_frame_runtime.cpp)：284
  - [tests/unit/rendering/legacy_action_renderers_test.cpp](../../../tests/unit/rendering/legacy_action_renderers_test.cpp)：17, 153

## PORT-20458ee417fa

- [ ] `LegacyBmpWriterPorts`：抽象接口；待复核。
- 定义：[include/openswd3/rendering/legacy_bmp_writer.hpp:29](../../../include/openswd3/rendering/legacy_bmp_writer.hpp:29)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：1923
  - [include/openswd3/rendering/legacy_bmp_writer.hpp](../../../include/openswd3/rendering/legacy_bmp_writer.hpp)：29, 31, 51
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：450
  - [src/rendering/legacy_bmp_writer.cpp](../../../src/rendering/legacy_bmp_writer.cpp)：35, 50
  - [tests/unit/battle/legacy_battle_frame_coordinator_test.cpp](../../../tests/unit/battle/legacy_battle_frame_coordinator_test.cpp)：560
  - [tests/unit/rendering/legacy_bmp_writer_test.cpp](../../../tests/unit/rendering/legacy_bmp_writer_test.cpp)：22

## PORT-89846717adb9

- [ ] `LegacyCountdownFlagQueryPorts`：抽象接口；待复核。
- 定义：[include/openswd3/rendering/legacy_countdown.hpp:24](../../../include/openswd3/rendering/legacy_countdown.hpp:24)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：1919
  - [include/openswd3/rendering/legacy_countdown.hpp](../../../include/openswd3/rendering/legacy_countdown.hpp)：24, 26, 32, 92
  - [src/rendering/legacy_countdown.cpp](../../../src/rendering/legacy_countdown.cpp)：160
  - [src/world_map/legacy_world_frame_coordinator.cpp](../../../src/world_map/legacy_world_frame_coordinator.cpp)：9

## PORT-ecf120dbfa3e

- [ ] `LegacyCountdownFlagPorts`：抽象接口；待复核。
- 定义：[include/openswd3/rendering/legacy_countdown.hpp:32](../../../include/openswd3/rendering/legacy_countdown.hpp:32)。
- 继承：`public LegacyCountdownFlagQueryPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：1433
  - [include/openswd3/rendering/legacy_countdown.hpp](../../../include/openswd3/rendering/legacy_countdown.hpp)：32, 34, 48
  - [src/rendering/legacy_countdown.cpp](../../../src/rendering/legacy_countdown.cpp)：136
  - [src/world_map/legacy_world_story_vm.cpp](../../../src/world_map/legacy_world_story_vm.cpp)：295
  - [tests/unit/battle/legacy_battle_action_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_action_dispatch_test.cpp)：453
  - [tests/unit/battle/legacy_battle_frame_coordinator_test.cpp](../../../tests/unit/battle/legacy_battle_frame_coordinator_test.cpp)：525
  - [tests/unit/battle/legacy_battle_group_a_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_a_frame_test.cpp)：190
  - [tests/unit/battle/legacy_battle_group_b_action_execution_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_action_execution_test.cpp)：185
  - [tests/unit/battle/legacy_battle_group_b_frame_test.cpp](../../../tests/unit/battle/legacy_battle_group_b_frame_test.cpp)：175
  - [tests/unit/battle/legacy_battle_opponent_action_dispatch_test.cpp](../../../tests/unit/battle/legacy_battle_opponent_action_dispatch_test.cpp)：182
  - [tests/unit/battle/legacy_battle_target_ready_test.cpp](../../../tests/unit/battle/legacy_battle_target_ready_test.cpp)：113
  - [tests/unit/battle/legacy_battle_transition_test.cpp](../../../tests/unit/battle/legacy_battle_transition_test.cpp)：438
  - [tests/unit/rendering/legacy_countdown_test.cpp](../../../tests/unit/rendering/legacy_countdown_test.cpp)：20, 30

## PORT-6f7b7ea2473d

- [ ] `LegacyEffectPanelActionPorts`：抽象接口；待复核。
- 定义：[include/openswd3/rendering/legacy_effect_panel.hpp:8](../../../include/openswd3/rendering/legacy_effect_panel.hpp:8)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/rendering/legacy_effect_panel.hpp](../../../include/openswd3/rendering/legacy_effect_panel.hpp)：8, 10, 49
  - [include/openswd3/rendering/legacy_pause_overlay.hpp](../../../include/openswd3/rendering/legacy_pause_overlay.hpp)：33
  - [src/rendering/legacy_effect_panel.cpp](../../../src/rendering/legacy_effect_panel.cpp)：37
  - [src/rendering/legacy_pause_overlay.cpp](../../../src/rendering/legacy_pause_overlay.cpp)：26
  - [tests/unit/rendering/legacy_effect_panel_test.cpp](../../../tests/unit/rendering/legacy_effect_panel_test.cpp)：16, 37
  - [tests/unit/rendering/legacy_pause_overlay_test.cpp](../../../tests/unit/rendering/legacy_pause_overlay_test.cpp)：19, 38

## PORT-7819f49903f3

- [ ] `LegacyPresentationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/rendering/legacy_presentation.hpp:90](../../../include/openswd3/rendering/legacy_presentation.hpp:90)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/rendering/legacy_pause_overlay.hpp](../../../include/openswd3/rendering/legacy_pause_overlay.hpp)：40
  - [include/openswd3/rendering/legacy_presentation.hpp](../../../include/openswd3/rendering/legacy_presentation.hpp)：90, 92, 136
  - [include/openswd3/world_map/legacy_world_load_progress.hpp](../../../include/openswd3/world_map/legacy_world_load_progress.hpp)：70
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1361, 1681, 1729, 5640, 6091
  - [src/rendering/legacy_pause_overlay.cpp](../../../src/rendering/legacy_pause_overlay.cpp)：33
  - [src/rendering/legacy_presentation.cpp](../../../src/rendering/legacy_presentation.cpp)：152
  - [src/world_map/legacy_world_load_progress.cpp](../../../src/world_map/legacy_world_load_progress.cpp)：78
  - [tests/unit/rendering/legacy_pause_overlay_test.cpp](../../../tests/unit/rendering/legacy_pause_overlay_test.cpp)：30, 100
  - [tests/unit/rendering/legacy_presentation_test.cpp](../../../tests/unit/rendering/legacy_presentation_test.cpp)：15, 28
  - [tests/unit/world_map/legacy_world_load_progress_test.cpp](../../../tests/unit/world_map/legacy_world_load_progress_test.cpp)：26, 126

## PORT-3f093075afdf

- [ ] `LegacyTimedMessageInputPorts`：抽象接口；待复核。
- 定义：[include/openswd3/rendering/legacy_timed_messages.hpp:17](../../../include/openswd3/rendering/legacy_timed_messages.hpp:17)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/rendering/legacy_timed_messages.hpp](../../../include/openswd3/rendering/legacy_timed_messages.hpp)：17, 19, 48, 60, 71
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1357
  - [src/rendering/legacy_timed_messages.cpp](../../../src/rendering/legacy_timed_messages.cpp)：18, 44
  - [tests/unit/rendering/legacy_timed_messages_test.cpp](../../../tests/unit/rendering/legacy_timed_messages_test.cpp)：26, 29

## PORT-d645d7d74025

- [ ] `LegacyTimedMessageRuntimePorts`：抽象接口；待复核。
- 定义：[include/openswd3/rendering/legacy_timed_messages.hpp:35](../../../include/openswd3/rendering/legacy_timed_messages.hpp:35)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/rendering/legacy_timed_messages.hpp](../../../include/openswd3/rendering/legacy_timed_messages.hpp)：35, 37, 45
  - [include/openswd3/world_map/legacy_world_frame_runtime.hpp](../../../include/openswd3/world_map/legacy_world_frame_runtime.hpp)：89
  - [tests/unit/world_map/legacy_world_frame_coordinator_test.cpp](../../../tests/unit/world_map/legacy_world_frame_coordinator_test.cpp)：143
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：335
  - [tests/unit/world_map/legacy_world_runtime_session_test.cpp](../../../tests/unit/world_map/legacy_world_runtime_session_test.cpp)：311

## PORT-8c97736660d8

- [ ] `LegacyBoundTimedMessageRuntimePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/rendering/legacy_timed_messages.hpp:44](../../../include/openswd3/rendering/legacy_timed_messages.hpp:44)。
- 继承：`public LegacyTimedMessageRuntimePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/rendering/legacy_timed_messages.hpp](../../../include/openswd3/rendering/legacy_timed_messages.hpp)：44, 47
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6623
  - [src/rendering/legacy_timed_messages.cpp](../../../src/rendering/legacy_timed_messages.cpp)：17, 28
  - [tests/unit/rendering/legacy_timed_messages_test.cpp](../../../tests/unit/rendering/legacy_timed_messages_test.cpp)：17, 92

## PORT-6150ecc45d27

- [ ] `LegacyStandardModeItemPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:85](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:85)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：85, 87, 7843
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：7170
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：22452
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：114, 1672

## PORT-b67baacff7a6

- [ ] `LegacyStandardModeStoryFlagPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:121](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:121)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：121, 123, 1089, 1470, 1484, 1990
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：22550

## PORT-5a66171a78a0

- [ ] `LegacyTitleMenuPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:231](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:231)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：231, 233, 337, 976, 1003, 1019, 1023, 1036, 1065, 1085, 1094
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6888, 6897
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：3502, 3564, 3618, 3841, 3939, 4053, 4217, 4271
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：344, 584, 1660, 4986, 11729, 28004, 28473

## PORT-0f6cfa14a197

- [ ] `LegacySystemMenuPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:540](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:540)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：540, 542, 607, 611, 667, 682, 686, 690, 694, 698, 702, 706, 710, 714, 738
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：362, 413, 510, 569, 619, 775, 913, 1052, 1110, 1173, 1293, 1427, 1722
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：597

## PORT-9d2976e0ec3e

- [ ] `LegacyCharacterAttributesPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:822](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:822)。
- 继承：`public virtual battle::LegacyBattleMonDatabasePort,
      public virtual battle::LegacyBattleLevelDatabasePort,
      public virtual battle::LegacyBattleFixedObjectState`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：822, 827, 910, 915, 920, 925, 930, 935, 940, 945, 955, 960
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：2584, 2683, 2756, 2878, 2899, 2913, 2953, 2988, 2994, 3016, 3087, 3447
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1533

## PORT-2883e01a95f2

- [ ] `LegacyStandardModeCallbackBindingPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1088](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1088)。
- 继承：`public virtual LegacyStandardModeStoryFlagPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：538, 543, 1088, 1091, 2021, 2087, 2139, 4639, 5126, 6183
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6867
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：4723, 25834
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：127, 571, 603, 1647

## PORT-226b70af16c1

- [ ] `LegacyStandardModeInputPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1135](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1135)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1135, 1137, 7835
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：7245
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：316, 22353
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：117, 1704

## PORT-60173815f0c7

- [ ] `LegacyStandardModeBarPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1191](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1191)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1191, 1193, 3392, 4708, 5686, 7790
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：21808
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：125, 1749, 1845, 4659, 8147, 8150

## PORT-15885508395d

- [ ] `LegacyGameMenuEntryAnimationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1258](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1258)。
- 继承：`public virtual battle::LegacyBattleLevelDatabasePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1258, 1261, 7804
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：7496
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：21924
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：171, 1939

## PORT-082801c27f01

- [ ] `LegacyStandardModePanelPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1323](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1323)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1323, 1325, 7815
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：7349
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：22098
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：150, 2026

## PORT-b2d3ee9c6ec2

- [ ] `LegacyStandardModeRenderPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1380](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1380)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1380, 1382, 7825
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：7283
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：22209
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：153, 2119

## PORT-890017339787

- [ ] `LegacyStandardModeSelectorPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1430](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1430)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1430, 1432, 7852
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6857
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：22507
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：170, 2230

## PORT-5ac1fc8dbe44

- [ ] `LegacyStandardSpecialModeInitializationPorts`：实现或转发类；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1469](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1469)。
- 继承：`public virtual LegacyStandardModeStoryFlagPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1469, 1472, 2130
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6825
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：22598
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：183, 277

## PORT-30a39f55d9df

- [ ] `LegacyStandardSpecialModePorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1494](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1494)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1494, 1496, 7860
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：7828
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：25935
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：184, 2301

## PORT-4f2130341b89

- [ ] `LegacyGameMenuInitializationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1591](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1591)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1591, 1593, 1626, 1668
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：347, 22684
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：28023

## PORT-98f0f9166d53

- [ ] `LegacyGameMenuCleanupPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1631](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1631)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1631, 1633, 1651, 1669, 5116
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：338, 10568, 22752
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：28102

## PORT-bbd2451bdf89

- [ ] `LegacyGameMenuMainInputPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1668](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1668)。
- 继承：`public LegacyGameMenuInitializationPorts,
                                     public LegacyGameMenuCleanupPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1668, 1671, 1739, 1777, 1815, 1858, 1900, 1929, 1956, 5211, 5248
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：22781, 23461, 23646, 23823, 24040, 24198, 24360, 24978, 25070
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：28555

## PORT-4e264659f423

- [ ] `LegacyGameMenuSelectionPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:1989](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:1989)。
- 继承：`public virtual LegacyStandardModeStoryFlagPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1989, 1992, 2008, 2016, 2020
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：25666, 25700

## PORT-1a79a30ffeff

- [ ] `LegacyGameMenuCommitPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2019](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2019)。
- 继承：`public virtual LegacyGameMenuSelectionPorts,
      public virtual LegacyStandardModeCallbackBindingPorts`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2019, 2023, 2045, 2090
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：25738

## PORT-11f51aa89fa6

- [ ] `LegacyGameMenuDrawPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2048](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2048)。
- 继承：`无`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2048, 2050, 2076
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：25791
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：28817

## PORT-401ec68b803d

- [ ] `LegacyGameMenuInputPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2090](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2090)。
- 继承：`public virtual LegacyGameMenuCommitPorts`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2090, 2092, 2115
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：25856
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：27962

## PORT-cb2a6f512755

- [ ] `LegacyStandardModeFilterQueryPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2169](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2169)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2169, 2171, 4596, 5124, 7099
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：13653
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：145, 3173

## PORT-b43455f5e226

- [ ] `LegacyStandardModeDialogSetupPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2223](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2223)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2223, 2225, 4597, 5125, 7113
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：13830
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：131, 3388

## PORT-e8dd396ef2c9

- [ ] `LegacyStandardModeQuantityPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2266](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2266)。
- 继承：`public virtual battle::LegacyBattleMonDatabasePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2266, 2269, 2321, 2361, 2429, 2445, 2648, 2711, 3863
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：4796, 4993, 5223, 5247, 5733
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：824

## PORT-301205ee587e

- [ ] `LegacyStandardModeRecordClonePorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2380](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2380)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2380, 2382, 2411, 3158, 4066, 4101
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：5195, 10239
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1499

## PORT-9ef02929206c

- [ ] `LegacyGuardianAttributeApplicationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2553](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2553)。
- 继承：`public virtual battle::LegacyBattleMonDatabasePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2553, 2556, 2571, 2770, 4599, 4807, 5127
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：2680, 5374
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：17965

## PORT-c234f0393e38

- [ ] `LegacySpecialModeRuntimeInitializationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2598](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2598)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2598, 2600, 2644, 3390
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：5529
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1067, 1841, 18154

## PORT-b83afa9c93eb

- [ ] `LegacySpecialModeRuntimeCleanupPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2647](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2647)。
- 继承：`public virtual LegacyStandardModeQuantityPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2647, 2650, 2674, 2847, 3157
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：5637, 5892
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：18366

## PORT-0a31ecd7834c

- [ ] `LegacySpecialModeEquipmentContributionPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2714](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2714)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2714, 2716, 2744, 2913
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：5744
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：890

## PORT-b32e1b2cdc82

- [ ] `LegacySpecialModeAttributeComparisonPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2769](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2769)。
- 继承：`public virtual LegacyGuardianAttributeApplicationPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2769, 2772, 2807, 2904
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：10083
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：903

## PORT-a98dd2e88ca0

- [ ] `LegacySpecialModeAttributeDeltaRenderPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2858](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2858)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2858, 2860, 2882, 3387
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：5975
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：929

## PORT-08f7de4b73f0

- [ ] `LegacySpecialModeModeOneAdvancePorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2903](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2903)。
- 继承：`public virtual LegacySpecialModeAttributeComparisonPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2903, 2906, 2912, 2955, 2995, 3035, 3073, 3108
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：6051, 6188, 6318, 6458, 6576

## PORT-190f4c266df9

- [ ] `LegacySpecialModeModeOneIncreasePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:2911](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:2911)。
- 继承：`public virtual LegacySpecialModeModeOneAdvancePorts,
      public virtual LegacySpecialModeEquipmentContributionPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2911, 2915, 3152, 3156
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：6660
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：960

## PORT-7b1f377f89e8

- [ ] `LegacySpecialModeModeOneConfirmPorts`：实现或转发类；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:3155](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:3155)。
- 继承：`public virtual LegacySpecialModeModeOneIncreasePorts,
      public virtual LegacySpecialModeRuntimeCleanupPorts,
      public virtual LegacyStandardModeRecordClonePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：3155, 3160, 3224, 3286, 3342, 3386
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：6821, 7361, 7569
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：990

## PORT-a0611b019956

- [ ] `LegacySpecialModeModeOneFramePorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:3385](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:3385)。
- 继承：`public virtual LegacySpecialModeModeOneConfirmPorts,
      public virtual LegacySpecialModeAttributeDeltaRenderPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：3385, 3389, 3454
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：7868
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1833

## PORT-8df545dcd7c2

- [ ] `LegacyInputMenuSavePreviewResetPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:3482](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:3482)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：3482, 3484, 3518, 3535
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：8551
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：8710, 8756
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1096

## PORT-27aeb06d8cfd

- [ ] `LegacyHighPriorityMenuFramePorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:3554](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:3554)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：3554, 3556, 3594
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：8774
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1172

## PORT-860e4fa62995

- [ ] `LegacyHighPriorityCommonInputPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:3606](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:3606)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：3539, 3595, 3606, 3608, 3633
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：8775, 8849
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1145

## PORT-27fbf26e4f06

- [ ] `LegacyPartyDialogColumnPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:3671](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:3671)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：3671, 3673, 3692, 3862
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：8958
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1201

## PORT-77debf8b3c2c

- [ ] `LegacyPartyDialogRowPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:3710](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:3710)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：3710, 3712, 3736, 3740
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：8990, 9045
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1219

## PORT-69b0e4310f94

- [ ] `LegacyPartyDialogReplaceRowPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:3739](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:3739)。
- 继承：`public virtual LegacyPartyDialogRowPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：3739, 3742, 3765, 3779
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：9057
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1251

## PORT-764b39aa7ad7

- [ ] `LegacyPartyDialogPagePorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:3778](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:3778)。
- 继承：`public virtual LegacyPartyDialogReplaceRowPorts,
      public virtual battle::LegacyBattleFixedObjectState,
      public virtual battle::LegacyBattleMonDatabasePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：3778, 3783, 3811, 3861
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：9082
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1265

## PORT-1f08ab418d75

- [ ] `LegacyPartyDialogPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:3860](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:3860)。
- 继承：`public virtual LegacyPartyDialogPagePorts,
      public virtual LegacyPartyDialogColumnPorts,
      public virtual LegacyStandardModeQuantityPorts,
      public virtual world_map::LegacyPartyMemberFieldWritePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：3860, 3866, 3934
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：9364, 9430, 9439, 9454
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1285

## PORT-c8fab518a22f

- [ ] `LegacyObjectLabelPanelPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:3979](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:3979)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：26, 308, 3979, 3981, 4038
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6901, 7143
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：9913
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：308, 527, 535, 1398

## PORT-d600df7b54bd

- [ ] `LegacyStandardModeRecordCleanupPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4069](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4069)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1629, 1634, 4069, 4071, 4097
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：10409
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：28103, 28105, 28141, 28582

## PORT-5f33d6080f39

- [ ] `LegacyStandardModeRecordInitializationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4100](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4100)。
- 继承：`public LegacyStandardModeRecordClonePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1526, 1600, 4100, 4103, 4133
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：10333
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：28025, 28040, 28140, 28573

## PORT-4c7f47a3eb96

- [ ] `LegacyStandardModeEquipmentRecordListPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4226](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4226)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4226, 4228, 4258, 4279, 4301, 4335, 4404
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：10763, 10827

## PORT-604d25c6e388

- [ ] `LegacyStandardModeEquipmentActionCountPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4282](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4282)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4282, 4284, 4297, 4302, 4531
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：10865

## PORT-f16180caea76

- [ ] `LegacyStandardModeEquipmentInitializationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4300](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4300)。
- 继承：`public virtual LegacyStandardModeEquipmentRecordListPorts,
      public virtual LegacyStandardModeEquipmentActionCountPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4300, 4304, 4331
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：10897
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：4166

## PORT-9e3fe2840361

- [ ] `LegacyStandardModeEquipmentCleanupPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4334](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4334)。
- 继承：`public virtual LegacyStandardModeEquipmentRecordListPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4334, 4337, 4358, 4598
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：13223
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：6920

## PORT-98b07ad2c5e5

- [ ] `LegacyStandardModeEquipmentAdvancePorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4403](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4403)。
- 继承：`public virtual LegacyStandardModeEquipmentRecordListPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4403, 4406, 4431, 4449, 4453, 4514, 4526
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：10968, 11055, 12541, 12637

## PORT-4ac389c112f2

- [ ] `LegacyStandardModeEquipmentPartyCyclePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4529](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4529)。
- 继承：`public LegacyStandardModeEquipmentPageAdvancePorts,
      public virtual LegacyStandardModeEquipmentActionCountPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4529, 4533, 4556, 4560
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：12190

## PORT-1b5618d52c4d

- [ ] `LegacyStandardModeEquipmentListKindCyclePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4559](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4559)。
- 继承：`public LegacyStandardModeEquipmentPartyCyclePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4559, 4562, 4585, 4595
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：12125

## PORT-9d47b870c013

- [ ] `LegacyStandardModeEquipmentCommitPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4593](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4593)。
- 继承：`public virtual battle::LegacyBattleMonDatabasePort,
      public LegacyStandardModeEquipmentListKindCyclePorts,
      public LegacyStandardModeFilterQueryPorts,
      public LegacyStandardModeDialogSetupPorts,
      public LegacyStandardModeEquipmentCleanupPorts,
      public virtual LegacyGuardianAttributeApplicationPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4593, 4601, 4634, 4638
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：11785

## PORT-ecf7b561c363

- [ ] `LegacyStandardModeEquipmentExitPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4637](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4637)。
- 继承：`public LegacyStandardModeEquipmentCommitPorts,
      public LegacyStandardModeCallbackBindingPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4637, 4641, 4662, 4743
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：11725

## PORT-c642760a2098

- [ ] `LegacyStandardModeEquipmentRenderPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4690](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4690)。
- 继承：`public virtual battle::LegacyBattleMonDatabasePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4690, 4693, 4739
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：11145
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：4604

## PORT-650ab29bc2af

- [ ] `LegacyStandardModeEquipmentInputPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4742](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4742)。
- 继承：`public LegacyStandardModeEquipmentExitPorts`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4742, 4745, 5277
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：12747
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：4919

## PORT-f448de2d7d21

- [ ] `LegacyStandardModeGuardianAttributeCachePorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4806](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4806)。
- 继承：`public virtual LegacyGuardianAttributeApplicationPorts,
      public virtual battle::LegacyBattleFixedObjectState`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4806, 4810, 4832, 5536, 7520, 7548, 7566, 7595, 7617
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：10210, 18337, 18396, 18474, 18510, 18617, 18682

## PORT-a9f046e81eb1

- [ ] `LegacyStandardModeGuardianListRefreshPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4831](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4831)。
- 继承：`public LegacyStandardModeGuardianAttributeCachePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4831, 4834, 5561, 5730, 7024, 7032
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：13321, 13345

## PORT-4bc9d20f3e26

- [ ] `LegacyStandardModeMissingNodePorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4842](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4842)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4842, 4844, 5079, 5123, 6040, 7085
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：13501
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：115, 2799

## PORT-071862d78d6d

- [ ] `LegacyGameMenuPageRenderPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4927](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4927)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1525, 2055, 4927, 4929, 4965
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：24558
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：28718, 28827

## PORT-4fae7fa4785d

- [ ] `LegacyStandardModeResourceCommitPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:4982](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:4982)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：4982, 4984, 5032, 5154
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：10447
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：28242, 28498

## PORT-be3fbc993329

- [ ] `LegacyStandardModeSpecialWorldTransitionPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5046](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5046)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5046, 5048, 5118, 5142
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：10570, 25239
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：28355, 28441

## PORT-3ade156458ce

- [ ] `LegacyStandardModeSpecialWorldReturnPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5078](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5078)。
- 继承：`public LegacyStandardModeMissingNodePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5078, 5081, 5110
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：10602
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：28212

## PORT-c81606c60245

- [ ] `LegacyGameMenuInteractionCommitPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5121](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5121)。
- 继承：`public virtual battle::LegacyBattleMonDatabasePort,
      public LegacyStandardModeMissingNodePorts,
      public LegacyStandardModeFilterQueryPorts,
      public LegacyStandardModeDialogSetupPorts,
      public LegacyStandardModeCallbackBindingPorts,
      public virtual LegacyGuardianAttributeApplicationPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1524, 1677, 5121, 5129, 5213, 5250
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：22797, 22821, 24980, 25072
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：28353, 28615

## PORT-78ad16d29f4e

- [ ] `LegacyStandardModeGuardianSelectionPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5560](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5560)。
- 继承：`public LegacyStandardModeGuardianListRefreshPorts`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5560, 5563, 5574, 7411, 7419, 7427, 7435, 7443, 7451, 7460
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：18744, 18774, 18967, 18983, 18999, 19015, 19031, 19093, 19105, 19118

## PORT-abf788bb8c88

- [ ] `LegacyStandardModeGuardianInteractionPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5573](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5573)。
- 继承：`public LegacyStandardModeGuardianSelectionPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5573, 5576, 5590, 7468
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：19256

## PORT-74a54ac015d2

- [ ] `LegacyStandardModeGuardianCommitPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5589](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5589)。
- 继承：`public LegacyStandardModeGuardianInteractionPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5589, 5592, 5717, 7477
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：20462
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：7072

## PORT-cad6ba627883

- [ ] `LegacyStandardModeGuardianRenderPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5683](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5683)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5683, 5685, 7626, 7638, 7646, 7656
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：19472, 19510, 19675, 19913
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：8146

## PORT-0e6fd985c0f9

- [ ] `LegacyStandardModeGuardianInputPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5716](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5716)。
- 继承：`public LegacyStandardModeGuardianCommitPorts`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5716, 5719, 7668
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：20582
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：9492

## PORT-ec7da6e39a5e

- [ ] `LegacyStandardModeGuardianInitializationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5729](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5729)。
- 继承：`public LegacyStandardModeGuardianListRefreshPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5729, 5732, 7676
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：20955
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：3936

## PORT-a335a80f59c9

- [ ] `LegacyStandardModeDatabaseRecordRefreshPorts`：实现或转发类；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5812](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5812)。
- 继承：`public virtual battle::LegacyBattleMonDatabasePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5812, 5815, 5867, 7264
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：15800
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：10830

## PORT-388be6ad964d

- [ ] `LegacyStandardModeDatabaseAdvancePorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5866](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5866)。
- 继承：`public LegacyStandardModeDatabaseRecordRefreshPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5866, 5869, 5947, 7221, 7250, 7257, 7378
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：15338, 15601, 15712, 17737
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：10115, 11415

## PORT-79248d20d342

- [ ] `LegacyStandardModeDatabaseRetreatPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5946](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5946)。
- 继承：`public LegacyStandardModeDatabaseAdvancePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5946, 5949, 6039, 7341, 7348, 7371, 7385
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：17367, 17434, 17444, 17654, 17816
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：11290, 11531

## PORT-4bc67aa34e87

- [ ] `LegacyStandardModeDatabaseForwardReleasePorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:5996](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:5996)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5996, 5998, 6011, 6379, 7236
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：15481

## PORT-faeb0576fd0f

- [ ] `LegacyStandardModeDatabaseForwardRefreshPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6010](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6010)。
- 继承：`public LegacyStandardModeDatabaseForwardReleasePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：6010, 6013, 6041, 6388, 7283
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：16010

## PORT-26b9671db8fa

- [ ] `LegacyStandardModeDatabaseCyclePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6038](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6038)。
- 继承：`public LegacyStandardModeDatabaseRetreatPorts,
      public LegacyStandardModeMissingNodePorts,
      public LegacyStandardModeDatabaseForwardRefreshPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：6038, 6043, 6127, 7356, 7364
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：17455, 17553
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：10225

## PORT-ad2f39b67ec2

- [ ] `LegacyStandardModeAltarSurfaceReleasePorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6080](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6080)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：6080, 6082, 6128, 6244, 7303
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：16907

## PORT-d76783483169

- [ ] `LegacyStandardModeDatabaseCommitPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6126](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6126)。
- 继承：`public LegacyStandardModeDatabaseCyclePorts,
      public LegacyStandardModeAltarSurfaceReleasePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：6126, 6130, 6182, 7327
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：16976

## PORT-f7be2b98551a

- [ ] `LegacyStandardModeDatabaseExitPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6181](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6181)。
- 继承：`public LegacyStandardModeDatabaseCommitPorts,
      public LegacyStandardModeCallbackBindingPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：6181, 6185, 6324, 7229, 7334
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：15417, 17062

## PORT-84be156866b8

- [ ] `LegacyStandardModeDatabaseRenderPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6243](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6243)。
- 继承：`public LegacyStandardModeAltarSurfaceReleasePorts`。
- 直接残留：operation。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：6243, 6246, 7296, 7309, 7315
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：16050, 16201, 16403
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：12667

## PORT-358eb3ea4fad

- [ ] `LegacyStandardModeDatabaseInputPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6323](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6323)。
- 继承：`public LegacyStandardModeDatabaseExitPorts`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：6323, 6326, 7395
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：17902
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：11664

## PORT-a6aad0a268b8

- [ ] `LegacyStandardModeDatabaseCleanupPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6378](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6378)。
- 继承：`public LegacyStandardModeDatabaseForwardReleasePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：6124, 6186, 6378, 6381, 7402
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：18202
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：11666, 11734, 13234

## PORT-657571ee675d

- [ ] `LegacyStandardModeDatabaseInitializationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6387](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6387)。
- 继承：`public LegacyStandardModeDatabaseForwardRefreshPorts,
      public virtual battle::LegacyBattleMonDatabasePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：6387, 6391, 7683
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：21059
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：9907

## PORT-3c29ec0d4f6a

- [ ] `LegacyStandardModeEntryInitializationPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6472](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6472)。
- 继承：`public virtual battle::LegacyBattleMonDatabasePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：6472, 6475, 6555, 7140
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：13944
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：140, 3650

## PORT-5d8eb418d0ae

- [ ] `LegacyStandardModeEntryConsumptionPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6554](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6554)。
- 继承：`public LegacyStandardModeEntryInitializationPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：6554, 6557, 6572, 6706, 7148, 7156, 7164
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：14101, 14251, 14497
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：13572

## PORT-e100d690c49a

- [ ] `LegacyStandardModeRuntimeInitializationPorts`：实现或转发类；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6571](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6571)。
- 继承：`public LegacyStandardModeEntryConsumptionPorts,
      public virtual battle::LegacyBattleFixedObjectState`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：6571, 6575, 7171
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：14594
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：154, 13378

## PORT-2e3ec804b45e

- [ ] `LegacyStandardModeInputDispatchPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6705](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6705)。
- 继承：`public LegacyStandardModeEntryConsumptionPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1527, 1737, 1776, 1814, 1857, 1899, 1928, 1955, 1978, 1986, 5210, 5247, 6705, 6708, 6737, 7179, 7187, 7195, 7203, 7706
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：14687, 14728, 14766, 14804, 14849, 14909, 22779, 23460, 23645, 23822, 24039, 24197, 24359, 24489, 24523, 24977, 25069
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：165, 14148, 28666

## PORT-eca06f1d07b0

- [ ] `LegacyStandardModeRuntimeRenderPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6840](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6840)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：1528, 4940, 6840, 6842, 7214, 7690, 7697
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：15145, 21160, 21232
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：157, 14876, 28719, 28750

## PORT-e91405a267f7

- [ ] `LegacyStandardModeAnimatedPanelPorts`：抽象接口；待复核。
- 定义：[include/openswd3/special_modes/legacy_standard_mode.hpp:6997](../../../include/openswd3/special_modes/legacy_standard_mode.hpp:6997)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：5681, 5688, 6997, 6999, 7770
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：21684
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：119, 8148, 8154, 16117

## PORT-c406ab9cca7d

- [ ] `LegacyDialogClosePorts`：抽象接口；待复核。
- 定义：[include/openswd3/story_scene/legacy_dialog_control.hpp:53](../../../include/openswd3/story_scene/legacy_dialog_control.hpp:53)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/story_scene/legacy_dialog_control.hpp](../../../include/openswd3/story_scene/legacy_dialog_control.hpp)：53, 55, 91
  - [include/openswd3/story_scene/legacy_dialog_runtime.hpp](../../../include/openswd3/story_scene/legacy_dialog_runtime.hpp)：113
  - [src/story_scene/legacy_dialog_control.cpp](../../../src/story_scene/legacy_dialog_control.cpp)：215
  - [tests/unit/story_scene/legacy_dialog_control_test.cpp](../../../tests/unit/story_scene/legacy_dialog_control_test.cpp)：11, 28

## PORT-a9c84532fd9a

- [ ] `LegacyDialogRuntimePorts`：抽象接口；待复核。
- 定义：[include/openswd3/story_scene/legacy_dialog_runtime.hpp:112](../../../include/openswd3/story_scene/legacy_dialog_runtime.hpp:112)。
- 继承：`public LegacyDialogTextPorts,
                                 public LegacyDialogClosePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_frame_coordinator.hpp](../../../include/openswd3/battle/legacy_battle_frame_coordinator.hpp)：1917
  - [include/openswd3/story_scene/legacy_dialog_runtime.hpp](../../../include/openswd3/story_scene/legacy_dialog_runtime.hpp)：112, 115, 179
  - [include/openswd3/world_map/legacy_world_dialog_runtime.hpp](../../../include/openswd3/world_map/legacy_world_dialog_runtime.hpp)：55
  - [include/openswd3/world_map/legacy_world_frame_runtime.hpp](../../../include/openswd3/world_map/legacy_world_frame_runtime.hpp)：94
  - [src/story_scene/legacy_dialog_runtime.cpp](../../../src/story_scene/legacy_dialog_runtime.cpp)：42, 65, 139
  - [tests/unit/battle/legacy_battle_frame_coordinator_test.cpp](../../../tests/unit/battle/legacy_battle_frame_coordinator_test.cpp)：473
  - [tests/unit/story_scene/legacy_dialog_runtime_test.cpp](../../../tests/unit/story_scene/legacy_dialog_runtime_test.cpp)：25, 59
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：353

## PORT-5560115ba4f6

- [ ] `LegacyDialogTextPorts`：抽象接口；待复核。
- 定义：[include/openswd3/story_scene/legacy_dialog_text.hpp:134](../../../include/openswd3/story_scene/legacy_dialog_text.hpp:134)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/story_scene/legacy_dialog_runtime.hpp](../../../include/openswd3/story_scene/legacy_dialog_runtime.hpp)：112
  - [include/openswd3/story_scene/legacy_dialog_text.hpp](../../../include/openswd3/story_scene/legacy_dialog_text.hpp)：134, 136, 186
  - [src/story_scene/legacy_dialog_text.cpp](../../../src/story_scene/legacy_dialog_text.cpp)：344
  - [tests/unit/story_scene/legacy_dialog_text_test.cpp](../../../tests/unit/story_scene/legacy_dialog_text_test.cpp)：20, 38

## PORT-980920a4709e

- [ ] `LegacyMovementCollisionPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_movement_collision.hpp:27](../../../include/openswd3/world_map/legacy_movement_collision.hpp:27)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_movement_collision.hpp](../../../include/openswd3/world_map/legacy_movement_collision.hpp)：27, 29, 43
  - [src/world_map/legacy_movement_collision.cpp](../../../src/world_map/legacy_movement_collision.cpp)：16, 39, 59, 74, 120
  - [tests/unit/world_map/legacy_movement_collision_test.cpp](../../../tests/unit/world_map/legacy_movement_collision_test.cpp)：19, 30

## PORT-d5fee01d31a7

- [ ] `LegacyPictureActionAudioPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_picture_actions.hpp:41](../../../include/openswd3/world_map/legacy_picture_actions.hpp:41)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_picture_actions.hpp](../../../include/openswd3/world_map/legacy_picture_actions.hpp)：41, 43, 78
  - [include/openswd3/world_map/legacy_world_roles.hpp](../../../include/openswd3/world_map/legacy_world_roles.hpp)：40
  - [src/world_map/legacy_picture_actions.cpp](../../../src/world_map/legacy_picture_actions.cpp)：57
  - [tests/unit/world_map/legacy_picture_actions_test.cpp](../../../tests/unit/world_map/legacy_picture_actions_test.cpp)：22, 87

## PORT-a955630ca2d7

- [ ] `LegacyWorldEncounterPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_random_encounter.hpp:148](../../../include/openswd3/world_map/legacy_random_encounter.hpp:148)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_random_encounter.hpp](../../../include/openswd3/world_map/legacy_random_encounter.hpp)：148, 150, 196
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1945
  - [src/world_map/legacy_random_encounter.cpp](../../../src/world_map/legacy_random_encounter.cpp)：299
  - [tests/unit/world_map/legacy_random_encounter_test.cpp](../../../tests/unit/world_map/legacy_random_encounter_test.cpp)：32, 472

## PORT-54a06032340a

- [ ] `LegacyWorldCollisionTalkPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_collision_talk.hpp:45](../../../include/openswd3/world_map/legacy_world_collision_talk.hpp:45)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_collision_talk.hpp](../../../include/openswd3/world_map/legacy_world_collision_talk.hpp)：45, 47, 105
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1884
  - [src/world_map/legacy_world_collision_talk.cpp](../../../src/world_map/legacy_world_collision_talk.cpp)：52, 79
  - [tests/unit/world_map/legacy_world_collision_talk_test.cpp](../../../tests/unit/world_map/legacy_world_collision_talk_test.cpp)：22, 48

## PORT-5d0a423dd6fb

- [ ] `LegacyWorldDebugHotkeyPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_debug_hotkeys.hpp:42](../../../include/openswd3/world_map/legacy_world_debug_hotkeys.hpp:42)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_debug_hotkeys.hpp](../../../include/openswd3/world_map/legacy_world_debug_hotkeys.hpp)：42, 44, 94
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1806
  - [src/world_map/legacy_world_debug_hotkeys.cpp](../../../src/world_map/legacy_world_debug_hotkeys.cpp)：59
  - [tests/unit/world_map/legacy_world_debug_hotkeys_test.cpp](../../../tests/unit/world_map/legacy_world_debug_hotkeys_test.cpp)：19, 25

## PORT-0004f3a1aedd

- [ ] `LegacyWorldDebugOverlayPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_debug_overlay.hpp:14](../../../include/openswd3/world_map/legacy_world_debug_overlay.hpp:14)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_debug_overlay.hpp](../../../include/openswd3/world_map/legacy_world_debug_overlay.hpp)：14, 16, 113
  - [include/openswd3/world_map/legacy_world_frame_coordinator.hpp](../../../include/openswd3/world_map/legacy_world_frame_coordinator.hpp)：22
  - [src/world_map/legacy_world_debug_overlay.cpp](../../../src/world_map/legacy_world_debug_overlay.cpp)：214, 236, 317, 369, 428, 566
  - [tests/unit/world_map/legacy_world_debug_overlay_test.cpp](../../../tests/unit/world_map/legacy_world_debug_overlay_test.cpp)：24, 38

## PORT-9a9f9f7e18af

- [ ] `LegacyWorldDialogExternalPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_dialog_runtime.hpp:15](../../../include/openswd3/world_map/legacy_world_dialog_runtime.hpp:15)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_dialog_runtime.hpp](../../../include/openswd3/world_map/legacy_world_dialog_runtime.hpp)：15, 17, 68, 126
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1356
  - [src/world_map/legacy_world_dialog_runtime.cpp](../../../src/world_map/legacy_world_dialog_runtime.cpp)：232
  - [tests/unit/world_map/legacy_world_dialog_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_dialog_runtime_test.cpp)：95

## PORT-c94c56f211eb

- [ ] `LegacyWorldDialogRuntimePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/world_map/legacy_world_dialog_runtime.hpp:54](../../../include/openswd3/world_map/legacy_world_dialog_runtime.hpp:54)。
- 继承：`public story_scene::LegacyDialogRuntimePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_dialog_runtime.hpp](../../../include/openswd3/world_map/legacy_world_dialog_runtime.hpp)：54, 57
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6639
  - [src/world_map/legacy_world_dialog_runtime.cpp](../../../src/world_map/legacy_world_dialog_runtime.cpp)：221, 241, 249, 261, 265, 276, 288, 316, 349, 380, 479, 491, 495, 499, 503, 556, 577, 583, 596
  - [tests/unit/world_map/legacy_world_dialog_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_dialog_runtime_test.cpp)：135

## PORT-66dcb15417c5

- [ ] `LegacyWorldFacingTalkPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_facing_talk.hpp:12](../../../include/openswd3/world_map/legacy_world_facing_talk.hpp:12)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_facing_talk.hpp](../../../include/openswd3/world_map/legacy_world_facing_talk.hpp)：12, 14, 50
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1883
  - [src/world_map/legacy_world_facing_talk.cpp](../../../src/world_map/legacy_world_facing_talk.cpp)：24, 39
  - [tests/unit/world_map/legacy_world_facing_talk_test.cpp](../../../tests/unit/world_map/legacy_world_facing_talk_test.cpp)：15, 20

## PORT-911f7d3cb272

- [ ] `LegacyWorldFramePorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_frame_composition.hpp:50](../../../include/openswd3/world_map/legacy_world_frame_composition.hpp:50)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：2642, 3452
  - [include/openswd3/world_map/legacy_world_frame_composition.hpp](../../../include/openswd3/world_map/legacy_world_frame_composition.hpp)：50, 52, 109
  - [include/openswd3/world_map/legacy_world_frame_runtime.hpp](../../../include/openswd3/world_map/legacy_world_frame_runtime.hpp)：72
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1351
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：5527, 7866
  - [src/world_map/legacy_world_frame_composition.cpp](../../../src/world_map/legacy_world_frame_composition.cpp)：88, 97, 106, 122, 177
  - [src/world_map/legacy_world_frame_coordinator.cpp](../../../src/world_map/legacy_world_frame_coordinator.cpp)：11, 20
  - [src/world_map/legacy_world_frame_runtime.cpp](../../../src/world_map/legacy_world_frame_runtime.cpp)：36
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：18137
  - [tests/unit/world_map/legacy_world_frame_composition_test.cpp](../../../tests/unit/world_map/legacy_world_frame_composition_test.cpp)：45, 61
  - [tests/unit/world_map/legacy_world_frame_coordinator_test.cpp](../../../tests/unit/world_map/legacy_world_frame_coordinator_test.cpp)：43, 142
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：69, 162, 1027
  - [tests/unit/world_map/legacy_world_runtime_session_test.cpp](../../../tests/unit/world_map/legacy_world_runtime_session_test.cpp)：307

## PORT-e5caa4d15024

- [ ] `LegacyWorldOuterFramePorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_frame_coordinator.hpp:21](../../../include/openswd3/world_map/legacy_world_frame_coordinator.hpp:21)。
- 继承：`public LegacyWorldMapRolePathPorts,
                                   public LegacyWorldDebugOverlayPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_frame_coordinator.hpp](../../../include/openswd3/world_map/legacy_world_frame_coordinator.hpp)：21, 24, 112
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1355
  - [src/world_map/legacy_world_frame_coordinator.cpp](../../../src/world_map/legacy_world_frame_coordinator.cpp)：99
  - [tests/unit/world_map/legacy_world_frame_coordinator_test.cpp](../../../tests/unit/world_map/legacy_world_frame_coordinator_test.cpp)：50, 89
  - [tests/unit/world_map/legacy_world_runtime_session_test.cpp](../../../tests/unit/world_map/legacy_world_runtime_session_test.cpp)：310

## PORT-bc68e5f72522

- [ ] `LegacyWorldFrameRuntimePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/world_map/legacy_world_frame_runtime.hpp:69](../../../include/openswd3/world_map/legacy_world_frame_runtime.hpp:69)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_frame_coordinator.hpp](../../../include/openswd3/world_map/legacy_world_frame_coordinator.hpp)：111
  - [include/openswd3/world_map/legacy_world_frame_runtime.hpp](../../../include/openswd3/world_map/legacy_world_frame_runtime.hpp)：69, 175
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6706
  - [src/world_map/legacy_world_frame_coordinator.cpp](../../../src/world_map/legacy_world_frame_coordinator.cpp)：98
  - [src/world_map/legacy_world_frame_runtime.cpp](../../../src/world_map/legacy_world_frame_runtime.cpp)：50, 505, 534
  - [tests/unit/world_map/legacy_world_frame_coordinator_test.cpp](../../../tests/unit/world_map/legacy_world_frame_coordinator_test.cpp)：44, 434
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：70, 498, 664, 746, 830, 910, 986, 1142
  - [tests/unit/world_map/legacy_world_runtime_session_test.cpp](../../../tests/unit/world_map/legacy_world_runtime_session_test.cpp)：1062

## PORT-45f210907f93

- [ ] `LegacyWorldIndexedObjectDrawPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_indexed_objects.hpp:67](../../../include/openswd3/world_map/legacy_world_indexed_objects.hpp:67)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_indexed_objects.hpp](../../../include/openswd3/world_map/legacy_world_indexed_objects.hpp)：67, 69, 83, 135
  - [src/world_map/legacy_world_indexed_objects.cpp](../../../src/world_map/legacy_world_indexed_objects.cpp)：236
  - [tests/unit/world_map/legacy_world_indexed_objects_test.cpp](../../../tests/unit/world_map/legacy_world_indexed_objects_test.cpp)：27, 141

## PORT-21921a6efc43

- [ ] `LegacyWorldIndexedObjectRuntimeDrawPorts`：实现或转发类；待复核。
- 定义：[include/openswd3/world_map/legacy_world_indexed_objects.hpp:82](../../../include/openswd3/world_map/legacy_world_indexed_objects.hpp:82)。
- 继承：`public LegacyWorldIndexedObjectDrawPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_indexed_objects.hpp](../../../include/openswd3/world_map/legacy_world_indexed_objects.hpp)：82, 85
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：8829
  - [src/world_map/legacy_world_frame_runtime.cpp](../../../src/world_map/legacy_world_frame_runtime.cpp)：158
  - [src/world_map/legacy_world_indexed_objects.cpp](../../../src/world_map/legacy_world_indexed_objects.cpp)：188, 189, 198, 205
  - [tests/unit/world_map/legacy_world_render_session_test.cpp](../../../tests/unit/world_map/legacy_world_render_session_test.cpp)：64, 644

## PORT-c524857f12ae

- [ ] `LegacyWorldInteractionPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_interaction.hpp:64](../../../include/openswd3/world_map/legacy_world_interaction.hpp:64)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_interaction.hpp](../../../include/openswd3/world_map/legacy_world_interaction.hpp)：64, 66, 130
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1744
  - [src/world_map/legacy_world_interaction.cpp](../../../src/world_map/legacy_world_interaction.cpp)：64, 86, 193, 261, 429
  - [tests/unit/world_map/legacy_world_interaction_test.cpp](../../../tests/unit/world_map/legacy_world_interaction_test.cpp)：23, 32

## PORT-3e4b216ba4ea

- [ ] `LegacyWorldItemListStatePort`：实现或转发类；待复核。
- 定义：[include/openswd3/world_map/legacy_world_item_lifecycle.hpp:80](../../../include/openswd3/world_map/legacy_world_item_lifecycle.hpp:80)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/battle/legacy_battle_action_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_action_dispatch.hpp)：183
  - [include/openswd3/battle/legacy_battle_growth_actor_selection.hpp](../../../include/openswd3/battle/legacy_battle_growth_actor_selection.hpp)：102
  - [include/openswd3/battle/legacy_battle_level_profile.hpp](../../../include/openswd3/battle/legacy_battle_level_profile.hpp)：65
  - [include/openswd3/battle/legacy_battle_script_dispatch.hpp](../../../include/openswd3/battle/legacy_battle_script_dispatch.hpp)：295
  - [include/openswd3/battle/legacy_battle_startup.hpp](../../../include/openswd3/battle/legacy_battle_startup.hpp)：152
  - [include/openswd3/world_map/legacy_world_item_lifecycle.hpp](../../../include/openswd3/world_map/legacy_world_item_lifecycle.hpp)：80, 93, 94
  - [tests/unit/battle/legacy_battle_level_profile_test.cpp](../../../tests/unit/battle/legacy_battle_level_profile_test.cpp)：35, 71

## PORT-6aa707fd82e4

- [ ] `LegacyWorldLoadProgressPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_load_progress.hpp:28](../../../include/openswd3/world_map/legacy_world_load_progress.hpp:28)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_load_progress.hpp](../../../include/openswd3/world_map/legacy_world_load_progress.hpp)：28, 30, 68
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1731
  - [src/world_map/legacy_world_load_progress.cpp](../../../src/world_map/legacy_world_load_progress.cpp)：76
  - [tests/unit/world_map/legacy_world_load_progress_test.cpp](../../../tests/unit/world_map/legacy_world_load_progress_test.cpp)：30, 44

## PORT-c08113b1b719

- [ ] `LegacyWorldMapRolePathPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_map_role_paths.hpp:19](../../../include/openswd3/world_map/legacy_world_map_role_paths.hpp:19)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_frame_coordinator.hpp](../../../include/openswd3/world_map/legacy_world_frame_coordinator.hpp)：21
  - [include/openswd3/world_map/legacy_world_map_role_paths.hpp](../../../include/openswd3/world_map/legacy_world_map_role_paths.hpp)：19, 21, 91
  - [include/openswd3/world_map/legacy_world_role_lifecycle.hpp](../../../include/openswd3/world_map/legacy_world_role_lifecycle.hpp)：103
  - [src/world_map/legacy_world_map_role_paths.cpp](../../../src/world_map/legacy_world_map_role_paths.cpp)：246, 309
  - [src/world_map/legacy_world_role_lifecycle.cpp](../../../src/world_map/legacy_world_role_lifecycle.cpp)：155
  - [tests/unit/world_map/legacy_world_map_role_paths_test.cpp](../../../tests/unit/world_map/legacy_world_map_role_paths_test.cpp)：29, 110
  - [tests/unit/world_map/legacy_world_role_lifecycle_test.cpp](../../../tests/unit/world_map/legacy_world_role_lifecycle_test.cpp)：22, 36
  - [tests/unit/world_map/legacy_world_story_vm_test_support.hpp](../../../tests/unit/world_map/legacy_world_story_vm_test_support.hpp)：1020

## PORT-f5d91ea8758f

- [ ] `LegacyWorldPartyPathPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_path_requests.hpp:100](../../../include/openswd3/world_map/legacy_world_path_requests.hpp:100)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_path_requests.hpp](../../../include/openswd3/world_map/legacy_world_path_requests.hpp)：100, 102, 123, 142
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：5614
  - [src/world_map/legacy_world_path_requests.cpp](../../../src/world_map/legacy_world_path_requests.cpp)：113, 265, 338, 530, 586
  - [tests/unit/world_map/legacy_world_path_requests_test.cpp](../../../tests/unit/world_map/legacy_world_path_requests_test.cpp)：23, 187

## PORT-1743d43fcf00

- [ ] `LegacyWorldPathScriptPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_path_script.hpp:68](../../../include/openswd3/world_map/legacy_world_path_script.hpp:68)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_path_script.hpp](../../../include/openswd3/world_map/legacy_world_path_script.hpp)：68, 70, 116, 143
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6105
  - [src/world_map/legacy_world_path_script.cpp](../../../src/world_map/legacy_world_path_script.cpp)：192, 454, 1418
  - [tests/unit/world_map/legacy_world_path_script_test.cpp](../../../tests/unit/world_map/legacy_world_path_script_test.cpp)：40, 102

## PORT-a913b1475959

- [ ] `LegacyWorldRoleExternalPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_roles.hpp:40](../../../include/openswd3/world_map/legacy_world_roles.hpp:40)。
- 继承：`public LegacyPictureActionAudioPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_roles.hpp](../../../include/openswd3/world_map/legacy_world_roles.hpp)：40, 42, 64, 86, 124
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1353, 8728
  - [src/world_map/legacy_world_roles.cpp](../../../src/world_map/legacy_world_roles.cpp)：428
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：76, 1028
  - [tests/unit/world_map/legacy_world_runtime_session_test.cpp](../../../tests/unit/world_map/legacy_world_runtime_session_test.cpp)：308

## PORT-e8a9ed211ce0

- [ ] `LegacyWorldRoleRenderPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_roles.hpp:64](../../../include/openswd3/world_map/legacy_world_roles.hpp:64)。
- 继承：`public LegacyWorldRoleExternalPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_frame_runtime.hpp](../../../include/openswd3/world_map/legacy_world_frame_runtime.hpp)：91
  - [include/openswd3/world_map/legacy_world_roles.hpp](../../../include/openswd3/world_map/legacy_world_roles.hpp)：64, 79, 171, 215
  - [src/world_map/legacy_world_roles.cpp](../../../src/world_map/legacy_world_roles.cpp)：140, 219, 541
  - [tests/unit/world_map/legacy_world_frame_coordinator_test.cpp](../../../tests/unit/world_map/legacy_world_frame_coordinator_test.cpp)：54, 267
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：79, 227
  - [tests/unit/world_map/legacy_world_roles_test.cpp](../../../tests/unit/world_map/legacy_world_roles_test.cpp)：41, 62

## PORT-7247e75a9812

- [ ] `LegacyWorldRoleRenderRuntimePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/world_map/legacy_world_roles.hpp:78](../../../include/openswd3/world_map/legacy_world_roles.hpp:78)。
- 继承：`public LegacyWorldRoleRenderPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_roles.hpp](../../../include/openswd3/world_map/legacy_world_roles.hpp)：78, 81
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6653, 8882
  - [src/world_map/legacy_world_roles.cpp](../../../src/world_map/legacy_world_roles.cpp)：423, 433, 439, 445, 470, 503, 509, 515, 521, 525
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：80, 1111
  - [tests/unit/world_map/legacy_world_roles_test.cpp](../../../tests/unit/world_map/legacy_world_roles_test.cpp)：42, 1052
  - [tests/unit/world_map/legacy_world_runtime_session_test.cpp](../../../tests/unit/world_map/legacy_world_runtime_session_test.cpp)：960

## PORT-aa1159223764

- [ ] `LegacySaveItemDefinitionPort`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_save_restore.hpp:172](../../../include/openswd3/world_map/legacy_world_save_restore.hpp:172)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_save_restore.hpp](../../../include/openswd3/world_map/legacy_world_save_restore.hpp)：172, 174, 202, 224
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：8113
  - [src/world_map/legacy_world_save_item_lists.cpp](../../../src/world_map/legacy_world_save_item_lists.cpp)：73, 159
  - [tests/unit/world_map/legacy_world_save_restore_test.cpp](../../../tests/unit/world_map/legacy_world_save_restore_test.cpp)：24, 213

## PORT-d2da76fbb056

- [ ] `LegacyWorldSpatialAudioPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_spatial_audio.hpp:14](../../../include/openswd3/world_map/legacy_world_spatial_audio.hpp:14)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_frame_runtime.hpp](../../../include/openswd3/world_map/legacy_world_frame_runtime.hpp)：92
  - [include/openswd3/world_map/legacy_world_roles.hpp](../../../include/openswd3/world_map/legacy_world_roles.hpp)：216
  - [include/openswd3/world_map/legacy_world_spatial_audio.hpp](../../../include/openswd3/world_map/legacy_world_spatial_audio.hpp)：14, 16, 64
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1354, 8729
  - [src/world_map/legacy_world_roles.cpp](../../../src/world_map/legacy_world_roles.cpp)：542
  - [src/world_map/legacy_world_spatial_audio.cpp](../../../src/world_map/legacy_world_spatial_audio.cpp)：93
  - [tests/unit/world_map/legacy_world_frame_coordinator_test.cpp](../../../tests/unit/world_map/legacy_world_frame_coordinator_test.cpp)：57, 311
  - [tests/unit/world_map/legacy_world_frame_runtime_test.cpp](../../../tests/unit/world_map/legacy_world_frame_runtime_test.cpp)：82, 287
  - [tests/unit/world_map/legacy_world_roles_test.cpp](../../../tests/unit/world_map/legacy_world_roles_test.cpp)：45, 182
  - [tests/unit/world_map/legacy_world_runtime_session_test.cpp](../../../tests/unit/world_map/legacy_world_runtime_session_test.cpp)：309
  - [tests/unit/world_map/legacy_world_spatial_audio_test.cpp](../../../tests/unit/world_map/legacy_world_spatial_audio_test.cpp)：21, 33

## PORT-80c4115ee102

- [ ] `LegacyPartyMemberFieldWritePorts`：实现或转发类；待复核。
- 定义：[include/openswd3/world_map/legacy_world_story_vm.hpp:252](../../../include/openswd3/world_map/legacy_world_story_vm.hpp:252)。
- 继承：`public virtual battle::LegacyBattleLevelDatabasePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/special_modes/legacy_standard_mode.hpp](../../../include/openswd3/special_modes/legacy_standard_mode.hpp)：3864
  - [include/openswd3/world_map/legacy_world_story_vm.hpp](../../../include/openswd3/world_map/legacy_world_story_vm.hpp)：252, 255, 300, 451
  - [src/world_map/legacy_world_story_vm.cpp](../../../src/world_map/legacy_world_story_vm.cpp)：202, 2110
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：1197

## PORT-a00d387a8a8c

- [ ] `LegacyWorldStoryVmPorts`：抽象接口；待复核。
- 定义：[include/openswd3/world_map/legacy_world_story_vm.hpp:451](../../../include/openswd3/world_map/legacy_world_story_vm.hpp:451)。
- 继承：`public LegacyPartyMemberFieldWritePorts`。
- 直接残留：operation。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/world_map/legacy_world_story_vm.hpp](../../../include/openswd3/world_map/legacy_world_story_vm.hpp)：451, 453, 627
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：5628
  - [src/world_map/legacy_world_story_vm.cpp](../../../src/world_map/legacy_world_story_vm.cpp)：324, 365, 750, 1038, 1132, 1240, 1592, 1734, 1818, 2017, 2199
  - [tests/unit/world_map/legacy_world_story_vm_test_support.hpp](../../../tests/unit/world_map/legacy_world_story_vm_test_support.hpp)：48, 353, 794

## PORT-3113c5c46779

- [ ] `ActionCompositionPortAdapter`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_action_dispatch_internal.hpp:385](../../../src/battle/legacy_battle_action_dispatch_internal.hpp:385)。
- 继承：`public LegacyBattleGroupBActionCompositionPort`。
- 直接残留：invoke, register, callee, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_action_dispatch_cases_high.cpp](../../../src/battle/legacy_battle_action_dispatch_cases_high.cpp)：771
  - [src/battle/legacy_battle_action_dispatch_internal.hpp](../../../src/battle/legacy_battle_action_dispatch_internal.hpp)：385, 388

## PORT-ef20d3a58313

- [ ] `ActionModePortAdapter`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_action_summary.cpp:86](../../../src/battle/legacy_battle_action_summary.cpp:86)。
- 继承：`public LegacyBattleInputDispatchPort`。
- 直接残留：invoke, register。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_action_summary.cpp](../../../src/battle/legacy_battle_action_summary.cpp)：86, 88, 207

## PORT-7f69a3107b73

- [ ] `DebugTextMessageAdapter`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_debug_hotkeys.cpp:98](../../../src/battle/legacy_battle_debug_hotkeys.cpp:98)。
- 继承：`public LegacyBattleTextMessagePort`。
- 直接残留：invoke, register, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_debug_hotkeys.cpp](../../../src/battle/legacy_battle_debug_hotkeys.cpp)：98, 100, 161

## PORT-c19e599fde1a

- [ ] `SecondaryRngBoundedAdapter`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_frame_coordinator.cpp:17](../../../src/battle/legacy_battle_frame_coordinator.cpp:17)。
- 继承：`public LegacyBattleBoundedRandomPort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_frame_coordinator.cpp](../../../src/battle/legacy_battle_frame_coordinator.cpp)：17, 19, 260

## PORT-e419a631f126

- [ ] `ActorTargetPreparationAdapter`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_group_a_frame.cpp:120](../../../src/battle/legacy_battle_group_a_frame.cpp:120)。
- 继承：`public LegacyBattleActorTargetPreparationPort`。
- 直接残留：invoke, register, callee, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：120, 123, 1519

## PORT-0e5c19ca16d1

- [ ] `GroupAFinalProcessingAdapter`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_group_a_frame.cpp:155](../../../src/battle/legacy_battle_group_a_frame.cpp:155)。
- 继承：`public LegacyBattleGroupAFinalProcessingPort`。
- 直接残留：invoke, register, callee, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：155, 158, 559

## PORT-a6765d0b70fb

- [ ] `GroupAAttributeEffectAdapter`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_group_a_frame.cpp:218](../../../src/battle/legacy_battle_group_a_frame.cpp:218)。
- 继承：`public LegacyBattleGroupAAttributeEffectPort`。
- 直接残留：invoke, register, callee, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_group_a_frame.cpp](../../../src/battle/legacy_battle_group_a_frame.cpp)：218, 221, 1374

## PORT-ba1fcfb22dfa

- [ ] `GroupAResetPort`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_group_a_startup_reset.cpp:76](../../../src/battle/legacy_battle_group_a_startup_reset.cpp:76)。
- 继承：`public LegacyBattleActorStartupResetPort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_group_a_startup_reset.cpp](../../../src/battle/legacy_battle_group_a_startup_reset.cpp)：76, 78, 225

## PORT-6c8d111752ec

- [ ] `GroupBResetPort`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_group_b_startup_reset.cpp:78](../../../src/battle/legacy_battle_group_b_startup_reset.cpp:78)。
- 继承：`public LegacyBattleActorStartupResetPort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_group_b_startup_reset.cpp](../../../src/battle/legacy_battle_group_b_startup_reset.cpp)：78, 80, 216

## PORT-e4c82312d72e

- [ ] `InputTextMessageAdapter`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_input_dispatch.cpp:29](../../../src/battle/legacy_battle_input_dispatch.cpp:29)。
- 继承：`public LegacyBattleTextMessagePort`。
- 直接残留：invoke, register, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_input_dispatch.cpp](../../../src/battle/legacy_battle_input_dispatch.cpp)：29, 31, 251
  - [src/battle/legacy_battle_target_selection_refresh.cpp](../../../src/battle/legacy_battle_target_selection_refresh.cpp)：53, 55, 973

## PORT-7b41058901bc

- [ ] `MessagePhaseGroupBActionItemPort`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_message_phase.cpp:50](../../../src/battle/legacy_battle_message_phase.cpp:50)。
- 继承：`public LegacyBattleGroupBActionItemSelectionPort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_message_phase.cpp](../../../src/battle/legacy_battle_message_phase.cpp)：50, 53, 683

## PORT-055a317c51be

- [ ] `OpponentGroupBActionSeventeenFramePort`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_opponent_action_dispatch.cpp:132](../../../src/battle/legacy_battle_opponent_action_dispatch.cpp:132)。
- 继承：`public LegacyBattleGroupBActionSeventeenFramePort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_opponent_action_dispatch.cpp](../../../src/battle/legacy_battle_opponent_action_dispatch.cpp)：132, 135, 1008

## PORT-8b216ea963fc

- [ ] `ScriptGroupBActionCompositionPort`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_script_dispatch_internal.hpp:866](../../../src/battle/legacy_battle_script_dispatch_internal.hpp:866)。
- 继承：`public LegacyBattleGroupBActionCompositionPort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_script_dispatch_cases_one_to_twenty_seven.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_one_to_twenty_seven.cpp)：1306
  - [src/battle/legacy_battle_script_dispatch_internal.hpp](../../../src/battle/legacy_battle_script_dispatch_internal.hpp)：866, 869

## PORT-243b0f85d63c

- [ ] `ScriptPartyItemDefinitionPort`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_script_dispatch_internal.hpp:945](../../../src/battle/legacy_battle_script_dispatch_internal.hpp:945)。
- 继承：`public LegacyBattlePartyItemDefinitionPort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp](../../../src/battle/legacy_battle_script_dispatch_cases_twenty_eight_to_fifty_four.cpp)：1265
  - [src/battle/legacy_battle_script_dispatch_internal.hpp](../../../src/battle/legacy_battle_script_dispatch_internal.hpp)：945, 948

## PORT-04502dd7fe77

- [ ] `StartupActorProgressRandomPort`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_startup.cpp:37](../../../src/battle/legacy_battle_startup.cpp:37)。
- 继承：`public LegacyBattleBoundedRandomPort`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：37, 40, 1590

## PORT-8a7773b9eefb

- [ ] `StartupGroupANpcMaterializationPort`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_startup.cpp:58](../../../src/battle/legacy_battle_startup.cpp:58)。
- 继承：`public LegacyBattleStartupSupplementalPort`。
- 直接残留：invoke, register, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：58, 61, 1572

## PORT-89ec329a0143

- [ ] `StartupGroupAAttributeAggregationPort`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_startup.cpp:190](../../../src/battle/legacy_battle_startup.cpp:190)。
- 继承：`public LegacyBattleGroupAAttributeAggregationPort`。
- 直接残留：invoke, register, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：190, 193, 1503

## PORT-46589e75e09f

- [ ] `StartupGroupAConfigurationDiagnosticPort`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_startup.cpp:251](../../../src/battle/legacy_battle_startup.cpp:251)。
- 继承：`public LegacyBattleGroupAConfigurationDiagnosticPort`。
- 直接残留：invoke, register, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_startup.cpp](../../../src/battle/legacy_battle_startup.cpp)：251, 254, 1402

## PORT-afd331603c89

- [ ] `ResourceSelectionAdapter`：类型已删除；所属整链待验收。
- 历史定义：src/battle/legacy_battle_target_selection_refresh.cpp；原类型已删除。
- 历史继承：`public LegacyBattleActorResourceSelectionPort`。
- 直接残留：对应类型、诊断操作及参数包调用已删除。
- 处理与验收记录：核心调用改为显式借用实际诊断接口。验证及平台接线边界见[诊断证据](battle-resource-selection-diagnostic-00431150.md)，所属整链未验收，暂不勾选。
- 引用位置（迁移前快照）：
  - [src/battle/legacy_battle_target_selection_refresh.cpp](../../../src/battle/legacy_battle_target_selection_refresh.cpp)：35, 38, 739

## PORT-f9d6f88adc33

- [ ] `InputTextMessageAdapter`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_target_selection_refresh.cpp:53](../../../src/battle/legacy_battle_target_selection_refresh.cpp:53)。
- 继承：`public LegacyBattleTextMessagePort`。
- 直接残留：invoke, register, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_input_dispatch.cpp](../../../src/battle/legacy_battle_input_dispatch.cpp)：29, 31, 251
  - [src/battle/legacy_battle_target_selection_refresh.cpp](../../../src/battle/legacy_battle_target_selection_refresh.cpp)：53, 55, 973

## PORT-48f4497cc0d5

- [ ] `TransitionTextMessageAdapter`：实现或转发类；待复核。
- 定义：[src/battle/legacy_battle_transition.cpp:31](../../../src/battle/legacy_battle_transition.cpp:31)。
- 继承：`public LegacyBattleTextMessagePort`。
- 直接残留：invoke, register, arguments。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/battle/legacy_battle_transition.cpp](../../../src/battle/legacy_battle_transition.cpp)：31, 33, 58

## PORT-5f00d9c08b8b

- [ ] `SdlExternalLaunchPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/external_launch_sdl3.hpp:7](../../../src/platform/sdl3/external_launch_sdl3.hpp:7)。
- 继承：`public app::ExternalLaunchPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/external_launch_sdl3.cpp](../../../src/platform/sdl3/external_launch_sdl3.cpp)：9, 14
  - [src/platform/sdl3/external_launch_sdl3.hpp](../../../src/platform/sdl3/external_launch_sdl3.hpp)：7
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：9702

## PORT-7a54eff2cc37

- [ ] `SdlLegacyTextInputPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:184](../../../src/platform/sdl3/main.cpp:184)。
- 继承：`public openswd3::input_time_rng::LegacyTextInputPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：184, 9336

## PORT-e9d262541f52

- [ ] `SdlLegacyAudioQueuePorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:386](../../../src/platform/sdl3/main.cpp:386)。
- 继承：`public openswd3::audio_video::LegacyAudioQueuePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：386, 389, 9668

## PORT-845dde81be82

- [ ] `SmokeWindowEventPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:447](../../../src/platform/sdl3/main.cpp:447)。
- 继承：`public openswd3::app::WindowEventPorts,
      public openswd3::app::ScreenshotPorts,
      public openswd3::rendering::LegacyBmpWriterPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：447, 452, 9804

## PORT-a77d845d4b85

- [ ] `SmokeCommandLinePorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:612](../../../src/platform/sdl3/main.cpp:612)。
- 继承：`public openswd3::app::CommandLinePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：612, 614, 9442

## PORT-70256fcd1195

- [ ] `SdlRngSeedPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:631](../../../src/platform/sdl3/main.cpp:631)。
- 继承：`public openswd3::app::RngSeedPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：631, 633, 9723

## PORT-9100b117e286

- [ ] `SdlSmokePlatformBackendPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:656](../../../src/platform/sdl3/main.cpp:656)。
- 继承：`public openswd3::app::PlatformBackendInitializationPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：656, 659, 9677

## PORT-03702411c458

- [ ] `SdlSmokeInitializationPorts`：抽象接口；待复核。
- 定义：[src/platform/sdl3/main.cpp:798](../../../src/platform/sdl3/main.cpp:798)。
- 继承：`public openswd3::app::InitializationPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：798, 801, 9687

## PORT-90a0dc6df60b

- [ ] `SdlSmokeStartupPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:915](../../../src/platform/sdl3/main.cpp:915)。
- 继承：`public openswd3::app::StartupPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：915, 917, 9706

## PORT-6dd594fe2948

- [ ] `SdlDisplayLifecyclePorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:975](../../../src/platform/sdl3/main.cpp:975)。
- 继承：`public openswd3::app::DisplayLifecyclePorts`。
- 直接残留：operation。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：975, 978, 9807

## PORT-d4cc14c65328

- [ ] `SmokeShutdownPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:1152](../../../src/platform/sdl3/main.cpp:1152)。
- 继承：`public openswd3::app::ShutdownPorts`。
- 直接残留：operation。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1152, 1154, 9827

## PORT-f68cf6e0ed1e

- [ ] `SdlProcessExitPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:1336](../../../src/platform/sdl3/main.cpp:1336)。
- 继承：`public openswd3::app::ProcessExitPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1336, 1338, 9835

## PORT-989ea10cefb9

- [ ] `SdlDeferredWorldFramePorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:1350](../../../src/platform/sdl3/main.cpp:1350)。
- 继承：`public openswd3::world_map::LegacyWorldFramePorts,
      public openswd3::asset_runtime::LegacyAniActivityPorts,
      public openswd3::world_map::LegacyWorldRoleExternalPorts,
      public openswd3::world_map::LegacyWorldSpatialAudioPorts,
      public openswd3::world_map::LegacyWorldOuterFramePorts,
      public openswd3::world_map::LegacyWorldDialogExternalPorts,
      public openswd3::rendering::LegacyTimedMessageInputPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1350, 1359, 2673, 5616, 5624, 6165, 6306, 6582

## PORT-c66d1579fe8c

- [ ] `SdlSmokeIdlePorts`：抽象接口；待复核。
- 定义：[src/platform/sdl3/main.cpp:1701](../../../src/platform/sdl3/main.cpp:1701)。
- 继承：`public openswd3::app::IdleRuntimePorts,
      public openswd3::app::FramePreparationPorts,
      public openswd3::app::FrameRuntimePorts,
      public openswd3::battle::LegacyBattleScriptDispatchPort,
      public openswd3::battle::LegacyBattleDialogFormatPort,
      public openswd3::battle::LegacyBattleGroupAConfigurationDiagnosticPort,
      public openswd3::battle::LegacyBattleGroupAAttributeAggregationPort,
      public openswd3::battle::LegacyBattleStartupSupplementalPort,
      public openswd3::battle::LegacyBattleBoundedRandomPort,
      public virtual openswd3::input_time_rng::LegacyMouseFrameStatePort,
      public virtual openswd3::battle::LegacyBattlePreFramePort,
      public virtual openswd3::battle::LegacyBattleDebugHotkeyPort,
      public openswd3::battle::LegacyBattleDebugRecordPort,
      public virtual openswd3::battle::LegacyBattleEffectCoordinatorStatePort,
      public virtual openswd3::battle::LegacyBattleEffectShiftStatePort,
      public virtual openswd3::battle::LegacyBattleActorMetricStatePort,
      public virtual openswd3::battle::LegacyBattleActorPublicationStatePort,
      public virtual openswd3::battle::LegacyBattleInputDispatchStatePort,
      public virtual openswd3::battle::LegacyBattleSharedPhaseStatePort,
      public virtual openswd3::battle::LegacyBattleMessagePhaseStatePort,
      public virtual openswd3::battle::LegacyBattleVictoryRewardStatePort,
      public virtual openswd3::battle::
          LegacyBattleTargetSelectionRuntimeStatePort,
      public virtual openswd3::battle::
          LegacyBattleFrameInputResolutionStatePort,
      public openswd3::battle::LegacyBattleLevelProfilePort,
      public virtual openswd3::battle::LegacyBattleFixedObjectState,
      public openswd3::rendering::LegacyPresentationPorts,
      public openswd3::audio_video::LegacyVideoFramePorts,
      public openswd3::world_map::LegacyWorldLoadProgressPorts`。
- 直接残留：invoke, register, operation, arguments, counter。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1701, 1809, 1878, 1948, 2043, 2049, 5631, 6082, 6827, 6841, 6859, 6869, 6894, 6952, 7149, 7172, 7188, 7229, 7286, 7351, 7470, 7498, 7594, 7710, 7831, 7902, 8115, 8169, 8553, 8579, 8732, 8805, 9845

## PORT-37479b034ea1

- [ ] `WorldInteractionPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:1743](../../../src/platform/sdl3/main.cpp:1743)。
- 继承：`public openswd3::world_map::LegacyWorldInteractionPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1743, 1746, 4751

## PORT-d1f9bc8a7ebc

- [ ] `WorldDebugHotkeyPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:1805](../../../src/platform/sdl3/main.cpp:1805)。
- 继承：`public openswd3::world_map::LegacyWorldDebugHotkeyPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1805, 1808, 4824

## PORT-04a280de8386

- [ ] `WorldPlayerPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:1882](../../../src/platform/sdl3/main.cpp:1882)。
- 继承：`public openswd3::world_map::LegacyWorldFacingTalkPorts,
          public openswd3::world_map::LegacyWorldCollisionTalkPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1882, 1886, 4932

## PORT-154fc1309069

- [ ] `WorldEncounterPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:1944](../../../src/platform/sdl3/main.cpp:1944)。
- 继承：`public openswd3::world_map::LegacyWorldEncounterPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1944, 1947, 5164

## PORT-ede30e3e1fab

- [ ] `PartyPathPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:5613](../../../src/platform/sdl3/main.cpp:5613)。
- 继承：`public openswd3::world_map::LegacyWorldPartyPathPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：5613, 5616, 6383

## PORT-8aa8e1888b1b

- [ ] `StoryVmPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:5627](../../../src/platform/sdl3/main.cpp:5627)。
- 继承：`public openswd3::world_map::LegacyWorldStoryVmPorts`。
- 直接残留：invoke, operation。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：5627, 5630, 6174

## PORT-6f083a3b2e59

- [ ] `PathScriptPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:6104](../../../src/platform/sdl3/main.cpp:6104)。
- 继承：`public openswd3::world_map::LegacyWorldPathScriptPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6104, 6107, 6389

## PORT-e72684e65fc8

- [ ] `InitializationPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:6823](../../../src/platform/sdl3/main.cpp:6823)。
- 继承：`public openswd3::special_modes::
                  LegacyStandardSpecialModeInitializationPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [include/openswd3/app/initialization.hpp](../../../include/openswd3/app/initialization.hpp)：15, 17, 34, 38
  - [src/app/initialization.cpp](../../../src/app/initialization.cpp)：6, 39
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：799, 920, 968, 6823, 6827, 6844
  - [tests/unit/app/initialization_test.cpp](../../../tests/unit/app/initialization_test.cpp)：24

## PORT-c25c01f66a08

- [ ] `SelectorPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:6856](../../../src/platform/sdl3/main.cpp:6856)。
- 继承：`public openswd3::special_modes::LegacyStandardModeSelectorPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6856, 6859, 7232

## PORT-f737d3106b78

- [ ] `BindingPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:6865](../../../src/platform/sdl3/main.cpp:6865)。
- 继承：`public openswd3::special_modes::
                          LegacyStandardModeCallbackBindingPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6865, 6869, 7154

## PORT-9cbf879ca236

- [ ] `TransitionPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:6896](../../../src/platform/sdl3/main.cpp:6896)。
- 继承：`public openswd3::special_modes::LegacyTitleMenuPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6896, 6951, 7494, 7498, 7597
  - [tests/unit/battle/legacy_battle_transition_test.cpp](../../../tests/unit/battle/legacy_battle_transition_test.cpp)：100, 107, 462, 495, 561, 658, 736, 830, 920, 1033, 1093, 1125, 1156, 1217, 1279, 1348, 1409

## PORT-c9ee92f626c0

- [ ] `ObjectLabelPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:6899](../../../src/platform/sdl3/main.cpp:6899)。
- 继承：`public openswd3::special_modes::
                                  LegacyObjectLabelPanelPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6899, 7150

## PORT-62d050835231

- [ ] `ItemPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:7169](../../../src/platform/sdl3/main.cpp:7169)。
- 继承：`public openswd3::special_modes::
                                            LegacyStandardModeItemPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：7169, 7172, 7191

## PORT-2ed5441b3267

- [ ] `InputPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:7244](../../../src/platform/sdl3/main.cpp:7244)。
- 继承：`public openswd3::special_modes::LegacyStandardModeInputPorts`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：7244, 7247, 7266
  - [tests/unit/rendering/legacy_timed_messages_test.cpp](../../../tests/unit/rendering/legacy_timed_messages_test.cpp)：29, 76
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：9491, 9495, 9661, 9686, 9698, 9712, 9740, 9755, 9771, 9798, 9820, 9836, 9849, 9863, 9881, 9891, 11663, 11878, 11914, 11923, 11934, 11947, 11967, 12007, 12073, 12119, 12157, 12188, 12210, 12223, 12243, 12314, 12362, 12395, 12412, 12415, 12434, 12465, 12493, 12510, 12542, 12563, 12578, 12597, 12610, 12626, 12648

## PORT-3f1ccdbb192c

- [ ] `RenderPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:7282](../../../src/platform/sdl3/main.cpp:7282)。
- 继承：`public openswd3::special_modes::LegacyStandardModeRenderPorts`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：7282, 7285, 7715
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：12667, 12774, 12782, 12797, 12826, 12846, 12867, 12889, 12900, 12912, 12922, 12928, 12957, 12980, 13007, 13021, 13054, 13101, 13120, 13136, 13156, 13170, 13184, 13204, 13217, 14876, 15054, 15080, 15115, 15116, 15117, 15118, 15119, 15120, 15121, 15161, 15170, 15171, 15180, 15188, 15189, 15198, 15229, 15255, 15269, 15270, 15279, 15288, 15289, 15301, 15310, 15311, 15322, 15340, 15353, 15354, 15355, 15380, 15430, 15450, 15468

## PORT-af1a87092cd0

- [ ] `PanelPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:7348](../../../src/platform/sdl3/main.cpp:7348)。
- 继承：`public openswd3::special_modes::
                                             LegacyStandardModePanelPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：7348, 7351, 7475
  - [tests/unit/special_modes/legacy_initial_menu_test.cpp](../../../tests/unit/special_modes/legacy_initial_menu_test.cpp)：16117, 16166

## PORT-570f1316ada1

- [ ] `TransitionPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:7494](../../../src/platform/sdl3/main.cpp:7494)。
- 继承：`public openswd3::special_modes::
                          LegacyGameMenuEntryAnimationPorts`。
- 直接残留：invoke。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：6896, 6951, 7494, 7498, 7597
  - [tests/unit/battle/legacy_battle_transition_test.cpp](../../../tests/unit/battle/legacy_battle_transition_test.cpp)：100, 107, 462, 495, 561, 658, 736, 830, 920, 1033, 1093, 1125, 1156, 1217, 1279, 1348, 1409

## PORT-4e1c3ee0b25e

- [ ] `StandardModePorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:7827](../../../src/platform/sdl3/main.cpp:7827)。
- 继承：`public openswd3::special_modes::LegacyStandardSpecialModePorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：7827, 7830, 7912

## PORT-e54d86864b22

- [ ] `PreviewPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:8549](../../../src/platform/sdl3/main.cpp:8549)。
- 继承：`public openswd3::special_modes::
                  LegacyInputMenuSavePreviewResetPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：8549, 8553

## PORT-62d8ed0cf5ce

- [ ] `WorldInterpolationExternalPorts`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/main.cpp:8727](../../../src/platform/sdl3/main.cpp:8727)。
- 继承：`public openswd3::world_map::LegacyWorldRoleExternalPorts,
          public openswd3::world_map::LegacyWorldSpatialAudioPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：8727, 8731, 8881

## PORT-20ac79adcfa2

- [ ] `WorldRoleRuntimeAdapter`：实现或转发类；待复核。
- 定义：[src/platform/sdl3/world_role_runtime_adapter.hpp:45](../../../src/platform/sdl3/world_role_runtime_adapter.hpp:45)。
- 继承：`无`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/platform/sdl3/main.cpp](../../../src/platform/sdl3/main.cpp)：1370, 1687, 6564
  - [src/platform/sdl3/world_role_runtime_adapter.cpp](../../../src/platform/sdl3/world_role_runtime_adapter.cpp)：59, 86, 96, 119, 137, 141, 147, 154, 166, 194, 208, 212
  - [src/platform/sdl3/world_role_runtime_adapter.hpp](../../../src/platform/sdl3/world_role_runtime_adapter.hpp)：45, 47
  - [tests/unit/app/world_role_runtime_adapter_test.cpp](../../../tests/unit/app/world_role_runtime_adapter_test.cpp)：51, 308, 349, 412, 487

## PORT-d643a4747dda

- [ ] `CharacterAttributeApplicationAdapter`：实现或转发类；待复核。
- 定义：[src/special_modes/legacy_standard_mode.cpp:2679](../../../src/special_modes/legacy_standard_mode.cpp:2679)。
- 继承：`public LegacyGuardianAttributeApplicationPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/special_modes/legacy_standard_mode.cpp](../../../src/special_modes/legacy_standard_mode.cpp)：2679, 2682

## PORT-12d0496618d5

- [ ] `SessionMovementCollisionPorts`：实现或转发类；待复核。
- 定义：[src/world_map/legacy_movement_collision.cpp:73](../../../src/world_map/legacy_movement_collision.cpp:73)。
- 继承：`public LegacyMovementCollisionPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/world_map/legacy_movement_collision.cpp](../../../src/world_map/legacy_movement_collision.cpp)：73, 76, 275

## PORT-d5738efc831a

- [ ] `RuntimeStagePorts`：实现或转发类；待复核。
- 定义：[src/world_map/legacy_world_frame_runtime.cpp:35](../../../src/world_map/legacy_world_frame_runtime.cpp:35)。
- 继承：`public LegacyWorldFramePorts,
      public asset_runtime::LegacyAniDriftServicePort,
      public asset_runtime::LegacyAniStreakServicePort,
      public asset_runtime::LegacyAniSparkServicePort,
      public asset_runtime::LegacyAniDirectionalServicePort`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/world_map/legacy_world_frame_runtime.cpp](../../../src/world_map/legacy_world_frame_runtime.cpp)：35, 42, 548

## PORT-9193c3470f12

- [ ] `StoryCountdownFlagPorts`：实现或转发类；待复核。
- 定义：[src/world_map/legacy_world_story_vm.cpp:294](../../../src/world_map/legacy_world_story_vm.cpp:294)。
- 继承：`public rendering::LegacyCountdownFlagPorts`。
- 直接残留：未命中；须检查纯转发及继承。
- 处理与验收记录：待填写。
- 引用位置：
  - [src/world_map/legacy_world_story_vm.cpp](../../../src/world_map/legacy_world_story_vm.cpp)：294, 297, 6867
