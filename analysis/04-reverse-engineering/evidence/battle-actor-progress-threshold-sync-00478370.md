# 战斗角色行动进度阈值同步 `0x00478370`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整范围与调用关系

权威LST函数为`0x00478370..0x0047837D`，主体只有3条指令、14字节，没有分支、callee或外部`FUNCTION CHUNK`：

```text
mov ax, word ptr [0x004A74CC]
mov [ecx+0x2A12], ax
retn
```

静态交叉引用共3处：

- 战斗画面转场`0x004527E0`内的`0x00452FF6`；
- 组B战斗帧`0x004576A0`内的`0x00457F2A`；
- 同一组B战斗帧内的`0x00457FD7`。

没有间接入口、跳入中段或独立chunk到达点。

## 2. 精确语义与ABI

入口`ECX`是组A角色token。函数先从共享行动阈值`0x004A74CC`读取低16位到`AX`，再把该word原样写到角色`+0x2A12`。因此：

- 阈值只消费低16位；负值、零和大于`0xFFFF`的值都按位截断，不夹值；
- 角色进度只替换低word，高word保持原内存位形；
- 返回`EAX`保留入口高16位并用阈值替换低16位；
- `ECX`仍是入口角色token；
- `EDX`和标志位不变；
- 无参数清理、无资源操作、无x87状态变化。

modern实现为`synchronize_legacy_battle_actor_progress_threshold`。阈值直接读取已关闭发布器的唯一owner `LegacyBattleStartupState::timing.action_threshold`；角色进度直接写对应`startup.party[index].progress.progress`，没有重新推导阈值或建立平行状态。

## 3. 访问顺序与typed-stop

访问顺序严格为“阈值word读取”后“角色进度word写入”：

1. 阈值读取不可达时报告原有读取失败，角色不写；
2. 阈值读取成功后，语义结果保留所读`threshold_word`；
3. 角色写入不可达时报告原有写入失败，保留已读阈值与未修改的角色进度；
4. 写入成功时只改角色低word。

接口只借用角色和共享时序状态。删除寄存器request/reply与读取、写入、调用计数。
三个caller通过状态判断失败并阻断后缀，不再发布没有业务意义的失败寄存器数值。

没有空对象继续、默认阈值、额外校验或失败后缀。

## 4. 三个caller

### `0x00452FF6`

第二条低概率事件仅在随机值`27..32`时进入组A循环。caller先以角色为`ECX`调用`0x0047CE80`；完整`EAX==1`或角色`+0x2B04`完整dword等于1时跳过。否则重载角色`ECX`并调用本函数，入口`EAX/EDX`来自`0x0047CE80`。成功后以固定参数1直连已关闭`0x0046E520`，再扫描十槽数组并把首个零槽写为`party_index+8`。

modern删除原来错误分离的特殊字段投影，直接读取同一startup角色owner的`scene_identity`。阈值同步typed-stop会保留视觉、释放、音乐、随机与角色查询前缀，并阻断进度推进、十槽写入、后续敌方循环、消息节点和bit `0x80`发布。

### `0x00457F2A`

全组A完成循环先清完成word、defeated低word和完成槽，执行组A清理，再调用`0x00478850`。本函数入口`EAX/EDX`继承该reset返回，`ECX`重载为当前组A角色。成功后caller覆盖`AX`为完成表word，因此玩家道具参数保留本函数返回`EAX`高word。

modern先执行既有角色reset，再直接同步startup组A角色进度；同步失败保留全部清理/reset前缀，阻断完成表读取、玩家道具数量步进和循环后缀。

### `0x00457FD7`

单组A完成分支同样先清完成状态并调用`0x00478850`，随后调用本函数。之后caller完整覆盖`EAX`和`EDX`，但只覆盖`CX`，所以玩家道具参数高word来自本函数保留的角色`ECX`高word，低word来自完成表。

modern直接传递完成表中的16位道具编号。同步失败阻断完成表、玩家道具步进、
目标reset、当前组B source reset及全部公共尾。

两个道具调用点不需要寄存器高word：`0x0045D187/0045D19B/0045D1D6`仅测试、
比较与写入`BX`。新建节点时虽在`0x0045D1CF`压入完整`EBX`，随后MON加载器在
`0x00476E96..00476E9F`将编号与`0xFFFF`相与。高word不影响查找、数量或资料加载。
因此全组与单目标路径都直接传表中编号，保留真实角色和道具副作用。

三个旧`0x00478370`端口调用均已删除；生产代码没有保留地址常量或opaque fallback。

## 5. 双向追溯

LST到C++：

- `mov ax,[0x004A74CC]`对应共享阈值可达检查及低word读取；
- `mov [ecx+0x2A12],ax`对应组A角色进度写可达检查和低word替换；
- `retn`之后caller继续原有业务顺序；现代接口返回访问状态与读取的阈值；
- 三处xref分别对应转场一次、组B帧全目标一次和单目标一次直接组合。

C++到LST：

- helper没有分支以外的业务门、callee、分配、x87、阈值推导或高word写；
- 每个caller的owner、读取与写入顺序、道具编号截断和失败后缀均有LST依据；
- 阈值与角色进度没有第二份物理owner。

## 6. 验证与动态差分

当前测试验证阈值低word截断、角色高word保留、读取失败优先于写入失败、写入失败
保留已读阈值，以及转场和组B两个分支中的实际角色进度、道具数量及失败后缀。
不再断言寄存器残值或同步调用次数。

本批6并发core/ASan定向setup测试各1/1通过，SDL应用链接通过。
保留既有`legacy_battle_outcome_resolution_test.cpp:133`窄化警告，未修改该文件。
完整差异复核与`git diff --check`通过，日志保留在
`build/tmp/runtime/actor-threshold-sync-semantic-{core,asan,sdl}.log`。
历史版本的全量验证记录为core/ASan各199/199、app205/205，不作为本批验证结果。

当前缺少原版动态`0x004A74CC`、组A完整对象、异常内存页和三callsite联合寄存器/SEH捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。本批证据为静态LST、实际状态测试和三个modern caller；不宣称完成原版动态差分。
