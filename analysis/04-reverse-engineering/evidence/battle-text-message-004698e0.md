# 战斗文字消息入链 `0x004698E0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x004698E0..0x0046995F`，从proc到endp共67行、43条带机器码和真实助记符的实际指令、2个call、3个跳转、3个局部/返回标签和1个返回点，没有外部`FUNCTION CHUNK`。第一个call固定向`0x00487C10`申请`0x24` byte；第二个call经IAT调用`lstrlenA`。

调用目录共40个静态调用点。已关闭caller共17处：画面转场1处、动作分派5处、组A主帧3处、组B主帧2处、调试热键2处、撤退提交1处、输入分派1处、目标选择刷新2处。尚未审计的`0x00470380`、`0x0047D640`、`0x0047E070`和`0x0047E950`共23处保持原opaque边界，留到各自工作包审计，不提前推导caller状态。

## 2. 节点布局与初始化顺序

分配返回token保存在ESI后，先从栈重读16-bit类型到DX，再设置ECX为9、EAX为0，以九次`rep stosd`清零完整36-byte节点。typed实现把动态token保留为`compat::u32`，节点存储由战斗启动状态唯一承接；分配token为零时在原首次节点写入前typed-stop，并保留分配callee返回、DX低word类型、ECX=9和EAX=0。

清零后按权威顺序写入：

- `+0x04`：第一个32-bit参数；
- `+0x08`：第二个32-bit参数；
- `+0x1C`：16-bit类型，高16位保持清零结果；
- `+0x20`：文字token；
- `+0x0C`：`lstrlenA`返回的32-bit长度。

文字token只作为地址语义token传给窄端口，不转换为主机指针。文字不可访问时在原`lstrlenA`访问点typed-stop，节点此前清零和四项字段写入均保留，不执行flags或入链。

## 3. flags与bit6覆盖

节点`+0x18`先读取清零值，再与第五个参数按32-bit OR写回。随后只测试入口flags的AL bit6；该位非零时覆盖`+0x08=1`和`+0x14=0xFFFFFFE0`，否则保留原第二参数及清零的`+0x14`。不能把测试对象改成OR后的节点值，也不能把`0xFFFFFFE0`夹值或改为宿主有符号指针。

## 4. 共享单链尾插

`0x005214F8`是共享链头物理槽，由`LegacyBattleStartupResetRecord::block_5214f8[0]`唯一承接。函数以EAX读取live链头，并把ECX初始化为链头槽token；链非空时逐节点读取`+0x00`直到首个零next。空链写链头槽，非空链写当前尾节点`+0x00`，最后始终把新节点`+0x00`写零。

遍历不增加现代上限。遇到尚未物化的动态链token时在首次真实next访问typed-stop，保留已初始化的新节点、遍历次数、最后EAX/ECX和此前链内容；不伪造节点或修复原链。正常返回EAX固定为0，ECX保留链头槽token或原尾token，EDX保留节点原flags与入口flags按32-bit OR后的完整值。

战斗启动reset同时清空链头槽及typed动态节点存储，避免把上一场战斗的token映射带入下一场。

## 5. caller回收

17处已关闭caller均在原调用位置直接传入五个参数，并在子typed-stop时阻断原调用后的副作用。原opaque调用槽只改名为reserved，保持既有枚举数值，生产代码零调用；新增分配和测长服务槽追加在各枚举尾部，不平移旧ABI值。

画面转场的两个C++互斥分支对应同一汇编共享callsite，单次执行仍只发布一个节点。动作分派、组A、组B、输入、目标刷新、调试热键和撤退路径均复用同一链头与同一typed动态节点owner，不建立按caller分裂的链表副本。

## 6. 验证

定向测试覆盖36-byte全清零、字段写入顺序、空链首插、非空链尾插、bit6覆盖、零分配停点、文字访问停点、缺失链节点停点、正常返回寄存器、启动reset清节点，以及17处已关闭caller直连与reserved槽零调用。

定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码构建零warning。

原版动态分配地址、真实CP950文字地址、共享链节点、40个caller联合寄存器及未审caller状态捕获后端尚不可用，`original_diff_verified`为`blocked_runtime_oracle`。
