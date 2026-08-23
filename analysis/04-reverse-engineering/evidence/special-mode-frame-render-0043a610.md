# 标准特殊模式画面呈现 `0x0043A610`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0043A610..0x0043A87E`。唯一调用者是`0x00439FD0`。调用点压入一个零dword，但函数从不读取该栈槽；调用者也不读取EAX。

入口读取的核心状态为：

- 帧计数`0x004FC784`。
- secondary字`0x004FC900`。
- primary派生索引低16位`0x004FC3C4`。
- transition extent `0x004FC3C8`。
- 特殊模式活动值`0x004B8740`。

## 2. transition入口算术

帧计数等于`0x41`时，extent先写为`0x320`。随后通常执行一次32位算术右移；只有以下任一条件成立时跳过：

- 派生索引`0x0F`、secondary大于1，且剧情标志`0x49`字面等于1。
- 派生索引`0x0B`且secondary无符号大于等于`0x1F4`。

算术右移保留符号位。extent后续的取负、左移两倍和比较也都保持32位回绕与有符号条件码，不能改成无符号尺寸运算。

## 3. surface建立、主动作和callback早退

函数固定先调用`0x00416F10`取得主surface token，再以`0x00416F60`建立该surface状态。

secondary不等于`0xEA60`时：

1. 帧计数无符号小于`0x4B`则取当前extent，否则取0。
2. 查询剧情标志`0x49`。
3. 若flag字面等于1且派生索引等于`0x0F`，强制offset为0。
4. 以32位回绕加`0xB4`。
5. 调用`0x0040EBF0(primary_record, offset, 0)`。

secondary不等于1时，随后调用间接槽`0x004FBEC8`。回调后立即重新读取特殊模式值；若已经清零，函数直接返回，不执行blocking gate、绘制、提交或终态快照。

## 4. blocking gate与两种transition路径

`0x004FBAB0 != 0`时，函数跳过全部主体绘制与提交，但仍执行终态快照。

未阻塞时，secondary等于`0xEA60`或派生索引等于`0x11`会直接进入公共呈现尾，不绘制transition。其他情况分为：

- 扩张路径：派生索引`0x0F`、secondary大于1且flag`0x49 == 1`；或派生索引`0x0B`且secondary大于等于`0x1F4`。
- 普通路径：其余组合。

扩张路径在extent为0时先写1，然后：

1. `0x0040EBF0(transition_record, -extent, 0)`。
2. `0x0043AAA0(extent)`。
3. 重新读取extent并左移一位。
4. 只有有符号结果大于`0x320`时钳为`0x320`。

普通路径先调用`0x0043A880`，再以相同负offset绘制transition record并调用`0x0043AAA0(extent)`，但不改变extent。

每个callee之后需要的共享值都重新读取，因此端口回调修改extent、secondary或派生索引会影响后续分支。

## 5. 公共呈现尾

主体路径结束后顺序固定为：

1. secondary等于1时调用`0x004117F0(0x27C, 0x1CC, 0, surface)`。
2. 写软件鼠标frame index `0x0D`。
3. 查询剧情标志9；返回0才调用`0x004149B0`。
4. 颜色delta非零时，以捕获surface、像素数`0x4B000`和三份相同delta调用`0x00420490`。
5. 调用`0x004153D0`。
6. 调用原空函数槽。
7. 以selector `0x2711`调用`0x00437DF0`，随后执行DirectDraw等待提交。

最后无论正常提交还是blocking gate，都重新读取派生索引，并把当前鼠标X/Y保存到终态快照。callback清mode早退是唯一跳过该快照的路径。

## 6. typed owner与验证

`LegacyStandardModeRenderState`保存extent、surface token、blocking/color状态、鼠标frame和终态快照。SDL已复用既有真实`0x0040EBF0`动作桥、`0x004149B0`软件鼠标和`0x00420490`颜色调整；未关闭的panel、transition、overlay与surface包装继续通过窄端口隔离。mode3保留现有真实初始菜单绘制，呈现端口不重复提交同一帧。

`special_modes.legacy_initial_menu`覆盖：

- entry extent写`0x320`后减半的高模式直达尾。
- secondary1普通面板、主动作offset、transition负offset和secondary surface参数。
- 派生索引`0x0F`的三次flag49查询、offset清零及extent `0 -> 1 -> 2`。
- callback清mode立即返回且不发布终态。
- blocking gate跳过主体但仍发布终态。
- flag9鼠标门、颜色参数、公共overlay、提交和快照顺序。
- flag查询、动作加载、callback、transition、鼠标和提交计数。

Linux core `186/186`与Linux app `192/192`通过。按阶段门禁，本单入口不重复执行Windows BUILD。workpack连续两轮生成均为`6/227`，SHA256均为`a5d803139a08653d833dda9ed715afd66496b9edd37534e477e29bec3679b98b`，只新增关闭`0x0043A610`。
