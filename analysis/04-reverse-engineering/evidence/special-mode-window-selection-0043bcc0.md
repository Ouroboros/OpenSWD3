# 标准模式窗口链选择与文本解析 `0x0043BCC0`

状态：`platform_adapted`、`unit_tested`

## 1. LST物理范围与调用图

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043BCC0..0x0043BD66`，下一入口为`0x0043BD70`。

函数有八个直接调用点、五个caller：`0x0043E080`、`0x0043E170`、`0x0043E3D0`、`0x00446700`四处和`0x00448650`。七参数归并为两组owner：

- 标准组：total=`4FCAD8`，offset=`4FCAD0`，cursor=`4FCBA4`，visible=`4FCB98`，limit=`16`，source head=`4FCAE0`，output head=`4FCD10`。
- 后段组：total=`4FD08C`，offset=`4FD164`，cursor=`4FD098`，visible=`4FD298`，limit=`13`，source head=`4FD1E8`，output head=`4FD150`。

直接callee为已关闭的`0x0043B980`、`0x0043B9A0`、`0x0043B9C0`、`0x0043B9E0`、`0x0043BC90`，以及仍未关闭的`0x0044D2D0`。后者只在空链路径以精确参数`source_head, 0xFFDC, 1, 0`调用；modern通过窄port保留请求，不提前关闭或计入该callee。

## 2. 全链计数与空链请求

入口先从source head变量读取链头并按offset0 next统计完整链长，把u32结果原位发布到total owner。链长为0时：

```text
total_count = 0
insert_missing_node(source_head, 0xFFDC, 1, 0)
```

调用后不重新计数，因此即使port把fallback节点发布到source head，total仍保持0。后续步骤读取可能已变化的source head变量。

## 3. offset与cursor归一化

函数按32位回绕计算`visible + offset`，再做signed i32比较。仅当`total <= visible + offset`时归一：

- 若`total > offset`，再计算回绕的`cursor + offset + 1`；当`total <`该值时，cursor写为回绕的`total - offset - 1`。
- 若`total <= offset`，offset先写为回绕的`total - 1`；signed负数时再写0，随后cursor无条件写0。

若`total > visible + offset`，offset与cursor均保持。比较、加减和写入顺序完全沿用LST，不把owner改成无符号或size_t。

## 4. window head、visible与选中节点

归一后按signed offset从source head复制并推进output head。source/output变量允许别名；别名时推进会改写后续选择所读取的source变量。随后从output head调用`0x0043BC90`，把最多limit个节点的计数覆盖写入visible owner。

选中索引为回绕的`offset + cursor`，但索引始终从当时的source head变量开始，而不是output head。signed非正索引返回该head，正索引只跟随offset0 next。

原`0x0043B9A0`在短链window推进中会解引用null，原`0x0043B9C0`或最终`0x0043B9E0`也会在选中null时解引用。modern分别在完全相同的危险读取点返回`window_head_unavailable`或`selected_node_unavailable` typed-stop，不补造节点、不改offset/cursor，也不把原崩溃伪装成成功。

## 5. 文本解析与返回

选中节点使用typed `text_index`表达原记录`+4` u16，再调用已关闭共享文本解析器。目录、terminator或buffer边界失败通过`text_resolution_failed`及内嵌解析结果传播。

原函数直接返回`0x0043B9E0`的路径相关EAX。`0x0043E3D0`以及`0x00446700`的四个callsite在清理栈后直接返回，传播该值；其余三处在消费前覆盖EAX。modern组合结果内嵌完整`LegacyStandardModeTextResolutionResult`，分别保留FFDC formatter返回与普通marker offset，不把两种原值伪造成单一宿主指针。

## 6. 验证

`special_modes.legacy_initial_menu`覆盖九类组合路径：

- 三节点链的首窗口计数、visible限制、首项选择和FFDC文本。
- cursor超出末项时按`total - offset - 1`修正并从原head选择。
- offset不小于total时退到最后节点并把visible改为1。
- 空链精确发布`FFDC/1/0`请求，port插入fallback后不重计total但可继续解析。
- 空链port不发布节点时在最终null解引用点typed-stop。
- 普通text index配空MAPS时传播文本解析typed-stop且不伪造buffer。
- source/output head变量别名时，后续选择读取已推进的source变量。
- offset超过短链时在window head推进危险点typed-stop。
- window推进有效但`offset + cursor`超链时在selected node危险点typed-stop。

定向测试通过。workpack连续生成两轮均为`23/227`，SHA256均为`e7b5c8b17175a8d999425ee11631f10a0b8ae63b7975de72dc0e3a569fb8785e`；只新增关闭`0x0043BCC0`，未关闭`0x0044D2D0`仍由port隔离，`0x0043BD70`为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
