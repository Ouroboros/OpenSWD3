# 标准模式数据库分页后退 `0x0043DFA0`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与caller

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043DFA0..0x0043E07A`，104行；direct caller是已关闭DA30，B480另以callback地址绑定。直接callee B9A0/BC60/BC90/F880/F1E0均已关闭；物品查询及sample保留平台边界。

DA30命中`address_0043DFA0`时直接调用本helper。至此DA30 phase1动态分页链中的DDF0、DD20、DFA0、DED0全部脱离通用地址port。

## 2. phase 1

`FCD20==1`时按顺序：

1. BC60以固定step16处理window/local。local非零时只清local；local为0时window减16，负值则window/local均钳0。
2. B9A0按新window offset重建current forward head。
3. BC90最多计16项并重写visible count。
4. F880。
5. first/second inline records进入F1E0。
6. display flags低字节`OR 0x03`，sample `0x2E`返回EAX。

UT建立40节点链，初始window16/local0，得到window0/local0、current node0、bounded node16；F880→F1E0依赖顺序保持，flags `0xAB00→0xAB03`，sample返回83，六步计数为6。

## 3. phase 2

phase2与DDF0相同：先查询物品`0x1BA9`，缺失返回EAX0；存在后runtime bit0可提前返回EAX1；gate清时读取toggle，非零才sample `0x107`，最后清toggle。

UT锁定item存在/toggle1时query后sample返回93并清0；item缺失时不sample且toggle9保持。

## 4. phase 3与其他phase

phase3写countdown `0xC8`并返回EAX0；其他phase保留DEC链EAX，UT锁定phase4返回1。

## 5. DA30全闭环分页链

闭环DFA0也不修改鼠标X。DA30测试使用重叠的两组动态strict边界表达原控制流可连续命中：

- `x=80`：DDF0→DFA0→DED0。
- `x=460`：DD20→DFA0→DED0。

三者均为直接typed helper，通用port事件为空；X保持80/460，callback count为3，last target为DED0，组合flags为`0x33`，最终EAX来自DED0 sample。

## 6. 验证

定向`special_modes.legacy_initial_menu`通过。workpack双生成稳定为`50/227`，SHA256均为`a3c74b1f054f6886676e1956a6da3086df39ed202b646b1ac26210792533becb`；下一单元为`0x0043E080`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
