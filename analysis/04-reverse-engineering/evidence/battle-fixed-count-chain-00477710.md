# 战斗固定键计数链累加 `0x00477710`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`。

## 1. 完整LST范围与ABI

权威LST函数范围为`0x00477710..0x0047777C`，从`proc`到`endp`共65个物理行、43条实际指令、1个call、4个跳转、4个局部标签和2个返回点。函数没有外部`FUNCTION CHUNK`，唯一callee是20字节分配包装器`0x00487C10`。

函数采用cdecl三参数ABI：第一个参数是链根地址，第二个参数只读取低word作为键，第三个参数按路径分别作为完整dword或低word增量。两个真实caller恰为`0x004539B0`和`0x0045AA00`各一处，均固定传入根`0x004B9F00`和增量1；caller分别在后续公共栈回收点统一回收本调用与相邻调用的参数。

## 2. 根记录与扫描顺序

入口把根保存在EBX、当前记录保存在ESI、键保存在DI。根不是纯哨兵：函数首先直接比较`word [root+4]`，相等时立即把根作为命中记录。只有不等时才循环：

1. 读取当前记录`+0x00`的完整dword next到EAX；
2. next为0时进入分配路径；
3. next非零时把ESI替换为该token；
4. 比较新记录`+0x04`的word键；
5. 不相等则继续从该记录读取next。

原函数没有长度上限、环检测或键排序。modern helper同样不增加这些条件；legacy token只在`LegacyBattleFixedObjectState`中查找，不解释为宿主指针。未知根在首次`+0x04`读取处typed-stop；未知next则在next已进入EAX后，于该记录首次`+0x04`键读取处typed-stop。

## 3. 已有记录路径

命中根或动态节点后，函数以`mov ax,[esi+6]`读取无符号word计数。该指令只替换EAX低word：

- 根首次命中时，EAX高word来自入口EAX；
- 动态节点命中时，EAX高word来自前驱next token；
- ESI始终保存记录token，不会因AX更新而被破坏。

计数以unsigned word和`0x14`比较。计数已经达到或超过20时直接返回，不读取第三参数、不写记录，ECX仍保持入口值。计数小于20时才把完整32位增量装入ECX并执行完整dword `EAX += ECX`，随后只把结果AX写回`[ESI+6]`。因此加法高位进位可改变返回EAX，但记录只保存结果低word；modern实现不饱和、不截断增量后再加，也不把门改成加法后的上限检查。

## 4. 缺键分配与故障前缀

next为0时以固定大小`0x14`调用`0x00487C10`。allocator reply的EAX、ECX进入后续寄存器线程，随后原函数立即把EDX清零，并严格按以下顺序执行：

1. 把allocator返回token写入前驱`+0x00`；
2. 对新token按`+0x00`、`+0x04`、`+0x08`、`+0x0C`、`+0x10`顺序写五个零dword；
3. 从前驱`+0x00`重新读取已链接token；
4. 只把第三参数低word装入AX；
5. 以word回绕加到新节点`+0x06`计数；
6. 把DI写入新节点`+0x04`键；
7. 以word回绕递增根`+0x04`。

链接发生在任何新节点访问之前。allocator返回0时也先把零写入前驱，随后才在原版`mov [eax],edx`访问点typed-stop。非零但不可访问的token相同。可访问前缀为0、4、8、12或16字节时，typed-stop分别保留前驱链接和此前已完成的0至4个零dword；完整20字节时才继续发布计数、键和根word递增。清零后重新读取link也保留原别名行为：allocator若返回前驱或根等已映射token，五次写入对同一物理owner生效，随后按清零后的link继续。

## 5. 唯一owner与平台适配

`0x004B9F00`根的五个物理dword与`0x004776F0`固定对象清零helper复用同一个`LegacyBattleFixedObjectStatePort` owner。动态20字节节点也存入该owner的`fixed_count_nodes`，没有在动作分派、最终角色状态或SDL profile中建立第二条影子链。

`LegacyBattleActionDispatchPort`虚继承固定对象owner和窄分配端口。默认分配适配仅把大小20及入口EAX/ECX/EDX转发给既有`0x00487C10`平台边界；非零token映射为20个可访问字节，零token保持不可访问。SDL现有战斗脚本allocator按会话从确定token序列返回值，战斗初始化同时重置该序列；当前生产脚本仍在既有整帧边界内，不为本函数新增SDL侧影子状态。

## 6. caller回收

动作主分派`0x004539B0`的case 6完成路径在目标阶段返回1并完成呈现门调用后，先查询目标动作键，再直接组合typed计数链helper。子helper typed-stop立即阻断packed actor计数、攻击顺序移除、选择状态和玩家物品数量后缀；成功后才继续原后缀。源码不再调用`0x00477710`，缺键时仅通过allocator适配调用`0x00487C10`。

最终角色步进`0x0045AA00`的组B路径在描述符和动作查询后直接组合同一helper。typed-stop阻断攻击顺序移除、处理计数、终止门和组B reset后缀；成功后才继续。两个caller固定传入delta 1，并共享同一端口物理owner，因此同键在后续调用命中既有记录而不会再次分配。

待审`0x00477780`和`0x00477800`虽访问相邻或同根链状态，本包没有修改其边界或提前解释其语义。

## 7. 双向追溯

- `0x00477717..0x00477722`：读取word键并先比较根；
- `0x00477724..0x00477730`：按`+0x00`扫描next并比较动态节点键；
- `0x00477732..0x00477749`：读取计数、unsigned 20门、完整dword增量和低word回写；
- `0x0047774A..0x00477753`：分配20字节、清EDX并先发布前驱link；
- `0x00477755..0x00477763`：五个dword顺序清零；
- `0x00477766..0x00477775`：重读link、低word增量、写键并递增根word；
- `0x00477779..0x0047777C`：公共寄存器恢复和plain返回。

C++到LST反向追溯只包含根首比较、next顺序扫描、已有记录门、一次窄分配、先链接、五次顺序清零、重读link、两次word写和一次根word递增。没有额外nil防护、环上限、排序、饱和、加法后门、节点预初始化或成功伪造。

## 8. 验证与动态差分

独立测试覆盖根命中、动态节点命中、unsigned计数20门、已有路径完整32位增量与低word回写、新建路径只装载增量低word、allocator寄存器线程、零分配、未知next、五个清零写入点的全部typed-stop前缀，以及根/节点物理内容。

两个caller聚合测试验证旧`0x00477710`端口调用为零、仅缺键调用一次`0x00487C10`、共享节点内容和后缀执行/阻断顺序。完整Linux与AddressSanitizer门见模块记录。

当前缺少原版20字节动态键链、allocator堆状态、两个caller剩余callee副作用及EAX/ECX/EDX联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`；该缺口不阻止完整LST静态闭合和Linux门禁。
