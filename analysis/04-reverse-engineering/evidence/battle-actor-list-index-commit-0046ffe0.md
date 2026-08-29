# 战斗角色列表索引提交 `0x0046FFE0`

状态：`platform_adapted`。完整LST、typed状态、寄存器测试、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x0046FFE0..0x0046FFEC`，从proc到endp共8行、3条实际指令、0个call、0个跳转、0个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。

函数是无栈参数thiscall。第一条读取actor dword `+0x2EC4`到EAX，第二条把完整EAX写入actor dword `+0x2EC0`，随后普通`retn`。没有比较、低位截断、条件跳过或其他副作用；即使两个值相等也必须执行写入。

## 2. ABI与typed-stop

正常返回EAX为next list index，ECX保持actor token，EDX保持入口。actor token或typed owner缺失时，在第一条`+0x2EC4`读取处停止，完整保留入口EAX/ECX/EDX且不写current index。

两个dword放入第184项每actor动作执行owner，不创建独立影子状态。

## 3. caller边界

全程序九个静态caller分布在六个函数。已关闭列表内容和窄网格caller各有两处；其余五处位于待审第188至191项actor/list函数。当前工作包机械锁定所有调用均以目标actor token置ECX，且callee返回后不以额外参数解释该写入。

待审caller将在所属工作包直接调用本typed原语。已关闭两个上层函数当前没有暴露actor物理owner绑定，相关四处生产回收登记为caller边界缺口，不伪造第二份actor数组。

## 4. 验证状态

单元测试覆盖首读取typed-stop、任意32位完整复制、EAX/ECX/EDX及相等值仍写。

验证结果：定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`186/422 = 177 platform_adapted + 9 assembly_exact + 236 pending_audit`，SHA256为`ff1f2a148d947032f7b568f189054169c61541330ff2c814479697d9db0272b7`。动态差分因原版actor索引状态与九处caller联合捕获后端缺失而登记为`blocked_runtime_oracle`。
