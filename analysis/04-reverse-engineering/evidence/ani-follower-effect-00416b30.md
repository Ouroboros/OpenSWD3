# ANI 目标跟随双帧效果（0x00416B30）

最后更新：2026-08-10

唯一行为真值：`swd3.exe.lst`。调用顺序、裁剪坐标交叉、状态更新和整数
回绕均以 LST 指令为准，IDA 伪码不作行为依据。

## 1. 状态与启用门

`0x0040C22F..0x0040C261` 将当前坐标和目标坐标同时初始化为 `(320, 240)`，
并将两个速度分量清零。这六个 `i32` 状态由 `LegacyAniFollowerState`
持有；后续剧情 opcode 150–152 会改写它们，因此不得缩成函数局部状态。

`0x00416B30` 首先调用 `0x0040DC50(0x13)`。返回零时直接返回，不更新
动作记录、不查询资产、不绘制也不移动状态。重写由调用者传入这个
service 结果，避免 asset runtime 反向拥有剧情 service。

## 2. 两帧动作与绘制顺序

启用后共用全局 `0x98` 动作记录 `0x004AD3F0`，严格执行两次：

1. 写 `action_id=0x232B` 和 `base_variant=78`，调用 `0x004321E0`，再以
   记录 `+0x4A/+0x4C` 的两个 word 调用 `0x004315D0` 取 TSW 帧并绘制。
2. 仅把 `base_variant` 改为 79，对同一记录重复更新、TSW 查询和绘制。

第一帧的实际绘制坐标按宽高正常居中，flags 为零。但它的裁剪矩形在
`0x00416B82..0x00416BA1` 交叉了宽高：

```text
left   = x - width / 2
top    = y - width / 2
right  = x + height / 2
bottom = y + height / 2
```

这是原指令的可观察 BUG，实现和 UT 故意保留。第二帧的裁剪矩形固定为
`(x-192, y-192, x+192, y+192)`，正常居中绘制，flags 为 `0x2C`。原调用者
不检查 `0x004170E0` 的返回值，所以第一帧 blit 失败不得阻止第二帧和
后续移动。

## 3. 裁剪 helper

`0x00416FF0` 只把负的 left/top 夹到零，只把超过 surface 上界的
right/bottom 夹回上界，然后保存 `right-left` 和 `bottom-top`。它不会对
left/top 做上界夹取，也不会对 right/bottom 做下界夹取；因此反向矩形
会真实产生负宽或负高。`set_legacy_clip_rectangle` 作为公共帧缓冲 helper
保留这一合同，原有 tiled-frame 路径也改为共用同一实现。

## 4. 移动规则

两帧全部绘制后才更新坐标：

- 当前 x/y 同时等于目标时立即返回，即使速度非零也不清除。
- 只有一轴已等于目标时，两个速度仍然都会加到当前坐标。
- 坐标加法保留 32 位回绕；不做越过目标的 clamp。
- 只在加法后某轴精确等于目标时，才将该轴速度清零。

## 5. 实现与验证

- 合成 UT 锁定 service 早退、两次 ACT→TSW→clip→draw 顺序、非方形帧的
  裁剪交叉 BUG、`0/0x2C` flags、blit 返回值忽略、单轴等值、越界不夹取
  及 `i32` 回绕。
- 真实六包 ACT/TSW 测试中，两个变体均能通过 runtime 解析并完成 blit；
  变体 79 最终键为 `(9225, 0)`，TSW 缓存保留两项，最终裁剪为
  `(128, 48, 384, 384)`。
- 资产更新或帧查询不可用时的显式错误状态是现代内存/资源边界；
  它不改变原版有效资产路径的指令顺序。

Linux Clang `core` 为 `92/92`，Windows LLVM `app` 为 `96/96` CTest，全套通过。
当前证据状态为 `assembly_exact` 和 `asset_verified`；尚未对原程序帧缓冲和帧内
调用轨迹做动态差分，保留 `blocked_runtime_oracle`。需要时由 Codex 准备 Frida spawn
一键工具并等待用户执行，OpenSWD3 不自行启动原 EXE。
