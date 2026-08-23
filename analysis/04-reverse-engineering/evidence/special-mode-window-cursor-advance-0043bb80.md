# 标准模式窗口游标预增helper `0x0043BB80`

状态：`platform_adapted`、`unit_tested`

## 1. LST物理范围与调用图

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043BB80..0x0043BBBE`，下一入口为`0x0043BBC0`；本函数无callee。

LST有八个直接调用点、七个caller：

- `0x0043C536`，caller为`0x0043C520`。
- `0x0043DD83`，caller为`0x0043DD20`。
- `0x00440B4B`，caller为`0x00440B20`。
- `0x0044347A`，caller为`0x00443450`。
- `0x004439CA`，caller为`0x004439A0`。
- `0x00445CFB`与`0x00445E3C`，caller为`0x00445C90`。
- `0x0044666A`，caller为`0x00446550`。

函数体只访问cdecl栈上的四个指针参数。八个callsite均在四指针之前额外压入一个立即数`8/10/13/15/16`，但函数体不读取第五项。modern只表达实际读取的四参数ABI。

实参owner归并为五组：

- `0x0043C536`：total=`dword_4FC974`，window offset=`dword_4FC90C`，local cursor=`dword_4FC928`，visible count=`dword_4FC914`。
- `0x0043DD83`：total=`dword_4FCAD8`，window offset=`dword_4FCAD0`，local cursor=`dword_4FCBA4`，visible count=`dword_4FCB98`。
- `0x00440B4B`：total=`dword_4FCF90`，window offset=`dword_4FCF88`，local cursor=`dword_4FCD3C`，visible count=`dword_4FCD34`。
- `0x0044347A`、`0x004439CA`、`0x00445E3C`和`0x0044666A`：total=`dword_4FC834`，window offset=`dword_4FBED4`，local cursor=`dword_4FC510`，visible count=`dword_4FB8D4`。
- `0x00445CFB`：total=`dword_4FD08C`，window offset=`dword_4FD164`，local cursor=`dword_4FD098`，visible count=`dword_4FD298`。

这些caller继续各自独立关闭，本helper不提前计入。

## 2. 无条件预增与早退

入口先保存第三参数地址于EAX，再无条件回绕递增local cursor：

```text
local_cursor = wrap_i32(local_cursor + 1)
legacy_eax = &local_cursor
if local_cursor < visible_count:
    return legacy_eax
```

`inc ecx`是32位自然回绕；`cmp ecx, esi`与`jl`按signed i32判断。未越过visible边界时，window offset、total和visible均不写，EAX仍是第三参数的裸地址，不是递增后的游标值。`INT_MAX`预增为`INT_MIN`后可按signed比较走这条指针返回路径。

## 3. 越界钳制与窗口推进

递增后的local cursor不小于visible count时，函数先钳制游标：

```text
local_cursor = 0
if visible_count >= 1:
    local_cursor = visible_count - 1
```

随后执行与`0x0043BB40`相同的回绕窗口推进：

```text
wrapped_end = wrap_i32(visible_count + window_offset)
legacy_eax = window_offset
if wrapped_end < total_count:
    window_offset = wrap_i32(window_offset + 1)
    legacy_eax = window_offset
return legacy_eax
```

加法、offset递增和游标预增都按32位自然回绕；比较全部按signed i32。函数不做饱和、overflow保护或null检查，total与visible只读。

## 4. 路径相关联合EAX

本函数的两类返回在原32位ABI中类型不同：

- 未越界：EAX是local cursor指针。
- 越界：EAX是最终window offset的32位整数值。

大多数callsite在消费前覆盖EAX。`0x00445E3C`所在switch分支只做聚合栈清理、内存位写、ECX装载与内存发布后直接返回，因此会继续传播该联合EAX。

modern不把64位宿主指针截断伪造成i32。`LegacyStandardModeWindowCursorAdvanceResult`以显式return kind区分`local_cursor_pointer`和`window_offset_value`：前者保存typed引用地址，后者保存原整数EAX；同时公开`cursor_clamped`和`window_offset_advanced`。这保留原可观察分支，又隔离32位裸指针宽度。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖十组边界：

- 普通预增后仍小于visible count，返回local cursor地址且不写offset。
- 多个未越界游标值保持指针返回。
- 预增恰好到visible边界时钳回`visible - 1`并推进offset。
- visible为0和1的钳制结果均为0。
- `visible + offset == total`时不推进。
- 负visible、负offset和负total按signed规则比较。
- local cursor从`INT_MAX`预增回绕为`INT_MIN`后走指针早退。
- window offset从`INT_MAX`递增回绕为`INT_MIN`。
- 回绕和total相等时不推进。
- 两类返回kind、typed指针、整数EAX和写入标记均与路径一致。

定向测试通过。workpack连续生成两轮均为`18/227`，SHA256均为`3c78e939eb8fff41dbdf4a848fba4052917c07d4e52293b35221b390452340ed`；只新增关闭`0x0043BB80`，`0x0043BBC0`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
