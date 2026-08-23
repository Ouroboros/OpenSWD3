# 标准特殊模式面板准备 `0x0043A880`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0043A880..0x0043AA90`。唯一调用者是`0x0043A610`普通transition路径。函数无栈参数，调用者不读取EAX。

共享记录对应：

- ghost记录`0x004FC790`，即18记录owner的记录1。
- terminal记录`0x004FBEE0`，即记录15。
- panel step `0x004FC780`。

## 2. panel step

帧计数等于`0x41`时，计算基值强制为0；否则读取旧step。

- secondary等于1：32位回绕加1，先写回，再以有符号比较把大于16的结果钳为16。
- secondary不等于1：32位回绕减1，先写回，再以有符号比较把小于8的结果钳为8。

因此entry frame的secondary1得到step1；其他secondary从`0 - 1`得到`0xFFFFFFFF`后钳为8。

## 3. ghost记录

函数第一次查询剧情标志`0x49`：

- 字面等于1选择spacing `0x46`。
- 其他结果选择spacing `0x50`。

随后先把ghost记录`+0x34 variant_delta`写为2，再第二次查询同一flag；只有字面等于1时改为3。之后调用：

```text
0x0043B080(ghost_record, 0, 9, panel_step)
```

回调返回后重新读取ghost记录`+0x18 mode_flags`并执行`&= 0x80000003`。`0x0043B080`及其ghost blit内部仍需独立闭环，本入口只锁定参数、顺序与回调后的实时字段重载。

## 4. step等于16

terminal记录action id固定写`0x232A`，base variant先写`derived_index + 5`。第三次查询flag`0x49`后，只有字面等于1时交换variant `20`与`21`，其他值不变。

绘制坐标保持32位回绕：

```text
x = (derived_index - 11) * spacing + 220
y = 10
```

随后调用`0x0040EBF0(terminal_record, x, y)`并立即返回，不进入手工frame路径。

## 5. step不等于16

base variant先写`derived_index + 25`。第三次flag查询字面等于1时交换variant `40`与`41`。

随后顺序固定为：

1. `0x004321E0(terminal_record)`更新动作；返回0直接结束。
2. 以更新后的`+0x4A/+0x4C`调用`0x004315D0`解析frame。
3. 保存frame首dword。
4. 把`panel_step - 16`的32位回绕结果写入三个共享delta槽。
5. 从frame实时读取无符号宽高，并从terminal记录实时读取offset与mode flags。
6. 调用`0x004170E0`，opacity固定为0。

最终坐标为：

```text
x = (derived_index - 11) * spacing - draw_offset_x + 220
y = 10 - draw_offset_y
```

原程序对无效frame继续解引用；现代受检端口以typed-stop报告frame失败，不伪造frame内容。

## 6. typed owner与验证

`LegacyStandardModePanelState`保存step、解析frame首word和三个signed delta。ghost与terminal直接复用18记录owner中的真实`LegacyActionRecord`。SDL terminal路径已接通动作更新、TSW frame解析和blit；ghost callee保持窄端口。

`special_modes.legacy_initial_menu`覆盖：

- secondary1从step15递增到16并走通用动作桥。
- flag49控制spacing `0x46`、ghost variant3和20/21交换。
- ghost回调后的mode flags掩码。
- secondary2从step10递减到9并走手工frame路径。
- flag非1的spacing `0x50`、40/41交换、三个负delta和实时offset坐标。
- entry frame负值钳8。
- 动作更新失败在frame解析前停止。
- 三次flag查询及ghost/update/resolve/draw顺序与计数。

Linux core `186/186`与Linux app `192/192`通过。按阶段门禁，本单入口不重复执行Windows BUILD。workpack连续两轮生成均为`7/227`，SHA256均为`7926b61e47ea21deda9ae8f6b90c495c7086c7af70b32fe1f52f66c7668e3615`，只新增关闭`0x0043A880`。
