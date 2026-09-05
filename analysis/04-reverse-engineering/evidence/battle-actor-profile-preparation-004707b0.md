# 战斗角色资料准备 `0x004707B0`

历史状态：`platform_adapted`。工作包282修正资料所有权；旧callee描述与全量门不代表当前完整验收。

## 1. 完整权威范围

权威LST主体为`0x004707B0..0x0047081B`，proc至endp共54行、29条实际指令、3个call、1个跳转、1个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。三个callee分别是局部记录加载`0x00476DB0`、actor所持MON记录文本释放`0x00478220`和资料加载`0x00476A80`；并不存在第四个“解析记录”调用。

## 2. 顺序与字段

函数先以arg0加载0xA4字节局部记录，再以actor `+0x0C`的记录token释放其文本。`0x004707D1`读取局部`+0x50` DWORD，`0x004707E0`屏蔽至低16位，`0x004707E5`写调用者DWORD。`0x004707DC`读取的profile参数是局部`+0x3E`完整DWORD，不是WORD；随后加载到actor `+0x0D90`缓冲。

资料加载后检查actor profile buffer `+0x10`word。仅当该word为零时，才把局部记录`+0x34`word写回profile buffer `+0x0E`。无论是否写fallback，最后都对actor mode byte OR `0x80`。fallback路径的`0x00470801 mov cx`替换ECX低WORD，保留高半；EAX/EDX仍为加载结果。

## 3. owner、callee与stop

工作包282将profile buffer统一到组A执行状态，本函数现在直接借用该状态，不再要求final-processing资料副本。mode byte仍借用物品效果状态。三个原始callee现由typed实现执行，多余`resolve_record`及其整套构造/解析port已删除。输出借用调用者的DWORD，按原地址写入顺序操作，不再只返回一个脱离真实存储的值。缺失actor owner时在首次actor `+0x0C`读取处停止，因此保留局部记录构造callee及其返回寄存器；缺失mode owner则在最终OR处停止并保留此前全部资料副作用。

两处静态caller属于已审行动调度与脚本调度。复核时发现仍分别使用`kCallPublishScene`和`pending_4707b0`，此前“不存在旧地址调用”的结论错误。当前行动22与脚本23组A路径均已直接调用该函数，两个旧opaque槽零调用；平台绑定与全局别名仍有缺口，不能凭这些定向测试放行。

## 4. 工作包282资料所有权修正

加载目标为实际组A执行状态的`+0x0D90`缓冲；fallback的`+0x0E` WORD写入与`+0x0C`标志WORD共用同一DWORD，保留未写半字。两项正常资料测试及旧缺失视图测试已迁移。core29/ASan19定向`1/1`通过（2.91/4.76秒），无匹配诊断、diff check通过。此处只记录资料存储迁移，不代表调用ABI已正确。

后续修正已删除额外回调、恢复先输出后加载的顺序、保留profile参数完整DWORD，并在fallback写入时替换CX低半。缺失输出在`0x004707E5`停下；profile视图缺失则以空借用视图交给实际加载callee，保留callee到首次写入之前已执行的读取和分配，不提前伪造停止。加载正常返回但未触及profile（例如打开失败）时，才在调用者`0x004707F7`的读取处检查该视图。

请求携带原局部地址、输出地址和局部初始字节；零默认值仅表示未捕获，不是原版运行证据。`0x00476DBA`先消费局部`+0xA0`，`0x00476DF2..0x00476DFB`再清零29个DWORD。因此定义文件打开失败不会保留旧`+0x34/+0x3E/+0x50`，且打开时ECX已清零。core30新增测试曾误判这两点，出现四项断言失败；已按callee LST修正，不以错误预期修改实现。

core31/ASan20定向`battle.legacy_battle_setup`均`1/1`通过（3.36/5.17秒），无匹配编译或sanitizer诊断，diff check通过。覆盖profile参数`0xBEEF0007`的首个callee读取入口与内部低16位索引、输出与profile同址、fallback高半保留、输出/文本释放/profile/mode各停止前缀、局部旧文本先消费及打开失败后的正常后缀。这仍不是本工作包全量放行。

## 5. 调用方回收进度

`0x00454BA7`使用source `0x5FD`，输出地址由`0x00454B92`计算为`0x004FE5D4 + actor_index*4`；其EDX来自准备callee或扫描/选择后最后到达的寄存器值，不能统一填零。第二遍扫描不改EDX；第一遍扫描每次在`0x00454A8E..0x00454A98`或`0x00454B3D..0x00454B47`推进actor指针并覆盖EDX。零计数、扫描后计数变化、命中选择和未命中路径均需跟踪。

行动22现借用startup的实际配置/物品状态、调度器的唯一执行状态和reset输出槽。新测试以actor槽9验证两种扫描方向、四种到达路径、正常/缺失配置/缺失输出共24组输入；同时保留槽0旧正常路径与已完成分支测试。故障保留已到达的MON操作、计数和scene写入，禁止执行`0x00478B30`及后续`0x8000`置位；正常路径把profile返回EAX/EDX带入该后续callee，ECX重新设为actor。调度结果新增显式寄存器捕获标志，未观测的其他分支不冒充零值捕获。

core32曾因结果缺少ECX/EDX字段编译失败；补充字段后，core33发现测试夹具在组装MON流前以`summon_profile`覆盖了定义种子。修正真实夹具输入后，core34/ASan21定向均`1/1`通过（3.14/4.49秒），无匹配诊断，diff check通过。当时尚未运行本工作包的新全量门；当前阶段结果见下文。

`0x0046ACB2`同样写startup reset的`block_4fe5d4`槽；`0x0046AC93`已把该输出地址放入EDX。其source不是第三个candidate WORD：`0x0046AC8B`重新读取`0x0053CCE8`（首个slot WORD的符号扩展值），并在`0x0046AC9B`push。脚本现直接传入该slot值，借用startup配置/物品状态、真实输出DWORD及`group_a_actors`执行状态span。明确提供的执行状态也用于该次脚本入口的坐标视图；只提供坐标视图不能承载资料写入。SDL入口尚未接入该span，真实actor来源与生命周期仍待收敛，不能增加同步副本或从placement快照伪造执行状态。两处caller的原始局部地址和运行态联合捕获继续登记为缺失。

脚本23已修正与调用紧邻的状态访问：`0x0046AC28..0x0046AC3D`先把旧`0x0053BD54`复制到`0x0053CE6C`，再写`0x0053BD50`，不能把新actor覆盖到queued_actor_code（`0x0053BD54`）。`0x0053BD50`由`LegacyBattleDebugHotkeyStatePort`的`LegacyBattleDebugHotkeyState::committed_actor_code`持有，queued仍由final actor状态持有。脚本port现通过虚继承复用该owner，不另建标量副本。`0x0046AC59`及`0x0046ACE6`的`block_520e90`按每actor五DWORD寻址，已恢复漏掉的乘五；`0x0046AD08/0x0046AD40`为EAX等于1，而非非零；`0x0046AD59`按原actor编码8..17写状态数组，而非actor_index0..9。实现已按这些宽度和条件修正，并在对应callee返回后重新读取committed actor。索引先按32位指令计算物理地址，再解析到借用记录；测试覆盖`0x40000009/0x40000011`产生的DWORD地址回绕。`0x0046ACC0`保留的索引用于下一次记录写入及查询，不受中途全局actor改变影响。

脚本组A路径不再提前覆盖`workspace.value_b/value_c`。候选WORD读取失败保留slot、旧queued复制和新committed写入；资料及后续callee故障阻断全部未到达后缀；frame故障不恢复frame gate、不递增cursor、不执行保存ECX的正常pop。正常返回才恢复入口ECX。

core35/ASan22定向均`1/1`通过（3.28/5.05秒），无匹配编译/sanitizer诊断，diff check通过。新增54组正常输入、1组callee状态改写/地址回绕和7组故障输入；同时校正原组A旧测试。不代表整个脚本调度或工作包282已完成。

后续发现`actor_state_words`还混入了`0x00502984`发布数组的读写。现已删除该重复数组：组B发布借用既有publication owner；脚本23/58的组A状态借用动作控制区，raw actor编码加2对应其槽。调试X开关及覆盖层读取同一区域第14项，旧`toggle_53af68`副本已删除。详见[控制状态同址访问](battle-script-control-state-aliases.md)。core37/ASan24定向均1/1通过（3.25/5.10秒），无匹配诊断。

脚本9组A的控制写入及帧后缀随后也已回收，core38/ASan25定向均1/1通过（3.35/5.20秒），无匹配诊断。

脚本78控制区写入及真实记录清理随后已修正，core39/ASan26定向均1/1通过；详见控制状态同址访问证据。仍待收敛：SDL的唯一组A执行状态及其生产绑定；`shared.selected_target`其余地址归属；脚本9组B路径；其他入口中动作控制区与真正记录清理的误用。

当前冻结阶段已通过全量core 199/199、ASan 199/199、app 205/205及十轮完整core复跑，每轮199/199。仅空白格式修正后再次通过core 199/199（20.27秒）、ASan 199/199（33.97秒）、app 205/205（80.97秒），相应日志无匹配编译/sanitizer诊断；修改范围格式检查、库存双生成与暂存区检查通过。日志统一保存在`build/workpack282/commit-preparation/`，与[坐标证据第8节](battle-actor-coordinates-004783b0.md#8-按用户要求冻结已有改动并整理提交)一致。

独立只读审查覆盖98个路径及完整生产代码diff，未发现新增且未登记的确定性代码阻断。审查指出验证文档口径冲突；主Agent已核对实际完成日志并统一两份文档。此处只放行现有改动的阶段提交，不关闭工作包282，不推进281/422库存，也不宣称SDL完整战斗组合或原版动态差分通过。

## 6. 历史验证状态

测试覆盖完整三callee顺序、输出word、profile id、buffer token、fallback写入、非零profile word抑制、最终mode flag与首actor读取typed-stop前缀。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`192/422 = 183 platform_adapted + 9 assembly_exact + 230 pending_audit`，SHA256为`2d26ed039dc48da07c0929878f47fe1489c3539ee0fe943220454b5b5429cfbc`。动态差分因原版局部记录、三个callee、actor profile buffer与caller联合捕获后端缺失而登记为`blocked_runtime_oracle`。
