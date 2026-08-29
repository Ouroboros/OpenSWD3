# 战斗角色资料准备 `0x004707B0`

状态：`platform_adapted`。完整LST、typed实现、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x004707B0..0x0047081B`，proc至endp共54行、29条实际指令、3个call、1个跳转、1个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。三个callee分别是待审局部记录构造`0x00476DB0`、待审记录解析`0x00478220`和待审资料加载`0x00476A80`。

## 2. 顺序与字段

函数先以arg0构造0xA4字节局部记录，再以actor `+0x0C` context token解析该记录。随后把局部`+0x50`word零扩展写调用者dword，以局部`+0x3E`word为profile id加载到actor `+0x0D90`缓冲。

资料加载后检查actor profile buffer `+0x10`word。仅当该word为零时，才把局部记录`+0x34`word写回profile buffer `+0x0E`。无论是否写fallback，最后都对actor mode byte OR `0x80`。返回寄存器保持资料加载callee结果。

## 3. owner、callee与stop

profile buffer复用第187项final owner，mode byte复用第180项物品效果owner。三个待审callee分别保留局部记录构造、记录解析和profile加载窄port，不保留整函数opaque边界。缺失actor owner时在首次actor `+0x0C`读取处停止，因此保留局部记录构造callee及其返回寄存器；缺失mode owner则在最终OR处停止并保留此前全部资料副作用。

静态caller位于待审目标选择与相邻战斗控制函数；随各caller工作包直连。生产源码、头文件和测试中不存在旧地址调用。

## 4. 验证状态

测试覆盖完整三callee顺序、输出word、profile id、buffer token、fallback写入、非零profile word抑制、最终mode flag与首actor读取typed-stop前缀。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`192/422 = 183 platform_adapted + 9 assembly_exact + 230 pending_audit`，SHA256为`2d26ed039dc48da07c0929878f47fe1489c3539ee0fe943220454b5b5429cfbc`。动态差分因原版局部记录、三个callee、actor profile buffer与caller联合捕获后端缺失而登记为`blocked_runtime_oracle`。
