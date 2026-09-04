# 战斗组A角色元素构造 `0x0046E490`

状态：`platform_adapted`。完整LST、typed元素状态、构造顺序、vector caller边界、验证和inventory双生成均已收敛。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046E490..0x0046E4C4`，从proc到endp共31行、19条实际指令、2个call、0个跳转和1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall，ECX为尺寸`0x2F34`的组A角色对象。

调用顺序固定：

1. 保存this并调用尚未审计的基础角色构造`0x00478250`。
2. 清EAX后依次把对象`+0x2F26`与`+0x2F18`两个word写零；基础构造返回EAX因此不可见。
3. 分配固定`0x38` bytes。
4. 先把分配token写入对象`+0`，再按14个dword清零分配记录。
5. 返回完整this到EAX；ECX为rep计数零，EDX保留分配callee结果。

原函数不检查零分配；token零时已经完成基础构造、两项字段清零和对象首dword写零，随后在首次`rep stosd`写入点故障。

## 2. typed对象与停止点

`LegacyBattleActorGroupAElementState`唯一承接对象token、公共前部typed owner、`+0`附属记录token、精确56-byte记录和两个尾部word。公共构造`0x00478250`已经独立关闭，caller直接调用`initialize_legacy_battle_actor_base()`；元素端口只保留尚待独立回收的分配器。公共前部typed-stop会阻断两个尾部word清零、分配、token发布和56-byte写入。

正常分配时清完整56 bytes并返回this，ECX固定零、EDX保留分配回复。零分配时发布`description_write_typed_stop`，保留公共前部和两项字段清零及零token，但不修改既有记录字节；停止寄存器为`EAX=0`、`ECX=0x0E`和分配callee的EDX，精确对应首次真实`rep stosd`写入点。

物理对象地址和分配地址均为`compat::u32` token，不转换为主机指针。记录使用单一内嵌typed存储，不与角色startup摘要或结算字段复制。

## 3. vector caller边界

本函数没有普通call caller，只有组A编译器向量构造迭代器DATA XREF。既有`0x004517B0`包装器仍按`base,size,count,constructor,destructor`传递10项固定边界。

元素构造现已关闭，但相邻析构回调和MSVC向量异常回滚语义尚未审计；因此包装器暂保留单一vector construction端口，不提前实现只有构造半边的伪EH循环。包装器证据已更新为只等待析构和编译器边界。

## 4. 验证状态

正常测试验证公共前部64次typed写→两word清零→56-byte分配→记录清零顺序、token发布、完整this返回、ECX零和EDX保留。公共前部typed-stop测试验证后续两个word与分配均未发生；零分配测试验证公共前部和两个word与token副作用完成、56-byte旧内容不变，并锁定首次记录写入点的`ECX=0x0E`。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning。

inventory生成器连续双跑逐字节一致，正式计数为`168/422 = 159 platform_adapted + 9 assembly_exact + 254 pending_audit`，SHA256为`ecc7299e3826e585f760568696ce83324239e2784feb5466b7dccafa10552141`。原版基础构造副作用、动态分配地址、全局对象字节和MSVC向量异常回滚缺少联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
