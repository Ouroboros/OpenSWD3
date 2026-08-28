# 战斗成长角色选择 `0x00468C80`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00468C80..0x00468CCE`，从proc到endp共245行、144条带机器码和真实助记符的实际指令、9个静态call、18个跳转、10个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。9个callsite依次为组A角色完成查询1次、道具定义加载3次、道具定义临时说明释放2次、道具存在查询1次、`0xB0`字节节点分配1次和`lstrcpyA`标题复制1次。

唯一静态caller位于已关闭消息阶段的消息112分支。原caller只在transition actor为`0xFF`时调用本函数；返回后重新读取actor，仍为`0xFF`才转消息113，否则继续播放、完成查询、transition分配和成长完成标题框。

## 2. live角色扫描与资格门

入口把live组A数量读入EAX，以i32不大于零直接结束；角色索引从零开始，最大道具编号工作值只在入口清零一次。每轮结束重新读取live组A数量并按i32比较，因此callee缩短或扩展数量会改变同一次扫描边界。

每个物理组A角色依次执行：

1. 两个角色结算跳过字段任一精确等于1即跳过；
2. 以组A基址`0x005029D0`、尺寸`0x2F34`构造对象token并调用完成查询，返回精确1才跳过；
3. 按物理角色索引读取动作标签，再以`label*3`访问四项成长profile；成长道具码精确为`0xFFFFFFFF`时跳过；
4. 道具链仍按物理角色索引读取，不改用动作标签索引。

角色对象、动作标签、live数量和两个跳过字段均复用既有typed owner。`0x004ACF5C`在PE中为未初始化全局，typed成长道具码因此从全零开始；只有本函数成功提交零编号后才写成`0xFFFFFFFF`并让后续扫描跳过。动作标签越出四项成长profile时在首次真实profile访问typed-stop；物理角色或道具链缺失时也只在原始首次访问点停止。

## 3. 道具链、signed门与精确1黑名单

角色道具链从共享sentinel的首链接开始，沿typed节点顺序遍历到空链接。只有定义快照`+0x5E`的u16类型精确等于`0x1F`才参与成长选择。每个匹配节点按u16读取节点道具编号，保留ECX高word后写入低word，再把定义加载到共享160-byte scratch；临时说明随后立即释放。节点编号按零扩展后与跨角色保留的最大编号工作值做i32比较。

每个匹配节点都会用本次scratch覆盖映射profile：`+0x44`低u16写成长限制dword，`+0x42`低u16与`0x80000000`组合为成长道具码。链中后一个匹配节点覆盖前一个，因此最后一个匹配定义决定角色资格。

链结束后把奖励计数与成长限制按i32比较；计数小于限制则跳过。成长道具码低31位为零也跳过。随后查询道具`0x1BB0`是否存在；只有返回值精确等于1时才启用黑名单，且仅成长道具码低31位为`0x0665`或`0x0669`时跳过，返回0、2或其他值都继续。

## 4. 派生节点、标题与提交顺序

通过资格门后，以成长道具码低u16再次加载共享scratch并立即释放临时说明，再沿当前物理角色道具链找到尾节点。原程序分配`0xB0`字节后先把新地址写入尾链接，再以44个dword清零分配区；空分配会在首次真实memset目标访问故障。modern在对应点返回`allocation_typed_stop`，不追加节点、不发布actor，也不执行后续标题与profile提交。

分配成功后建立唯一typed节点，保留`compat::u32`生命周期token，把成长道具码低u16写入节点编号，并把定义加载到新节点`+0x0C`对应的160-byte快照。追加编号来自成长profile，不是被扫描的旧节点编号。

随后先把transition mode写为1，再按`lstrcpyA`顺序把新节点定义首字符串复制到共享24-byte成长标题。第24个非NUL byte已经写入后，下一次NUL目标访问才typed-stop；此时分配、尾链接、节点定义加载、mode写入和24-byte复制副作用全部保留，但actor尚未发布，profile也尚未清零。

标题正常终止后发布当前物理角色byte，并重建其映射profile：奖励计数清零，成长限制取共享scratch `+0x44`低u16，成长道具码取`+0x42`低u16并置bit31；低u16为零时再把道具码改为`0xFFFFFFFF`。扫描不会首成功早退，后续角色继续执行，最后一个成功角色和标题成为最终可观察结果。正常返回EAX为最后一次live组A数量，ECX由入口栈槽恢复，EDX保留最后一次profile提交值。

## 5. owner、caller回收与验证

四项奖励计数、成长限制和成长道具码属于胜利奖励state的连续profile owner；全局重置按原写集合保留这三组profile。角色道具链与新节点继续复用`LegacyWorldItemListState`，不建立battle平行链。共享定义scratch只存在于本函数state；24-byte标题复用`LegacyBattleLevelAdvancementState::growth_caption_text`；transition mode/actor复用目标选择runtime。后续消息113现由已关闭法宝完全成长提示框直接消费同一24-byte标题，不复制文字owner。

消息112在actor为`0xFF`时现已直连本实现；子typed-stop阻断caller的sample、完成查询、transition分配、成长完成标题框、timer和消息切换。选择后仍无actor的既有转113路径不变。旧选角槽保留枚举数值并改名为reserved，生产代码零调用。主帧端口映射组A完成查询、定义加载、道具存在查询和节点分配四类服务，定义说明使用权威最大有效长度覆盖的固定256-byte载荷与显式长度。

定向测试覆盖零/负数量、live数量重读、两个精确1跳过门、完成查询、无效profile、缺失sentinel、非成长节点、最后匹配定义覆盖、signed计数门、低31位零门、精确1黑名单、物理链索引与映射profile索引分离、多角色最后成功发布、派生编号追加、三次定义加载寄存器、分配失败、24-byte标题边界、尾链接和profile提交顺序、消息112直连/旧槽零调用/子stop传播及主帧服务映射。验证：定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过。源码构建零warning；Linux app仅保留既有ALSA开发库CMake提示。

当前缺少原版组A对象、完成查询callee、真实MON定义加载/说明释放、共享道具链、内部bit查询、分配器、全局scratch/profile/标题联合状态、动态堆地址、`lstrcpyA`返回及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
