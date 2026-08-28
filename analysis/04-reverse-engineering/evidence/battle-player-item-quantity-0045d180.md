# 战斗玩家道具双数量步进 `0x0045D180`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威函数为`0x0045D180..0x0045D242`，从proc到endp完整109行、2个静态call、7个条件或无条件分支与6个局部标签，无外部FUNCTION CHUNK。

导航表登记七个唯一caller、共13处调用。其中已关闭动作分派`0x004539B0`两处、组B帧`0x004576A0`两处、结果奖励整理`0x0045E9C0`两处、消息阶段分派一处与胜利奖励两处均已删除旧地址边界并直接调用typed实现；其余四处调用属于后续尚未关闭函数，不提前改写。

两个callee分别是一次`0xB0`字节分配和一次以`node+0x0C`为目标的道具定义初始化。两者尚未关闭，继续通过动作端口保留完整调用顺序和分配EAX。

## 2. 入口、头别名与链遍历

参数一的完整dword传给新节点初始化，但查找、节点item id与零入口门只使用低16位。低word为0时不读取真实链，直接返回固定`0x004A994C`。

函数并非从`[0x004A9940]`立即开始：先把地址`0x004A9940`本身当作伪节点，比较`[0x004A9944]`的item id。因此typed共享状态显式保留头别名的item id、两项数量和定义快照。头别名命中时返回同一固定payload token，不访问真实节点。

头别名未命中后才读取共享head token，并按每个节点offset0的32位next token遍历。节点offset4是精确item id，不执行`0x3FFF`掩码。未知非零token只在首次真实节点字段访问处typed-stop，不把物理token转换为主机指针。

玩家道具节点直接复用世界道具生命周期的`LegacyWorldItemNode`和`LegacyWorldItemListState`。`legacy_token`与`legacy_next_token`只保存32位物理身份；世界生命周期释放真实玩家链后同步清head token，缺失必需根的事务性早停则保持链与head不变。

## 3. 双数量与99上限

已有节点分别读取offset`+0x08`和`+0x0A`两个u16数量，先各自按i16符号扩展，再在i32域相加：

- signed总和大于等于99时原样返回；
- 第二参数精确等于1时，低16位回绕递增offset`+0x0A`数量B；
- 其他全部值递增offset`+0x08`数量A；
- 原程序不夹值，因此负数量、`0xFFFF -> 0`回绕和高位参数均保留。

每条正常路径都返回匹配物理node token低32位加`0x0C`；头别名返回固定`0x004A994C`。

## 4. 新节点创建与故障时序

遍历到null时保持以下顺序：

1. 保存旧head token；
2. 请求分配固定`0xB0`字节；
3. 立即把分配EAX发布为共享head；
4. 从新token开始清零44个dword；
5. 写next为旧head、item id为参数低word；
6. 以`node+0x0C`和完整item参数调用定义初始化；
7. selector为1时数量B写1，并把`node+0x2C` dword的bit15置位；否则数量A写1。

分配返回0时，原程序先把全局head写0，再在第一次`rep stosd`访问零地址故障。typed实现同样先发布零head，再返回`allocation_typed_stop`；旧typed节点保留为不可达前缀。主机list节点分配失败也保留已经发布的新物理head，并以独立typed-stop报告。

## 5. caller回收

动作分派的phase-six完成路径和action-twenty-three消息路径直接发布selector-one数量；组B帧两处全目标完成路径直接发布selector-zero数量。结果整理的两项玩家奖励和动态组B固定奖励均发布selector-zero数量，保留入口或前一callee EAX高word，并在子typed-stop时阻断三项尾store。六处caller都保留子端口调用与停止时序，普通返回token按原调用点分别忽略、继续使用或成为下一奖励的陈旧高word。

caller测试确认六处源代码不再包含旧地址token；动作路径写数量B与bit15，组B路径和结果整理写数量A，并保留各自查询、表值或前一callee组合出的完整item参数。

## 6. 验证与动态差分

定向测试覆盖零item、头别名、真实next token遍历、精确item匹配、signed 99上限、selector双数量、u16回绕、新节点头插、完整参数传递、定义payload token、bit15、未知head停点、零分配发布顺序、共享世界道具链释放以及六处已关闭caller。

当前缺少原版分配器、道具定义初始化callee、真实玩家道具链、头别名相邻全局和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
