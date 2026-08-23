# 标准模式数据库分页推进 `0x0043DED0`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与caller

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043DED0..0x0043DF91`，94行；direct caller是已关闭DA30，B480另以callback地址绑定。直接callee为已关闭B9A0/BBE0/BC90、尚未关闭F880/F1E0及sample owner。

DA30命中`address_0043DED0`时直接调用本helper，DED0不再出现在通用地址port事件中。它与DD20共享F880/F1E0/sample最小边界，但分页状态转移由已关闭BBE0直接执行。

## 2. phase 1

`FCD20==1`时按顺序：

1. BBE0以完整forward count、window offset、local selection、visible count和固定step16推进页。
2. B9A0按新window offset从共享head重建current forward head。
3. BC90从current head最多计16项并重写visible count。
4. F880。
5. first/second inline records进入F1E0。
6. display flags低字节`OR 0x30`，sample `0x2E`返回最终EAX。

UT建立40节点链，初始window0/local15/visible16。BBE0得到window16/local15，B9A0发布node16，BC90返回node32；F880→F1E0记录依赖顺序保持，flags `0xAB00→0xAB30`，sample返回81，六步计数为6。

## 3. phases 2、3与其他phase

phase2与DD20保持原顺序：toggle不等于1时先sample `0x107`，再检查runtime bit1；gate清才写toggle1。UT锁定toggle0/bit1置位仍先sample并保持toggle0，toggle已1时不sample且EAX0。

phase3写countdown `0xC8`并返回EAX0；其他phase保留DEC链EAX，UT锁定phase4返回1。

## 4. DA30回接

DA30两条动态路径的最终DED0现直接执行。测试确认通用port只观察未关闭DFA0；DED0通过closed helper设置`0x30` flags并返回sample EAX，DA30 callback count及last target仍包含DED0。

## 5. 验证

定向`special_modes.legacy_initial_menu`通过。workpack双生成稳定为`49/227`，SHA256均为`eedad1b039d678862dae44defa748f89ef2b32459ec2948067cece165ff1fe8e`；下一单元为`0x0043DFA0`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
