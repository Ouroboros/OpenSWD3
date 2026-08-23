# 标准模式窗口游标归一化helper `0x0043BB40`

状态：`platform_adapted`、`unit_tested`

## 1. LST物理范围与调用图

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043BB40..0x0043BB7B`，下一入口为`0x0043BB80`；本函数无callee。

LST有四个直接调用点：

- `0x0043F934`，caller为`0x0043F880`。
- `0x004410F5`，caller为`0x00441060`。
- `0x004414CF`，caller为`0x00441160`。
- `0x00443AFA`，caller为`0x00443A60`。

函数体只访问cdecl栈上的四个指针参数：`[esp+4]`、`[esp+8]`、`[esp+0x0C]`和`[esp+0x10]`。四个caller均在这四个指针之前额外压入一个立即数，分别为`16`、`10`、`10`和`24`；函数体从不读取该第五项。modern只表达实际读取的四参数ABI，不把caller遗留的未读栈项伪造成行为输入。

四处实参owner依次为：

- `0x0043F934`：total=`dword_4FCAD8`，window offset=`dword_4FCAD0`，local cursor=`dword_4FCBA4`，visible count=`dword_4FCB98`。
- `0x004410F5`与`0x004414CF`：total=`dword_4FCF90`，window offset=`dword_4FCF88`，local cursor=`dword_4FCD3C`，visible count=`dword_4FCD34`。
- `0x00443AFA`：total=`dword_4FCFB8`，window offset=`dword_4FCF98`，local cursor=`dword_4FD070`，visible count=`dword_4FCFA0`。

前三个未接caller和第四个未接caller继续各自独立关闭；本helper不提前计入它们。

## 2. 局部游标门与改写

入口先读取local cursor和visible count并执行signed i32比较：

```text
legacy_eax = local_cursor
if local_cursor < visible_count:
    return legacy_eax
```

早退路径不写任何owner，EAX保留入口读取的local cursor。

只有`local_cursor >= visible_count`时才改写游标：

```text
local_cursor = 0
if visible_count >= 1:
    local_cursor = visible_count - 1
```

`visible_count`的比较同样是signed i32。值小于1时保持刚写入的0；值至少为1时写`visible_count - 1`。该路径即使最终数值与原值相同也真实执行写入。

## 3. 窗口offset推进与32位回绕

游标改写后，函数重新读取visible count、window offset和total：

```text
wrapped_end = wrap_i32(visible_count + window_offset)
legacy_eax = window_offset
if wrapped_end < total_count:
    window_offset = wrap_i32(window_offset + 1)
    legacy_eax = window_offset
return legacy_eax
```

`add edx, eax`和`inc eax`都是32位自然回绕；后续`cmp/jge`按signed i32解释回绕结果。函数不做溢出保护、不饱和，也不读取算术overflow flag。total count与visible count始终只读。

`0x0043F880`在call后只做聚合栈清理并直接返回，因此传播本helper的路径相关EAX；其余三个caller在使用前覆盖EAX。modern仍通过typed结果保留完整返回合同：早退返回原local cursor，改写路径返回最终window offset。

## 4. modern typed边界

`adjust_legacy_standard_mode_window_cursor`使用四个typed i32输入/引用替代裸全局：

- total count与visible count按值只读。
- window offset与local cursor按引用表达原指针写入。
- `std::bit_cast<u32>`加法及递增后再bit-cast回i32，锁定x86回绕。
- `LegacyStandardModeWindowCursorResult`公开`cursor_rewritten`、`window_offset_advanced`和`legacy_return_value`。

原函数假定四个指针有效，modern引用合同不新增null分支。函数无资源、平台或业务副作用，归为`platform_adapted`只因32位裸owner被typed引用隔离。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖九组边界：

- local cursor严格小于visible count时零写入并返回原cursor。
- 负cursor与零visible count的signed早退。
- visible count为0时把cursor写0并按窗口容量推进offset。
- visible count为1时把cursor写0。
- visible count大于1时把cursor写为`visible - 1`。
- `visible + offset == total`时不推进。
- 负visible、负offset及负total的signed比较。
- `INT_MAX + 1`回绕为`INT_MIN`后成立的推进及offset递增回绕。
- 同一回绕结果与`INT_MIN` total相等时不推进。

定向测试通过。workpack连续生成两轮均为`17/227`，SHA256均为`9fa7ef74a307b40c6ff041ee187482160bd35825de828c2ca2af2486fdbbc5e5`；只新增关闭`0x0043BB40`，`0x0043BB80`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
