# 战斗法宝成长结果角色选择 `0x00468FF0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00468FF0..0x0046907F`，从proc到endp共77行、46条带机器码和真实助记符的实际指令、5个静态call、6个跳转、4个局部标签和2个返回点，没有外部`FUNCTION CHUNK`。5个callsite依次为组A完成查询、法宝成长结果选择、道具定义加载、定义临时说明释放和`lstrcpyA`标题复制。

唯一静态caller位于已关闭消息阶段的消息113分支。原caller只在transition actor为`0xFF`时调用本函数；返回后重新读取actor。成功选角才继续sample、组A完成查询、`8,1`分配和法宝完全成长提示框；仍无actor则转消息102并清timer。

## 2. live组A扫描与资格门

入口把live组A数量读入EAX，以i32不大于零直接结束。角色索引从零开始，物理组A对象从`0x005029D0`起按`0x2F34`步进。每个角色依次执行：

1. `+0x2B00`或`+0x2B04`任一完整dword精确等于1即跳过；
2. 以当前物理角色为ECX调用完成查询，返回EAX精确等于1即跳过；
3. 压入固定成长结果profile token，再以当前物理角色为ECX调用成长结果选择callee；
4. 只测试返回`AX`。低16位为零即跳过，高16位不参与资格判断。

每次跳过都重新读取live组A数量，再递增角色索引并按i32执行`index < live_count`。callee缩短或扩展数量会改变同一次扫描边界。数量超过十时在首次真实组A跳过字段访问typed-stop；零或负数量不会访问角色。

## 3. 首成功提交、标题与返回链

成长结果选择返回`AX!=0`时，完整32-bit EAX原样作为道具定义编号参数，不截断为u16。函数把该值与共享定义scratch token依次传给定义加载，再把同一scratch传给临时说明释放。两处callee的EAX/ECX/EDX返回逐项串联，不伪造统一寄存器快照。

释放完成后先把transition mode写1，再以共享scratch首byte为源、24-byte成长标题为目标执行`lstrcpyA`，最后才把当前物理角色索引的低byte发布为transition actor。标题24 byte内没有NUL时，保留mode写入和完整24-byte目标前缀，在第25个目标byte首次真实写入处typed-stop；actor保持旧值，caller的sample、fallback、提示框和timer均不执行。

标题正常终止时函数立即返回，不继续扫描后续角色。因此本函数是“首个成功角色获胜”；与`0x00468C80`持续扫描并由最后成功角色获胜的行为不可合并。正常成功返回`lstrcpyA`的完整EAX/ECX/EDX；无成功结果时返回最后一次live组A数量及最后callee留下的ECX/EDX；入口数量不大于零时ECX/EDX保持入口值。

## 4. owner、caller与验证

本函数不新增持久state。两个跳过dword复用胜利奖励state，组A数量复用actor metric，160-byte定义和临时说明复用`LegacyBattleGrowthActorSelectionState`中的唯一scratch，24-byte标题复用`LegacyBattleLevelAdvancementState::growth_caption_text`，transition mode/actor复用目标选择runtime。固定profile地址、scratch地址和标题地址只作为`compat::u32` token，不转换为宿主指针。

消息113在actor为`0xFF`时已直连本实现。子typed-stop直接返回并阻断sample、完成查询、分配、无actor fallback、法宝完成提示和timer；正常返回仍按原顺序重新读取actor。旧消息113选角槽保留枚举数值并改名为reserved，生产代码零调用。主帧适配分别映射完成查询、成长结果选择、定义加载、说明释放和标题复制五类服务；定义使用既有160-byte载荷和256-byte显式说明长度。

定向测试覆盖零/负数量、两项精确1跳过、完成查询精确1、live数量重读、`AX`零而高word非零、完整32-bit定义编号、五处参数和寄存器链、说明释放、首成功早退、24-byte标题边界、第十一个物理角色访问、消息113直连/旧槽零调用/子stop后置阻断及主帧五服务映射。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。

当前缺少原版组A对象、完成查询callee、成长结果profile链及选择callee共享副作用、真实MON定义加载/说明释放、共享scratch/标题/transition联合状态、`lstrcpyA`返回及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
