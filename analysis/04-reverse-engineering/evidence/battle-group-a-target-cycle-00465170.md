# 战斗组A目标轮转 `0x00465170`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00465170..0x004651CF`，从proc到endp共45行、25条实际指令、0个静态call、2个跳转、2个局部标签、1个`retn`，没有外部`FUNCTION CHUNK`。

两个静态caller都位于已关闭目标选择状态刷新，并在同一属性回退共享路径中收敛。函数无callee、随机、文件、对象或平台依赖。

## 2. 边界与目标值

入口先读取live group-A count到EDX，再把目标效果dword高word与启动补充人数word分别零扩展并依次相减：

```text
bound = group_a_count - target_effect_high_word - supplemental_count_word
```

全部减法按u32回绕，后续只在游标比较时按i32 signed解释。随后读取live queued actor code并按u32减8形成零基匹配值；不提前过滤code 0..7或大于11。

## 3. 无界轮转与物理表

EAX从共享target cursor加载。每轮先按u32加1，再以signed `EAX > bound`决定是否把局部EAX回绕1；循环期间不回写共享cursor。随后从`0x004A796C + EAX*4`读取dword，并与`queued-8`完整比较；不匹配则无上限地继续原循环。

该读取不是新数组。物理基址位于第127项连续八dword候选表`10/9/8/11/2/1/0/3`的第4项，因此正常逻辑索引1..4读取`2/1/0/3`；u32负索引`-3/-2/-1/0`仍可回读同一已知表的`10/9/8/11`。已知八dword owner之外只在首次真实读取typed-stop，不增加现代迭代上限。

## 4. 成功发布与返回

匹配后只对ECX加1，再按原顺序：

1. 把匹配时的局部EAX写回target cursor；
2. 把`candidate+1`写入共享published actor；
3. 把当前group-B目标清0；
4. 打开selection input gate。

正常返回EAX为匹配游标、ECX为一基发布值、EDX为入口三项值低32位相减结果。typed-stop返回访问前EAX和bound EDX；ECX在首轮读取即停止时仍为补充人数零扩展值，在已有匹配失败轮次后则保留最后一次物理候选。四项成功后缀全部未写。

## 5. caller回收与验证

目标选择刷新删除原`prepare_alternate_target` opaque调用，旧枚举数值改为`reserved_group_a_target_cycle_slot`且生产代码零调用。属性回退在unsigned remaining至少4时直接组合本typed实现；普通返回寄存器继续进入原调用点后缀，typed-stop保留此前属性查询与角色记录写前缀，并阻断message、输入记录和动画尾部。

定向测试覆盖首项匹配、多项扫描、signed上界回绕、三项边界减法、负逻辑索引回读前置共享候选、首个一过尾读取typed-stop、正常caller尾部和caller stop传播。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。

函数自身为确定性无callee叶函数，完整LST、共享常量表和固定状态测试覆盖全部可终止路径，不依赖动态oracle。
