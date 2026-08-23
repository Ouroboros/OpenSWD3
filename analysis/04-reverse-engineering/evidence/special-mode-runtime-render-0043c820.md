# 标准模式运行时列表渲染 `0x0043C820`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与owner

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043C820..0x0043C9B5`，无外置chunk，直接运行时caller仅`0x00447100`的`0x004471B8`。下一workpack入口为`0x0043C9C0`。

函数使用共享17项action表：`0x0043C0D0`写record0，已关闭split bar `0x0043AE40`使用records6–9。typed runtime state由此前不足的单record修正为17项数组；selected preview另用`FC650`对应的独立typed action record。

平台render port只隔离颜色组合、已关闭split-bar owner、已关闭D470所需viewport/resource load/draw、preview动作、已关闭CC20所需的raw/formatted rendering资源和已关闭矩形效果owner。列表与D470/CC20控制流、所有owner写入及原表读取均在typed helper内。

## 2. 双nibble衰减与bar比例

入口固定请求颜色`(0x19,0x17,0x11)`，并把完整u32返回值传给已关闭CC20；此前窄port截成低16位的synthetic行为已纠正。

只有signed `total_count > 15`时执行衰减和bar：

- mode flags低nibble非零则减1，overlay bit0置1。
- 更新后的高nibble非零则减`0x10`，overlay bit1置1。
- 高24位和低字节其他nibble均保持。

例：`0x0000A5B6 → 0x0000A5A5`，overlay=`3`。

比例按LST的signed dword→x87 double除法→float存储表达：

```text
first_ratio  = float(double(window_offset) / double(total_count))
second_ratio = float(double(wrapping_i32(window_offset + visible_count)) /
                     double(total_count))
```

随后以`x=0xCE,y=0x62,height=0x15E`调用split bar，并把四个输出直接发布到runtime state。total不大于15时flags、动态边界和action records均不由此路径改写。

## 3. alias链与selected路径

D470 mode strip完成后，从typed `entry_alias_index`对应的原64项entry base读首项。首项为0时直接返回D470 viewport恢复EAX。

非零链最多受Y门限制：row0从Y=`0x5E`开始，每项加`0x18`，进入一轮前signed要求`Y < 0x1C6`。每轮：

1. 若row等于local cursor，按u32回绕计算`window_offset + row`。
2. selected绝对索引先读取short text槽首字节；非零时把对应entry写入preview action ID，只写base variant=`0x44`和variant delta=0，调用service `0x1FC`/selector `0x3C`。
3. 直接调用已关闭CC20 typed helper：`absolute index,row,完整color,selected`；caller压入但callee未读的固定0不进入typed API。
4. selected行调用矩形效果：`(0x0E,Y,0xBD,0x18,0x14,0x0D,0,5)`。
5. 无条件读取alias链下一项，再递增row/Y；下一项0时返回0。

因此即使下一轮Y将达到`0x1C6`，上一轮末尾仍会先读取next alias并把它作为返回EAX。

## 4. typed-stop

- split-bar owner报告typed-stop时，不进入mode-strip/list路径。
- D470 resource load停止时传播`mode_strip_stopped`，不读alias且不伪造viewport恢复。
- entry alias index负值或大于63在对应首次/next alias读取点停止。
- selected绝对索引负值或大于63在short text和entry读取点停止。
- CC20在entry index或即将扫描的short/long text无NUL时传播`entry_render_stopped`；不执行后续selection frame、row计数和next alias读取。

停止前已经发生的颜色、bar、mode-strip资源绘制和前序row绘制保持；不会伪造后续preview/entry/frame。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- 双nibble衰减、overlay3、精确bar几何与`5/30`、`15/30`float比例。
- split-bar四输出发布及共享action record6可见变更。
- alias2链两行：absolute5普通、absolute6 selected。
- preview action ID/base variant/variant delta、cached字段保持及`0x1FC/0x3C`。
- CC20完整color、名称/百分比/selected详情请求、selected frame Y=`0x76`、事件顺序和链终止EAX0。
- total15跳过bar并返回D470 viewport恢复EAX，flags/动态边界保持。
- split bar停止、首次alias越界、selected索引越界、alias63的post-row next读取越界。
- `0x0043C3C0`动态命中改读runtime bar outputs，不再由调用参数伪造。
- `0x0043C0D0`与cleanup只写共享action record0，其他record保持。

定向测试通过。workpack连续生成两轮均为`35/227`，SHA256均为`4856353c4390b409498d64a75f8fb47c9137c4ffe1f5753ff8eb10bd39066fe6`；只新增关闭`0x0043C820`，`0x0043C9C0`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
