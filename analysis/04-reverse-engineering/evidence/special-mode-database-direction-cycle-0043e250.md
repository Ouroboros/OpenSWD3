# 标准模式数据库方向循环 `0x0043E250`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与边界

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043E250..0x0043E305`，97行；无direct code caller，由B480 callback地址绑定。直接callee仅为物品查询和sample。

实现使用typed database state与最小RetreatPorts边界；不因B480地址绑定提前关闭任何后续callback。

## 2. phase 1

对direction执行32-bit加1，signed比较大于1时写0；随后无条件sample `0x107`，sample返回为EAX。UT锁定direction1→0、sample ID、interface owner及helper count1。

## 3. phase 2

严格顺序为：

1. toggle执行32-bit加1，signed值大于1时写0。
2. 查询物品`0x1BA9`；存在则写toggle1。
3. 读取runtime flags低字节；bit0清则写toggle1。
4. 同一已读取AL上检查bit1；bit1清则写toggle0，因此该写入可覆盖前一步。
5. 无条件sample `0x107`并返回sample EAX。

UT覆盖缺物品且仅bit1置位后写1、两bit均置位保留环绕0、物品存在且两gate关闭后写1，并锁定query→sample顺序与helper count2。

## 4. 其他phase

phase3写countdown `0xC8`并返回EAX0；其他phase保留三次DEC链EAX，不调用边界。

## 5. 验证

定向测试通过。workpack双生成稳定为`53/227`，SHA256均为`e99f41464c78cb7d81fb9189765f2b210f5237edf311fa6601eb7cf1c9ba6541`；下一单元为`0x0043E310`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
