# 战斗玩家道具链排序 `0x0045D250`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威函数为`0x0045D250..0x0045D292`，从proc到endp完整42行、26条实际指令、0个静态call、6个条件或无条件分支与3个局部标签，无外部FUNCTION CHUNK。

唯一caller是已关闭战斗启动协调器`0x00451B10`中的一处调用。函数不接收参数，不调用其他函数，只读写玩家道具head与节点next、item id和selected count。

## 2. 入口与单节点早退

入口先读取玩家道具head token到EAX，再把ECX设为固定head地址：

- head为0时直接返回EAX 0；
- head非0时立即读取首节点next；未知物理token在这次真实读取处typed-stop，EAX仍为head token；
- 首节点next为0时直接返回入口head token，并且不清首节点selected count。

因此单节点链不是普通循环的一次迭代，不能现代化为“遍历时顺便清全部节点”。

## 3. 比较、清零与稳定顺序

循环把ECX视为“指向当前link的地址”：它最初是全局head地址，正常前进后变为上一节点的next字段地址。每轮按以下顺序执行：

1. 从当前link读取当前节点token；
2. 从当前节点读取next token并写入EAX；
3. next为0时立即返回EAX 0；
4. 读取当前节点item id到DI；
5. 把当前节点offset`+0x06`的selected count写0；
6. 读取next节点item id并执行u16无符号比较。

当前item id小于等于next时不交换，ECX前进到当前节点next字段。相等值走同一路径，因而排序稳定。已经升序的链只清最终尾节点之前的每个selected count；最终尾节点保持原值。

next token未知时，typed-stop发生在next节点item id首次真实读取处。此前当前节点selected count已经写0，链结构尚未修改。

## 4. 相邻交换与全链重扫

当前item id大于next时，函数按原三次link写入做相邻交换：

1. 当前节点next改为next节点原next；
2. next节点next改为当前节点；
3. 上一link改为next节点。

交换后ECX不是停在交换位置，而是无条件重置为固定head地址，从全链开头重新扫描。因此实现不替换成一次稳定排序库调用，也不增加modern迭代上限；环链与原程序一样可能不终止。

Typed实现以`compat::u32`保存物理token，不转换为主机指针。每次交换同时重连`legacy_next_token`并用`std::list::splice`同步唯一typed玩家道具链的可观察顺序，不复制节点、定义快照或description owner。

## 5. 返回寄存器与caller回收

空链返回EAX 0，单节点链返回入口head token，多节点正常结束时返回最后一次next读取的0。typed-stop结果额外记录真实故障token；caller原本不消费正常EAX。

战斗启动协调器在初始组A角色配置后，已删除原`post_party_phase_a` opaque枚举和端口调用，直接调用本typed排序；完成后才继续第二个尚未关闭的全局阶段。子typed-stop立即阻断第二阶段、资料绑定、补位与最终收束，保留排序前已经完成的战斗启动副作用和排序内部前缀。

启动端口与动作分派端口通过虚继承复用同一个`LegacyWorldItemListStatePort`，玩家道具head、节点和物理link只保留一份typed存储。

## 6. 验证与动态差分

定向测试覆盖空链、单节点早退、已排序尾节点不清、逆序交换、每次交换从head重扫、稳定相等值、物理link与host顺序同步、未知head首访问停点、未知next在selected count清零后的停点，以及startup直连成功与typed-stop阻断第二全局阶段。

当前缺少原版真实玩家道具链、相邻全局、寄存器与战斗启动联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
