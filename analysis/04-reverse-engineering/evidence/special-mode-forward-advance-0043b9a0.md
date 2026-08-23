# 标准模式单链推进helper `0x0043B9A0`

状态：`platform_adapted`

## 1. LST物理范围与ABI

权威范围为`swd3.exe.lst`的`0x0043B9A0..0x0043B9BD`：

```text
0043B9A0 mov eax, [esp+8]
0043B9A4 mov ecx, [eax]
0043B9A6 mov eax, [esp+0Ch]
0043B9AA mov [eax], ecx
0043B9AC mov ecx, [esp+4]
0043B9B0 test ecx, ecx
0043B9B2 jle 0043B9BD
0043B9B4 mov edx, [eax]
0043B9B6 dec ecx
0043B9B7 mov edx, [edx]
0043B9B9 mov [eax], edx
0043B9BB jnz 0043B9B4
0043B9BD retn
```

函数是caller-cleanup的三参数cdecl：

```text
arg_0 = signed i32 count
arg_4 = source head变量地址
arg_8 = output head变量地址
```

入口先无条件读取`*source_head`并写入`*output_head`。EAX随后保持output head变量地址，所有出口都以该地址作为返回值。函数无callee，也不直接访问固定全局；全部状态通过两个pointer-to-pointer参数读写。

## 2. 精确语义

等价的32位语义为：

```text
*output_head = *source_head
if signed(count) <= 0:
    return output_head
repeat count times:
    node = *output_head
    count = count - 1
    *output_head = node->next_at_offset_zero
return output_head
```

必须保留：

- source和output变量地址在入口即被无条件解引用。
- output旧值总是先被source当前值覆盖。
- count按signed `i32`解释；0及任意负数只复制，不读取节点。
- 正count每轮先读取output当前节点，再递减count，再读取节点偏移0的next并写回output。
- source与output变量地址相同时，后续推进直接改写该同一变量。
- distinct source变量和全部节点next保持不变。
- count大于可用链长时会解引用空指针；原函数没有null检查、长度检查或typed-stop。
- 有环链仍严格推进有限的positive count次，不做环检测。

## 3. 全部调用域

LST共有37个直接调用点，分布于35个caller；`DialogFunc`与`0x004437C0`各调用两次。

四个调用点的source、output和count均来自caller寄存器：

- `DialogFunc`两次。
- `0x0043BCC0`一次。
- `0x0043F940`一次。

其余33个调用点归并为以下参数owner族：

- source`0x004FCAE0`到output`0x004FCD10`：6次。
- source`0x004FCD70`到output`0x004FCD64`：6次。
- 动态source到output`0x004FCD58`：1次。
- source`0x004FCD70`到动态output：1次。
- source`0x004FCFC0`到output`0x004FCFBC`：6次。
- source`0x004FCFC0`到动态output：1次。
- source`0x004FD1E8`到output`0x004FD150`：4次。
- source`0x004FD1E8`到动态output：1次。
- source`0x004FD580`到output`0x004FD4C0`：5次。
- source`0x004FD580`到动态output：2次。

count通常来自caller寄存器。`0x004437C0`的首个调用显式传0，直接证明copy-only路径是实际调用合同。各caller业务、链owner生命周期和紧邻helper继续独立关闭；本单元不提前计入它们。

## 4. typed平台适配

现代实现复用`LegacyStandardModeForwardNode`表达偏移0的intrusive next前缀，并保留source/output pointer-to-pointer边界。节点和source通过const类型保持只读，output仍是唯一写owner；返回值保持output变量地址。

该边界只隔离32位裸地址与宿主指针宽度，不增加非法参数保护。调用者仍必须提供有效的source/output变量地址；positive count还必须保证每轮当前节点可解引用。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- count 0覆盖output旧值，只复制source并返回output地址。
- 负count按signed分支只复制，不遍历。
- count 1、2、3分别落到第二节点、第三节点和null。
- 每次调用都从source当前值重新开始。
- distinct source与全部next保持不变。
- source/output变量地址别名时，同一变量按count推进。

定向测试通过。workpack连续生成两轮均为`13/227`，SHA256均为`6b2f98c7c36ca283c5f58aa0a0b028275cb3febc7f918ddb5aa24acabe8ddcd3`；只新增关闭`0x0043B9A0`，`0x0043B9C0`仍为下一独立单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
