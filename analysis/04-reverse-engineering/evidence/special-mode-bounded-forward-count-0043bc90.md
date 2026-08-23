# 标准模式有界单链计数helper `0x0043BC90`

状态：`platform_adapted`、`unit_tested`

## 1. LST物理范围与调用图

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043BC90..0x0043BCB7`，下一入口为`0x0043BCC0`；本函数无callee。

LST有22个直接caller，各调用一次：

```text
0x0043BCC0  0x0043D530  0x0043DD20  0x0043DDF0
0x0043DED0  0x0043DFA0  0x0043E3D0  0x0043F000
0x0043F880  0x00440B20  0x00440C20  0x00440D20
0x00440E10  0x00441060  0x00441160  0x00443450
0x00443570  0x00443A60  0x00445C90  0x00445E90
0x00446090  0x00446260
```

三参数均被读取：offset0单链head值、输出count指针和signed limit。常见固定limit为`10/13/16/24`，`0x0043BCC0`还传入动态limit。各caller的具体owner与后续链索引继续独立关闭，本helper不提前计入。

## 2. 入口清零与停止条件

函数先把head值装入EAX，再无条件把输出count清零。即使head为空，count也会先写0：

```text
node = head
output_count = 0
if node == null:
    return null
```

非空链循环在每次解引用前先比较count与limit：

```text
while node != null:
    if output_count >= limit:
        return node
    output_count += 1
    node = node->next
return null
```

`cmp ecx, esi`与`jge`按signed i32。因为count从0开始，limit为0或负数时立即返回head且不读取`head->next`。正limit下每次只跟随节点offset0的next；达到limit先返回当前节点，链先结束则返回null。

## 3. 返回、循环链与边界

返回值是停止位置：

- limit先达到：返回前进`limit`步后的当前节点。
- null先达到或恰好同时达到：返回null。
- signed非正limit：返回原head。

正limit也为循环链提供有限步停止；函数不做环检测。`INT_MAX` limit配短链时仍由null提前终止，不发生count溢出。

`0x0043F06B`callsite在聚合栈清理后直接返回，因此传播本helper返回节点。`0x0043BCC0`及其余caller主要消费输出count或在使用前覆盖EAX。modern仍完整返回typed节点指针。

## 4. modern typed边界

`count_legacy_standard_mode_forward_nodes_bounded`复用`LegacyStandardModeForwardNode`的offset0 next合同，使用typed head、i32 count引用和i32 limit：

- count入口值无条件覆盖为0。
- 不新增null之后的读取。
- 不把signed limit改为size_t。
- 不新增环检测、步数修正或饱和。
- 返回`const LegacyStandardModeForwardNode*`保持只读链语义。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- 空链先清count再返回null。
- limit为负数与0时返回head、count为0且不遍历。
- limit为1和2时返回对应当前节点并发布精确count。
- limit为3与三节点链尾恰好相遇时返回null、count为3。
- limit大于链长时由null提前停止。
- 两节点循环链在limit 5时有限步返回并保持链不变。
- `INT_MAX` limit配短链由null停止且count不溢出。
- 入口count非零时仍被无条件重置。

定向测试通过。workpack连续生成两轮均为`22/227`，SHA256均为`30d3b81078ac67ee36c669effd889f5c3ce79cd1ef25c630a9731273298f09f2`；只新增关闭`0x0043BC90`，`0x0043BCC0`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
