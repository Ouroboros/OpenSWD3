# 标准模式窗口分页前移helper `0x0043BBE0`

状态：`platform_adapted`、`unit_tested`

## 1. LST物理范围与调用图

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043BBE0..0x0043BC52`，下一入口为`0x0043BC60`；本函数无callee。

LST有六个直接调用点、五个caller：

- `0x0043C616`，caller为`0x0043C3C0`的共享chunk，step=`15`。
- `0x0043DF33`，caller为`0x0043DED0`，step=`16`。
- `0x00440D4B`，caller为`0x00440D20`，step=`10`。
- `0x0044369B`，caller为`0x00443670`，step=`8`。
- `0x004460E7`与`0x00446209`，caller为`0x00446090`，step分别为`13`与`8`。

五参数均被函数体读取，顺序为total指针、window offset指针、local cursor指针、visible count指针和signed step。owner归并为五组：

- `0x0043C616`：`4FC974/4FC90C/4FC928/4FC914`。
- `0x0043DF33`：`4FCAD8/4FCAD0/4FCBA4/4FCB98`。
- `0x00440D4B`：`4FCF90/4FCF88/4FCD3C/4FCD34`。
- `0x0044369B`与`0x00446209`：`4FC834/4FBED4/4FC510/4FB8D4`。
- `0x004460E7`：`4FD08C/4FD164/4FD098/4FD298`。

地址依次表示total、offset、cursor和visible。caller继续各自独立关闭，本helper不提前计入。

## 2. 非末项游标归一化

入口先以32位回绕计算`visible_count - 1`，再与local cursor按位相等比较。两者不相等时不翻页，只归一游标：

```text
local_cursor = 0
legacy_eax = visible_count
if visible_count >= 1:
    local_cursor = visible_count - 1
    legacy_eax = local_cursor
return legacy_eax
```

`visible_count >= 1`为signed i32规则。visible为0或负数时cursor保持刚写入的0，但EAX返回原visible值；visible至少为1时cursor与EAX均为`visible - 1`。total、offset和step在该路径不读写。

## 3. 末项分页与继续当前页

当local cursor等于回绕后的`visible - 1`时，函数先无条件推进offset：

```text
window_offset = wrap_i32(window_offset + step)
second_boundary = wrap_i32(window_offset + step)
```

随后把second boundary与total按signed i32比较。若`second_boundary < total_count`，继续当前visible窗口：

```text
cap = wrap_i32(total_count - window_offset - 1)
legacy_eax = cap
if local_cursor > cap:
    local_cursor = cap
return legacy_eax
```

cursor与cap按signed i32比较。EAX总是返回cap，即使cursor原本不大于cap且未写。visible count不变。

## 4. 最后一页重建

second boundary不小于total时重建最后一页：

```text
window_offset = wrap_i32(total_count - step)
if window_offset < 0:
    window_offset = 0
visible_count = wrap_i32(total_count - window_offset)
local_cursor = wrap_i32(visible_count - 1)
legacy_eax = local_cursor
```

所有加减均按32位自然回绕，比较均按signed i32。offset在进入末页路径前已经写过一次`old + step`，随后再覆盖为`total - step`或0；modern结果将该路径标为offset已写，而不省略原写入顺序。

## 5. 返回与typed边界

本函数所有路径都返回i32：游标归一化值、当前页cap或末页cursor。`0x00446209`所在switch分支清理栈并只改ECX/内存后直接返回，因此传播本helper EAX；其他callsite在消费前覆盖EAX。

`advance_legacy_standard_mode_window_page`使用total与step值参数，以及offset/cursor/visible三个typed引用。`LegacyStandardModeWindowPageAdvanceResult`区分`cursor_normalized`、`page_advanced`和`final_page_rebuilt`，同时保存legacy EAX与三个owner写入标记。函数不加饱和、范围修复或null分支。

## 6. 验证

`special_modes.legacy_initial_menu`覆盖十组边界：

- 非末项cursor归一到`visible - 1`。
- visible为0和负数时cursor写0而EAX保留visible。
- 当前页推进但cursor无需cap。
- 当前页推进并把cursor cap到`total - offset - 1`。
- 精确second boundary进入末页重建。
- `total - step`为负时offset钳0并重算visible/cursor。
- offset与step加法跨`INT_MAX/INT_MIN`回绕后继续当前页。
- visible为`INT_MIN`时`visible - 1`回绕比较及末页重建。
- signed负step的当前页推进。
- 三类路径、legacy EAX和三项写入标记均锁定。

定向测试通过。workpack连续生成两轮均为`20/227`，SHA256均为`aea28751bf54f377927b19787250cd7256f9963978769eaba4a8c548e354bbb0`；只新增关闭`0x0043BBE0`，`0x0043BC60`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
