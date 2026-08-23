# 特殊模式ghost动作绘制 `0x0043B080`

状态：`platform_adapted`

## 1. LST锁与调用者

权威范围为`swd3.exe.lst`的`0x0043B080..0x0043B10C`。五个调用点来自：

- `0x0043A880`一次。
- `0x0043AAA0`四次。

四参数ABI为`(action_record, x, y, caller_value)`，调用者均不读取EAX。

## 2. 更新与失败顺序

函数首先调用`0x004321E0(action_record)`。返回0时只提交原诊断参数并立即返回：

- 不解析frame。
- 不保存caller value。
- 不执行blit。

更新成功后，以动作记录实时`+0x4A/+0x4C`调用`0x004315D0`解析frame。原程序直接解引用返回值；现代受检端口在frame缺失时typed-stop，不伪造frame。

## 3. caller值与blit参数

frame成功后，机器分别保存：

- frame首dword到共享frame槽。
- 第四参数caller value到`0x004CC2F0`。

caller value不参与本次blit flags计算。最终flags严格为：

```text
(action.mode_flags & 0x80000017) | 0x14
```

坐标在frame解析后重新读取动作offset并执行32位回绕：

```text
draw_x = x - action.draw_offset_x
draw_y = y - action.draw_offset_y
```

frame宽高按无符号16位读取，opacity固定为0，随后调用`0x004170E0`。函数保留该blit的机器返回值，但所有调用者忽略。

## 4. typed owner与验证

`LegacyStandardModeGhostState`保存解析frame槽和caller value；核心直接复用`LegacyActionDrawPorts`。SDL的`0x0043A880`和`0x0043AAA0`端口现均进入真实ghost桥，不再是no-op。

`special_modes.legacy_initial_menu`覆盖：

- update、frame request和blit顺序。
- caller value存储与flags隔离。
- `0x80000017`掩码及`0x14`强制位。
- live offset坐标。
- update失败在frame前返回。
- frame缺失typed-stop。
- update/frame/draw/blit failure计数。

Linux core `186/186`与Linux app `192/192`通过。按阶段门禁，本单入口不重复执行Windows BUILD。workpack连续两轮生成均为`10/227`，SHA256均为`bfd4559554dfc5113b0981e1a5b372fe645c62caae16003d227199c009ba276e`，只新增关闭`0x0043B080`。
