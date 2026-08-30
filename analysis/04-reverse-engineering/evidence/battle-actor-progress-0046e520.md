# 战斗角色行动进度更新 `0x0046E520`

状态：`platform_adapted`。完整LST、typed进度状态、两个caller、验证和inventory双生成均已收敛。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046E520..0x0046E690`，从proc到endp共179行、118条实际指令、0个call、12个跳转和2个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall并由callee弹出一个i32参数；正常返回ECX恢复this，EDX为零。

入口依次检查状态word bit6、bit14、特殊完成dword和动作完成dword；任一门命中都不写状态并返回0。随后把u16 progress零扩展，与全局i32阈值做signed比较。

## 2. 完成路径

progress不小于阈值时先把动作完成写1并清transition值。若frame-started精确1，再清frame-started；其前读取的scene identity为零时额外清post-action值。最终固定清两项cache并把update-ready写1，返回1。

frame-started非1时不读取scene identity，也不清post-action；typed实现保持这个访问与副作用门。

## 3. 继续推进路径

progress小于阈值时，从对象首token指向记录的`+0x16`读取u16基值并右移2；delay flags低bytebit6置位时再算术右移1。调用参数精确1时追加`trunc(base/4)+1`。

三类调整保持原低32位乘法和signed向零除法：

- bit29：正向增加`30*base/100`；
- bit27：负向扣除`30*base/100`；
- bit31：再扣`(u16 multiplier*base)/100/2 + 4`。

最后按低32位顺序计算`old-progress - negative + positive + base`，只把低16位写回progress并保留owner高16位，清动作完成并返回0。第238项回收同型组B进度函数时补充高word回归，纠正早期typed实现误清高半。原版magic multiply除100与C++ signed向零结果一致；不增加夹值或饱和。

## 4. typed owner与caller回收

`LegacyBattleActorProgressState`统一承接两类角色对象中本函数触及的状态、基值和multiplier。组A逐帧状态直接使用该结构；startup的组A/组B记录各持有对应物理角色progress视图，供转场caller复用，不通过旧函数地址。

组A帧caller在AI准备后直接调用typed进度更新，只有返回1才进入后续AI协调；旧callee token生产零调用。战斗转场的一个静态callsite服务party/enemy两条循环，typed实现分别更新startup对应角色状态；原`refresh_actor_message`槽改为reserved且两条循环均零调用。转场忽略本函数返回，保持后续消息准备和计数顺序。

## 5. 验证状态

纯函数测试覆盖入口bit早退、阈值完成尾、bit29正向30%、bit31 multiplier负向调整、固定4点扣除和进度dword高16位保留。组A帧回归验证达到阈值后update-ready置位、继续AI且旧token零调用；转场两条罕见分支分别验证party/enemy状态直接更新及reserved槽零调用。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning。

inventory生成器连续双跑逐字节一致，正式计数为`170/422 = 161 platform_adapted + 9 assembly_exact + 252 pending_audit`，SHA256为`308250c723c812ae5b277abb1241c514893dc3c6a9c3a25b9836f7134527b894`。原版完整角色对象、首记录指针、全局阈值、caller寄存器和转场/逐帧共享对象后端缺少联合捕获，`original_diff_verified`登记为`blocked_runtime_oracle`。
