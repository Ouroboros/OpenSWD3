# 战斗角色模式四收尾 `0x004708C0`

状态：`platform_adapted`。完整LST、typed实现、双caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x004708C0..0x004708F7`，proc至endp共26行、14条实际指令、0个call、1个跳转、1个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。

## 2. 行为与owner

函数先读取actor `+0x2F18`replacement action word，再对`+0x2A87`mode byte OR 2，并把`+0x2B0C`completion latch写1。replacement非零时复制到`+0x2A6C`action kind；零值保留旧action kind。最后用`rep stosd`清零actor `+0x0AF0..+0x0C1F`共0x4C dword、304字节。

replacement与latch复用第187项final state，mode和action kind复用第180项item state，304字节工作区复用第174项workspace state，未新增重复物理owner。完成路径返回EAX=0、ECX=0，EDX保留actor token。

## 3. caller与typed-stop

两个静态caller分别位于已关闭组A帧`0x00456680`和目标选择刷新`0x00462740`，均已改为typed直连。startup party span沿frame/input/message/target bindings传递，仅引用唯一party owner。旧地址在生产源码、头文件和测试中调用为零。

缺失final、item或workspace owner时依次在首次对应访问处typed-stop，并保留此前副作用；特别是缺失workspace时保留mode、latch及条件action kind写入。

## 4. 验证状态

测试覆盖mode OR、latch、非零replacement复制、304字节完整清零、寄存器结果及目标选择caller端口调用回收。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`195/422 = 186 platform_adapted + 9 assembly_exact + 227 pending_audit`，SHA256为`bc01de2599a2a2d3436786582e0de1a79c26ab46281f05ba0b236f27b25e8ba6`。动态差分因原版actor四段物理状态、两个caller及寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
