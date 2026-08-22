# 剧情 VM 物品存在条件重载 `0x0042BEFE`

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042BEFE..0x0042BF71`

共享opcodes：129、130、167、168

## 1. 物理记录与固定访问顺序

四条物理记录均为8字节：

```text
+0  u16 opcode
+2  u16 item id
+4  u32 same-file target
```

handler先读取`+2`并无条件调用：

```text
sub_44D680(&dword_4A9940, item_id)
```

返回后重新从当前物理脚本指针读取`+2`，再无条件调用：

```text
sub_44D650(item_id)
```

因此即使某variant最终只使用角色物品root，玩家库存查询仍先发生；即使玩家查询已命中，64槽角色root查询仍发生。只有最终谓词taken时才读取`+4` target。

not-taken不读取target，固定把物理脚本指针与u16 IP各加8，设置ESI=1，经common join发布normalized previous并同调用继续。

taken读取target后进入`0x0042CCD6`共享窗口重载：`sub_42E430`先执行audio service，提交当前TALK data offset、IP=0并从同文件`target+0x200`读入`0x8000`窗口；返回后从新窗口offset0同调用继续，common join发布当前previous。

## 2. 两个查询helper

### 玩家普通库存 `sub_44D680`

范围：`0x0044D680..0x0044D6A6`。

输入是无哨兵玩家库存链首`dword_4A9940`的地址。helper从头到尾逐节点读取`+0x04 item_id`，先执行`AND 0x3FFF`，再与脚本完整u16 item id比较；首个匹配返回该节点，空链或全miss返回零。

因此节点item id高两位是被屏蔽的节点侧标志；脚本item id自身不屏蔽。脚本`0x4123`不会匹配节点`0xC123`，而脚本`0x0123`会匹配。

### 64槽角色物品root `sub_44D650`

范围：`0x0044D650..0x0044D67C`。

helper严格扫描`0x004C8AD0..0x004C8BD0`的64个root指针。每槽直接解引用root并只比较哨兵根自身的`+0x04 item_id`，使用完整u16相等；不遍历`root->next`普通节点。首个匹配立即返回该root，全64槽miss返回零。

原有效初始化域会为64槽建立`sub_44D5D0`哨兵。空root在原版是无条件解引用崩溃域；后续槽是否损坏在先前root已命中时不可观察。

## 3. 四个谓词

机器先把两个helper返回指针相加，再用`neg/sbb/neg`归一化为“任一非零”。opcodes167/168在归一化前把玩家查询结果清零。opcodes130/168再以XOR 1反转谓词：

```text
129  玩家普通链存在masked-id匹配，或64槽任一root完整id匹配时taken
130  上述两类都不存在时taken
167  64槽任一root完整id匹配时taken；玩家查询结果被清零
168  64槽全部root完整id不匹配时taken；玩家查询结果被清零
```

原32位裸指针求和理论上保留两地址加法回绕；合法Win32 owner域只把它消费为存在性。现代实现以等价布尔OR表达该有效域，不重建宿主地址偶然和为零的无所有权状态。

## 4. 现代owner与失败边界

现代VM继续借用`LegacyWorldItemListState::player_inventory`，并新增只读借用同一owner的`role_item_lists[64]`；没有VM私有镜像或新平台端口。

玩家链使用`std::list`保留头到尾首匹配。角色数组使用既有`optional<LegacyWorldSentinelItemList>`；逐槽缺root只在原`[root+4]`危险点返回`runtime_unavailable`。先前root已经命中时立即返回，不检查后续槽，保持helper早退。

四variant都保持“operand → 玩家owner/query → 角色root owner/query → 谓词 → taken-only target”的顺序。checked窗口I/O失败保留audio、target、IP0、loader调用和previous publication后返回`load_failed`。

## 5. 资产与验证

线性TALK目录锁定48条物理记录/48 probes，全部为基础raw、长度8：

```text
opcode 129  15  TALK1/2/3/4 = 1/0/5/9
opcode 130  31  TALK1/2/3/4 = 9/1/10/11
opcode 167   1  TALK1
opcode 168   1  TALK3
```

共31种item id，范围`0x0192..0x03EC`；37个按文件区分的唯一target全部落在对应TALK有效范围。target首opcode为1026共33条、38共10条、105共3条、27共2条。

所有48条线性记录的raw分别为`0x0081/0x0082/0x00A7/0x00A8`。高位alias的零散字节候选未被提升为资产记录；尤其`0x40A7`和`0xC0A7`虽在原始文件出现，均不是线性入口。

真实代表记录：

```text
129  TALK1.DAT@0x0004D692  item 799   target 0x0004D868
130  TALK1.DAT@0x00029C8F  item 402   target 0x00029AC9
167  TALK1.DAT@0x000238AB  item 1004  target 0x00023785
168  TALK3.DAT@0x0002452E  item 578   target 0x0002435E
```

四条均按各自predicate taken，并由测试窗口同调用执行`1026→FFFF`完成来源Talk。

synthetic覆盖四variant各四raw alias、四个互补not-taken、玩家item id低14位、角色root完整u16、不遍历角色child链、role-only忽略玩家结果、玩家owner先于role owner、空root、首root命中后不检查后续损坏root、taken-only target、partial/exact tail和I/O失败publication。

Story VM synthetic、real及initial-session三项通过。Linux core 186/186与app 192/192均以exit 0通过。未启动原版或OpenSWD3游戏EXE。
