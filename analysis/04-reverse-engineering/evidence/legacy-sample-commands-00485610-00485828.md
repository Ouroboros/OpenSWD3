# 游戏侧 sample 命令与空间音效规格

最后更新：2026-08-09

状态：`assembly_exact`；已恢复调用点接线完成

完整 LST 是唯一行为真值；IDA 伪码只用于定位。实现和验证均未启动原版 EXE。

## 1. 六个入口的固定合同

| 地址 | 输入处理 | 提交到 sample manager | 返回值 |
|---:|---|---|---:|
| `0x00485610` | sound ID 截为 `u16`；第二参数为 signed `i32` | `play(id, level*128/11, pan=0, loop=1, aux=0)` | 原样返回 `play`，恒为零 |
| `0x00485650` | sound ID 截为 `u16`；pan 不变 | `set_pan(id, pan)` | 原样返回 manager 的转换后 pan，manager 未启用时为零 |
| `0x00485670` | sound ID 和 level 都截为 `u16` | 与 `0x00485610` 相同的播放请求 | 忽略 `play` 结果，固定返回一 |
| `0x00485720` | sound ID 截为 `u16` | `stop(id)` | 忽略 `stop` 结果，固定返回一 |
| `0x00485740` | 无参数 | `stop_all()` | 忽略 manager 状态，固定返回一 |
| `0x00485750` | sound ID 不截断；坐标为 signed `i32` | 距离内依次 `play`、`set_volume`、`set_pan` | 远距离返回距离；近距离返回 `set_pan` 结果 |

前三个播放入口都不提交已有 buffer，最终辅助参数固定为零。不能把 `0x00485670`、
`0x00485720` 或 `0x00485740` 的固定一改成 backend 成功布尔值。

## 2. `0x00485610/0x00485670` 的音量缩放

汇编先在 32 位寄存器中把 level 左移七位，再用 signed `imul 0x2E8BA2E9` 的高半、
算术右移一位和符号修正实现向零除以十一：

```text
scaled = signed_truncate(wrapping_i32(level << 7) / 11)
```

`0x00485610` 在左移前不截断 level；`0x00485670` 先执行 `& 0xffff`。重写使用显式
无符号位运算保存左移回绕，再做 C++20 signed 向零除法。除法等价性已对边界值及
一百万个 32 位输入抽样比较原 magic-multiply 序列。

## 3. `0x00485750` 的距离、衰减与声像

监听者位置来自当前受控角色 `dword_4AB378` 所选记录的 `+4/+8` 坐标。重写不复制
全局数组，而由 owner 以 `LegacySpatialSampleState` 提交监听者坐标和全局混音档位
`dword_4AB784`。

距离路径保持 32 位整数回绕顺序：

```text
dy = wrapping_i32(listener_y - target_y)
dx = wrapping_i32(listener_x - target_x)
sum = wrapping_i32(dy*dy + dx*dx)
distance = truncate(sqrt(signed_i32(sum)))
```

原程序通过 x87 `fild/fsqrt` 和 `0x00489654` 向零转换。若平方和回绕为负，x87 的
masked invalid 结果经 64 位 integer-indefinite 只留下低 32 位零；实现显式得到同一
距离零。正常非负 `i32` 范围用 double `sqrt` 后截断不会改变整数结果。

`distance >= 512` 时不提交任何音频请求，并直接返回 distance。否则严格按以下顺序：

1. 以原 sound ID（不做 `u16` 截断）、volume/pan 零、loop 一发起播放；
2. 计算 `attenuation = truncate(wrapping_i32(distance << 7) / 512)`；
3. 计算 `volume = truncate(wrapping_i32((128-attenuation)*mix_level) / 11)` 并更新；
4. 计算 `pan = truncate(wrapping_i32((target_x-listener_x) << 6) / 512)` 并更新；
5. 返回 manager 的 pan 转换结果。

因此 511 距离仍播放，按 mix level 十一得到 volume 一；512 距离刚好被排除。即使
sound ID 无效，近距离路径仍继续执行两次参数更新并返回 manager 的声像转换值。

## 4. 接线边界

当前已经恢复的显示停用、总退出与战斗胜利结算`0x00485740`调用点统一经过
`stop_all_legacy_samples()`，再进入同一个`LegacySampleManager`；胜利结算随后也以
`0x12C`和live mix level直连`0x00485610`播放；角色升级属性提交则先按单sample停止语义停止`0x12C`，再以`0x12B`和live mix level播放升级提示。其他入口作为剧情、世界和战斗后续模块的
公共命令边界保留；对应调用者尚未实现时不伪造调用。

启动对话框前的 `play_startup_sound()` 不能接到 `0x00485610`：原 LST
`0x0040A465–0x0040A472` 调用的是 Win32 `PlaySoundA` 资源 120，属于另一条平台音效
路径。把它替换成 SND sample 会改变原行为。

## 5. 验证

fake backend 与合成 SND 固定验证：

- 三处 `u16` sound ID 截断和空间入口的不截断；
- `5*128/11 = 58`、u16 level 十一先得到 128 再由 manager 钳至 127；
- pan 返回值、单项停止、全部停止及 manager 禁用时仍固定返回一；
- 511/512 距离边界、3-4-5 距离、衰减 volume 和水平 pan；
- 空间入口的 play→volume→pan 顺序由 manager/backend 可见结果锁定。

Linux Clang core 为 70/70；Linux Clang app 与 Windows LLVM app 均为 74/74 CTest
通过。只执行测试程序，没有启动原版或重写 EXE。
