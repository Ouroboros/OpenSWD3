# 标准模式数据库forward刷新 `0x0043F000`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043F000..0x0043F073`，55行；direct caller为D530、E080、E170各一次。调用顺序严格为：

1. F080刷新forward/adjustment外部列表。
2. F0D0以`page_selection`和外部目录重建`forward_head`。
3. 仅当head为null时调用D5D0分配fallback并写回。
4. B980统计完整forward count。
5. 写window offset0、local selection0、current head。
6. BC90以limit16写bounded count并返回bounded node指针。

F080现直接复用已关闭typed helper；F0D0、D5D0保持两个最小typed port，B980与BC90直接复用已关闭helper。F000结果区分是否分配fallback，并保留BC90指针返回联合。

## 2. caller回接

原typed代码曾把F000整体压成`initialize_*forward_list`单port，并由caller重复或遗漏owner写入。本工作包删除该合并边界：

- D530在动作/phase owner初始化后直接调用F000，不再重复B980/BC90。
- E080/E170在page回绕后直接调用F000；随后以F000新count/head执行已关闭BCC0，再继续F880/F1E0边界。
- 其他DD20/DED0/DFA0/DDF0中的`refresh_database_records`并非F000 direct call，保持原callee边界不误接。

## 3. 测试

独立UT覆盖：

- F080观察旧forward/adjustment owner。
- F0D0接收精确page selection。
- 三节点head得到count3、bounded count3、window/list清0、current=head，helper数4。
- F0D0返回null时才调用D5D0，单节点fallback得到count1，helper数5。
- 既有D530、E080、E170测试继续通过，证明三caller直接回接未改变后续顺序。

## 4. 验证

定向`special_modes.legacy_initial_menu`通过。workpack双生成稳定为`58/227`，SHA256均为`174d13dfe6e7583041a1df0f5e876cea5eb469ce3ac253eba3a79c92f78a76cc`；下一单元为`0x0043F080`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
