# 标准模式单链计数helper `0x0043B980`

状态：`platform_adapted`

## 1. LST物理范围与ABI

权威范围为`swd3.exe.lst`的`0x0043B980..0x0043B993`：

```text
0043B980 mov eax, [esp+4]
0043B984 mov ecx, [eax]
0043B986 xor eax, eax
0043B988 test ecx, ecx
0043B98A jz 0043B993
0043B98C mov ecx, [ecx]
0043B98E inc eax
0043B98F test ecx, ecx
0043B991 jnz 0043B98C
0043B993 retn
```

函数是caller-cleanup的单参数cdecl。参数是链头指针变量的地址；每个节点偏移0保存下一节点指针。函数无callee、无全局写入、无链写入，返回EAX中的32位节点数。

## 2. 全部调用域

LST共有11个直接调用点，分布于10个caller：

- `0x0043BCC0`：传入caller的`arg_14`链头地址。
- `0x0043D530`、`0x0043E3D0`、`0x0043F000`、`0x0043F880`：传入`0x004FCAE0`。
- `0x00441160`、`0x00442050`：传入`0x004FCD70`。
- `0x00444E80`：传入`0x004FCFC0`。
- `0x00448230`：传入`0x004FD1E8`。
- `0x0044E4A0`的两个调用点：传入`0x004FD580`。

因此调用域归并为一个caller参数链和五个固定全局链，共6个链owner。各caller业务与紧邻的`0x0043B9A0`推进helper继续独立关闭；本单元不提前计入它们。

## 3. 精确语义

等价的32位语义为：

```text
node = *head_address
count = 0
while node != null:
    node = node->next_at_offset_zero
    count = count + 1 modulo 2^32
return count
```

必须保留：

- 空链返回0，且不读取节点。
- 非空链先读取当前节点的next，再递增计数。
- 只跟随偏移0，不读取任何payload。
- 不修改head或任一next。
- 不检测环；循环链在原函数中不终止。
- 计数使用32位回绕，不做饱和或上限保护。

## 4. typed平台适配

现代实现使用`LegacyStandardModeForwardNode`表达偏移0的intrusive next前缀，并直接接收typed head值。这样消除32位裸地址与宿主指针宽度差异，但不改变空链、遍历顺序、只读副作用或`u32`计数。

原函数会无条件解引用参数本身；typed引用边界不暴露“head地址为空”的非法调用。调用者仍必须提供有效owner，不能把该适配解释为原程序新增空参数保护。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- 空链返回0。
- 单节点返回1。
- 从中间节点开始返回2。
- 三节点返回3。
- head与全部next在调用后保持不变。

`special_modes.legacy_initial_menu`定向测试通过。workpack连续生成两轮均为`12/227`，SHA256均为`6c9092652090a83464cc5b5cf491be7ed409abc52f27aa3cb3e2a69ee6615ec4`；只新增关闭`0x0043B980`，`0x0043B9A0`仍为下一独立单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
