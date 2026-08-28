# 战斗过渡控制选择 `0x004694E0`

状态：`assembly_exact`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x004694E0..0x00469542`，从proc到endp共53行、31条带机器码和真实助记符的实际指令、0个静态call、6个跳转、5个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。唯一静态caller位于已关闭消息阶段的消息99分支：完成双方对象重置后调用本函数，再读取共享控制pair决定转消息100、返回消息98或继续动作初始化。

## 2. 入口短路与四行扫描

函数先读取共享控制pair高word。高word非零时仅保存/恢复ESI和EDI并立即返回；EAX、ECX、EDX、控制pair和40-word表全部保持入口值。

高word为零时，函数按4行×10列、row-major顺序扫描共享40-word表。每行开始时EAX清零、ECX指向该行首word；每个零值依次递增EAX并把ECX加2，EAX按signed小于10继续。整行全零时，EDX加20、ESI加1、EDI加10并进入下一行；四行全零时返回EAX 10，ECX和EDX都为表尾token。此路径不清共享控制pair，旧低word即使非零也原样保留。

## 3. 首个非零项提交与寄存器

发现首个非零word时，函数先把列号EAX加当前行的十倍偏移EDI形成0..39扁平索引，再把共享控制pair低word写为行号ESI。随后以扁平索引重新读取完整u16值到CX，只替换ECX低word并保留物理表地址高16位；对应表word立即清零，邻接半word不变。最后把该值写入控制pair高word。值非零时立即返回。

因此正常选中路径返回EAX为扁平索引、ECX为`0x00520000 | selected_value`、EDX为选中行首token；共享pair低word为0..3行号，高word为被消费值。原代码在无call区间内二次读取同一word，非零比较保证正常执行不进入清零后继续扫描分支；typed实现仍保留该机械顺序。

## 4. owner、caller回收与验证

40-word物理表`0x0052022C..0x0052027B`继续唯一复用`LegacyBattleStartupResetBlocks::block_52022c`的20个u32存储；低/高半word访问不建立第二份数组。已关闭动作分派case 13的前四个角色行现直接写该表，后六行由相邻物理tail承接；缺少startup owner只在首次真实前缀槽访问typed-stop。共享控制pair`0x0053BF1E..0x0053BF21`继续唯一复用`LegacyBattleTargetSelectionRuntimeState::transition_control_words`。全局重置清零同一startup表与target-selection pair。

消息99已直连本实现，并用函数返回EAX/ECX/EDX继续后续LST链；动作13生产者与消息99消费者现共享同一typed表；旧准备控制槽保留枚举数值并改名为reserved，生产代码零调用。无表项且pair全零时转消息100；选中行号作为组A角色索引，被消费值作为组B索引继续动作初始化。子函数无typed-stop或平台回调。

定向测试覆盖高word短路、首个低半word、奇数高半word、跨行扁平索引、邻接半word保留、四十项全零、陈旧低word保留、精确EAX/ECX/EDX、动作13共享前四行/后六行tail/缺owner真实访问stop、消息99空表/已完成角色/完整道具链/动态trace及主帧stop传播。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。

本函数纯粹执行固定宽度整数、半word表访问和确定性控制流，不需要平台替代。原版动态oracle仍缺少共享表/pair与caller寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
