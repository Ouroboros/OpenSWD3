# 标准模式derived text `0x0043D370`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与边界

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043D370..0x0043D46F`，152行，六个caller全部来自已关闭`0x0043D050`；唯一callee为RNG helper `0x00439070`。

原六参数ABI为destination、4-byte CP950 label、signed status、threshold、value和maximum。typed helper保留完整格式化控制流；port只隔离唯一RNG owner。D050删除整函数`format_derived_text`高层占位，六处直接调用本helper并传播destination typed-stop。

## 2. 初始写入与delta

入口先计算scale候选：默认10；value严格大于100时为100；value严格大于1000时为1000。随后无条件执行`"%4s  "`，即label最小宽度4且尾随两个空格。该写入发生在任何RNG调用之前。

`delta = threshold - status`按u32回绕后解释为signed i32，保留x86 `sub`。四类路径如下。

## 3. delta小于等于0

直接以原字节模板格式化：

```text
%s <AC 4F> %4d
```

label不做宽度填充；value按signed decimal最小宽度4。返回`wsprintfA`写入字节数，return kind为`formatter_result`，不调用RNG。

UT以status5、threshold5、value -12锁定exact 12-byte输出与EAX12。

## 4. delta等于1或2

先选择scale，再计算：

```text
upper = scale / (3 - delta)
random = rng(upper)
published = random - upper / 2 + value
published = max(published, 0)
published = min(published, maximum)
```

加减按u32回绕后解释为signed；上下界比较为signed。

- delta1使用字节模板`%4s <A4 6A B7 A7 AC 4F> %4d`。
- delta2使用字节模板`%4s <A6 FC A5 47 AC 4F> %4d`。

返回最终formatter字节数。UT锁定：value100使用scale10、upper5、random4得到102；value101使用scale100、upper100、random0得到51后被maximum50钳制。

## 5. delta大于等于3

在初始`%4s  `后追加ASCII`" ???"`，最终为4-byte label、三个空格、三个问号。maximum严格大于1000时原程序再写destination第7字节为`'?'`；即使通常已为问号，此写入仍保留。

返回`lstrcatA` destination owner指针，而非formatter长度。typed返回联合避免64位宿主截断；不调用RNG。UT锁定exact 10-byte输出、pointer kind及EAX不伪造。

## 6. typed-stop与验证

原owner容量为D050锁定的32字节。初始或最终格式化若超出destination，在原写入点返回`destination_out_of_range`；停止前已发布的初始`%4s  `保持。D050收到失败后传播`derived_text_stopped`，不执行后续D370请求或related loads。

定向UT覆盖delta 0/1/2/4、10/100/1000三档scale、RNG上界、居中偏移、零/maximum钳制、三种CP950模板、formatter EAX和destination pointer联合。D050真实status19路径也验证六个输出slot和零RNG调用。

`special_modes.legacy_initial_menu`定向测试通过。workpack连续两轮稳定为`42/227`，SHA256均为`f481edf973f9909147d6eda7a6f6e7d51369a69183f1ae1e46c0f3675ac3e538`；下一独立单元为`0x0043D470`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
