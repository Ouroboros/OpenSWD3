# 执行历史：剧情VM第一段

状态：冻结历史；不得作为当前执行状态或行为真值。

来源：重构前`execution-plan-pi.md` v853第1369..1900行，剧情VM P1/P2前段。

完整性与当前资料入口见[`../execution-history-index-pi.md`](../execution-history-index-pi.md)。当前状态见[`../execution-state-pi.md`](../execution-state-pi.md)。

---


- 剧情 VM 追加 PLAN P1 随后完成 scope lock。`build_story_vm_dispatch_inventory.py` 已从
    失效的 ASM/PE 双源改为只锁完整 LST SHA-256，并从 LST label、首 dword 字节、两张
    internal jump table 和 157/73 byte selector 可重复生成分派。198 行 dispatch、146 个
    一级入口组、25 个共享入口和 2 个 internal switch 重建后无目标漂移。新增 146 行
    handler workpack 与 17 行 runtime-path 表；50 个现代 case、125 行旧人工语义、143/55
    资产观察、static triage 和十类候选端口全部只作导航，所有 handler 固定从
    `pending_audit` 开始，P1 结束时闭环为 0/146。ruff、py_compile、生成器硬断言、TSV 宽度和
    Markdown 链接通过。

- 剧情 VM P2 第一组默认非法入口 `0x0042D230` 随后完成独立闭环。显式 opcode 0 的四个
    raw alias 与默认范围 `194..1023,1027..16382` 均恢复 `MessageBeep(0)`、previous/current
    诊断快照、IP 不推进、`dword_4CF6D8` 写回、一次 `_AIL_serve` 和返回一。现代 default
    先判原默认域，避免把显式但尚未实现的 `1..193/1024/1025` 误作非法值；SDL beep 保持
    平台 no-op，audio 接实际 maintenance owner。UT 固定四 alias、四边界、两帧 previous
    rollover、调用顺序与显式未实现隔离；story VM synthetic/real/initial-session-real 3/3，
    Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192 通过，三进程均 exit 0，
    未启动游戏 EXE。workpack 当前 1/146，即 `1 platform_adapted + 145 pending_audit`；其他 handler 的公共 join
    仍独立待审，不继承本组 previous/audio 证据。

- 剧情 VM P2 第二组共享对话入口 `0x00427B8F` 完成独立闭环。一次恢复 opcode
    `1-6,89-90` 的 mode 0/1/2 不同固定头、奇偶 flag/gate/counter、`FFF0/FFFD`、一次性
    anchor/center/text globals、`sub_40B7F0` prepared-text 测量、detached 0x4C record、
    两次 `_AIL_serve`、IP/reset/previous 顺序和受检失败点；modern case 从 50 增至56。
    `%T/mon.dat` 通过可失败窄 port 保留原 resolver failure，不伪造名字。UT 固定八变体、
    四 raw alias、三档 mode0、real physical records、valid→default 和失败顺序；4392/4392
    TALK 记录满足 6/14/10-byte 头与 `%Q`。定向 story VM 3/3、Linux core 186/186、
    Linux app 192/192、Windows LLVM app 192/192 通过，三进程均 exit 0，未启动游戏 EXE。
    workpack 当前 2/146，即 `2 platform_adapted + 144 pending_audit`。

- 剧情 VM P2 第三组 `0x00427E72` / opcode7 完成独立闭环。LST 直线合同固定
    `text_control_flags &= 0x7FFFFFFF`、IP+2、common join 先发布 previous=7、无 audio 并
    同帧继续。修复旧 case 遗漏 previous 的差异；四 raw alias、`7→default`、`7→dialog2`
    与 `TALK1.DAT@0x4518` 真记录回放通过，2781/2781 条 TALK 记录均为 raw `0007`、长度2。
    定向 story VM 3/3、Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192
    通过，三进程均 exit 0，未启动游戏 EXE。workpack 当前 3/146，即
    `1 assembly_exact + 2 platform_adapted + 143 pending_audit`；全局 common_join 仍 pending。

- 剧情 VM P2 第四组 `0x00427E9A` / opcode8 完成独立闭环。恢复 u16 零扩展、
    pending/value one-shot 暂存、IP+4、previous=8、无 audio 与同帧 continue；修复旧 case
    遗漏 previous 的差异。四 raw alias、短操作数、`8→dialog2` 消费/reset 与
    `TALK1.DAT@0x451A` 的 `08 00 FF FF` 真记录通过；2832/2832 条 TALK 记录均长度4。
    定向 story VM 3/3、Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192
    通过，三进程均 exit 0，未启动游戏 EXE。workpack 当前 4/146，即
    `2 assembly_exact + 2 platform_adapted + 142 pending_audit`；全局 common_join 仍 pending。

- 剧情 VM P2 第五组 `0x00427EC2` / opcode9 完成独立闭环。固定 bit30 clear、IP+2、
    previous=9、无 audio 与同帧 continue；修复旧 case 遗漏 previous 的差异。四 raw alias、
    `9→default`、`9→dialog2` 与 `TALK1.DAT@0x451E` 真记录通过；1537/1537 条 TALK 记录
    均为长度2。定向 story VM 3/3、Linux core 186/186、Linux app 192/192、Windows LLVM
    app 192/192 通过，三进程均 exit 0，未启动游戏 EXE。workpack 当前 5/146，即
    `3 assembly_exact + 2 platform_adapted + 141 pending_audit`。

- 剧情 VM P2 第六组 `0x00427ED0` / opcode10 完成独立闭环。恢复 selector/FFF0、
    live role base variant、wait reset、raw-next chain gate、action update、IP+6、previous=10 与
    missing-role MAPS source patch；修复旧实现错误返回 `role_not_found` 和遗漏 previous。
    四 raw alias、短载荷、fallback、chain 与 `TALK1.DAT@0x4A24` 真记录通过；1693/1693 条
    TALK 记录均长度6。定向 story VM 3/3、Linux core 186/186、Linux app 192/192、Windows
    LLVM app 192/192 通过，三个有效门禁进程均 exit 0，未启动游戏 EXE。workpack 当前
    6/146，即 `3 assembly_exact + 3 platform_adapted + 140 pending_audit`。

- 剧情 VM P2 第七组 `0x00427FEB` / opcode11 完成独立闭环。恢复 selector/FFF0、
    live role variant delta、wait reset、flags OR `0x1000`、raw-next chain gate、action update、
    IP+6、previous=11 与 missing-role MAPS source patch；修复旧实现错误 `role_not_found` 和遗漏
    previous。独立 REVIEW 还修复共享 opcode10/11 的 invalid controlled-role 误 patch：现在在
    原首个危险 live 写入前 checked-stop。四 raw alias、短载荷、fallback、11→45 chain 与
    `TALK1.DAT@0x4A2E` 真记录通过；1234/1234 条 TALK 记录均长度6。定向 story VM 3/3、
    Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192 通过，三进程均
    exit 0，未启动游戏 EXE。workpack 当前 7/146，即
    `3 assembly_exact + 4 platform_adapted + 139 pending_audit`。

- 剧情 VM P2 第八组 `0x0042811F` / opcode12 完成独立闭环。新增语义常量
    `OP_12_SET_ROLE_POSITION`，并确立“已知语义用 `OP_<编号>_<语义>`、未知暂用
    `OP_<编号>`”规则。恢复 FFF0 lookup、raw-selector cache reset、16-bit 坐标左移回绕、
    `sub_42DAF0` typed story-path schedule、ordinary missing consume、controlled bit15、IP+8、
    previous=12 与同帧继续；补齐 invalid controlled/runtime/两类短载荷危险点顺序。四 raw alias
    与组合测试通过；58,782 条物理 TALK 记录中 opcode12 为0，因此不声称 real_asset_tested。
    定向 story VM 3/3、Linux core 186/186、Linux app 192/192、Windows LLVM app
    192/192 均以 exit 0 通过，未启动游戏 EXE。workpack 当前 8/146，即
    `3 assembly_exact + 5 platform_adapted + 138 pending_audit`。

- 剧情 VM P2 第九组 `0x0042822A` / opcode13 完成独立闭环。恢复 selector/FFF0、
    role bit25 gate、`sub_42E280` 三态返回忽略、IP+4、previous=13、直接 audio service 与 yield；
    补齐 ordinary missing、invalid controlled、runtime/slot/cursor/direction checked failure 顺序。
    四 raw alias、return0/1/2、短载荷与组合测试通过；58,782 条物理 TALK 记录中 opcode13 为0，
    因此不声称 real_asset_tested。定向 story VM 3/3、Linux core 186/186、Linux app
    192/192、Windows LLVM app 192/192 均以 exit 0 通过，未启动游戏 EXE。workpack 当前
    9/146，即 `3 assembly_exact + 6 platform_adapted + 137 pending_audit`。

- 剧情 VM P2 第十组 `0x0042829C` / opcode14 完成独立闭环。修复 FFF0/FFFD selector
    顺序、role/context action status wait、IP+4、previous、直接 audio service 与逐帧 yield；
    补齐 missing/invalid controlled/短载荷危险点。四 raw alias、FFFD synthetic 与真实
    `TALK1.DAT@0x471F` 回放通过；全 2,894 条物理记录均为 raw `0x000E`、长度4。
    定向 story VM 3/3、Linux core 186/186、Linux app 192/192、Windows LLVM app
    192/192 均以 exit 0 通过，未启动游戏 EXE。workpack 当前 10/146，即
    `3 assembly_exact + 7 platform_adapted + 136 pending_audit`。

- 剧情 VM P2 第十一组 `0x00428310` / opcode15 完成独立闭环。恢复 6-byte 同文件
    绝对跳转、audio/offset/previous 顺序、无清空窗口加载与同调用继续取指；I/O 失败仅在
    无效域 checked-stop。四 raw alias、窗口尾、失败顺序与真实 `TALK1.DAT@0x8A85`
    跳转回放通过；全 68 条物理记录均为 raw `0x000F`、长度6且 target 有效。
    定向 story VM 3/3、Linux core 186/186、Linux app 192/192、Windows LLVM app
    192/192 均以 exit 0 通过，未启动游戏 EXE。workpack 当前 11/146，即
    `3 assembly_exact + 8 platform_adapted + 135 pending_audit`。

- 剧情 VM P2 第十二组 `0x00428318` / opcode16 完成独立闭环。恢复 8-byte 角色路径
    条件跳转、72-slot predicate、prepared/no-slot 顺序、resolver miss `FFFFFFFF`、branch-only
    target 读取与同调用继续取指；I/O/controlled index 仅在无效域 checked-stop。四 raw alias、
    branch/no-branch 窗口尾与真实 `TALK2.DAT@0xF963` 跳转回放通过；全3条物理记录均为
    raw `0x0010`、长度8且 target 有效。定向 story VM 3/3、Linux core 186/186、Linux app
    192/192、Windows LLVM app 192/192 均以 exit 0 通过，未启动游戏 EXE。workpack 当前
    12/146，即 `3 assembly_exact + 9 platform_adapted + 134 pending_audit`。

- 剧情 VM P2 第十三组 `0x004283AC` / opcode17 完成独立闭环。恢复 8-byte 已准备
    角色路径条件跳转、72-slot predicate、prepared/unprepared 极性、resolver miss `FFFFFFFF`、
    branch-only target 读取与同调用继续取指；I/O/controlled index 仅在无效域 checked-stop。
    四 raw alias、selector/type/窗口尾边界与真实 `TALK2.DAT@0x74A6` 跳转回放通过；全83条
    物理记录均为 raw `0x0011`、长度8且 target 有效。定向 story VM 3/3、Linux core
    186/186、Linux app 192/192 均以 exit 0 通过；Windows 依 v254 门禁分层留到剧情 VM
    P3 大阶段统一执行，未启动游戏 EXE。workpack 当前 13/146，即
    `3 assembly_exact + 10 platform_adapted + 133 pending_audit`。

- 剧情 VM P2 第十四组 `0x0042845A` / opcode18 完成独立闭环。恢复 4-byte 角色路径
    释放、bit31/type>1 slot helper 返回合同、zero-return 单次同调用重试、previous18 与
    wait 清零；typed owner 只在 chained path 实际需要时检查。四 raw alias、selector/runtime/
    短载荷边界与真实 `TALK1.DAT@0x54136` 回放通过；全301条物理记录均为 raw `0x0012`、
    长度4。定向 story VM/path owner 4/4、Linux core 186/186、Linux app 192/192 均以 exit 0 通过；
    Windows 依 v256 门禁分层留到剧情 VM P3 大阶段统一执行，未启动游戏 EXE。workpack
    当前 14/146，即
    `3 assembly_exact + 11 platform_adapted + 132 pending_audit`。

- 剧情 VM P2 第十五组 `0x004284C2` / opcode19 完成独立闭环。恢复无 operand 的全角色
    路径释放循环、role0跳过、bit31过滤、helper return忽略、按 index 顺序停止与最终
    previous19同调用继续。四 raw alias、count-one/slot/runtime/窗口尾边界与唯一真实
    `TALK2.DAT@0x10C93` 回放通过；该物理记录 raw `0x0013`、长度2。定向 story VM/path
    owner 4/4、Linux core 186/186、Linux app 192/192 均以 exit 0 通过；Windows依 v257
    门禁分层留到剧情 VM P3大阶段统一执行，未启动游戏 EXE。workpack当前15/146，即
    `3 assembly_exact + 12 platform_adapted + 131 pending_audit`。

- 剧情 VM P2 第十六组共享入口 `0x0042ADB7` / opcode20,169完成独立闭环。恢复6/12-byte
    批量路径records、schedule/wait双阶段、自修改count/phase、bit15转发、selected-role fallback、
    source-role action字段及phase-specific危险点；补齐opcode169。两种opcode各四raw alias、count0、
    selector/runtime/窗口尾与真实双样本回放通过；全607条物理指令、846个records长度公式零差异。
    定向story VM/path owner 4/4、Linux core 186/186、Linux app 192/192均以exit 0通过；Windows依v258
    门禁分层留到剧情VM P3大阶段统一执行，未启动游戏EXE。workpack当前16/146，即
    `3 assembly_exact + 13 platform_adapted + 130 pending_audit`。

- 剧情VM P2第十七组共享入口`0x00428533` / opcode21,22完成独立闭环。恢复全局bit查询的
    精确XOR反谓词、branch-only target读取、audio/offset/reload/previous顺序及no-branch窗口尾；
    两opcode各四raw alias、I/O失败与两级截断通过。全1,314条真实指令长度/target零差异，
    TALK1双样本完整回放内部前缀链。按用户要求，已审计opcode名称集中为固定底层类型
    `LegacyWorldStoryOpcode : u16`枚举，生成器同步支持enum解析。定向剧情VM 3/3、Linux core
    186/186、Linux app 192/192均以exit 0通过；Windows依v259留到剧情VM P3，未启动游戏EXE。
    workpack当前
    17/146，即`3 assembly_exact + 14 platform_adapted + 129 pending_audit`。

- 剧情VM P2第十八组`0x0042857F` / opcode23完成独立闭环。恢复FF00终止bit列表、完整
    非短路扫描、empty-list无条件跳、all-set同文件reload与clear-bit变长顺序路径；迁移旧测试
    哨兵到中性`OP_1025`。四raw alias、I/O失败、列表/sentinel/target窗口尾与真实TALK1回放通过；
    全10条资产、39个bit、7个target长度/范围零差异。定向剧情VM 3/3、Linux core 186/186、
    Linux app 192/192均以exit 0通过；Windows依v260留到剧情VM P3，未启动游戏EXE。
    workpack当前18/146，即
    `3 assembly_exact + 15 platform_adapted + 128 pending_audit`。

- 剧情VM P2第十九组`0x004285ED` / opcode24完成独立闭环。复用23的FF00列表扫描器，
    同时维护all/any谓词，恢复any-set跳转、empty/all-clear顺序路径、完整非短路扫描与
    phase-specific target读取。四raw alias、I/O失败与target窗口尾通过；完整58,782条资产
    确认opcode24零命中，因此记录`asset_absence_verified`而不虚构real replay。定向三项
    剧情VM CTest 3/3、Linux core 186/186、Linux app 192/192均以exit 0通过；Windows依v261
    留到P3，未启动游戏EXE。workpack当前19/146，即
    `3 assembly_exact + 16 platform_adapted + 127 pending_audit`。

- 剧情VM P2第二十组`0x0042865B` / opcode25完成独立闭环。恢复全局bit幂等OR置位、
    IP+4/previous/same-call合同，保留`fON`纯诊断省略；25改为语义枚举，26暂保中性枚举。
    四raw alias、已置bit、typed owner末bit、operand窗口尾及TALK1真实置位→opcode59 sound回放通过；
    全635条资产、488个不同bit、范围4..7084长度零差异。定向剧情VM 3/3、Linux core
    186/186、Linux app 192/192均以exit 0通过；Windows依v262留到P3，未启动游戏EXE。
    workpack当前20/146，即
    `3 assembly_exact + 17 platform_adapted + 126 pending_audit`。

- 剧情VM P2第二十一组`0x00428679` / opcode26完成独立闭环。恢复全局bit补码AND清除、
    同字节保留、幂等清位、IP+4/previous/same-call合同与`fOFF`纯诊断省略；26升级为语义枚举。
    四raw alias、已clear bit、typed owner末bit、operand窗口尾及TALK1真实清位→opcode59 sound回放通过；
    全312条资产、145个不同bit、范围4..7073长度零差异。定向剧情VM 3/3、Linux core
    186/186、Linux app 192/192均以exit 0通过；Windows依v263留到P3，未启动游戏EXE。
    workpack当前21/146，即
    `3 assembly_exact + 18 platform_adapted + 125 pending_audit`。

- 剧情VM P2第二十二组`0x004286C5` / opcode27完成独立闭环。修正handler半开边界为
    `0x004286C5..0x00428713`，恢复14-byte六u16、`sub_42E790`五段清理/process-bit顺序、
    后三项FFFF低16继承、旧role preload、同步world reload、IP+14/previous/same-call合同。
    SDL以pending-session双缓冲立即重绑VM span/runtime，离开旧引用作用域后再提交；连续reload、
    后续role-source patch及teardown后fatal failure均有明确owner。`dword_4C8BE0`接入持久VM state，
    item 0x0192接入player inventory。全647条资产、TALK1/2/3/4为360/6/77/204，全部raw001B、
    长度14；唯一全继承记录`TALK3@0x16095`真实回放通过。定向剧情VM 3/3、Linux core 186/186、
    Linux app 192/192均以exit 0通过；生成器`py_compile`及双重生成幂等通过。Windows依v264留到
    P3，未启动游戏EXE。workpack当前22/146，即
    `3 assembly_exact + 19 platform_adapted + 124 pending_audit`。

- 剧情VM P2第二十三组`0x00428713` / opcode28完成独立闭环。恢复6-byte selector/path-id、
    live-role旧Path payload条件释放、固定72槽首匹配扫描、type2四个link word清除、type1按方向
    步长反向减法对齐、surface清理、spatial重插与整槽reset；missing-role仅向MAPS source patch
    Path id和`0x1000` flag。保留`+4`在两支各自副作用之后的staged read顺序，以及IP+6、
    previous发布、`AIL_serve`和跨帧yield合同；SDL以窄typed port释放角色Path owner。全45条资产
    TALK1/2/3/4为7/27/2/9，全部raw001C、长度6；`TALK2@0x1938D` live-role与
    `TALK2@0x10C93` opcode19→28 missing-role真实回放通过。定向剧情VM 3/3、Linux core 186/186、
    Linux app 192/192均以exit 0通过；生成器`py_compile`及双重生成哈希幂等通过。Windows依v265
    留到P3，未启动游戏EXE。workpack当前23/146，即
    `3 assembly_exact + 20 platform_adapted + 123 pending_audit`。

- 剧情VM P2第二十四组`0x0042B074` / shared opcodes29–33完成独立闭环。恢复`s16` index和
    value/threshold、set/add/sub的32位回绕、opcode31结果符号位归零，以及所有正常路径共享的
    element0符号位归零尾；opcode32/33在先读target后按无符号`>=`/`<=`执行同文件跳转并同调用
    继续。`index>=64`严格保留不读target、不推进、不执行共享clamp、发布previous、audio service
    与yield/retry；负index只在原始首次数组越界点由typed owner隔离，32/33仍保留先读target顺序。
    全44条资产按29/30/31/32/33=`9/22/0/8/5`，index仅`0,2,41,50,62`；TALK1真实opcode30与
    `33→32→29→FFFF`链回放通过，opcode31资产缺席另有synthetic覆盖。二级入口`0x0042B070`的
    181–185未继承关闭。定向剧情VM 3/3、Linux core 186/186、Linux app 192/192均以exit 0通过；
    生成器`py_compile`及双重生成哈希幂等通过。Windows依v266留到P3，未启动游戏EXE。
    workpack当前24/146，即`3 assembly_exact + 21 platform_adapted + 122 pending_audit`。

- 剧情VM P2第二十五组`0x0042890F` / opcode34完成独立闭环。恢复`u16`零扩展写入共享
    `script_clock` owner，值`<=1000`保留、`>1000`按机器先写后覆零；不修改21帧分频计数或
    snapshot，固定IP+4、previous发布并同调用继续。四个TALK资产均为0条线性记录、0个entry
    probe；501处raw `0x0022`双字节序列均非已证明入口，高位alias原始出现数为0，因此使用
    asset-absence证据而不伪造real replay。四raw alias、0/1000/1001/FFFF、共享owner隔离与窗口尾
    截断synthetic覆盖通过。定向剧情VM 3/3、Linux core 186/186、Linux app 192/192均以exit 0
    通过；生成器`py_compile`及双重生成哈希幂等通过。Windows依v267留到P3，未启动游戏EXE。
    workpack当前25/146，即`4 assembly_exact + 21 platform_adapted + 121 pending_audit`。

- 剧情VM P2第二十六组`0x00428934` / opcode35完成独立闭环。恢复`u8(+2)`与共享
    `script_clock & 0xFFFF`的无符号比较，相等及更小值taken；`+3` padding从不读取，target只在
    taken后读取。no-jump即使窗口仅余3字节仍按物理长度IP+8；taken通过typed同文件loader直接
    service audio、发布目标offset/IP=0并同调用继续，checked I/O failure保留调用后previous顺序。
    四个TALK资产均为0条线性记录、0个entry probe；四种raw word共1145处字节候选全非已证明
    入口，因此使用asset-absence证据。四raw alias、clock低16屏蔽、相等taken、无padding/target
    no-jump、taken target截断及load failure均通过。定向剧情VM 3/3、Linux core 186/186、Linux
    app 192/192均以exit 0通过；生成器`py_compile`及双重生成哈希幂等通过。Windows依v268留到
    P3，未启动游戏EXE。workpack当前26/146，即
    `4 assembly_exact + 22 platform_adapted + 120 pending_audit`。

- 剧情VM P2第二十七组`0x0042896C` / opcode36完成独立闭环。恢复完整32位
    `script_clock_origin + u16 delta`按u32回绕后与完整32位`script_clock`进行无符号严格`>`比较；
    target仅在taken后读取。taken调用typed同文件loader后仍进入机器共享`+8`尾，因此从新窗口
    offset8而非offset0同调用继续；checked I/O failure同样保留loader返回后IP+8与previous发布。
    四个TALK资产均为0条线性记录、0个entry probe；四种raw word共376处字节候选全非已证明
    入口，因此使用asset-absence证据。四raw alias、threshold回绕、相等no-jump、完整clock宽度、
    branch-only target、delta/target截断、新窗口offset8及load failure顺序均通过。定向剧情VM 3/3、
    Linux core 186/186、Linux app 192/192均以exit 0通过；生成器`py_compile`及双重生成哈希幂等
    通过。Windows依v269留到P3，未启动游戏EXE。workpack当前27/146，即
    `4 assembly_exact + 23 platform_adapted + 119 pending_audit`。

- 剧情VM P2第二十八组`0x004289BE` / opcode37完成独立闭环。恢复完整32位`script_clock`
    到`script_clock_origin`的快照复制；物理记录仅两字节，无operand、audio、callback或yield，
    不修改clock与21帧divider，固定IP+2、previous发布并同调用继续。窗口IP=`0x7FFE`只剩opcode
    时仍先完成snapshot，再在下一fetch越界。四个TALK资产均为0条线性记录、0个entry probe；
    四种raw word共202处字节候选全非已证明入口，因此使用asset-absence证据。四raw alias、完整
    32位复制、clock/frame-counter隔离、同调用继续与窗口尾副作用顺序均通过。定向剧情VM 3/3、
    Linux core 186/186、Linux app 192/192均以exit 0通过；生成器`py_compile`及双重生成哈希幂等
    通过。Windows依v270留到P3，未启动游戏EXE。workpack当前28/146，即
    `5 assembly_exact + 23 platform_adapted + 118 pending_audit`。

- 剧情VM P2第二十九组`0x004289DE` / opcode38完成独立闭环。恢复四字节记录与u16 role selector；
    raw `0xFFF0`只对live lookup替换为context GUID，ordinary miss的MAPS patch仍重读原operand并仅
    执行flags `OR 0`/`AND 0x7FFF`。live path按顺序清role全部高位、清surface occupancy、用清flag
    后GUID重新取得第一个可用role的完整u32 index，再扫描72个object槽并整槽清空所有u16 index
    匹配项；修正了原实现遗漏的missing-role fallback、previous发布及replacement index低16位截断。
    typed session/surface失败保持已发生副作用顺序并归为platform adaptation。资产确认786条唯一记录、
    790个entry probe，均为raw `0x0026`/长度4；`TALK1.DAT@0x00004656`真实回放通过。四raw alias、
    FFF0 raw fallback、MAPS字段保持、受控owner隔离、surface顺序、同GUID重查、72槽首尾、u16对u32
    宽度、窗口尾与same-call continuation均通过。定向剧情VM 3/3、Linux core 186/186、Linux app
    192/192均以exit 0通过；生成器`py_compile`及双重生成哈希幂等通过。Windows依v271留到P3，
    未启动游戏EXE。现代显式opcode仍为76；workpack当前29/146，即
    `5 assembly_exact + 24 platform_adapted + 117 pending_audit`。

- 剧情VM P2第三十组`0x00428ADC` / opcode39完成独立闭环。恢复四字节记录与u16 role selector；
    raw `0xFFF0`只对live lookup替换为context GUID，ordinary miss的MAPS patch重读原operand并仅
    执行flags `OR 0x8000`/`AND 0xFFFF`。live path严格先对完整role flags置bit15，再清surface，
    返回后才把role `+0x60/+0x7C`两个one-shot dword写成`0xFFFFFFFF`；修正了原实现遗漏的
    missing-role fallback、previous发布与裸数字case。typed session/surface failure保留已发生的
    flag及部分surface副作用而不提前写one-shot。资产确认553条唯一记录、558个entry probe，
    均为raw `0x0027`/长度4；224种selector中无`0xFFF0/0xFFFE`，synthetic仍覆盖两种特殊值，
    `TALK1.DAT@0x000049F0`真实回放通过。四raw alias、MAPS字段保持、surface失败顺序、完整u32
    one-shot、窗口尾、无audio与same-call continuation均通过。定向剧情VM 3/3、Linux core
    186/186、Linux app 192/192均以exit 0通过；生成器`py_compile`及双重生成哈希幂等通过。
    Windows依v272留到P3，未启动游戏EXE。现代显式opcode仍为76；workpack当前30/146，即
    `5 assembly_exact + 25 platform_adapted + 116 pending_audit`。

- 剧情VM P2第三十一组`0x00428BA0` / opcode40完成独立闭环。恢复八字节记录与raw u16 role
    selector；不替换`0xFFF0`。live path在lookup后按`+6/+4`顺序读取tile Y/X，以u16宽度左移四位，
    依次调用路径调度与完成helper，再清role bit31；仅当raw selector等于context GUID时清两个cache
    dword。ordinary miss通过MAPS patch保存raw selector与tile坐标，flags仅`OR 0`/`AND 0xFFFF`。
    移除了原先合并两个helper的私有近似，并验证未误清`action.wait_remaining`。资产确认222条唯一记录、
    222个entry probe，均为raw `0x0028`/长度8；91种selector中无`0xFFF0/0xFFFE`，
    `TALK1.DAT@0x0000464E`真实回放通过。四raw alias、FFF0字面量、受控owner隔离、16位shift回绕、
    type2槽完成、cache条件、typed runtime failure、operand/window尾、无audio与same-call continuation
    均通过。定向剧情VM 3/3、Linux core 186/186、Linux app 192/192均以exit 0通过；生成器
    `py_compile`及双重生成幂等通过。Windows依v273留到P3，未启动游戏EXE。现代显式opcode仍为76；
    workpack当前31/146，即`5 assembly_exact + 26 platform_adapted + 115 pending_audit`。

- 剧情VM P2第三十二组`0x00428C9F` / opcode41完成独立闭环。恢复以`0xFF00FF00` dword
    sentinel结束的变长目标表；使用世界交互共享的完整u32 selector，`selector > count`回退index 0，
    而`selector == count`保留选择sentinel作为target的原BUG。选定完整u32 target后服务audio并重载
    同一TALK窗口，helper返回后清共享selector、发布previous，并从新窗口offset 0同调用继续；无顺序
    IP推进。SDL runtime直接绑定既有`LegacyWorldInteractionState::selected_choice_index`，未复制VM私有
    状态。资产确认64条唯一记录、65个entry probe，均为raw `0x0029`，target数2..10、长度14..46；
    `TALK1.DAT@0x000042E6`真实26字节记录以selector 3回放到`0x0000410C`。四raw alias、完整u32
    越界回退、equality sentinel bug、load failure副作用顺序、owner缺失、terminator截断、精确窗口尾及
    same-call continuation均通过。定向剧情VM 3/3、Linux core 186/186、Linux app 192/192均以exit 0
    通过；生成器`py_compile`及双重生成幂等通过。Windows依v274留到P3，未启动游戏EXE。现代显式
    opcode为77；workpack当前32/146，即`5 assembly_exact + 27 platform_adapted + 114 pending_audit`。

- 剧情VM P2第三十三组`0x00428D18` / 共享opcodes42/43完成独立闭环。两者均为两字节无operand
    记录：42对完整u32共享值置bit15、把受控角色action `+0x08`清零并恰好刷新一次，refresh零返回
    只保留诊断并继续；43只清bit15，不访问角色或刷新action。确认原版`dword_4A9920`的low15 dialog
    counter与bit15 interaction lock是同一owner，修复modern曾拆分的真实集成缺口：剧情VM、map-event
    写、鼠标方向门及世界移动门现均绑定`world_dialogs_.close.flagged_dialog_counter`，模块测试保留无镜像
    fallback。资产确认42/43分别84/62条唯一记录及84/62个entry probe，全部raw `0x002A/0x002B`、
    长度2，文件分布68/15/1/0与52/8/2/0；`TALK1.DAT@0x000046EE/@0x0000A164`真实回放通过。
    两组四raw alias、完整u32位保持、update失败顺序、42→43组合、各自受控owner边界与`0x7FFE`窗口尾、
    previous及same-call continuation均通过；world-interaction shared-owner定向回归通过。修复3个aggregate
    initializer warning后，联合定向4/4及最终VM 3/3均以stderr空通过；Linux core 186/186、Linux app
    192/192均以exit 0通过。生成器`py_compile`及双重生成幂等通过，workpack hash为
    `8d32ce5f29ffa215143643e8fed378407beffd7b470459f40e61938a2a03d0b8`。Windows依v275留到P3，未启动
    游戏EXE。现代显式opcode仍为77；workpack当前33/146，即
    `5 assembly_exact + 28 platform_adapted + 113 pending_audit`。

- 剧情VM P2第三十四组`0x00428DB8` / opcode44完成独立闭环。补齐此前缺失的六字节handler：
    raw `0xFFF0`先替换为context source GUID但不自修改operand，`0xFFFE`再由`sub_40C0D0`直接
    解析为受控index；ordinary selector按u16 GUID跳过bit28角色并取首个clear匹配。live path按机器
    顺序先把`u16(+4)`写入action `+0x48` wait override，再清action `+0x44` wait remaining，恰好
    refresh一次；零返回只诊断并同调用继续。ordinary miss保留selector→lookup→value→首次unsafe
    action访问顺序，typed边界在完整value读取后返回`role_not_found`，没有MAPS fallback。资产确认8条
    唯一记录及8个entry probe，均为raw `0x002C`/长度6，分布5/2/0/1；selector仅027F/0143/0019/
    0316/0027、value仅0/1/2，`TALK1.DAT@0x00041D04`真实回放通过。四raw alias、FFF0/FFFE、bit28
    首匹配、u16字段宽度、refresh失败、两级operand截断、missing/受控owner边界、`0x7FFA`精确窗口尾、
    previous、无audio与same-call continuation均通过。最终定向剧情VM 3/3、Linux core 186/186、
    Linux app 192/192均以exit 0通过；生成器`py_compile`及双重生成幂等通过，workpack hash为
    `31ae91228bd780b83f3d0368c8e5019b61cbc21ad5fe707f9e8db3e23700e1e8`。Windows依v276留到P3，
    未启动游戏EXE。现代显式opcode为78；workpack当前34/146，即
    `5 assembly_exact + 29 platform_adapted + 112 pending_audit`。

- 剧情VM P2第三十五组`0x00428E52` / opcode45完成独立闭环。六字节handler现按机器顺序先解析
    角色，再读取并零扩展写入完整u32 action id；live role只在紧邻raw opcode精确为10/11/45且
    selector解析到同一index时合并action refresh，next alias与`0xFFF0`均不继承current handler的
    归一化，refresh零返回只诊断，随后完整u32 flags OR `0x1000`。missing live role经MAPS source仅
    patch action id与flags。REVIEW同时修正共享`sub_42E740`在opcodes10/11中的旧checked近似：found
    path会在当前action字段写后强制读取next opcode，recognized next还会读取selector；任一窗口越界
    都保留已完成写入但阻断refresh/flags/IP/previous。资产确认65条唯一记录、68个entry probe，均为
    raw `0x002D`/长度6，文件分布47/1/14/3；真实lookahead含9次same-role合并及5次recognized但
    different-role refresh，`TALK1.DAT@0x000051C9`真实回放通过。四raw alias、FFF0/FFFE、bit28
    首匹配、45→10/11/45合并、next alias/FFF0、refresh失败、MAPS fallback、分阶段operand截断、
    found/missing两类窗口尾、受控owner、previous、无audio与same-call continuation均通过；共享
    opcode10/11窗口尾回归也通过。最终剧情VM定向3/3、Linux core 186/186、Linux app 192/192均
    以exit 0通过；app仅保留既有ALSA开发库warning。生成器`py_compile`及双重生成幂等通过，workpack
    hash为`877dc274b26c9b6befd67adbbf23c37c2f4e885190c4852c5ec64953e52a86e7`。Windows依v277
    留到P3，未启动游戏EXE。现代显式opcode仍为78；workpack当前35/146，即
    `5 assembly_exact + 30 platform_adapted + 111 pending_audit`。

- 剧情VM P2第三十六组`0x00428F7B` / 共享opcodes46–49完成独立闭环。四条4字节记录均把
    selector原样交给lookup：`0xFFF0`是ordinary字面GUID，`0xFFFE`由helper选择受控index，ordinary
    GUID跳过bit28并取首个clear匹配。opcode46无条件把三个pending u32复制到active action字段，
    即使值为`FFFFFFFF`也照常覆盖，再精确复用`sub_40DC00`清三个pending dword、三个wait word、
    command cursor与external mode；47/48只在各自pending非`FFFFFFFF`时复制完整u32并清pending，
    49严格只把u16 wait override写为`FFFF`。四条无论条件写是否发生都恰好refresh一次，零返回只
    诊断并保留效果；随后推进4字节、发布previous并同调用继续，无MAPS fallback、audio或yield。
    ordinary miss在各分支首次unsafe action访问处typed-stop，selector截断、受控owner和`0x7FFC`
    精确窗口尾顺序均已锁定。TALK目录对四条均为0条物理记录/0个entry probe，raw及高位alias的
    零散byte-word候选未被伪报为入口，闭环标记为`asset_absence_verified`而不伪造real replay。
    四raw alias、FFF0/FFFE、bit28首匹配、pending有/无、精确字段宽度、四类missing/tail、refresh
    失败、previous、无MAPS与无audio回归均通过。最终剧情VM定向3/3、Linux core 186/186、Linux
    app 192/192均以exit 0通过；app仅保留既有ALSA开发库warning。生成器`py_compile`及双重生成
    幂等通过，workpack hash为`b52f7cc28d9f578155b5b0a9289ab4a67ac199071c7bb6ce678057ae0ca5f2f0`。
    Windows依v278留到P3，未启动游戏EXE。现代显式opcode为82；workpack当前36/146，即
    `5 assembly_exact + 31 platform_adapted + 110 pending_audit`。

- 剧情VM P2第三十七组`0x00429066` / 共享opcodes50、70、73完成独立闭环。原有C++仅接入
    opcode70且提前整段验长、预夹取目标并拒绝zero map；现按LST恢复relative tile、absolute target
    tile和role-centered viewport三分支。最终REVIEW另纠正opcode70必须先读完X/Y target word再写remaining，
    Y target截断不得保留新X。共同路径严格按X/Y分阶段写tile displacement与raw u16 step，
    再转wrapping pixel、signed IDIV检查整除、非整除回退4、最后定方向；非零位移配zero step在两项
    pixel remaining/raw step已经写入后由`camera_step_divide_by_zero`隔离CPU fault。viewport四边与
    map pixel边界夹取发生在step确定后，不重算step，zero map仍保留原wrap。opcode73不替换FFF0，
    lookup miss在viewport copy之后的role coordinate unsafe点typed-stop；共同clamp诊断还保留原版把
    8-byte记录`+8`误读为下一opcode word的条件window边界。TALK目录锁定17/62/34条物理记录与
    同数entry probe，合计113/113，全部基础raw、长度10/10/8；三条TALK1代表性记录真实回放通过。
    三opcode四raw alias、已有movement覆盖、relative/absolute/role target、FFF0/FFFE/bit28、zero axis、
    X/Y divide fault、四边clamp、zero map、分阶段operand截断、两类opcode73精确窗口尾、owner顺序、
    previous、无MAPS/audio/yield均通过。最终剧情VM定向3/3、Linux core 186/186、Linux app 192/192
    均exit 0；app仅保留既有ALSA开发库warning。生成器`py_compile`及双重生成幂等通过，workpack
    hash为`99403c210d8784b38c4d8a31478598023370762ff6d1a66e93e6d72884b412b4`。Windows依v279
    留到P3，未启动游戏EXE。现代显式opcode为84；workpack当前37/146，即
    `5 assembly_exact + 32 platform_adapted + 109 pending_audit`。

- 剧情VM P2第三十八组`0x00429362` / opcode51完成独立闭环。LST按remaining X、remaining Y、
    step X、step Y四个dword顺序短路；任一非零不推进并让出，四者全零才推进2字节并同调用继续。
    两路都经过共同join发布normalized previous。原C++的四字段谓词与推进语义已正确，但等待和完成
    两路均漏发previous；现已按机器共同出口补齐，并保留`camera_pan` owner缺失在首次状态读取前
    typed-stop。四raw alias、四字段各自非零、全零续取、owner缺失、`0x7FFE`等待/完成两类精确尾、
    previous与无audio副作用均通过；`TALK1.DAT@0x000046C2`真实记录等待→完成回放通过。TALK目录
    锁定108条物理记录/108个entry probe，分布35/25/9/39，全部raw `0x0033`、长度2；高位alias
    字样为0且未伪造资产入口。最终剧情VM定向3/3、Linux core 186/186、Linux app 192/192均
    exit 0；app仅保留既有ALSA开发库warning。生成器`py_compile`及双重生成幂等通过，workpack
    hash为`01670e5c0f0423325eaa4d91afc81dcf8e6d541fc28c23844616948075f9193a`。Windows依v280
    留到P3，未启动游戏EXE。现代显式opcode仍为84；workpack当前38/146，即
    `5 assembly_exact + 33 platform_adapted + 108 pending_audit`。

- 剧情VM P2第三十九组`0x004293AC` / opcode52完成独立闭环。LST按三个current operand逐读逐写，
    三个target operand则全部读完后才按RGB写入，随后零扩展u16 duration、写countdown并依次计算
    三项`(target-current)/duration`单精度step；完整路径推进16字节、发布normalized previous并同调用
    继续。原C++预验完整记录，既丢失截断窗口的前序写入，也漏发共同出口previous；现已恢复机器顺序。
    零duration未被正规化，正差/负差/零差分别固定为x87的`+Inf/-Inf/0xFFC00000`；资产54种唯一
    delta/duration及零时长/极值共59组与宿主x87逐位比较0差异。四raw alias、signed极值、u16最大
    duration、owner缺失、operand 0..6可用、`0x7FF0`精确尾与无audio/yield均通过；
    `TALK1.DAT@0x000043B8`真实记录回放通过。TALK目录锁定1361条物理记录/1361个entry probe，
    分布733/25/174/429，全部raw `0x0034`、长度16，duration为1..46；唯一高位字样候选不是入口。
    最终剧情VM定向3/3、Linux core 186/186、Linux app 192/192均exit 0；app仅保留既有ALSA开发库
    warning。生成器`py_compile`及双重生成幂等通过，workpack hash为
    `143ccfc37a73dd2cfa9e54bc83d1844a1ecbc140d29636294b92bc3ac6be8c15`。Windows依v281留到P3，
    未启动游戏EXE。现代显式opcode仍为84；workpack当前39/146，即
    `5 assembly_exact + 34 platform_adapted + 107 pending_audit`。

- 剧情VM P2第四十组`0x0042949D` / opcode53完成独立闭环。LST把frame-color countdown按signed
    dword判断：正值不推进并让出，零或负值推进2字节并同调用继续；dispatch每轮先清ESI，等待和
    完成两路再共同发布normalized previous。原C++的signed谓词与IP语义已正确，但两路均漏发
    previous且case仍为裸数字；现已补齐。四raw alias、`1/INT32_MAX`等待、`0/-1/INT32_MIN`
    完成、owner缺失、全部颜色状态不变、无audio、`0x7FFE`等待/完成精确尾均通过；
    `TALK1.DAT@0x000043B8`真实opcode52→53序列验证先等待再完成。TALK目录锁定1360条物理记录/
    1360个entry probe，分布732/24/174/430，全部raw `0x0035`、长度2；7个高位字样候选均不是
    指令入口。最终剧情VM定向3/3、Linux core 186/186、Linux app 192/192均exit 0；app仅保留
    既有ALSA开发库warning。生成器`py_compile`及双重生成幂等通过，workpack hash为
    `0457c65d909021fa566b665082a14458122cec34feefd595709a87ef8d12363f`。Windows依v282留到P3，
    未启动游戏EXE。现代显式opcode仍为84；workpack当前40/146，即
    `5 assembly_exact + 35 platform_adapted + 106 pending_audit`。

- 剧情VM P2第四十一组`0x004294C0` / opcode54完成独立闭环。LST先读角色selector并查找，再读
    signed repeat；总刷新次数固定为`1 + max(repeat, 0)`。初始刷新前清等待与命令位置，正repeat
    的每一轮只先清等待、刷新，随后清角色动作辅助字段；刷新失败仅记诊断，循环、字段回写、IP、
    previous和同调用继续均不取消。原版忽略missing lookup并可形成`-1`索引，modern只在repeat
    可读后的首次动作写入点typed-stop，保留前序访问且不伪造fallback。四raw alias、signed非正值、
    三次刷新失败、source/受控角色selector、两级operand截断、missing角色、`0x7FFA`精确尾和
    callback刷新前后快照均通过；`TALK1.DAT@0x00005A6B`真实记录完成初始加一次重复刷新。TALK
    目录锁定256条物理记录/258个entry probe，分布99/65/80/12与99/65/82/12，全部raw `0x0036`、
    长度6，repeat为0..8；高位alias字样为0。最终剧情VM定向3/3、Linux core 186/186、Linux app
    192/192均exit 0；app仅保留既有ALSA开发库warning。生成器`py_compile`及双重生成幂等通过，
    workpack hash为`e8620a0a9f1c07e47c988bd75c6f9b5a9682e18ede15be3f79936311cbd6ab70`。Windows依v283
    留到P3，未启动游戏EXE。现代显式opcode增至85；workpack当前41/146，即
    `5 assembly_exact + 36 platform_adapted + 105 pending_audit`。

- 剧情VM P2第四十二组共享`0x004295F3` / opcodes55–57完成独立闭环。LST先保存角色flags低两位
    旧空间分组，再清低两位并分别写为1/0/2，随后用旧分组解链、按新flags分组重插；起始行严格为
    `world_y`无符号逻辑右移4后减1并按signed传入。helper在有效链中找不到角色只诊断，caller仍推进
    4字节、发布previous并跨帧让出；missing selector、owner和损坏链则在各自原unsafe点typed-stop，
    保留此前flags或已完成解链。三opcode×四raw alias、source/受控角色selector、not-found继续、
    `world_y=0xFFFFFFFF`旧行为、missing/损坏owner、解链后重插失败不回滚和`0x7FFC`精确尾均通过；
    `TALK4.DAT`四条真实记录逐条完成旧组到新组迁移。资产锁定4条物理记录/4个probe，55/56各1条、
    57两条，全部raw低位形式、长度4；5个高位原始字样均不是入口。最终剧情VM定向3/3、Linux core
    186/186、Linux app 192/192均exit 0；app仅保留既有ALSA开发库warning。生成器`py_compile`
    及双重生成幂等通过，workpack hash为
    `00573e529a323489b77a3dff9344f3c6fdec30bf13c27d7aef47bfa29953c1f9`。Windows依v284留到P3，
    未启动游戏EXE。现代显式opcode增至88；workpack当前42/146，即
    `5 assembly_exact + 37 platform_adapted + 104 pending_audit`。

- 剧情VM P2第四十三组共享`0x0042B1F1` / opcodes58、153完成独立闭环。LST在读取任何operand
    前先分配并清零`0xA4`字节节点、初始化内嵌`0x98`字节动作记录，再按`+2/+4/+6/+8`逐word
    写入坐标、action id和base variant；全部完成后58前插主图片动作链，153前插次图片动作链。
    两路均推进10字节、发布normalized previous并跨帧让出。原C++整条预验、提前owner检查与链入，
    且漏发previous；现以未链接临时list节点保留分配/初始化/分阶段写入顺序，并只在typed-stop
    无效域释放节点、隔离unchecked malloc和平台owner。两opcode×四raw alias、完整初始化、u16
    零扩展、已有链前插、四级operand尾、owner缺失和`0x7FF6`精确尾均通过；一条主链与连续两条
    次链TALK1真实记录回放固定归属和前插顺序。资产锁定84条物理记录/88个probe，其中58为73/77、
    153为11/11，全部低位raw、长度10，高位alias字样均为0。最终剧情VM定向3/3、Linux core
    186/186、Linux app 192/192均exit 0；app仅保留既有ALSA开发库warning。生成器`py_compile`
    及双重生成幂等通过，workpack hash为
    `dc926de280fbe48bda49790b2ec97ea206087d6f2b4d5489babd59897bb93484`。Windows依v285留到P3，
    未启动游戏EXE。现代显式opcode仍为88；workpack当前43/146，即
    `5 assembly_exact + 38 platform_adapted + 103 pending_audit`。

- 剧情VM P2第四十四组`0x0042967B` / opcode59完成独立闭环。LST先读取当前样本混音等级，
    再读取u16一基声音编号，并由既有audio_video wrapper按32位回绕执行`level << 7`后signed
    `/11`，提交居中、单次音效；0编号、资源或后端失败及成功均返回0，VM忽略结果，推进4字节、
    发布normalized previous并跨帧让出。原C++的编号、yield和SDL端口已正确，但case仍为裸数字且
    漏发previous；现已补齐，并把所有同调用继续到59的组合测试最终previous改为59。四raw alias、
    `0/1/0x1234/0xFFFF`、operand截断、`0x7FFC`精确尾、六类同调用组合与TALK2/TALK3最小/最大
    观察编号真实回放均通过。资产锁定740条物理记录/740个probe，分布224/155/279/82，全部raw
    `0x003B`、长度4；93种编号范围1..656，高位alias字样为0。最终剧情VM定向3/3、Linux core
    186/186、Linux app 192/192均exit 0；app仅保留既有ALSA开发库warning。生成器`py_compile`
    及双重生成幂等通过，workpack hash为
    `5dc5f7fec17379af4ac0571e06caf2aaa3056a9183df820774e61242529da271`。Windows依v286留到P3，
    未启动游戏EXE。现代显式opcode仍为88；workpack当前44/146，即
    `5 assembly_exact + 39 platform_adapted + 102 pending_audit`。

- 剧情VM P2第四十五组`0x00429693` / shared opcodes60、61完成独立闭环。LST两路先共同清
    `dword_4C9A18` bit0：60直接推进2字节，61在bit0已清状态下以`rep stosd`清零
    `0x25800`个dword / `0x96000`字节完整16位framebuffer，再只把低字节bit0置回1；两路均
    发布normalized previous并让出。原C++的60低位flag效果与SDL清屏端口已存在，但61漏了清屏
    前清bit、两路均漏发previous且case为裸数字；现已合并语义case并恢复顺序。两opcode×四raw
    alias、初始`0xA5`其余bit保留、清屏时中间值`0xA4`、两路owner缺失、两路`0x7FFE`精确尾与
    TALK1各一条真实记录均通过。资产锁定60为21条、61为20条，共41条物理记录/41 probes，全部
    低位raw、长度2；唯一`0x403D`字样位于TALK1文件头dword目录，不是指令入口。最终剧情VM定向
    3/3、Linux core 186/186、Linux app 192/192均exit 0；app仅保留既有ALSA开发库warning。
    生成器`py_compile`及双重生成幂等通过，workpack hash为
    `853625a20a99f0d342b25d5d7478c0467dd96acd670fe71829d1a4dad8aa6909`。Windows依v287留到P3，
    未启动游戏EXE。现代显式opcode仍为88；workpack当前45/146，即
    `5 assembly_exact + 40 platform_adapted + 101 pending_audit`。

- 剧情VM P2第四十六组`0x004296DE` / opcode62完成独立闭环。LST先按selector清理旧运行角色：
    重置72槽全部关联对象、保存flags低16和Talk id、清bits14/15、清地表占用并置bit28；随后按
    map/path/X/Y/action/base/variant各自独立的`FFFF`继承规则调用`sub_40D460`写MAPS源。仅目标map
    等于当前map时，才分配清零`0xD8`临时角色，执行动作更新和地表映射，再从索引1按GUID替换或
    追加、重建空间链；flags bit9还会保留原版“填满所有空粒子槽且不清head链”的bug。初版REVIEW
    发现误写成直接yield，已按`ESI=1`修正为推进18、发布previous并同调用继续。四raw alias、非当前
    map、缺失源diagnostic、所有继承、旧角色清理、同GUID替换、缺失追加、动作失败、空间/地表/
    粒子顺序、`+2`后截断、owner缺失与`0x7FEE`精确尾均通过；TALK1真实记录回放通过。资产锁定
    443条物理记录/443 probes，TALK1/2/3/4分布`77/59/128/179`，全部raw`0x003E`、长度18。
    剧情VM定向3/3、Linux core 186/186、Linux app 192/192均exit 0，SDL主程序完成链接；未启动
    游戏EXE。生成器`py_compile`及双重生成幂等通过，workpack hash为
    `a6e90a3952acfcbe092c91a9d7fc60d7b72bc492f621466ba932f871e3ea0289`。Windows依v288留到P3。
    对外进度为已实现89/198、已验收69/198；内部workpack当前46/146，即
    `5 assembly_exact + 41 platform_adapted + 100 pending_audit`。

- 剧情VM P2第四十七组`0x00429A1B` / opcode63完成独立闭环。LST从`+4`无界扫描`FF00`
    terminator；count不超过56时先以`CFCF`清空64-word目标表，按dword对复制、读取viewport top、
    再复制奇数尾word，把`+2` prefix零扩展写入interval/remaining，最后读取left并保存top/left
    快照，cursor保持不变；推进`6+2*count`、发布previous并同调用继续。count超过56则不访问owner、
    不推进IP，只诊断、发布previous并原地yield重试。初版合成测试误把脚本terminator写成目标空值
    `CFCF`，真实TALK回放发现后已修正为`FF00`并重新REVIEW。四raw alias、0/3/56项、57项超限、
    无terminator、table/camera/scroll三个owner分阶段失败、`0x7FF8`精确尾均通过。资产锁定7条物理
    记录/7 probes，TALK1/2/3分布`2/1/4`，全部raw`0x003F`、count8、长度22；TALK1代表记录回放
    通过。剧情VM定向3/3、Linux core 186/186、Linux app 192/192均exit 0，SDL主程序完成链接；
    未启动游戏EXE。生成器`py_compile`及双重生成幂等通过，workpack hash为
    `7fdc78f5712ecc44ae558cc333ddfac520b93f91a1456ec5fdf8d95ed27d5ab3`。Windows依v289留到P3。
