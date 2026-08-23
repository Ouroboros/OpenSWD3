# 标准模式运行时输入分派 `0x0043C3C0`

状态：`platform_adapted`、`unit_tested`

## 1. LST完整范围

唯一行为真值为`swd3.exe.lst`。不能只读取主块`0x0043C3C0..0x0043C519`；函数还拥有四个外置chunk：

- `0x0043C2F0..0x0043C3BB`：完整释放路径。
- `0x0043C600..0x0043C66F`：15项翻页、重建、消费与点击音。
- `0x0043C7E0..0x0043C7F5`：mode刷新与点击音。
- `0x0043C800..0x0043C819`：exit counter精确500门。

直接caller是`0x004455E0`。`0x0043B480`只保存函数地址，不是运行时direct call。已关闭的`0x0043BBE0`、`0x0043C090`、`0x0043C520`、`0x0043C590`与`0x0043C670`直接复用typed helper；其余未关闭callee继续由窄port隔离。

## 2. 第一与第二矩形

入口把pointer Y装入EAX、pointer X装入ECX、输入位低字节装入DL。

第一矩形使用全部unsigned严格边界：`94 < y < 454`、`18 < x < 203`且`input_bits & 3`。行号为`(y - 94) / 24`；若signed行号不小于visible count，则先改为`visible_count - 1`，随后无条件再减1并写local cursor，最后tail-call `0x0043C520`。这意味着进入callee前的钳制值是`visible_count - 2`，不能“修正”为末项；已关闭callee随后再推进cursor、重建alias、刷新、消费entry、写flags并返回sample EAX。

第二矩形为`60 < y < 78`、`10 < x < 206`且`input_bits & 3`。signed delta为`(x - 106) / 20`，向零截断。LST的位运算精确形成：

```text
if delta < 0 and mode == 0: return mode
if delta > 0 and mode == 14: return mode
if delta == 0: return mode
mode = mode + (delta <= 0 ? -1 : 1) - 1
refresh_mode()
return play_sample(0x2E, sample_handle)
```

所以负delta把mode减2，而正delta保持mode不变但仍刷新和播放点击音。这是原始可观察BUG，modern原样保留。

## 3. availability与顺序命中

若前两矩形未命中，函数查询16字节记录15。可用时先要求unsigned `206 < x < 224`；进入后Y的后续范围全部为signed严格比较，并按顺序独立执行，不是互斥else-if：

1. `82 < y < 96`调用已关闭`0x0043C590`，保留后退、alias重建、刷新、entry消费、flags OR `0x03`和sample副作用后重新载入pointer Y。
2. `452 < y < 464`调用已关闭`0x0043C520`，保留其全部状态副作用后重新载入pointer Y覆盖sample EAX。
3. `first_lower < y < first_upper`调用已关闭`0x0043C670`，保留page retreat、alias重建、刷新、entry消费、flags OR `0x03`和sample副作用后重新载入pointer Y。
4. `second_lower < y < second_upper`进入翻页chunk。

前三个callee返回后LST都重新把pointer Y装入EAX，因此最终未翻页路径返回Y。重叠动态范围可在同一帧依次执行upper的完整`0x0043C590`链、first dynamic的完整`0x0043C670`链，再执行翻页；每段都必须消费前一段产生的实时cursor、offset与flags。

翻页chunk以step 15调用`0x0043BBE0`，随后严格执行entry alias重建、page刷新、从原entry base读取`entries[window_offset + local_cursor]`、消费entry、对mode flags低字节OR `0x30`、播放sample `0x2E`。typed实现以u32回绕形成selected index，并仅在原entry读取点隔离负值或越界值。`0x0043C0D0`同时补齐LST的`FC920 = FC91C`，typed表达为`entry_alias_index = 0`。

availability不可用时EAX为0；availability可用但X严格边界失败时EAX保留X。availability表不足16项在原记录读取点typed-stop。

## 4. exit=500释放chunk

只有未进入可用记录X区间时，`input_bits & 0x0C`才会进入exit检查；`word_4FC900`必须精确等于500。命中后先写2，再：

1. scratch `+0xAC` token非零才release并清0。
2. 依次release scratch、loaded-status表、queried-status表、long-slot pointer table。
3. 把`FC974`清0并释放16个long slot；循环后`FC974 == 16`。
4. 再把`FC974`清0并释放64个short slot；循环后`FC974 == 64`。
5. release entry表，并保留该release的EAX。
6. 只写action ID=`0x232A`、base variant=`0x43`；其他action字段保持。

因此有record token时是1次record release加85次storage release；无token时仍有85次storage release。指针全局在原函数中不清0，typed实现也不伪造额外清理。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- 第一矩形行计算、signed钳制、额外减1以及tail-dispatch `0x0043C520`后的最终cursor/entry/flags/sample EAX。
- 第二矩形负delta减2、正delta不变、delta零、mode 0/14边界。
- upper `0x0043C590`、first dynamic `0x0043C670`、翻页的三轮alias重建/page刷新/entry消费/点击音顺序，以及entry13/0/14、实时cursor与flags `0x33`。
- bottom `0x0043C520`的cursor/entry/flags/sample副作用保持，但sample EAX被pointer Y覆盖。
- X等于206的严格边界与路径EAX。
- availability记录15越界typed-stop。
- selected entry越界只在原表读取点停止，且保留此前upper/rebuild/refresh副作用。
- record token、85项storage释放的种类/索引顺序、exit counter 500→2、`FC974`最终64、action字段和末次release EAX。
- exit counter 499不释放。
- `0x0043C0D0` entry alias写0且exit counter不被该初始化器改写。

定向测试通过。workpack连续生成两轮均为`30/227`，SHA256均为`f9dc7e93f48dba950d034372bfb2830b0cef40e93357d0c9afebc4e353c3e137`；只新增关闭`0x0043C3C0`，`0x0043C520`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
