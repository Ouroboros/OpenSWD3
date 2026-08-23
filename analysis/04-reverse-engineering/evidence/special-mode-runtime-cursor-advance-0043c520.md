# 标准模式运行时游标推进组合器 `0x0043C520`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与调用点

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043C520..0x0043C58F`，无外置chunk，下一workpack入口为`0x0043C590`。

LST有三个运行时callsite、两个caller：

- `0x0043C3C0`在`0x0043C416` tail-jump一次，在`0x0043C4D2` call一次。
- `0x00445C90`在`0x00445C9E` tail-jump一次。

`0x0043B480`只保存函数地址，不是direct call。workpack的caller字段仅为导航快照，不能覆盖本次完整LST审计。

## 2. 固定调用顺序

函数无参数并使用标准模式全局owner。LST顺序固定为：

1. 以`FC974` total、`FC90C` window offset、`FC928` local cursor、`FC914` visible count调用已关闭`0x0043BB80`。
2. 以当前window offset、原64项entry base和`FC920` alias owner调用`0x0043CC00`。
3. 调用`0x0043CBD0`刷新page。
4. 以32位回绕计算`window_offset + local_cursor`，从原entry base读取u32并调用`0x0043CEF0`。
5. 重新读取mode flags，只对低字节OR `0x30`；等价于整个u32 OR `0x30`。
6. 调用sample `0x2E`与当前sample handle，并返回sample调用EAX。

`0x0043BB80`的指针/整数联合EAX在本函数中立即被后续载入和call覆盖，不是`0x0043C520`最终返回。未关闭的alias重建、page刷新、entry消费和sample播放由现有窄port隔离，不提前计数。

## 3. typed边界与caller回接

`advance_legacy_standard_mode_runtime_cursor`复用`advance_legacy_standard_mode_window_cursor`，随后严格执行重建、刷新、selected entry读取、消费、flags和sample。

selected index使用u32回绕相加。负值或超出64项时只在原`[entry_base + index*4]`读取点返回`selected_entry_out_of_range`；此前cursor推进、alias重建和page刷新保持，后续消费、flags与sample不执行。

`0x0043C3C0`的两个caller不再停在抽象`dispatch_list_row` port：

- 第一矩形写入`visible_count - 2`式local cursor后，tail-dispatch本helper；因此最终cursor还会再由`0x0043BB80`推进一次，最终EAX是sample结果。
- bottom矩形调用本helper后重新载入pointer Y，覆盖sample EAX；helper状态副作用仍完整保留。

## 4. 验证

`special_modes.legacy_initial_menu`覆盖：

- local cursor命中边界后钳回末项、window offset推进一项。
- alias接收更新后的window offset，64项entry base保持。
- selected entry消费、mode flags `| 0x30`、sample `0x2E`和handle、sample EAX返回。
- selected index等于64时，在原entry读取点typed-stop且只保留cursor/rebuild/refresh副作用。
- `0x0043C3C0`第一矩形tail路径最终cursor、entry、flags与sample EAX。
- `0x0043C3C0`bottom路径保留helper副作用但最终返回pointer Y。

定向测试通过。workpack连续生成两轮均为`31/227`，SHA256均为`3b77b39c834345e45eb3b555273d34ac609db66abcf7482273be302a39bd3e0a`；只新增关闭`0x0043C520`，`0x0043C590`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
