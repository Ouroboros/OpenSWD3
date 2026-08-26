# 战斗四队伍道具链排序 `0x0045D2A0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威函数为`0x0045D2A0..0x0045D2E9`，从proc到endp完整50行、34条实际指令、0个静态call、7个条件或无条件分支与4个局部标签，无外部FUNCTION CHUNK。

唯一caller是已关闭战斗启动协调器`0x00451B10`中的一处调用。函数不接收栈参数，依次访问固定根数组`0x004A9490..0x004A949C`的四个队伍道具sentinel链。

## 2. 四根循环与入口EAX

EDI从固定根数组首地址开始，每条链结束后加4，按signed地址比较严格小于`0x004A94A0`继续，因此固定处理四条根。

每轮先把根token读入ESI，再以`[esi]`读取sentinel的head link。原程序不检查根token是否为0；typed状态缺失对应optional根时在这次无条件解引用处typed-stop。

函数进入时不初始化EAX。第一根就在无条件解引用处故障时，EAX仍是caller传入的陈旧值。typed接口显式接收`entry_eax`；startup caller传入前一玩家道具排序的返回EAX。正常读取某根head后，EAX变为该head token：

- head为0时进入下一根；
- 单节点head的next为0时进入下一根，并暂时保留该head token；
- 多节点完成时最后一次next读取把EAX写0；
- 最终返回值由第四根的实际路径决定，caller不消费普通EAX。

## 3. 稳定升序与链内重扫

每条链把ECX视为sentinel或上一节点的next字段地址。循环读取当前节点与next；next为0时结束当前根。存在next时，读取当前与next的offset`+0x04` item id并执行u16无符号比较。

当前item id小于等于next时不交换，ECX前进到当前节点next字段。相等值保持原物理顺序，因此排序稳定。本函数没有`0x0045D250`的offset`+0x06`写零，所有selected count保持不变。

当前item id大于next时按三次link写入交换相邻节点：当前next改为next原next、next的next改为当前、上一link改为next。交换后ECX恢复当前sentinel根，只从当前队伍链head重扫，不回到四根数组首项。

Typed实现不使用库排序，不增加modern迭代上限。每次交换同时重连`legacy_next_token`并用`std::list::splice`同步对应唯一sentinel typed链的host可观察顺序，不复制节点或owner。

## 4. Typed-stop时序

- optional队伍根缺失：在该根首次无条件解引用处停止，保留之前各根已完成的排序与入口/上一根EAX；
- 非零head token未知：在初始head next读取处停止，EAX为head token；
- 非零next token未知：在next节点item id首次读取处停止，EAX为next token；本函数在该比较前没有节点写副作用；
- 已发生的前序交换不回滚。

环链与原程序一样可能不终止，不增加现代循环上限。

## 5. Caller回收与共享状态

战斗启动协调器在已关闭玩家道具排序后，删除原`post_party_phase_b` opaque枚举和端口调用，直接把前一阶段EAX传入本typed排序。四根全部完成后才继续组A资料绑定、三组比率、补位与最终收束；子typed-stop立即阻断这些后续阶段。

两项排序和战斗动作分派通过虚继承共享同一个`LegacyWorldItemListStatePort`。四个sentinel、各自节点、玩家链和全部物理token只保留一份typed存储。

## 6. 验证与动态差分

定向测试覆盖四空根、第四根单节点EAX、跨根排序、无符号稳定相等值、selected count完全不清、物理link与host顺序同步、首根缺失保留入口EAX、后根缺失保留前缀、未知head、未知next，以及startup成功直连、玩家排序停点阻断和队伍根停点阻断资料绑定。

当前缺少原版四个真实sentinel根、队伍道具链、前后全局状态和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
