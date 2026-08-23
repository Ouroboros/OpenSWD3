# 标准模式数据库初始化 `0x0043D530`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与callback边界

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043D530..0x0043D873`，338行。它没有direct call指令caller，而由`0x00444FC0`写入callback表后间接调用。直接callee包括已关闭B980/BC90、尚未关闭F000、record loader、interface sample owner及malloc/release owner。

16次malloc改为固定typed owner，避免宿主地址和分配失败差异：四个1200×i32表、scan record、两个0xB0 runtime records、四个0xF0 buffer、四个0x1B8 buffer及一个0x400 mirror表。原函数不清零0xF0/0x1B8/mirror malloc内容，typed state保留调用前字节；只写LST明确位置。

## 2. 1200项record扫描

四个`0x12C0`表先逐dword填`0xFFFFFFFF`。scan index从0到1199，每轮：

1. 清零整个0xB0 scan record。
2. 以zero-extended u16 index调用loader，destination为`+0x0C`。
3. 成功时发布：
   - table0=`u16 +0x5E`。
   - table1=`u16 +0x60`。
   - table2=`u32 +0x2C`按位解释i32。
   - table3=`signed i8 +0xA7`扩展i32。
4. 无论成功失败都读取`u32 +0xAC`并release token。
5. index递增。

扫描后再release scan storage自身。UT锁定1200次load/release、首项/末项成功、失败项四表保持-1、`0xFE -> -2`及最后scan token仍在storage内。

## 3. 链表修正与常量owner

两个0xB0 runtime records精确清零。与forward owner共享节点类型的外部adjustment单链逐节点执行16-bit `combined = first + second`；`0xFFFF+2`回绕为1。

随后只写原字段，其他action内容保持：

- primary action ID=`0x232A`、base variant=`0x3B`。
- secondary action ID=`0x233B`、base variant=0。
- 五个reset owner最终为0。
- enable flag=1。
- scan index重置0。
- callback phase最终写u16 2。

UT预置两个action cached字段和buffer字节，证明未被synthetic全清。

## 4. F000、forward list与sample

尚未关闭F000以精确port边界表达，返回forward head。D530直接调用已关闭B980得到完整节点数，再用已关闭BC90从调用前head最多计16项并发布bounded count/current node。

两个文本索引均写`0xFFDC`。interface sample owner固定调用sample ID `0x0136`和外部interface source value。UT用3节点链锁定count3、bounded count3、current nullptr及sample参数。

## 5. buffer与mirror表

四个0xF0和四个0x1B8 owner只分配，不初始化。0x400 mirror owner也不整体初始化。源表只读取127个i32：

```text
for i = 0..126:
    mirror[128 + i] = source[i] / 2
    mirror[128 - i] = source[i] / -2
```

除法为signed向零截断。i=0两次写同一center，负半值后写并胜出；indices0、1、255完全保持原malloc字节。最后EAX为`source[126] / -2`。UT以奇数1..253锁定center0、index127=-1、index2=-126、index254=126、未写位置777及EAX=-126。

mirror source不足127项时在原首次越界读取点typed-stop：此前1200扫描、链表、常量、F000和sample均保留；mirror及最后两个reset/callback phase不写。独立UT锁定该顺序。

## 6. 验证

`special_modes.legacy_initial_menu`定向测试通过。workpack连续两轮稳定为`44/227`，SHA256均为`87f11ac9fefc9d31d6e69347fb52003c4dfe4797db9df11036fbd719a6133e91`；下一独立单元为`0x0043D880`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
