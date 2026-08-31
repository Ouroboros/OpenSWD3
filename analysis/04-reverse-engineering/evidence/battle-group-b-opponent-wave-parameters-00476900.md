# 战斗组B对手wave参数读取 `0x00476900`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威主体为`0x00476900..0x0047691D`，从`proc`到`endp`共15行、7条实际指令、0个call、0个跳转、0个局部标签和1个返回点。没有外部`FUNCTION CHUNK`，也没有直接callee。

唯一静态caller是已关闭的对手动作分派`0x00455D60`，调用站点位于其动作15初始化路径。该caller先按组B索引形成actor token，再依次压入第二输出、第一输出，把actor token写入ECX并发布phase低word `0x8019`，随后调用本函数；返回值没有成功门，两个输出直接控制后续记录动作号和wave数量。

## 2. 输入、owner与字段映射

入口约定：

- `ECX`：组B actor token；
- 第一个栈参数：special-action u16输出地址；
- 第二个栈参数：spawn-count u16输出地址；
- `EAX`：caller在phase写入前读取的完整旧phase dword；
- `EDX`入口值立即被第一个输出地址覆盖。

actor物理字段均已存在于`LegacyBattleActorGroupBElementState::action_configuration.profile_buffer`唯一owner。该40-byte buffer对应actor `+0x0D90`，因此：

- buffer `+0x20/+0x21`对应actor `+0x0DB0`的special-action word；
- buffer `+0x24`对应actor `+0x0DB4`的spawn-count byte。

不新增平行special-action、spawn-count或profile owner。caller继续把结果写入既有`LegacyBattleActionDispatchState::opponent_special_action`与`opponent_spawn_count`两个u16共享槽。

## 3. 精确访问与写入顺序

权威指令顺序为：

1. 把第一输出地址写入EDX；
2. 从actor `+0x0DB0`读取word到AX；
3. 把AX写入第一输出；
4. 从actor `+0x0DB4`读取byte并以带操作数前缀的`MOVZX AX, byte`只覆盖AX；
5. 把第二输出地址写入ECX；
6. 把AX写入第二输出；
7. `retn 8`。

modern typed函数严格保持该顺序。第一次输出成功而第二次输出故障时，special-action写入必须保留；不得为了事务性而回滚。第二个源值按byte零扩展，因此正常返回EAX高16位来自入口旧phase，低16位为`0x00xx`的spawn count，而不是保留special-action或入口AX低字。

## 4. 返回寄存器

正常返回：

- `EAX = (entry_eax & 0xFFFF0000) | zero_extended_spawn_count`；
- `ECX = second_output_token`；
- `EDX = first_output_token`。

第一输出故障前，EAX已只覆盖AX为special-action，ECX仍是actor token，EDX已是第一输出token。第二输出故障前，EAX已覆盖为零扩展spawn count，ECX已是第二输出token，EDX仍是第一输出token。

## 5. typed故障点

- actor缺失：停止在actor `+0x0DB0`首次读取点；两个输出保持不变，EAX保持入口值，ECX保持actor token，EDX已发布第一输出token；
- 第一输出缺失：停止在首次word写入点；spawn byte尚未读取，第二输出保持不变；
- 第二输出缺失：第一输出已写，spawn byte已读取并覆盖AX，ECX已发布第二输出token，随后停止；
- 有效actor的40-byte profile同时覆盖两个源范围，不增加不存在于原版的中间容量门。

## 6. caller回收

对手动作分派动作15删除旧`0x00476900`地址常量与generic opaque调用，改为直接调用typed函数。调用前仍先把phase低word写`0x8019`；actor typed-stop保留该phase前缀和两个旧共享输出，阻断wave循环及全部后缀。正常结果继续驱动既有wave循环、记录动作号、组B行动配置和最终spawn-count清零。

旧地址在production源码中为零调用。generic action port不再负责本函数两个输出的发布；组Bactor仍借用startup中的八槽生命周期唯一owner。

## 7. 验证与动态差分

纯函数测试覆盖：

- actor首次读取停止；
- 第一输出首次写入停止；
- 第二输出写入停止并保留第一输出；
- special-action word与spawn-count byte的精确偏移；
- spawn byte零扩展；
- EAX高word保留及ECX/EDX输出token返回。

caller集成测试覆盖：

- 源actor profile发布两项wave参数；
- 双wave记录生成与完成路径；
- 第9项在8条完整副作用后停止；
- 后续组B行动配置typed-stop；
- 缺失actor保留phase与旧输出前缀；
- 旧opaque地址生产零调用。

Linux core `188/188`、完整core AddressSanitizer `188/188`和Linux app `194/194`全部通过；最终日志零OpenSWD3源码warning、测试失败和sanitizer finding。app仅有既有ALSA/JACK开发库环境提示。

当前缺少原版组B actor完整对象、40-byte profile尾部、两个真实共享输出地址和唯一caller寄存器联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
