# 标准模式MAPS条件筛选记录构建 `0x0043BE90`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与owner

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043BE90..0x0043BFB4`，下一入口为`0x0043BFC0`。直接caller为`0x00443BD0`与`0x00446700`。

函数先释放旧`dword_4FB8D0`，再分配固定`0x800`字节指针表并把`dword_4FC834`计数清零。modern以最多512项的typed vector表达相同生命周期和容量；每次构建先清旧记录并预留容量，分配失败显式返回。

MAPS记录流从payload `+0x5C`的u32相对offset开始。首word为`FFFF`表示空流。

## 2. 名称与记录布局

每条记录先把固定64字节临时名称buffer清零，再逐字节复制到首个unaligned `%Q`标记`25 51`。标记后的typed布局为：

```text
+0  u32 first_value
+4  u16 second_value
+6  u16 condition[...]
```

condition列表以`FFFF`终止。condition sentinel后的下一个u16若为`FFFF`，表示整个记录流结束；否则该位置就是下一记录名称起点。

原临时buffer只允许64次物理写。第65字节会越界；恰好64个非NUL字节命中后会让`lstrlenA`越界读。modern分别在这两个原危险点返回`name_buffer_overflow`。若名称含embedded NUL，扫描仍继续到`%Q`，但命中记录只保存`lstrlen/lstrcpy`可见的首个NUL前缀。

## 3. 条件查询与入表

每个非`FFFF` condition都执行：

```text
query_id = zero_extend_u16(condition) + 0x1388
if query(query_id) == 1:
    accepted = true
```

命中后不短路，仍查询后续所有condition。只有返回精确1才命中，其他正值也不接受。

任一condition命中时，原函数分配`lstrlen(name)+7`字节，复制6字节头，再把C字符串写到`+6`，最后count加1。modern记录保留相同u32/u16头、NUL终止名称和显式length。第513条命中会越过原`0x800`字节指针表，modern在该原写点返回`record_capacity_overflow`并保留前512项。

## 4. 窄port与安全边界

`0x0040DC50`服务查询通过`LegacyStandardModeFilterQueryPorts`隔离；B9不复制其业务。`malloc/free/lstrlen/lstrcpy`由typed容器与数组替代，不改变记录筛选语义。

其他原危险点按访问顺序隔离：缺`+0x5C`目录、相对记录越界、缺`%Q`、缺condition sentinel及下一记录标记越界。已接受记录不因后续解析失败回滚，保持原逐项提交顺序。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- 旧表先清除，两条记录四个condition全部按顺序查询。
- 只有精确返回1命中，返回2不命中且查询不短路。
- 两条命中记录复制精确u32/u16头和名称。
- embedded NUL后字节不进入最终C字符串。
- 空流仍清旧表并零查询返回。
- 缺`+0x5C`、缺`%Q`和缺condition sentinel的typed-stop。
- 未命中的完整64字节名称不调用lstrlen，合法完成。
- 命中的64字节无NUL名称在原lstrlen过读点停止。
- 第65个名称字节在原CmdLine越界写点停止。
- 513条命中记录在第513个表项写入前停止，保留512项且执行513次query。

定向测试通过。workpack连续生成两轮均为`26/227`，SHA256均为`3b4136d16656acd22dabeaf2d93b693733cbc8cecf41c1210e68c3bb66e3c772`；只新增关闭`0x0043BE90`，`0x0043BFC0`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
