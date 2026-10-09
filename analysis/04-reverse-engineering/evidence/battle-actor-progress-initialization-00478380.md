# 战斗角色行动进度随机初始化 `0x00478380`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整范围与调用关系

权威LST函数为`0x00478380..0x004783A5`，共14条指令、38字节，没有分支或外部`FUNCTION CHUNK`。唯一callee是已关闭的第二套随机数有界入口`0x00439070`：

```text
push esi
mov esi,ecx
push 9
call 0x00439070
mov ecx,eax
mov eax,150
add esp,4
inc ecx
cdq
idiv ecx
add eax,300
mov [esi+0x2A12],ax
pop esi
retn
```

静态交叉引用只有一处：战斗启动协调器`0x00451B10`内的`0x0045278E`。没有间接入口、跳入中段或独立chunk到达点。

## 2. 精确语义与ABI

入口`ECX`是组A角色token；入口`ESI`压栈后暂存该token。函数以固定上界9调用`0x00439070`，因此合法随机结果为`0..8`。随后执行signed整数运算：

```text
divisor = random + 1
quotient = trunc_toward_zero(150 / divisor)
value = quotient + 300
```

合法域内除数为`1..9`且均为正，故结果依次为`450, 375, 350, 337, 330, 325, 321, 318, 316`。函数只把完整结果的低16位写入角色`+0x2A12`；该字段宿主dword的高16位保持原位形。

返回寄存器为：

- `EAX`：完整`value`，不是只含低word；
- `ECX`：`random+1`；
- `EDX`：signed `idiv`余数；
- `ESI`：恢复入口值；
- `EBX/EBP/EDI`：未修改；
- 返回标志位来自最后一次`add eax,300`，后续word写、`pop`和`retn`不改标志。

modern实现为`initialize_legacy_battle_actor_progress`，接收调用方已抽取的随机值，
返回访问状态与计算出的`initial_progress`，不接受Port或传递寄存器残值。
startup原位置执行一次`random_bounded(9)`，再传入数值，不建立新随机算法或并行状态。
上层排序和随机调度仍使用既有随机Port及startup适配，尚待后续迁移；本批不宣称整条
随机链已完成清理。具体入战边界见[入战接线](battle-startup-order-progress-runtime-binding.md)。

## 3. 访问顺序与typed-stop

可观察顺序严格为：保存角色token、调用一次RNG、形成除数、执行`cdq/idiv`、加300、尝试角色进度word写入。

角色写入不可达时保留原有写入失败结果：随机抽样和进度计算已经完成，语义结果保留
`initial_progress`，角色进度完全不写。caller索引超出固定组A owner时也先完成RNG
与除法，不提前截断随机副作用。没有空对象继续、默认进度、夹值、重抽随机数或失败后缀。
除数和余数不再通过公共结果传输；唯一caller在正常尾部不消费这些寄存器残值。

`random(9)`由已关闭callee保证结果小于9；modern没有为违反callee合同的伪造返回添加原程序不存在的继续路径。

## 4. 唯一caller `0x0045278E`

caller先读取补位后的组A总数`0x0053BCE4`；unsigned大于零时清`ESI`和`ECX`，按索引计算：

```text
actor = 0x005029D0 + index * 0x2F34
```

每个角色调用本函数一次。返回后立即重新读取总数到`EAX`，递增循环索引并用其低16位进行unsigned循环比较，因此本函数的`EAX/ECX/EDX`残值不流入正常尾部；最后一次迭代完成后才读取补位计数和两个尾部计数，计算返回值并可选发布共享消息`0x67`。

modern直接把同一startup组A角色owner `state.party[index].progress`交给typed helper。任一角色进度写typed-stop会保留此前全部启动副作用、已完成角色写入和当前RNG/除法前缀，并阻断后续角色初始化、尾部两次减法与`0x67`发布。

旧`finalize_party_actor` opaque调用已删除；对应枚举位置保留为`reserved_initialize_party_actor_progress`以稳定内部编号，生产与测试均验证其调用数为零。

## 5. 双向追溯

LST到C++：

- `push 9; call 0x00439070`对应一次`random_bounded(9)`；
- `mov ecx,eax; mov eax,150; inc ecx; cdq; idiv ecx`对应固定被除数、正除数和截断商；
- `add eax,300`对应计算出的行动进度；余数没有业务消费者；
- `mov [esi+0x2A12],ax`对应可达检查后的低word替换；
- 唯一xref对应startup最终组A循环的直接组合。

C++到LST：

- helper没有额外随机、重试、范围修正、完成阈值读取、分配、x87或高word写；
- startup owner、`0x2F34`索引、一次每角色调用、停止后缀和旧opaque零调用均有唯一LST依据；
- 角色进度继续与`0x00478370`同步、`0x00478340`宽度查询和`0x0046E520`推进共用同一物理owner。

## 6. 验证与动态差分

定向测试覆盖全部九个合法随机值的计算结果、实际角色高word保留、不可写及缺失角色
的失败结果。startup测试保留固定上界9的抽样轨迹、动态人数读取、四名组A角色实际
写入`450/375/350/316`、补位后的角色进入循环，以及首名写入失败阻断尾部消息发布。
删除初始化函数的随机测试Port、寄存器断言、调用及写入计数。startup按角色索引保存
语义结果，不再用调用次数作为数组索引或向外汇总次数。

当前缺少原版第二套RNG动态状态、完整组A对象、异常内存页及唯一callsite寄存器/SEH联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。原版动态验证边界没有因此扩大；本批证据为静态LST、实际状态测试及modern caller组合。

本批6并发core与ASan定向setup测试各1/1通过，SDL应用链接通过，完整差异复核及
`git diff --check`通过。保留既有`legacy_battle_outcome_resolution_test.cpp:133`窄化警告，
该文件不属于本批改动。日志为
`build/tmp/runtime/actor-initial-progress-semantic-{core,asan,sdl}.log`。
