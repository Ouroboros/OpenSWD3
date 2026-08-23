# 标准模式MAPS值分组查找 `0x0043BE40`

状态：`platform_adapted`、`unit_tested`

## 1. LST物理范围与调用图

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043BE40..0x0043BE8B`，下一入口为`0x0043BE90`；本函数无callee。三个直接caller为`0x00443BD0`、`0x00446700`和`0x0044BDA0`，均以null/非null消费返回分组指针。

函数接收full-width i32 target，读取MAPS payload基址及`+0x58`相对目录。modern接收typed payload span并返回相对group offset，不保留32位裸基址。

## 2. 起始组与空目录

原函数读取：

```text
group = base + u32(base + 0x58)
if u16(group) == 0xFFFF:
    return null
```

相对offset保持u32回绕。缺`+0x58`字段、起始group越界或后续扫描越界只在原非法读点返回`maps_payload_out_of_range` typed-stop。

## 3. 组内值扫描

每个group从`group + 6`开始逐个读取u16：

```text
value_cursor = group + 6
while u16(value_cursor) != 0xFFFF:
    if zero_extend_u16(value_cursor) == target_i32:
        return group
    value_cursor += 2
```

LST先`and ecx, 0xFFFF`再与完整ESI比较，因此target为负数或大于65535时永不命中，不能把target截断成u16。u16读取允许unaligned地址。

## 4. 下一组与总结束

组内`FFFF`之后的下一个u16同时是下一group的首word：

```text
next_group = value_cursor + 2
if u16(next_group) == 0xFFFF:
    return null
group = next_group
```

因此连续语义是“当前组值终止符”后再读“下一组首word”；第二个`FFFF`表示全表结束。命中返回当前group起始地址，不返回值位置。modern以`found + group_offset`表达；合法双`FFFF`未命中为`not_found`。

## 5. 验证

`special_modes.legacy_initial_menu`使用unaligned synthetic payload覆盖：

- 第一组多个值命中均返回第一group offset。
- 跨内层`FFFF`进入第二组并命中。
- 普通未命中走末尾双`FFFF`返回`not_found`。
- target为负数或`0x10000`不截断匹配u16。
- 起始group首word为`FFFF`表示空目录。
- payload缺`+0x58`目录字段时typed-stop。
- u32相对目录为`0xFFFFFFFE`时保留回绕值后在范围门停止。
- 组值列表到payload末尾仍无`FFFF`时typed-stop。

定向测试通过。workpack连续生成两轮均为`25/227`，SHA256均为`1675589164a3f4811b024c4bbf4146bb838c3944d2c24f03a2a8b6e2ec50a2cf`；只新增关闭`0x0043BE40`，`0x0043BE90`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
