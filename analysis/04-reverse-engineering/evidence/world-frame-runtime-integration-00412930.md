# 世界帧已恢复阶段运行时接线（`0x00412930`）

状态：`assembly_exact_order`、`asset_verified`；尚未 `original_diff_verified`

本证据块记录 OpenSWD3 如何把世界帧协调器、两条空间角色路径、两条图片动作和两条 `0xB4` 动作链链接成一个
可执行纵向切片。原调用顺序仍只以 `swd3.exe.lst` 为真值；运行时适配器本身是现代所有权
边界，不被误写成原 EXE 内存在的类。

## 1. 接线边界

`compose_legacy_world_runtime_frame` 继续由 `compose_legacy_world_frame` 保持
`0x00412930` 的全部 service 短路、clip、底图和 stage 顺序，只在四个已恢复的 stage
到达时截获调用：

| stage | 实际执行 |
|---|---|
| `flagged_spatial_objects_00413ea0` | `draw_legacy_world_flagged_roles` |
| `world_spatial_objects_00413870` | `draw_legacy_world_roles`，内部包含 `0x00413910` 与条件 `0x00413CA0` |
| `primary_picture_actions_004147e0` | 主图片动作链的 `update_draw_legacy_picture_actions` |
| `moving_action_sprites_00414b60` | `0xB4` 世界移动动作链的 `update_draw_legacy_moving_actions` |
| `secondary_picture_actions_004147e0` | 副图片动作链的同一 owner；clear-only 仍只执行此槽 |
| `role_head_sprites_00414ce0` | `0xB4` 角色头顶动作链的 `update_draw_legacy_role_head_actions` |

其余十三个 stage 仍逐项转交 `remaining_stages`，不会静默当作已经实现。最终平台提交仍在
`0x004120B0` 的原位置；本适配器没有把 presentation 搬进 `0x00412930`。

## 2. 同一运行态

恢复路径共享同一份帧状态和运行端口：

- `LegacyRoleSpatialIndex` 和可变 `LegacyWorldRoleRecord` 数组；
- `LegacyFramebuffer` 与当前 `LegacyRasterGeometryState` clip；
- 相机坐标和 talk target；
- `LegacyRleRowJitterState`；
- TSW runtime、效果参数以及距离音频端口。
- `story_scene` 借出的主/副图片动作链、动作 updater、TSW 绘制端口和位置音效端口。
- `story_scene` 借出的 moving action 链；其物理节点仍保留完整动作记录、float 运动扩展
  和旧 `+0xB0` next 槽。
- `story_scene` 借出的 role-head action 链；其同尺寸节点使用独立的四个 i16 运动字段、
  16 字节保留段和旧 `+0xB0` next 槽。

普通角色的 camera 和 talk target 直接由 `LegacyWorldFrameState` 构造，不允许调用者再传
一份可能不一致的坐标。frame counter、三项 flash 颜色和空间音频数组仍是本帧显式状态。

## 3. 失败边界

原程序默认所有全局指针和资源有效，失败通常会继续访问无效内存。OpenSWD3 只在这些
不可能的现代状态建立隔离：

- 空间索引不足、链索引无效；
- TSW frame 无法解析；
- 普通角色覆盖 action、标签或距离数组无效；
- 尚未接线的外部 stage 主动报告失败；
- stage 内部抛出异常。

图片动作、moving action 和 role-head action 的动作更新或帧请求失败保留原函数的非致命诊断边界：继续后续音效、运动、摘链和节点，
并在结果计数中暴露失败，不把整帧改成提前返回。

`LegacyWorldFramePorts::execute_stage` 因此返回成功标记。失败会在对应的原 stage 位置停止，
恢复全屏 clip，并以 `stage_failed` 返回；后续 stage 不执行。正常资产下所有端口均返回成功，
原汇编的调用顺序和次数不变。

## 4. 验证

合成 UT 固定了以下事实：

- normal 路径仍有十九次 stage 调用，其中六个 stage 实际执行、十三个明确转交；
- bit-29 路径先于 `2 → 0 → 1` 普通角色遍历；
- 空间失败停在 `0x00413EA0`，不会继续执行 `0x00413870`；
- 转交 stage 的失败不会把整帧误报为完成；
- full clip 在受检失败后恢复。

真实 `all_char.tsw` 的 resource 1 / frame 0 同时经过 bit-29 固定透明路径、普通角色、
moving action 和 role-head action 路径，叠加在 640×480 direct-16 底图后的逻辑
framebuffer FNV-1a64 为 `0x3EAF7C3143994E65`。

Linux Clang `core` 157/157、Windows LLVM `app` 161/161 CTest 均通过。原程序逐帧
framebuffer、音频调用和 jitter 差分仍为 `blocked_runtime_oracle`；需要时只准备 Frida
spawn 工具并等待用户执行，不由开发流程启动原版。
