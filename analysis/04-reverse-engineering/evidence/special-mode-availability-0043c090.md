# 标准模式16字节记录可用性判断 `0x0043C090`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与调用图

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043C090..0x0043C0C8`，下一入口为`0x0043C0D0`；本函数无callee。LST有12个callsite、7个caller：`0x004070A0`六次，以及`0x0043C3C0`、`0x0043DA30`、`0x004407F0`、`0x00442F40`、`0x004455E0`和`0x0044B070`各一次。

函数以参数乘16定位记录，只读取offset0 enabled dword与offset12 state dword。modern以typed record span表达，不读取中间8字节。

## 2. 判断顺序

LST顺序为：

```text
if enabled == 0:
    return 0
if state == 1:
    return 1
if state <= 10:          // signed i32
    return 0
return (state % 2) == 0
```

`state > 10`使用signed比较。后续`and 0x80000001`及负余数修正是编译器生成的signed `% 2`序列；因为该路径已保证state为正且大于10，可观察结果就是正整数偶数可用、奇数不可用。不能把`state >= 1`或任意非零误判为可用。

## 3. typed边界与验证

`query_legacy_standard_mode_availability`返回typed status、bool与原0/1 EAX。负index或超出record span在原16字节表读取点返回`record_index_out_of_range`，不伪造不可用记录。

`special_modes.legacy_initial_menu`覆盖：

- enabled为0时即使state为1也不可用。
- enabled任意非零值均通过第一门。
- state精确1可用。
- state为负、0、2与10不可用。
- state 11/13奇数不可用，12偶数可用。
- `INT_MAX`奇数不可用，`INT_MAX-1`偶数可用。
- 负index与上界index在原表读点typed-stop。
- bool与legacy EAX始终一致。

定向测试通过。workpack连续生成两轮均为`28/227`，SHA256均为`79f22567a8aca4a9a2e73bb42912ddeb26c57d4184927d8981507e0893f5caaf`；只新增关闭`0x0043C090`，`0x0043C0D0`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
