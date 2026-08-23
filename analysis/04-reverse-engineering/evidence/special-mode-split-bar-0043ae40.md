# 标准特殊模式分段bar `0x0043AE40`

状态：`platform_adapted`

## 1. LST锁与10参数ABI

权威范围为`swd3.exe.lst`的`0x0043AE40..0x0043B07F`。10个调用点分布于7个特殊模式入口。函数读取10个32位栈槽：

```text
x, y, height, overlay_flags低字节,
first_ratio(float), second_ratio(float),
out_top*, out_first_split*, out_second_split*, out_bottom*
```

IDA旧签名把它概括成7参数，不可作为实现依据。现代`LegacyStandardModeBarRequest`和`LegacyStandardModeBarOutputs`显式保持全部输入与四个输出。

## 2. 第一段bar

函数先以原10参数中的几何字段调用`0x0043BAB0`准备区域，再计算`bottom = y + height`，执行32位回绕，并立即发布：

- `*out_top = y`。
- `*out_bottom = bottom`。

随后调用`0x00416FF0(x, y, x + 32, bottom)`。

记录6作为第一bar动作：

1. 调用`0x004321E0`更新。
2. 返回0只执行诊断，仍继续解析frame。
3. 以更新后的`+0x4A/+0x4C`调用`0x004315D0`一次。
4. 从`y`开始，只要有符号`cursor < bottom`，就在`(x,cursor)`调用`0x004170E0`，flags实时读取记录`+0x18`，opacity为0。
5. 每轮以frame无符号height推进cursor。

frame height为0且区间非空会形成原始无限循环；现代实现以typed-stop隔离。

## 3. float分割与第二段bar

机器以x87加载整数height，分别乘两个32位float，并由`0x00489654`把舍入模式临时改为向零截断后执行64位`fistp`。现代实现以`double(height) * double(float_operand)`后截为`int64`，再保留低32位。

发布：

```text
first_split  = y + trunc(height * first_ratio)
second_split = y + trunc(height * second_ratio)
```

随后调用`0x00416FF0(x, y, x + 32, second_split)`。

记录7作为第二bar动作，保持与第一段相同的“更新失败仍继续、解析一次、按frame height平铺”合同；绘制X为`x + 3`，区间为`[first_split, second_split)`。

## 4. 清屏与overlay动作

两段bar后固定调用`0x00416FF0(0,0,640,480)`。然后依次绘制：

1. 记录8：base variant `0x1A`，坐标`(x, y - 16)`。
2. 记录9：base variant `0x1B`，坐标`(x, bottom)`。
3. `overlay_flags & 1`时，记录8改为variant `0x1E`并在同坐标再绘制。
4. `overlay_flags & 2`时，记录9改为variant `0x1F`并在同坐标再绘制。

四个动作均调用`0x0040EBF0`；位1与位2独立，同帧可同时追加两个overlay。

## 5. typed owner与验证

`LegacyStandardModeBarPorts`隔离区域准备、矩形、动作更新、frame解析、tile blit与通用动作绘制。已关闭的动作/frame接口可直接适配；10个调用点在各自入口关闭时接入。

`special_modes.legacy_initial_menu`覆盖：

- 10参数请求与4个输出。
- 第一动作更新失败仍继续解析和tile。
- 两个bar的X偏移、区间、实时flags与tile序列。
- `0.25F/0.75F`的x87向零截断结果。
- 三次矩形调用及全屏参数。
- variants `0x1A/0x1B/0x1E/0x1F`与两个独立flag位。
- frame/update/rectangle/action计数。
- 零height typed-stop。

Linux core `186/186`与Linux app `192/192`通过。按阶段门禁，本单入口不重复执行Windows BUILD。workpack连续两轮生成均为`9/227`，SHA256均为`51f6fa2f32795996d7688c1f84d3ff203de46092fd17e81fc623336a02bfba33`，只新增关闭`0x0043AE40`。
