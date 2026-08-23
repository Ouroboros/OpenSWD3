# 内部逻辑时钟与显示刷新时钟解耦

状态：`platform_adapted`

## 1. 原行为与适配目标

权威LST中的`0x0040A570`使用32位毫秒时钟门控游戏帧。默认门槛为35ms；剧情ANI可临时写70ms；写0时每次idle调用都接受。接受帧时直接把`previous`写为当前毫秒，不补跑丢失帧。

此前SDL主程序只在legacy presentation请求中上传framebuffer并立即调用`SDL_RenderPresent`，因此宿主显示提交频率与内部游戏帧请求耦合。

用户要求宿主显示FPS可配置，同时不得改变内部游戏计时。适配只拆分SDL上传和最终呈现，不修改`LegacyFrameClockState`、`frame_interval`、35/70ms切换或任何legacy业务状态。

## 2. 配置合同

EXE同目录`openswd3.toml`新增：

```toml
[display]
fps = 0
```

- `fps = 0`是默认值，保持旧的耦合呈现路径。
- `fps = 1..1000`启用独立显示刷新时钟。
- 缺少`[display]`时使用0。
- 非table、非整数、负数或大于1000时记录配置警告并回退0。
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

正常runtime的legacy presentation在独立模式中只完成primary surface composition和纹理上传。到显示deadline后才执行clear、render texture和`SDL_RenderPresent`。没有新内部帧时重复显示最近一次成功上传的纹理。启动首帧和显示设备恢复仍各自执行一次同步呈现，以避免首帧或恢复后黑屏；它们不构成持续刷新时钟。

因此额外显示帧不会：

- 调用`run_frame_preparation`或`run_accepted_frame`。
- 采样或规范化游戏输入。
- 推进剧情、动作、粒子、碰撞或战斗。
- 消费任一RNG。
- 执行音频维护。
- 修改35/70ms内部帧门槛。

本适配不生成插值帧；提高显示FPS不会凭空增加角色动画或游戏逻辑帧。

## 5. 验证

定向测试覆盖：

- 默认0关闭独立显示门。
- 60 FPS精确deadline前拒绝、deadline接受。
- 120 FPS迟到只接受一次，不catch up。
- 四种idle action都先完成原动作，再检查显示刷新。
- `[display]`缺失、60、0、非法table、超范围值。
- 保存窗口配置后仍保留显示FPS。
- app生命周期中的游戏帧先于显示检查。

Linux core完整门`187/187`、Linux app完整门`193/193`通过。按用户阶段门禁，本工作包不单独执行Windows BUILD。
