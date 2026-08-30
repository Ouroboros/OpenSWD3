# 战斗组B角色元素构造 `0x00475560`

状态：`platform_adapted`。完整LST、typed元素状态、构造顺序、停止点、vector callback边界与验证均已收敛。

## 1. 完整权威范围与调用图

权威行为真值仅为`swd3.exe.lst`。完整主体是`0x00475560..0x00475588`，从`proc`到`endp`共22行、16条实际指令、2个call、0个跳转、0个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。

函数是thiscall，入口ECX为尺寸`0x2B28`的组B角色对象。它没有普通call caller；`0x00451810`只把函数地址作为MSVC向量构造迭代器的callback参数，LST因此登记为DATA XREF。两个callee固定为公共角色基础构造`0x00478250`和分配器`0x00487C10`。

## 2. 构造顺序与ABI

指令顺序固定为：

1. 保存ESI/EDI，把入口this保存到ESI，然后调用`0x00478250`。基础构造返回寄存器不参与后续结果。
2. 向`0x00487C10`传入固定大小`0xA4`，把分配EAX保存到EDI。
3. 依次写`ECX=0x29`、`EAX=0`，由caller回收四字节参数，再把分配token写入对象`+0x0C`。
4. `rep stosd`从分配token开始精确清零41个dword，即164 bytes。
5. 正常完成时把完整this写回EAX，恢复EDI/ESI并返回；ECX为零，EDX保留分配callee的陈旧结果。

原函数不检查零分配，也不把基础构造或分配返回值归一化为布尔值。

## 3. typed owner与停止点

`LegacyBattleActorGroupBElementState`唯一承接对象token、对象`+0x0C`资源token和精确164-byte资源记录。物理地址继续只作为`compat::u32` token；资源内容由单一typed数组承接，不复制到动作dispatch或startup摘要状态。

公共基础构造和分配器尚未独立审计，分别保留只接收对象token与固定大小的窄typed端口；没有重新引入整个构造函数opaque调用。

分配EAX为零时，原版已经完成基础构造、参数回收和对象`+0x0C`零token发布，随后在第一次`rep stosd`访问地址零时故障。modern在同一访问点发布`resource_write_typed_stop`：保留旧资源bytes不变，返回诊断寄存器`EAX=0`、`ECX=0x29`及分配callee的EDX。正常路径清满164 bytes并返回`EAX=this`、`ECX=0`和同一EDX。

同轮也修正了同型组A构造`0x0046E490`的零分配停止寄存器：其首次`rep stosd`故障点ECX为14，而不是分配callee的旧ECX；此前字段清零、零token与旧记录bytes仍保持不变。

## 4. vector callback边界

`0x00451810`仍按`base=0x00525508,size=0x2B28,count=8,constructor=0x00475560,destructor=0x00475590`调用MSVC向量构造迭代器。

构造callback与后续析构callback`0x00475590`现均已typed关闭。编译器helper的八对象前向构造、失败逆向回滚与异常传播仍未审计，因此vector包装器继续保留单一窄construction port，不提前实现不完整的主机循环或伪造EH行为。callback token继续作为编译器ABI数据存在，不代表执行原地址。

## 5. 双向追溯

- `0x00475560..0x00475562`：保存ESI/EDI并把ECX this保存到ESI；
- `0x00475564`：调用公共基础构造；
- `0x00475569..0x0047556E`：压入固定`0xA4`并调用分配器；
- `0x00475573..0x0047557C`：保存分配token，设置41-dword计数与零填充值并回收参数；
- `0x0047557F`：先把分配token写入对象`+0x0C`；
- `0x00475582`：按41个dword清零资源记录，零token时在这里typed-stop；
- `0x00475584..0x00475588`：正常路径返回this并恢复被保存寄存器。

C++到LST反向追溯覆盖全部16条实际指令、两个callee、资源token发布、164-byte清零、停止点与最终寄存器。

## 6. 验证与动态差分

定向回归覆盖基础构造到分配的调用顺序、对象token、固定大小`0xA4`、资源token发布、164-byte全清零、完整this返回、ECX归零和EDX保留。零分配回归覆盖基础构造仍发生、对象token清零、旧bytes保持、首次真实写入点停止及`ECX=0x29`；同时锁定组A同型停止点`ECX=0x0E`。

定向测试、AddressSanitizer、Linux core `188/188`与Linux app `194/194`全部通过，源码零warning。

当前没有原版八个组B完整对象、公共基础构造字节、真实分配地址、全局数组、析构回滚与MSVC向量EH联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。该阻塞不影响完整16条指令的静态闭环和Linux验证。
