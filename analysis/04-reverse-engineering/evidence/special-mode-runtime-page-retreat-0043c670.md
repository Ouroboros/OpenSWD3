# 标准模式运行时翻页后退组合器 `0x0043C670`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与调用点

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043C670..0x0043C6DF`，无外置chunk，下一workpack入口为`0x0043C760`。

LST有两个运行时callsite、两个caller：`0x0043C3C0`在`0x0043C4EC`调用一次，`0x00446260`在`0x0044626E` tail-jump一次。`0x0043B480`只保存函数地址；workpack caller字段仅作导航。

## 2. 固定顺序

函数依次：

1. 以window offset、local cursor与step `0x0F`调用已关闭`0x0043BC60`。
2. 用实时window offset、原64项entry base与entry alias owner调用`0x0043CC00`。
3. 调用`0x0043CBD0`刷新page。
4. u32回绕相加`window_offset + local_cursor`并从原entry base读取，调用`0x0043CEF0`。
5. mode flags低字节OR `0x03`。
6. 播放sample `0x2E`并返回sample EAX。

当local cursor非零时，`0x0043BC60`只清cursor而不改window offset；cursor已为0时window offset按u32回绕减15，signed负值钳0。其返回指针被后续调用覆盖。

## 3. typed边界与caller回接

`retreat_legacy_standard_mode_runtime_page`复用已关闭page-retreat helper。selected index只在原entry读取点检查64项边界；typed-stop保留page retreat、alias重建与page刷新，不执行消费、flags或sample。

`0x0043C3C0` first-dynamic caller已真实回接。重叠upper→first dynamic→page时，实时状态链为：

- upper `0x0043C590`：cursor 14→13，消费entry13，flags低字节1→3。
- first dynamic `0x0043C670`：非零cursor 13→0，消费entry0，flags保持3。
- page chunk `0x0043BBE0`：cursor 0归一化到14，不推进offset，消费entry14，flags 3→`0x33`。

三轮均执行alias重建、刷新、消费与sample；caller在前两轮后重新加载pointer Y，最后返回第三轮sample EAX。

## 4. 验证

`special_modes.legacy_initial_menu`覆盖：

- 非零cursor清0且offset保持30，消费entry30。
- 零cursor使offset 20减15到5，消费entry5。
- flags `0x30→0x33`、sample ID/handle/EAX与固定调用顺序。
- offset64/cursor1产生selected index64，在原表读取点typed-stop。
- 重叠caller链三轮消费entry13、entry0、entry14及12个port事件的精确顺序。

定向测试通过。workpack连续生成两轮均为`33/227`，SHA256均为`96b94897a246543feac8c35a3ab60bf66f907f10f63a5ec78480fd2c00d75298`；只新增关闭`0x0043C670`，`0x0043C760`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
