# 标准模式单链索引helper `0x0043B9C0`

状态：`platform_adapted`

## 1. LST物理范围与ABI

权威范围为`swd3.exe.lst`的`0x0043B9C0..0x0043B9D3`：

```text
0043B9C0 mov eax, [esp+8]
0043B9C4 mov ecx, [esp+4]
0043B9C8 test ecx, ecx
0043B9CA mov eax, [eax]
0043B9CC jle 0043B9D3
0043B9CE mov eax, [eax]
0043B9D0 dec ecx
0043B9D1 jnz 0043B9CE
0043B9D3 retn
```

函数是caller-cleanup的二参数cdecl：

```text
arg_0 = signed i32 count
arg_4 = head变量地址
return = 选中的节点地址
```

函数无callee，也不直接读写固定全局。head变量地址总会被解引用一次；之后只通过节点偏移0读取next。

## 2. 精确语义

等价的32位语义为：

```text
node = *head
if signed(count) <= 0:
    return node
repeat count times:
    node = node->next_at_offset_zero
return node
```

必须保留：

- head变量地址在count分支前被无条件解引用。
- count按signed `i32`解释；0及任意负数返回刚读取的head。
- 正count严格读取偏移0的next count次。
- head变量和所有节点均只读。
- 最后一次推进落到null可以正常返回null；若后续仍需推进，则原函数解引用null。
- 有环链按有限positive count继续推进，不检测环，也不提前停止。
- 不增加head变量地址、当前节点或链长检查。

## 3. 全部调用域

LST共有45个直接调用点，分布于37个caller。多调用caller为：

- `0x00443BD0`：3次。
- `0x00446700`：6次。
- `0x0044E4A0`：2次。

一个调用点的head变量地址来自EBP。其余44次按固定head owner归并：

- `0x004FCD70`：8次，count分别来自EAX、ECX或EDX。
- `0x004FCFC0`：14次，count分别来自EAX、ECX或EDX。
- `0x004FD1E8`：16次，count分别来自EAX、ECX或EDX。
- `0x004FD580`：6次，count分别来自EAX或ECX。

全部count均来自caller寄存器，LST未见本helper调用点传立即数。caller对返回节点的业务消费、固定链owner生命周期及相邻helper继续独立关闭；本单元不提前计入它们。

## 4. typed平台适配

现代实现复用`LegacyStandardModeForwardNode`表达偏移0的intrusive next前缀。接口接收typed head pointer-to-pointer，保留入口无条件读取head变量的边界，并以const节点指针返回选中节点。

该适配只隔离32位裸地址与宿主指针宽度。实现不加入非法参数保护；调用者仍必须提供有效head变量地址，positive count还必须保证除最后一步外的当前节点均可解引用。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- 空head加count 0返回null。
- 负count和count 0返回已读取head。
- count 1、2、3分别返回第二节点、第三节点和null。
- head变量及全部next保持不变。
- 两节点循环链按count 5返回第二节点，不做环检测或提前停止。

定向测试通过。workpack连续生成两轮均为`14/227`，SHA256均为`59848c73b0139fe2c720bdf6cce6d4ad5a8230d7e9a82d94581db525ade46783`；只新增关闭`0x0043B9C0`，`0x0043B9E0`仍为下一独立单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
