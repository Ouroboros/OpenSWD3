# 战斗固定键计数链设置与上限 `0x00477780`

状态：`platform_adapted`、`unit_tested`、`caller_partially_reclaimed`。

## 1. 完整LST范围与ABI

权威LST函数范围为`0x00477780..0x004777F9`，从`proc`到`endp`共70个物理行、45条实际指令、1个call、5个跳转、5个局部标签和2个返回点。函数没有外部`FUNCTION CHUNK`，唯一callee是20字节分配包装器`0x00487C10`。

函数采用cdecl三参数ABI：第一个参数是链根地址，第二、第三参数都只读取低word，分别作为键和待设置计数。五个真实caller站点为启动资料载入`0x00409DD0`一处，以及共享角色、道具调试对话`0x0040F890`四处。

## 2. 根记录与扫描顺序

入口把根保存在EBX、当前记录保存在ESI、键保存在DI。根不是纯哨兵，函数先比较`word [root+4]`；不匹配时才按以下顺序扫描动态节点：

1. 从当前记录`+0x00`读取完整next到EAX；
2. next为零时进入分配路径；
3. next非零时把ESI替换为该token；
4. 比较新记录`+0x04`的word键；
5. 不相等则继续读取该记录next。

原函数没有长度上限、环检测、排序或nil修复。modern helper只在`LegacyBattleFixedObjectState`内解析legacy token，不把token解释为宿主指针。未知next已先进入EAX，随后只在该记录首次`+0x04`键访问点typed-stop。

## 3. 已有记录的两次写入上限

命中根或动态节点后，函数把第三参数低word装入AX，以无符号word和20比较，然后无条件先把原始AX写入`[ESI+6]`。输入大于20时再执行第二次word写，把同一计数字段覆写为20；输入小于等于20时只保留第一次写入。

该路径不是现代饱和赋值：比较发生在第一次存储之前，原始值写入和覆写是两个独立可观察写入。返回EAX仍保留原始输入低word而不是夹值；根命中时EAX高word来自入口EAX，动态节点命中时高word来自扫描得到的节点token。ECX和EDX在整个已有记录路径保持入口值。

## 4. 缺键分配、清零与故障前缀

next为零时以固定大小20调用分配包装器。callee返回后原函数立即清零ECX，保留callee EDX，并严格执行：

1. 把allocator返回EAX写入前驱`+0x00`；
2. 以ECX零按`+0x00`、`+0x04`、`+0x08`、`+0x0C`、`+0x10`顺序清五个dword；
3. 从前驱`+0x00`重新读取已链接token到ESI；
4. 把输入计数低word装入AX并与20作无符号比较；
5. 先写新节点`+0x04`键，再写`+0x06`原始计数；
6. 输入大于20时第二次写`+0x06`为20；
7. 以word回绕递增根`+0x04`。

前驱link发布早于任何新节点访问。allocator返回零时也先写零link，随后才在原`mov [eax],ecx`访问点typed-stop。非零不可访问token相同；可访问前缀0、4、8、12或16字节时分别保留link和此前完成的0至4次清零。完整五次清零后才允许写键、写计数、可选覆写和递增根word。

分配路径终端EAX高word来自allocator token、低word来自未夹的输入计数；ECX固定为零，EDX保留allocator reply。modern结果分别记录原始计数写和上限覆写，不把两次写合并为一次。

## 5. 唯一owner与共享低层目标

固定根`0x004B9F00`、相邻两个固定header以及全部动态20字节节点继续只由`LegacyBattleFixedObjectStatePort`持有。设置helper和前一项累加helper共用同一`LegacyBattleFixedCountAllocationPort`，没有在Dialog、Fame载入或SDL建立第二条链。

为避免完整`battle`目标依赖`special_modes`同时出现反向依赖，固定计数链实现下沉到既有低层`openswd3_battle_mon`目标；完整战斗和特殊模式都只链接这一个实现。`LegacyPartyDialogPorts`虚继承固定对象owner和分配端口，使已关闭Dialog直接组合typed helper而不是继续保留第三类整函数opaque更新接口。

## 6. caller范围

启动caller `0x00409DD0`先分配并清零1024字节，尝试读取`Fame.dat`，然后以键1至500和对应u16槽调用本函数。该caller含文件对象、SEH和外部载入生命周期，当前仍为`pending_audit`；SDL的`load_fame_table()`仍为空，本工作包没有伪造Fame载入、没有提前改写启动流程。

已关闭Dialog在新增和直接修改命令中各有两处调用：第三分类mask匹配调用一次，记录ID位于1至500时再调用一次，因此两条件同时成立可对同一键重复设置。typed组合保持原参数来源：链键来自解析后的命令参数，低ID重复门读取记录自身ID；两者在低14位查找到带高位记录时不得混同。第一次调用后的ECX返回值继续线程到可能发生的第二次调用。

Dialog的玩家库存写入和第一、第二分类副作用先于本helper。typed-stop立即阻断填页、清三编辑框和scratch释放，保留已经完成的库存与分类前缀；正常新增仍为填页、清框、释放，正常直接修改仍为释放、填页、清框。源码不再声明或调用第三分类opaque更新接口。

## 7. 验证覆盖

独立固定计数链测试覆盖根命中、动态节点命中、输入20、无符号大于20、原始写后覆写计数、EAX高word来源、分配寄存器线程、新节点键/计数/根递增、allocator零返回、未知next、已有节点计数写故障，以及五个清零写入点的全部typed-stop前缀。

Dialog聚合测试覆盖mask与低ID重复调用只分配一个共享节点、计数大于20覆写、命令键与记录高位ID分离、allocator失败时库存和前两类副作用保留且不刷新/清框/释放。启动Fame caller保持未改。

## 8. 动态差分

当前缺少原版固定键链、`Fame.dat`载入状态、Dialog命令参数、allocator堆状态以及五个caller寄存器联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。该缺口不阻止完整LST静态闭合、typed故障前缀和Linux门禁。
