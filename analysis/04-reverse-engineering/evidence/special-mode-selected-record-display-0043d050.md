# 标准模式selected record显示 `0x0043D050`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与边界

唯一行为真值为`swd3.exe.lst`。函数物理范围为`0x0043D050..0x0043D36B`，365行，唯一caller是已关闭`0x0043CEF0`。直接callee为六次下一单元`0x0043D370`、三次record loader、一次临时`0xB0`分配及四次release。

C0D0表明`dword_4FC934..dword_4FC960`是12个连续文本owner，每个精确分配`0x20`字节。typed state据此新增12×32字节`display_text_slots`。平台port只隔离category资源解析、D370所需RNG、已有record loader/release和临时storage release；D050全部控制流、固定字节、格式化输入、去重及返回联合均在helper内。D370现已独立关闭并由六处直接调用。

## 2. 固定文本与status门

入口依次写三个首栏默认值：

- slot0：ASCII `????`。
- slot1：字节`3F 3F 3F 3F AF C5`。
- slot2：ASCII `????`。

原程序随后六次覆盖同一`CmdLine` owner，中间无读取；typed state发布最终可观察值`B1 D3 B1 B6 20 20 3F 3F 3F 3F`。

absolute index读取64项status表并按signed i8解释：

- status≥1：以对应entry值解析category名称，替换slot0。
- status≥2：scratch `+0x60` u16按`%4d`最小宽度写slot1。
- status≥3：scratch `+0x0C` NUL字符串按`%-12s`右侧空格填充写slot2；长度超过12不截断。

absolute index越界、selected name无NUL或超过原32字节owner容量均在原读取/写入点typed-stop。

## 3. 六次D370请求

无论signed status为何，均按原顺序调用下一单元D370六次：

```text
slot  label bytes       threshold  value                         maximum
3     A5 CD A9 52       5          signed i16 scratch+0x70       0x270F
4     C6 46 A4 4F       8          second_record_offset          0x270F
5     C5 E9 A4 4F       10         first_record_offset           0x270F
6     A7 F0 C0 BB       13         u16 scratch+0x62              0x03E7
7     A8 BE BF 6D       16         u16 scratch+0x64              0x03E7
8     B1 D3 B1 B6       18         u16 scratch+0x66              0x03E7
```

请求精确携带label span、signed status、threshold、value和maximum；现直接调用已关闭D370，失败时传播`derived_text_stopped`并保留此前输出。

## 4. related名称加载与去重

slots9..11先各写12个问号。signed status小于19时，函数立即返回最后一次`lstrcpy`的slot11 owner指针；typed返回联合避免64位宿主截断。

status≥19时使用一个`0xB0`临时record，按scratch `+0x72/+0x76/+0x7A`加载最多三条名称：

1. 第一条成功即替换slot9。
2. 第二条只有与slot9不同才替换slot10。
3. 第三条只有同时与slot9和slot10不同才替换slot11。

每轮不论ID为0或load失败都读取临时record `+0xAC`并release token，下一轮前清零整个临时record。成功load后的名称无NUL或超过32字节owner容量时在原字符串读取/写入点typed-stop。

第三次loader保留原BUG：EAX先装scratch owner指针，再只覆盖AX为`+0x7A` ID，因此实参是`scratch legacy address high16 | ID`。typed state以可配置`scratch_record_legacy_address_high_word`隔离该32位地址依赖；组合时强制只保留高16位。UT锁定`0xABCD0000 | 0x3333 = 0xABCD3333`。

三轮后release临时storage，并返回该release EAX；返回kind改为`temporary_release_result`。

## 5. CEF0回接与验证

CEF0删除synthetic `dispatch_selected_record` port，直接调用本helper并传播status、返回kind、文本owner指针或release EAX。零entry仍在CEF0内早退，不进入D050。

定向UT覆盖：

- status19的category、base42、`Hero`右填充12字节。
- 六次D370的exact threshold/value/maximum及signed `0xFFFE -> -2`。
- related IDs `0x1111/0x2222/0xABCD3333`、三次token release及最终临时release EAX `-777`。
- 第一/第二同名时slot10保留12问号，第三唯一名称进入slot11。
- 第三loader高16位BUG、联合返回和CEF0绝对索引回绕越界typed-stop。

`special_modes.legacy_initial_menu`定向测试通过。workpack连续两轮稳定为`41/227`，SHA256均为`13f61f90e5d60664520c11df18d26ad5d47a972710b477d3b7f8318d6de6e92f`；下一独立单元为`0x0043D370`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
