# 标准特殊模式transition项目块 `0x0043AAA0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0043AAA0..0x0043AE35`。两个调用点均来自`0x0043A610`，唯一栈参数是当前transition extent；调用者不读取EAX。

函数固定调用`0x004239D0(0x1D, 0x1B, 0x15)`建立文字token，然后扫描4个项目。对应owner为：

- 项目记录：`0x004A6A38`起的4个`0x1C`记录。
- byte stage：`0x004A6AAC[0..3]`。
- 指标记录：`0x004AB79A`附近4组、步长`0x38`。
- 主ghost动作：18记录owner中的记录11..14。
- 三个装饰动作：记录3、4、5。

## 2. byte stage与可用门

每个项目先更新stage：

- `low16(item_count) == item_index`：8位回绕加3，再以有符号比较把大于16的结果钳为16。
- 否则：8位回绕减1，再以有符号比较把小于6的结果钳为6。
- `item_count == 5`或secondary等于1时，最终强制stage为16。

stage更新发生在可用门之前，因此`source_index == 0xFFFF`的不可用项目仍会更新stage，随后跳过该项目其余工作。原stage以`movsx`传给四个ghost调用。

## 3. 四次ghost与比例线

可用项目按固定顺序调用：

1. 主动作记录11+index：`(anchor_x - extent, anchor_y, stage)`。
2. 装饰记录3：`(100 - extent, anchor_y - 39, stage)`。
3. 装饰记录4：`(100 - extent, anchor_y - 17, stage)`。
4. 装饰记录5：`(100 - extent, anchor_y + 5, stage)`。

主动作之后以及三个装饰动作之前，各绘制一条比例线。三条X坐标分别为：

```text
signed((79 * metric[0]) / metric[3]) + 100
signed((79 * metric[1]) / metric[4]) + 100
signed((79 * metric[2]) / metric[5]) + 100
```

机器使用`idiv`。合法数据的三个分母必须非零；现代实现以typed-stop隔离零除，不继续伪造后续项目。

## 4. 五段文字

每个可用项目按顺序绘制5段文字，token与style `4`保持不变：

1. primary owner label：`(anchor_x - extent - 88, anchor_y - 94)`。
2. primary owner level：`(92 - extent, anchor_y - 76)`；显示`level_count`与`level.dat读取值 - level_base`。
3. secondary owner第一对：`(108 - extent, anchor_y - 52)`，显示`metric[0]/metric[3]`。
4. secondary owner第二对：同X、`anchor_y - 30`，显示`metric[1]/metric[4]`。
5. secondary owner第三对：同X、`anchor_y - 8`，显示`metric[2]/metric[5]`。

`0x00477290`的参数严格为`entry_index=item_index+1`、`count=level_count+1`。文字格式化与`0x00436AD0`内部仍需独立闭环；当前端口保留owner、种类、坐标、数值、token与style。

## 5. 全宽线与marked动作

五段文字后固定调用`0x00416FF0(0,0,640,480)`，即端口中的全宽线。指标记录`+0x1B`的位`0x80`置位时，再调用：

```text
0x0040ECC0(main_action, anchor_x - extent, anchor_y, 0x28)
```

随后推进项目记录`0x1C`、指标记录`0x38`、动作记录`0x98`和label槽`0x10`，直至4项全部处理。

## 6. typed owner与验证

`LegacyStandardModeTransitionState`保存4个stage和4组强类型指标。项目锚点显式锁定在`LegacyStandardModeItemRecord +0x06/+0x08`并保持整个记录`0x1C`布局。SDL marked动作复用既有`update_draw_legacy_action_with_flags()`；ghost、比例线、level读取和文字callee保持窄端口。

`special_modes.legacy_initial_menu`覆盖：

- 可用项完整4次ghost、4条线、5段文字和marked动作顺序。
- 主动作记录11及装饰记录3/4/5映射。
- stage加3/减1、有符号6..16钳位和不可用项仍更新stage。
- 三个`79*n/d+100`比例坐标。
- 全部ghost、文字和marked坐标。
- level读取`(1,3)`、level差值和三对指标值。
- item count5对已访问stage强制16。
- 零除typed-stop不继续后续项目。
- active/ghost/line/text/marked计数。

Linux core `186/186`与Linux app `192/192`通过。按阶段门禁，本单入口不重复执行Windows BUILD。workpack连续两轮生成均为`8/227`，SHA256均为`f4725f41ceba9417b1751f0dc8e5df121f4b56b1d1dd55bed41716e0b95e3c9d`，只新增关闭`0x0043AAA0`。
