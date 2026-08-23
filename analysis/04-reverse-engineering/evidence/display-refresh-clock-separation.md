# 内部逻辑时钟与显示刷新时钟解耦

状态：`platform_adapted`

## 1. 原行为与适配目标

权威LST中的`0x0040A570`使用32位毫秒时钟门控游戏帧。默认门槛为35ms；剧情ANI可临时写70ms；写0时每次idle调用都接受。接受帧时直接把`previous`写为当前毫秒，不补跑丢失帧。

此前SDL主程序只在legacy presentation请求中上传framebuffer并立即调用`SDL_RenderPresent`，因此宿主显示提交频率与内部游戏帧请求耦合。

用户要求宿主显示FPS可配置，同时不得改变内部游戏计时。第一步适配拆分SDL上传和最终呈现；后续又按用户实际观感要求，为普通世界画面的镜头与角色增加场景级运动插值。两步均不修改`LegacyFrameClockState`、`frame_interval`、35/70ms切换或任何legacy业务状态。

## 2. 配置合同

EXE同目录`openswd3.toml`新增：

```toml
[display]
fps = 0
world_motion_interpolation = false
```

- `fps = 0`是默认值，保持旧的耦合呈现路径。
- `fps = 1..1000`启用独立显示刷新时钟。
- `world_motion_interpolation = false`是默认值，仅重复current完整帧。
- 只有`fps > 0`且`world_motion_interpolation = true`才启用普通世界运动插值。
- 缺少`[display]`时使用0与false。
- 非table、非法FPS或非布尔插值值记录配置警告并整体回退默认值。
- 保存`[window]`时保留既有`[display]`。

独立模式必须关闭SDL renderer VSync。若SDL无法确认该请求成功，主程序回退0，避免阻塞式present反向拖慢游戏tick。

## 3. 双时钟所有权

内部逻辑时钟继续使用：

- `SDL_GetTicks()`截断为32位毫秒。
- `LegacyFrameClockState`。
- 当前`frame_interval`值，默认35ms，剧情可写70ms。
- 原版无catch-up的接受语义。

显示刷新时钟独立使用：

- `SDL_GetTicksNS()`的64位纳秒值。
- `DisplayRefreshClockState`。
- `1,000,000,000 / fps`的整数纳秒门槛。
- 迟到时只呈现一次并把display previous写为当前值，不补跑历史显示帧。

两种状态没有共享时间字段。显示时钟API不接收也不返回legacy frame clock、输入、剧情、RNG或音频状态。

## 4. 帧内顺序

每次host idle iteration严格执行：

1. 按原`IdleAction`完成视频加音频、yield、游戏帧或pause composition。
2. 最后检查一次独立显示deadline。

正常runtime的legacy presentation在独立模式中只完成primary surface composition和纹理上传。到显示deadline后才执行纹理选择、clear、render texture和`SDL_RenderPresent`。显式启用`world_motion_interpolation`且普通世界帧存在两个兼容视觉快照时，纹理选择会使用场景级插值帧；配置关闭、首个世界帧、地图或角色身份不兼容、插值重绘失败以及非普通世界presentation均回退最近一次成功上传的primary surface。启动首帧和显示设备恢复仍各自执行一次同步呈现，以避免首帧或恢复后黑屏；它们不构成持续刷新时钟。

因此额外显示帧不会：

- 调用`run_frame_preparation`或`run_accepted_frame`。
- 采样或规范化游戏输入。
- 推进剧情、动作、粒子、碰撞或战斗。
- 消费任一RNG。
- 执行音频维护。
- 修改35/70ms内部帧门槛。

## 5. 普通世界运动插值

只有配置显式启用后，`compose_legacy_world_runtime_frame`才在任何stage执行前捕获只读视觉快照。快照拥有当帧背景视图、空间索引、composition相机、完整角色记录、角色绘制计数与闪色字段，以及空间音频的两个数组副本。presentation后的玩家对齐可能改写live角色与空间链，因此插值重绘只读取快照索引，不读取post-frame后的链。捕获失败只禁用本次插值，不改变原世界帧结果。

每个成功普通世界帧把旧current移动为previous，并把新快照提交为current。插值时间原点取该逻辑帧进入frame preparation前的`SDL_GetTicksNS()`，不取world composition、presentation及纯运动底图全部完成后的时间；因此原帧渲染耗时也计入35/70ms区间，不会在下一逻辑tick前残留尚未走完的插值尾段。显示deadline使用`elapsed / frame_interval`在previous与current之间计算相机和所有角色的世界坐标；比例在当前逻辑间隔处钳制，不外推。单坐标跨度超过128像素时按传送处理并直接使用current，避免跨地图或瞬移扫屏。角色数量或GUID顺序变化时整帧回退current presentation。

插值不是两个framebuffer混色。SDL平台在独立显示deadline中：

1. 清空独立目标framebuffer。
2. 以插值相机重绘indexed object、世界背景和flagged role。
3. 在角色记录及空间音频数组副本上重绘普通角色。
4. 对比current位置的纯运动底图与当帧完整primary surface。
5. 仅把两者不同的current像素作为残差覆盖回插值底图，以保留对话、文字、光效、粒子、picture action和其他非运动层。

重绘端口禁止音频、粒子和文字副作用；角色action、jitter及空间音频写入只发生在副本。TSW读取可复用资源缓存，但不推进游戏owner。该路径因此会生成真正不同坐标的世界显示子帧，同时不执行输入、碰撞、路径、剧情、RNG、动作更新或音频维护。角色动画帧仍取current逻辑状态；本阶段只解决镜头与人物位移的平滑度。previous到current的标准插值会引入一个逻辑帧的显示延迟，以换取不预测、不外推且连续的运动。

## 6. 验证

定向测试覆盖：

- 默认0关闭独立显示门。
- 60 FPS精确deadline前拒绝、deadline接受。
- 120 FPS迟到只接受一次，不catch up。
- 240 FPS使用`4,166,666 ns`整数deadline。
- 四种idle action都先完成原动作，再检查显示刷新。
- `[display]`缺失、60、0、非法table、超范围值。
- 世界运动插值缺省false、显式true及非法非布尔值。
- 保存窗口配置后仍保留显示FPS和插值开关。
- app生命周期中的游戏帧先于显示检查。
- composition入口在任一stage前复制视觉快照。
- 插值时间原点绑定accepted逻辑帧，而不是帧渲染完成时刻。
- 半间隔相机与正负角色坐标插值、迟到钳制及无外推。
- 传送坐标直接snap，普通角色坐标仍可插值。
- 地图或角色身份不兼容时拒绝跨场景插值。
- current底图与完整帧的残差仅覆盖非运动像素。

Linux core完整门`188/188`、Linux app完整门`194/194`通过；SDL应用目标成功链接。按用户阶段门禁，本工作包不单独执行Windows BUILD。
