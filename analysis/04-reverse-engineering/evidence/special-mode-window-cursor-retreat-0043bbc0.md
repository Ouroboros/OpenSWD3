# 标准模式窗口游标回退helper `0x0043BBC0`

状态：`platform_adapted`、`unit_tested`

## 1. LST物理范围与调用图

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043BBC0..0x0043BBDE`，下一入口为`0x0043BBE0`；本函数无callee。

LST有八个直接调用点、七个caller：

- `0x0043C5A6`，caller为`0x0043C590`。
- `0x0043DE6C`，caller为`0x0043DDF0`。
- `0x00440C4B`，caller为`0x00440C20`。
- `0x0044359A`，caller为`0x00443570`。
- `0x0044390A`，caller为`0x004438E0`。
- `0x00445EFB`与`0x00446037`，caller为`0x00445E90`。
- `0x0044652B`，caller为`0x00446420`。

八个callsite均压入五项，但函数体只读取`[esp+8]`和`[esp+0x0C]`两个指针。按cdecl顺序，第一项total指针、第四项visible指针和第五项立即数全部未读；只有第二项window offset与第三项local cursor是行为输入。modern只接受这两个typed引用，不伪造三个未读参数。

两个实际owner归并为五组：

- `0x0043C5A6`：window offset=`dword_4FC90C`，local cursor=`dword_4FC928`。
- `0x0043DE6C`：window offset=`dword_4FCAD0`，local cursor=`dword_4FCBA4`。
- `0x00440C4B`：window offset=`dword_4FCF88`，local cursor=`dword_4FCD3C`。
- `0x0044359A`、`0x0044390A`、`0x00446037`和`0x0044652B`：window offset=`dword_4FBED4`，local cursor=`dword_4FC510`。
- `0x00445EFB`：window offset=`dword_4FD164`，local cursor=`dword_4FD098`。

这些caller继续各自独立关闭，本helper不提前计入。

## 2. 无条件预减与早退

入口先把第三参数地址留在EAX，再无条件回绕递减local cursor：

```text
local_cursor = wrap_i32(local_cursor - 1)
legacy_eax = &local_cursor
if local_cursor >= 0:
    return legacy_eax
```

`dec ecx`是32位自然回绕，`jns`直接消费其sign flag。递减结果非负时，window offset不读不写，EAX仍是local cursor参数地址。`INT_MIN - 1`回绕为`INT_MAX`后走该指针返回路径。

## 3. 负游标钳制与offset回退

递减结果为负时：

```text
local_cursor = 0
legacy_eax = window_offset
if window_offset > 0:
    window_offset = window_offset - 1
    legacy_eax = window_offset
return legacy_eax
```

window offset通过`test/jle`按signed i32判断。只有严格正数才递减；0与负数保持原值。由于递减只发生在正数域，不存在offset下溢路径。local cursor已在判断前执行32位回绕预减。

## 4. 路径相关联合EAX

原32位ABI有两类返回：

- local cursor预减后非负：EAX为local cursor指针。
- local cursor预减后为负：EAX为最终window offset整数值。

大多数callsite在消费前覆盖EAX。`0x00446037`所在switch分支只清理栈、写内存、装载ECX并发布状态后直接返回，因此传播该指针/整数联合EAX。

modern使用`LegacyStandardModeWindowCursorRetreatResult`和显式return kind区分typed cursor地址与i32 offset值，并公开`cursor_clamped`和`window_offset_retreat`。不把64位宿主地址截断为伪32位整数，也不新增null分支。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖九组边界：

- 普通cursor预减后仍为正数，返回cursor地址。
- cursor从1减到0仍走非负指针返回。
- cursor从0减到-1后钳0并递减正offset。
- offset从1减到0。
- offset为0时不递减。
- offset为负数及`INT_MIN`时不递减。
- cursor从`INT_MIN`预减回绕为`INT_MAX`并返回cursor地址。
- 正offset最大值正常减一。
- 两类return kind、typed指针、整数EAX及写入标记均与路径一致。

定向测试通过。workpack连续生成两轮均为`19/227`，SHA256均为`1ab952b8fbe8927855dc0644a28926d30b3beaa5b54df64a2cb84c2676dea681`；只新增关闭`0x0043BBC0`，`0x0043BBE0`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
