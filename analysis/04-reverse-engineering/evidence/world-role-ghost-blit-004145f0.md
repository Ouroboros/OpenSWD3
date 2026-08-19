# 普通世界角色残影绘制（`0x004145F0`）

状态：`assembly_exact`、`unit_verified`、`platform_adapted`、
`sdl_runtime_integrated`；尚未 `original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。`sub_4145F0` 的完整物理范围是
`0x004145F0..0x004146E2`，唯一调用点是 `sub_413910:0x00413A0E`，唯一直接被调函数是
`sub_4170E0:0x004146D6`。

## 1. ABI 与调用边界

函数是六参数 cdecl，调用者与本函数都分别清理自己发出的 `0x18` 字节参数：

| 参数 | 物理来源 | 现代映射 |
| --- | --- | --- |
| `arg_0` | 角色记录指针 | `role` |
| `arg_4` | 已装载帧记录指针 | `frame` |
| `arg_8` | 冻结 world X/camera left 与 live action X offset 算出的基础 X | `base_x` |
| `arg_C` | 冻结 world Y/camera top 与 live action Y offset 算出的基础 Y | `base_y` |
| `arg_10` | 全局 frame counter | `counter` |
| `arg_14` | `0x004995D4[(role.flags>>20)&0xF]` | 三通道 `color` |

函数保存 `EBX/EBP/ESI/EDI`，plain `retn` 不清入口参数。它把 `sub_4170E0` 留下的 `EAX`
原样带回，外层调用者不读取，因此没有独立返回值合同。现代 `ghost_drawn/draw_count/status`
只用于诊断，不控制游戏状态。

外层在首帧 load callback 返回后重读 role flags、frame counter 和 action offsets；只有
flash bit 非零且 `counter & 7 < 4` 才调用本函数。world/camera 仍使用 `sub_413910`
入口快照。`draw_ghost` 在最终 draw port 前没有 callback，因此把六个物理参数重建为局部
请求不会移动任何有效域 reload 点。

## 2. 帧几何、位移与 flags

入口把 `frame[+0]` 写入源图全局，零扩展读取 `frame[+0x0E]` 高度：

```text
target_height = height >> 1
displacement  = -(height >> 2)
if counter bit 0: displacement = -displacement
flags = (role.action.mode_flags & 0x8000000F) | 0x0000000C
```

`counter bit 3` 非零时，target height 再执行一次向零截断的有符号除二，action Y offset
也向零除二，并把 Y 调整初值设为四；否则两个初值为零。随后只看 `counter & 7`：

```text
if low3 >= 2:
    flags |= 2
    y_adjust += action.draw_offset_y + 4
else:
    y_adjust += trunc((action.draw_offset_y + half_offset_y + 8) / 2)
```

所有 action offset 加减按 32 位回绕；负奇数除二必须向零截断。目标坐标为：

```text
destination_x = base_x + (counter bit 0 ? -4 : +4)
destination_y = base_y + y_adjust
```

最终 `sub_4170E0` 接收 destination、源宽高、flags 与零 auxiliary。残影调用发生在本角色
jitter group/phase 写入之前，因此继承前一共享状态；被调 blitter 的返回值不改变本函数
控制流。

高度三且 bit 3 有效时，`target_height == 0` 且 `displacement == 0`。这不是空操作；请求仍
进入 blitter，并按其已独立关闭的零目标高度继承规则执行。现代请求保留该特殊路径。

## 3. 颜色表

`0x004995D4` 的 16 个有符号 dword 为：

```text
-8, -7, -6, -5, -4, -3, -2, 0,
 0,  0, -14, -13, -12, -11, -10, -9
```

调用者按当时 role flags 的 bits 20..23 选一项；本函数在 draw 前把同一值依次写入红、绿、
蓝三个全局。现代 `kGhostOffsets` 和 request 三通道逐项一致。

## 4. 实现与验证边界

`legacy_world_roles.cpp:draw_ghost` 拥有本函数的有效域行为。原始裸 role/frame 指针、共享
blitter scratch globals 和函数表分派改为受检角色/帧 owner、typed request 与 draw port；
源宽高、flags、坐标、三色、零 auxiliary 及调用顺序保持不变，因此 closure disposition
为 `platform_adapted`。

独立 UT 固定：

- counter `0/1/2/8/9/10` 的 parity、bit 3、low3 两分支组合；
- signed odd action offset 与 odd frame height 的向零截断；
- height 3 产生的零 target height/零 displacement 特殊请求；
- mode flags 的 `0x8000000F` mask 与 `0x0C/0x02` 位；
- `0x004995D4` 全 16 项 RGB 三通道；
- residual jitter 在主角色状态装载前继承；
- 首帧 callback 后 live action/flags/counter 与冻结 world/camera 的组合。

synthetic 与真实 TSW 两项 role CTest 通过；Linux core `185/185`、Linux app
`191/191`、Windows LLVM app `191/191` 完整门禁通过，两端应用成功链接且未启动原版或
OpenSWD3 游戏 EXE。原 framebuffer/blitter 动态差分仍等待用户 oracle。
