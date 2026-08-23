# 标准模式运行时游标后退组合器 `0x0043C590`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与调用点

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043C590..0x0043C5FF`，无外置chunk，下一workpack入口为`0x0043C670`。

LST有两个运行时callsite、两个caller：`0x0043C3C0`在`0x0043C4BA`调用一次，`0x00445E90`在`0x00445E9E` tail-jump一次。`0x0043B480`只保存函数地址，不是direct call；workpack caller字段仍仅作导航。

## 2. 固定调用顺序

函数严格执行：

1. 以`FC90C` window offset和`FC928` local cursor调用已关闭`0x0043BBC0`。
2. 以当前window offset、原64项entry base与`FC920` alias owner调用`0x0043CC00`。
3. 调用`0x0043CBD0`刷新page。
4. u32回绕相加`window_offset + local_cursor`，从原entry base读取并调用`0x0043CEF0`。
5. 重新读取mode flags，只对低字节OR `0x03`；等价于整个u32 OR `0x03`。
6. 调用sample `0x2E`与sample handle并返回其EAX。

`0x0043BBC0`的指针/整数联合EAX被后续调用覆盖，不是组合器最终返回。page刷新已直接复用关闭的`0x0043CBD0`；未关闭alias/consume/sample继续由与`0x0043C520`共享的窄port隔离。

## 3. typed边界与caller回接

`retreat_legacy_standard_mode_runtime_cursor`复用已关闭的window retreat helper。alias重建后先执行CBD0并传播其alias读取typed-stop；CBD0完成后selected index只在原`[entry_base + index*4]`读取点检查64项边界。停止时保留此前cursor/alias/visible副作用，不执行消费、flags或sample。

`0x0043C3C0`的upper caller已从抽象port改为真实helper：helper完成全部副作用后，caller重新加载pointer Y覆盖sample EAX。若upper范围与first dynamic及page范围重叠，顺序为：

```text
C590 retreat/rebuild/refresh/consume/OR3/sample
first dynamic callee
BBE0 page/rebuild/refresh/consume/OR30/sample
```

因此初始flags低字节1最终变为`0x33`；upper先把local cursor 14后退到13，随后page helper因13不等于visible末项14而只归一化回14，不推进window offset。这一顺序不能用占位upper回调推断。

## 4. 验证

`special_modes.legacy_initial_menu`覆盖：

- local cursor从0减到负值后钳0，window offset从2后退到1。
- alias、page刷新、entry 1消费、flags低字节`0x30→0x33`、sample ID/handle/EAX。
- selected index等于64时在原表读点typed-stop，仅保留retreat/rebuild/refresh。
- `0x0043C3C0`重叠upper→dynamic→page的两轮alias/refresh/consume/sample顺序、两个entry和最终flags `0x33`。
- page selected-entry越界用不命中upper的Y=99独立覆盖，避免caller副作用污染边界场景。

定向测试通过。workpack连续生成两轮均为`32/227`，SHA256均为`23447b5e37aeac637e55272f7920bbfa8c75af181e445a531859f7ff0e4303c9`；只新增关闭`0x0043C590`，`0x0043C670`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
