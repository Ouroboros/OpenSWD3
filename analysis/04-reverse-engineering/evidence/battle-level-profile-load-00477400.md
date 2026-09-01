# 战斗 LEVEL.DAT 等级资料与队伍物品读取 `0x00477400`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`。

## 1. 完整权威范围

唯一行为真值为 `swd3.exe.lst`。函数主体为 `0x00477400..0x00477698`，从 `proc` 到 `endp` 共 298 个物理行、196 条带机器码和真实助记符的实际指令、14 个静态 call、15 个跳转、11 个局部标签和 3 个返回点，没有外部 `FUNCTION CHUNK`。

唯一直接 caller 是已关闭的战斗升级函数 `0x00467C50`，调用点为 `0x00467D39` 和 `0x00467D4D`。两个调用分别以旧等级填充 `0x005028C0` 的 56-byte基准资料、以新等级填充 `0x00520F80` 的56-byte升级资料。此前导航把本函数称为“MON定义派生记录读取”；完整LST证明其实际职责是从共享 `LEVEL.DAT` 读取等级资料，并按记录内引用补齐对应队伍的物品定义链。

14个call包括路径复制/拼接和文件打开各1次、seek/read各2次、`0x400`流分配/释放各1次、零物品诊断1次、`0xB0`物品节点分配1次、`0x00476DB0` MON定义读取1次及成长标题复制1次。

## 2. 共享 LEVEL 会话与记录定位

函数复用与 `0x00477290` 相同的共享open标志、handle和路径缓冲。首次调用形成 `level.dat` 路径并按只读、共享读、`OPEN_ALWAYS`及普通属性打开；失败正常返回0且不分配流。成功后缓存会话，后续旧/新等级两次调用及其他caller都复用同一handle。

目录计算保持原32位回绕：

```text
index = party_number_one_based * 100 + level
slot  = 0x70 + 4 * index
```

函数seek到目录槽，读取4-byte little-endian相对偏移，再以 `relative + 0x200` seek到记录。ReadFile返回和bytes-read不形成现代成功门；目录短读保留的陈旧dword仍参与寻址。随后分配、清零并读取固定 `0x400` 字节流。分配零只在原 `rep stosd` 首次store处形成typed-stop；首word非零则释放流并正常返回0。

真实8206字节 `LEVEL.DAT` 只读枚举确认每个样本均有一条tag 0资料，部分等级追加一条tag 1物品引用。固定联合样本为party 1、level 5：目录槽 `0x214`、相对偏移1702、文件偏移2214、26-byte资料 `3f0014001e0010000d0014001d000a001200020002005a000000`，随后引用物品1501。

## 3. tag循环与56-byte资料写入

循环每次只以 `movsx ax, byte ptr [cursor]` 读取tag低byte，但本地指针固定推进2字节。实现保留先写AX、tag 5比较、`and eax, 0xFFFF`、tag 0分支及随后 `dec eax` 的原寄存器顺序，因此负byte未知tag在故障前的EAX也保持符号扩展后的低word。命令为：

- tag 0：从当前payload向caller输出 `+0x0A` 复制6个dword和1个word，共26字节；随后把输出 `+0x0A/+0x0C/+0x0E` 三个word镜像到 `+0x04/+0x06/+0x08`，并把请求等级低byte写到 `+0x2C`；
- tag 1：读取一个u16物品ID并执行队伍物品链逻辑；
- tag 5：释放临时流并正常返回1；
- 其他tag：不跳现代化payload，直接从下一word继续解释。

资料复制严格按原 `rep movsd` 与末尾 `movsw` 的逐访问顺序；输出故障保留已复制前缀、剩余ECX和未推进的本地cursor。复制成功后才一次性把cursor推进26字节。三个镜像word分别只替换EAX/ECX/EDX低16位，高16位保持原目标或源指针状态。

## 4. 临时队伍链头与物品引用

原数组表达式以 `0x004A948C + 4 * party_number_one_based` 访问四个有效链头，即 `0x004A9490..0x004A949C`；`0x004A948C`不是另一条有效队伍链。实现以一基party 1..4映射到唯一 `LegacyWorldItemListState` owner，并显式保存、推进及恢复每条链的 `legacy_head_token`，使typed-stop前的临时全局链头可观察。

读取tag 1的物品ID后：

1. 保存当前队伍全局链头；ID为0时先调用原模态诊断，正常忽略后继续，Abort/Retry trap以 `diagnostic_typed_stop` 保留；
2. 从当前链头比较节点 `+0x04` 的ID；不相等就读取 `+0x00` next并把全局链头推进到next；损坏token只在首次真实节点访问处停止；
3. 找到既有ID时不重新加载MON、不修改过渡模式或标题，直接恢复保存的链头；
4. 到达尾节点且ID为 `0x8000` 时不分配，直接恢复链头；
5. 其他ID分配固定 `0xB0` 字节节点，先把分配token写入旧尾 `+0x00`，再按44个dword清零；零分配因此保留已发布的null尾链接，并在原清零store处停止；
6. 清零完成后把全局链头推进到新节点，写入ID，以节点 `+0x0C` 和ID直连 `0x00476DB0`；MON typed-stop保留已链接、已清零、已写ID的新节点；
7. MON正常返回后把共享过渡模式写1，再以无界 `lstrcpyA` 顺序把节点 `+0x0C` 名称复制到固定24-byte成长标题；最后才恢复保存链头并让记录cursor跳过物品ID。

真实 `MON.DAT` 联合回归固定物品1501目录槽6520、相对偏移139290、文件偏移139802及CP950名称字节 `af50a4f5b34e00`。同一加载器连续遇到相同ID时命中既有节点，不重复分配或读取MON。

## 5. 原访问点故障与寄存器

`LegacyBattleLevelProfileLoadResult`记录目录、相对/绝对偏移、流token/cursor、输出与标题停止偏移、临时链头、节点遍历/追加次数、LEVEL和MON调用计数及live EAX/ECX/EDX。

- `stream_access_typed_stop`、`output_access_typed_stop`分别只在原tag、payload或输出访问点触发；
- `party_index_typed_stop`和`party_sentinel_typed_stop`对应原一基数组或根指针访问；`item_node_typed_stop`保留已经推进的全局链头；
- `item_allocation_typed_stop`保留旧尾已发布的null链接；host `std::list` 分配失败另以 `host_item_allocation_typed_stop` 隔离，绝不冒充原版正常返回；
- `mon_definition_load_typed_stop`、`transition_mode_typed_stop`及标题源/目标typed-stop均不恢复链头、不释放LEVEL流，并保留此前新节点、MON资料或模式写入；
- open失败和首word非零是正常0返回；tag 5是正常1返回。只有正常返回路径在原位置释放临时LEVEL流。

正常与故障路径保留 `movsx ax`、pointer高16位、`rep`剩余ECX、MON调用入口以及诊断reply寄存器；平台化文件、分配、诊断和MON边界返回的寄存器由typed port显式发布。

## 6. owner与caller回收

LEVEL文件会话继续由唯一 `LegacyBattleLevelDatabaseState` owner持有；MON定义、动态说明和文件会话继续由 `LegacyBattleMonDatabasePort` owner持有；四条队伍物品链继续使用世界物品生命周期的唯一 `LegacyWorldItemListState`。profile port本身不持有独立状态，默认从已有 `LegacyWorldItemListStatePort` 定位物品owner；SDL显式继承profile port并把访问转发到唯一 `world_item_lists_`，不建立影子链或第二文件会话。后续已关闭的`0x004776A0`在战斗全局重置尾部借用这两个文件owner，按MON再LEVEL顺序关闭有效句柄并清两会话门。

`0x00467C50`的两处旧 `build_level_profile` opaque槽已删除生产调用，分别直连本实现，并在结果中保存基准/升级两次loader证据。任一次profile typed-stop都传播为 `level_profile_typed_stop`，阻断第二次profile、属性差值提交、音频和完成门；旧枚举数值只保留reserved身份且生产零调用。消息101既有caller继续传播升级函数的子stop。

## 7. 验证范围

独立测试覆盖真实LEVEL+MON联合样本、26-byte资料与三word镜像、既有节点命中、新节点尾插、ID `0x8000`、零ID诊断与trap、共享文件缓存、打开失败、非零首word、LEVEL零分配、输出部分复制、非法party、缺失哨兵、损坏next、物品零分配、MON子stop、模式不可访问及标题越界。战斗升级测试覆盖旧/新两次直接profile调用、共享一次open及第一profile故障传播。

最终验证为独立profile与战斗升级聚合定向测试、Linux core `191/191`、完整AddressSanitizer `191/191`和Linux app `197/197`全部通过；最终三份日志没有OpenSWD3源码warning、测试失败、sanitizer finding或runtime error。ASan曾在历史战斗聚合测试的单函数栈帧逼近默认8 MiB后于新增端口虚基处暴露栈耗尽；profile port移除独立状态，物品分配token并入现有世界物品owner，并只把该聚合函数首个测试块的fixture和port放到堆上，未禁用sanitizer且未改变生产行为或测试断言。inventory生成器连续双跑逐字节一致，工作包为`263/422 = 254 platform_adapted + 9 assembly_exact + 159 pending_audit`，SHA256为`164b20599ba5046ee1a87a13b145ed06907342e3d0814bb88dce4787a620f0e7`。

当前缺少原版共享LEVEL/MON文件对象、短读后的真实陈旧缓冲、原堆token、四条队伍链原物理地址、模态诊断选择、真实caller输入及跨LEVEL/MON/caller的EAX/ECX/EDX联合捕获后端；`original_diff_verified`登记为`blocked_runtime_oracle`。
