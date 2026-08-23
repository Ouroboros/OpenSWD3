# 标准模式entry绘制 `0x0043CC20`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与caller

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043CC20..0x0043CEE2`，324行，唯一caller是`0x0043C820`的`0x0043C978`。callee为14次原始字节文字绘制`0x00436AD0`和一次已关闭格式化文字`0x004306C0`。

五个stack位置中实际只读取absolute entry index、row index、color与selected；caller压入的固定0完全未读，typed API不伪造该参数。

平台port只隔离已关闭rendering owner所需的字形/帧缓冲资源。名称、百分比、详情文本选择、坐标、格式化、条件和EAX控制流均在本helper内。

## 2. 名称与百分比

入口先把共享临时文本设为ASCII `????????`。随后按absolute index读取64项short text指针：

- 首字节0：名称保持8个问号。
- 非空：按Windows `wsprintfA("%-12s")`形成至少12字节的右侧空格填充；原16-byte槽最长15字节，超过12不截断。

名称无条件以name owner绘制：`x=0x12`、`y=wrapping_i32(row*24+0x5E)`、caller完整u32 color、style4。此前C820窄port把color截成u16，现已纠正为完整dword透传。

随后临时文本设为`"  0%"`。只有名称非空才读取entry status为signed i8，按signed dword乘5，并以`"%3d%%"`格式化；百分比绘制为`x=0xA8`、`y=wrapping_i32(row*24+0x67)`、相同color/style4。空名称不绘制百分比。

## 3. selected详情

函数在名称/百分比后重新载入selected参数；只有它精确等于1才进入详情，其他值原样作为EAX返回。

selected=1时先无条件绘制long text slots 0..8：

```text
x = 0xF6
y = 0x48,0x5C,0x70,0x84,0x98,0xAC,0xC0,0xD4,0xE8
owner = detail, color = caller color, style = 4
```

九次绘制后重新取得当前short text指针。名称为空则直接返回该指针；不会执行后三个条件详情或格式化文字。

名称非空时检查long slots 9..11首字节：首字节等于`'?'`跳过，否则即使为空字符串也调用detail owner。三组坐标为：

- `(0xF6,0x126)`。
- `(0x174,0x126)`。
- `(0xF228,0x126)`。

第三个X=`0xF228`看似异常但为LST原立即数，按bug-for-bug保留，不改成`0x228`。

最后读取scratch `+0xAC` u32 token，调用已关闭格式化文字owner，精确参数为token、`x=0xF2`、`y=0x150`、maximum line count5、maximum width`0x168`、style4，并返回其EAX。

## 4. typed-stop与返回联合

- absolute index负值或大于63，在原short pointer读取点停止。
- 16-byte short槽或即将绘制的32-byte long槽没有NUL时，在原字符串扫描点停止。
- optional槽首字节为`'?'`时不要求NUL，因为原程序不会调用renderer。

返回结果区分：非selected参数值、selected空名称的short text指针、最终formatted text EAX，避免在64位宿主截断指针。停止前已经完成的raw draw保持。

## 5. C820回接与验证

`0x0043C820`已删除高层`draw_entry`占位，逐row直接调用本helper；CC20停止时保留此前preview等副作用，不绘制selection frame、不计完成row、不读取next alias。

`special_modes.legacy_initial_menu`覆盖：

- 空名称问号与row负值的u32回绕Y。
- `Hero`右填充12字节、status `0xFE`按signed得到`-10%`。
- caller完整color `0xAABBCCDD`和style4。
- 9个固定详情、slot10问号跳过、slot11空字符串仍绘制、`0xF228`坐标。
- scratch token `0x11223344`及最终格式化请求/EAX。
- selected非1返回参数；selected空名称先画9项再返回short指针。
- entry越界、short无NUL及C820真实caller顺序。

定向测试通过。workpack连续生成两轮均为`39/227`，SHA256均为`8028d9e05900777cf5f8f89860330050bd76523189757ad63340cb66edb5eca1`；只新增关闭`0x0043CC20`，`0x0043CEF0`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
