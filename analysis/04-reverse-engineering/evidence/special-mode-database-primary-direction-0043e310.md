# 标准模式数据库主方向循环 `0x0043E310`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与边界

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043E310..0x0043E3C2`，98行；DA30有两个direct call点，B480另以callback地址绑定。直接callee仅为物品查询和sample。

E310与E250共享同一typed内部实现，仅把phase1 sample ID参数化，避免两份phase2/3逻辑漂移。DA30命中E310时直接调用typed helper，不再走通用地址port。

## 2. phase 1

对direction执行32-bit加1，signed比较大于1时写0；随后无条件sample `0x2E`并返回sample EAX。此处与E250唯一差异是E250使用sample `0x107`。

UT锁定direction1→0、sample2E、interface owner与helper count1。DA30两个矩形分别先写direction1/0再调用E310，因此最终分别为0/1；UT锁定两次直接调用、sample2E两次及通用target事件为空。

## 3. phase 2

严格复用E250控制流：toggle加1、signed值大于1回0；查询物品`0x1BA9`并条件写1；读取runtime低字节，bit0清写1，随后同一AL的bit1清写0；最后sample `0x107`。UT通过E310公开入口锁定缺物品、仅bit1置位时的query→sample顺序、toggle1与sample107。

## 4. 其他phase

phase3写countdown `0xC8`并返回EAX0；其他phase保留三次DEC链EAX，不调用边界。

## 5. 验证

定向测试通过。workpack双生成稳定为`54/227`，SHA256均为`29bf22b0a4bc9e09ac71098a6dc0c5fc8e9ccb76f18dc28a0e6a8f7d23323e69`；下一单元为`0x0043E3D0`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
