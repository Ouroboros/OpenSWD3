# 标准模式共享文本解析helper `0x0043B9E0`

状态：`platform_adapted`、`unit_tested`、`real_asset_tested`

## 1. LST物理范围与输入

权威范围为`swd3.exe.lst`的`0x0043B9E0..0x0043BA39`。函数接收一个记录指针，只读取记录`+0x04`的u16：

```text
0043B9E0 mov eax, [esp+4]
0043B9E4 mov ax, [eax+4]
0043B9E8 cmp ax, 0FFDCh
0043B9EC jnz 0043BA02
```

`mov ax`只覆盖EAX低16位，但后续特殊路径只比较AX，普通路径在使用索引前执行`and eax, 0FFFFh`。因此两条合法路径的可观察输入都精确等于记录`+0x04`的u16；宿主typed边界直接接收该值，不继承32位记录裸指针布局。

函数写固定128字节共享buffer`0x004FC2A0`。LST共有46个直接调用点，分布于38个caller；参数寄存器分布为EAX 41次、ECX 3次、EDX 2次。八个caller各调用两次：`0x00440B20`、`0x00440C20`、`0x00440D20`、`0x00440E10`、`0x00440F00`、`0x00440FB0`、`0x00441160`和`0x0044E4A0`。战斗列表内容caller `0x004658EB`与网格列表帧caller `0x00465D9C`现均直接调用本typed实现：从组A对象field关联的记录`+4`取得text index，MAPS payload与固定128字节buffer由外部现有owner以span绑定，不在battle复制共享buffer。

## 2. `0xFFDC`特殊路径

当输入word为`0xFFDC`时，原函数执行：

```text
wsprintfA(byte_4FC2A0, byte_49F9FC)
```

`byte_49F9FC`的固定字节为`B5 4C 00`，即CP950“無”，且不含格式占位符。`wsprintfA`因此只把两个字节和NUL写入共享buffer，EAX返回2。

现代实现直接写相同三字节，并在结果中保留`formatter_return=2`。该路径不访问MAPS payload；空payload定向测试通过。

## 3. 普通MAPS相对目录路径

非`0xFFDC`输入执行：

```text
base = dword_4C9A10
relative_table = u32(base + 0x4C)
entry = relative_table + zero_extend_u16(index) * 4   // u32 wrap
record = u32(base + entry)
source = base + record
```

随后从source开始，在每个字节位置做unaligned u16比较：

```text
while u16_le(source) != 0x5125:   // bytes 25 51, "%Q"
    shared_buffer[count] = *source
    count++
    source++
shared_buffer[count] = 0
```

embedded NUL不终止扫描，也原样复制。空文本直接在buffer首字节写NUL。正常出口EAX保留指向source `%Q` marker的原始指针；modern以相对payload的`source_cursor_offset`表达同一位置。

原函数不检查目录、目录项、record、terminator或128字节buffer边界。modern保持已提交copy顺序，并只在原始非法读写点typed-stop：

- 缺`+0x4C`目录或目录项：`maps_payload_out_of_range`，buffer不写。
- 扫描需要读取payload外的unaligned word：`text_terminator_not_found`，保留此前已复制字节，不追加NUL。
- 下一普通字节或最终NUL将写到buffer索引128：`destination_overflow`，保留前128字节。

不把embedded NUL、长文本或缺marker改写为普通C字符串成功。

## 4. 返回值与caller边界

原始EAX具有路径相关类型：特殊路径是`wsprintfA`返回的长度2，普通路径是MAPS source marker指针。46个直接call site均未用该值在helper后的同一控制点分支；部分路径在后续call/EAX写入前忽略它，少数caller直接把当时EAX继续带到自身出口。

modern结果同时保留特殊路径的`formatter_return`和普通路径的`source_cursor_offset`，避免把两种互不兼容的32位裸值伪装成单一宿主指针。caller业务及共享buffer消费者继续独立关闭，本单元不提前计入。

## 5. synthetic与真实资产验证

`special_modes.legacy_initial_menu`覆盖：

- `0xFFDC`在空MAPS上输出`B5 4C 00`，其后buffer字节不变。
- 普通文本复制embedded NUL并继续到首个unaligned `%Q`。
- `table_offset + index * 4`显式u32回绕。
- 128字节文本在最终NUL越界点返回`destination_overflow`，前128字节已提交。
- source只余一个字节时在unaligned word读取点返回`text_terminator_not_found`。
- payload不足`+0x4C`时在任何buffer写入前停止。

真实`MAPS.DAT`固定证据：

```text
file size      = 162929
payload size   = 162417
payload +0x4C  = 0x0001D993
index 1 record = 0x0001F8D7
text bytes     = "Nullitm6  "
marker offset  = 0x0001F8E1
copied bytes   = 10
```

真实资产测试读取原文件、去掉`0x200`前缀后调用typed实现，得到完全相同的10字节、NUL和marker offset。测试继续使用既有`legacy_real_assets`全局锁。

定向synthetic与真实资产测试均通过。workpack连续生成两轮均为`15/227`，SHA256均为`84d8a2d5bb81a2af2ca1d6f495be9a0aff3c2681b67cd56a9937337440fe8346`；只新增关闭`0x0043B9E0`，`0x0043BA40`仍为下一独立单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
