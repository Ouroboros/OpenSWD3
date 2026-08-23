# 标准模式窗口分页回退helper `0x0043BC60`

状态：`platform_adapted`、`unit_tested`

## 1. LST物理范围与调用图

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043BC60..0x0043BC8E`，下一入口为`0x0043BC90`；本函数无callee。

LST有六个直接调用点、五个caller：

- `0x0043C686`，caller为`0x0043C670`，step=`15`。
- `0x0043E01C`，caller为`0x0043DFA0`，step=`16`。
- `0x00440E3B`，caller为`0x00440E10`，step=`10`。
- `0x004437EB`，caller为`0x004437C0`，step=`8`。
- `0x004462B7`与`0x004463CF`，caller为`0x00446260`，step分别为`13`与`8`。

每个callsite均压入五项，但函数体只读取`[esp+8]`的window offset指针、`[esp+0x0C]`的local cursor指针和`[esp+0x14]`的step。第一项total指针与第四项visible指针未读。modern只接受两个typed引用和step值，不伪造未读owner。

实际owner归并为五组：

- `0x0043C686`：offset=`dword_4FC90C`，cursor=`dword_4FC928`。
- `0x0043E01C`：offset=`dword_4FCAD0`，cursor=`dword_4FCBA4`。
- `0x00440E3B`：offset=`dword_4FCF88`，cursor=`dword_4FCD3C`。
- `0x004437EB`与`0x004463CF`：offset=`dword_4FBED4`，cursor=`dword_4FC510`。
- `0x004462B7`：offset=`dword_4FD164`，cursor=`dword_4FD098`。

caller继续各自独立关闭，本helper不提前计入。

## 2. 非零cursor路径

入口把local cursor参数地址装入EAX并先检查cursor：

```text
legacy_eax = &local_cursor
if local_cursor != 0:
    local_cursor = 0
    return legacy_eax
```

非零cursor不区分正负，均只清cursor；window offset与step不读取。EAX始终保留cursor参数地址。

## 3. offset回退路径

cursor为0时：

```text
window_offset = wrap_i32(window_offset - step)
if window_offset < 0:
    local_cursor = 0
    window_offset = 0
return &local_cursor
```

减法按32位自然回绕，`jns`按signed i32判断结果。结果非负时保留回绕减法值且不重写cursor；结果为负时按原顺序先写cursor 0，再写offset 0。函数不读取total或visible，也不做饱和与step合法化。

## 4. 返回与typed边界

所有路径都返回local cursor参数地址。六个callsite均在可观察消费前覆盖EAX；modern仍让`retreat_legacy_standard_mode_window_page`直接返回typed `i32*`，不截断为32位整数，也不新增null分支。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖九组边界：

- 正cursor非零时只清cursor。
- 负cursor非零时同样只清cursor。
- cursor为0时正常执行`offset - step`。
- 结果精确为0时保留0。
- 结果为负时offset与cursor归零。
- `INT_MIN - 1`回绕为`INT_MAX`并保留。
- `INT_MAX - (-1)`回绕为`INT_MIN`后归零。
- `0 - INT_MIN`得到`INT_MIN`后归零。
- 负offset减负step得到正数并保留。
- 所有路径均返回原local cursor引用地址。

定向测试通过。workpack连续生成两轮均为`21/227`，SHA256均为`eac0784e38c2ae6dbde7aca7b15de9c632ac50ce689e93748254bbcd131ab3d5`；只新增关闭`0x0043BC60`，`0x0043BC90`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
