# 战斗队伍物品定义准备 `0x00477BD0`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`。

## 1. 完整LST范围

唯一行为真值为`swd3.exe.lst`。完整主体为`0x00477BD0..0x00477C8E`，从`proc`到`endp`共102个物理行、68条实际指令、4个call、5个跳转、5个局部标签和2个返回点，没有外部`FUNCTION CHUNK`。四个call依次属于零ID诊断`0x00431150`、`0xB0`字节分配`0x00487C10`、MON定义读取`0x00476DB0`和Win32 `lstrcpyA`；后三个只在普通缺失项路径执行。

唯一物理caller为`0x0046CE75`，位于已关闭的战斗脚本分发器`0x00469D20`的case 55。该caller先有符号扩展两个word操作数，再以`actor - 8`和item ID调用本函数。

## 2. 参数收窄、诊断和链搜索

入口只把第二参数低word保存到BX。ID为零时先以文本token `0x004A7D38`、源文件`org.cpp`和源行`0x50A`调用`0x00431150`；诊断正常返回后继续，不把零ID改写成替代值。

第一个参数只保留低16位，并未经范围检查地索引`0x004A9490`的四个队伍物品链头。选中链头首先复制到EAX和EBP，EBP保存原链头，ESI保存当前链头槽地址。随后的第一次真实节点访问无条件读取当前节点`+0x04`的word ID，因此空头、越界索引或不可访问节点均在原访问点停止，不增加nil回退。

当前头未命中时，循环严格执行：读取当前节点`+0x00`后继；零后继进入缺失路径；非零后继先写回选中全局链头，再读取新头`+0x04` ID。没有环检测或现代遍历上限。根命中返回0且不写链头；后继命中也返回0，并保留链头移动到命中节点的副作用，绝不执行尾部恢复。后继命中时EDX仍是前驱token。

## 3. 缺失项分支与分配顺序

缺失ID恰为`0x8000`时不调用分配、MON或字符串复制，直接在`0x00477C84`恢复EBP保存的原链头并返回1。其他ID严格按以下顺序处理：

1. 调用`0x00487C10`申请`0xB0`字节；
2. 读取当前尾节点并先把分配结果写入尾节点`+0x00`；
3. 再从该link读取新token，以ECX=`0x2C`、EAX=0执行44次dword清零；
4. 从当前全局头重新读取`+0x00`，把链头移动到新节点；
5. 写新节点`+0x04`的word ID；
6. 从新节点ID做零扩展，以新节点`+0x0C`为输出调用`0x00476DB0`；
7. 以当前链头`+0x0C`为源、`0x0053C154`为目标调用`lstrcpyA`；
8. 仅在字符串复制正常返回后恢复原链头，并固定EAX为1。

分配返回零时，零token已经发布到尾节点link；随后第一次`rep stosd`目标访问在零token停止。短分配对象按每个dword的原始写入顺序保留已清零前缀和递减后的ECX。宿主无法映射非零分配token时也只在尾link已经发布之后隔离，不撤销任何原副作用。

MON正常open失败仍是普通路径：节点定义区域保持loader已完成的清零状态，随后字符串复制空名称并恢复链头。MON typed-stop保留已链接、已清零、已选中和已写ID的新节点，并阻断字符串复制与恢复。

## 4. 字符串复制与寄存器残值

`lstrcpyA`的源地址严格由当前选中链头加`0x0C`计算，不复用caller提供的名称或替代缓存。typed实现先调用窄端口以保留Win32调用返回的EAX/ECX/EDX，再逐字节执行相同源读、目标写和NUL终止顺序。源或目标访问失败时保留完整已复制前缀，不恢复链头。

成功附加路径最终只覆盖EAX为1；ECX和EDX保留`lstrcpyA`返回残值。已存在路径不调用字符串复制：根命中保留入口EDX，后继命中保留前驱token。`0x8000`缺失路径保留队伍索引ECX和尾节点EDX。所有typed-stop返回到组合caller时均保存叶函数到达故障点的寄存器与共享状态前缀。

## 5. 共享owner与唯一caller回收

`0x004A9490`继续由`LegacyWorldItemListState::party_item_lists`唯一持有，没有建立第二份物理状态模型。节点复用既有`LegacyWorldItemNode`，增加的`legacy_accessible_bytes`仅描述该物理节点从偏移零起可访问的字节数，支持按`+0x00`、`+0x04`和44次清零原访问点隔离。

MON定义直接复用`load_legacy_battle_mon_definition()`及`LegacyBattleMonDatabasePort`。全局caption `0x0053C154`直接复用`LegacyBattleLevelAdvancementState::growth_caption_text`；零ID诊断的`hWnd`直接取自caller已有`LegacyBattleStartupState::window_token`。脚本分发端口通过虚继承共享两类owner，避免影子链表或影子caption。

case 55严格保留caller顺序：先把`[cursor+2]`有符号扩展到`value_a`，以该值减8形成零基队伍索引；再把`[cursor+4]`有符号扩展到`value_b`并调用typed叶函数。叶函数返回1才把transition mode写1；随后EAX清零、两个workspace值清零、cursor增加6、恢复caller入口ECX并返回1。叶函数typed-stop不执行transition、清零或cursor推进；第二操作数读取故障则只保留已发布的`value_a`。

旧`pending_477bd0` opaque调用已删除，对应枚举位置保留为`reserved_party_item_definition`以维持数值稳定。唯一caller直接组合typed叶函数并暴露`party_item_definition_typed_stop`。

## 6. 验证与阻塞

独立leaf回归覆盖根/后继命中、后继链头保留、`0x8000`缺失、普通附加、零ID诊断后继续、MON正常open失败、越界队伍索引、空头与不可映射后继、allocator调用trap、零分配、短分配清零前缀、宿主映射失败、MON typed-stop、字符串调用trap及caption写入边界。caller回归覆盖case 55普通附加、已存在项不改transition、叶函数typed-stop前缀和第二操作数读取故障前缀。

发布验证为定向测试`2/2`、Linux core`195/195`、AddressSanitizer`195/195`、Linux app`201/201`、连续10轮完整core、changed-range clang-format、库存连续双生成和release审计；最终日志零OpenSWD3源码warning、测试失败、sanitizer finding或runtime error。工作包为`274/422 = 264 platform_adapted + 10 assembly_exact + 148 pending_audit`，库存SHA256为`854922b90879fa0cfb79548531f5ebfe8aa6ce9c530f86251124da59425b3e69`。未启动原版或OpenSWD3游戏程序。

原版四条队伍物品链、allocator与MON对象状态、Win32诊断/字符串复制ABI，以及caller与callee联合寄存器捕获后端缺失，`original_diff_verified`登记为`blocked_runtime_oracle`。该阻塞不影响完整LST静态闭环、typed故障隔离和Linux验证。
