# 标准模式运行时表初始化 `0x0043C0D0`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与生命周期

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043C0D0..0x0043C2DC`，workpack下一入口为`0x0043C3C0`。LST直接caller为`0x00446700`。

原函数依次分配：`0xB0` scratch记录、两个`0x200`字节状态表、`0x40`字节长字符串指针表及16个`0x20`字符串、64个`0x10`字符串、一个`0x100`字节/64项entry表。modern以固定typed数组表达这些尺寸；字符串槽严格只写首字节0，不伪造原malloc未初始化的其余字节。

## 2. 两轮1..500扫描

第一轮ID从1到500，每轮先清整个`0xB0` scratch，再把`scratch + 0x0C`与u16 ID传给记录加载port。加载成功时：

- `loaded_status[id] = scratch[0x5E]`。
- 读取`scratch[0xAC]` u32 token并调用release port。
- 把`scratch[0xAC..0xAF]`清0。

第一张`0x200`表入口先全部填`FF`，索引0保持`FF`。

第二轮同样扫描1..500，把query port返回u8逐项写入第二张`0x200`表；该表入口先全部清0，索引0保持0。

## 3. 字符串槽、entry与action

随后按顺序：

- 为16个长槽和64个短槽只写首字节NUL。
- 把`FC974` total、`FC90C` window offset、`FC928` local cursor、`FC914` visible count与`FC910` mode index五个owner清0。
- 以值0调用`0x0043C9C0`窄port初始化64项entry表。
- 只写action ID=`0x232A`与base variant=`0x33`，其他action字段保持。
- 把entry[0]传给`0x0043CEF0`消费port，保留其EAX为函数返回。
- 最后把mode flags清0；该mov不改变返回EAX。

未关闭的记录加载、状态查询、entry初始化与消费callee均由窄port隔离，不提前计入。

## 4. 验证

`special_modes.legacy_initial_menu`使用500次load与500次query的synthetic port覆盖：

- 两轮ID精确为1..500且顺序不交错。
- scratch每轮进入load前完整清零，destination尺寸为`0xA4`。
- 仅ID 1与500成功时，`+0x5E`状态和两个u32 token精确发布/释放。
- token字段在成功后清0。
- 两张512字节表的`FF/00`初始化与边界索引500。
- 16×32与64×16槽只清首字节，其余预存字节保持。
- 五个LST owner清0、entry port接收64项和值0。
- action只改ID/base variant，cached字段保持。
- entry[0]消费顺序、mode flags最终清0和consumer EAX返回。

定向测试通过。workpack连续生成两轮均为`29/227`，SHA256均为`4d43482df73105a50a831f9da35a35f89fab5916af1d7ffae22e5e3f9ad3f940`；只新增关闭`0x0043C0D0`，`0x0043C3C0`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
