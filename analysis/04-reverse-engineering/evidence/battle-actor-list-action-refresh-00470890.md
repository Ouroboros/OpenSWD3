# 战斗角色链表动作刷新 `0x00470890`

状态：`platform_adapted`。完整LST、typed实现、caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00470890..0x004708BC`，proc至endp共23行、11条实际指令、0个call、2个跳转、1个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。

## 2. 行为

入口先把actor secondary required word装入DX，保留EDX高word。required为零时立即返回，不读取live actor record，也不清字段。

required非零时从actor `+0x04`取得live记录token，对记录`+0x08`word做16位回绕减法，再按signed word比较。负结果夹为零，非负结果保留。最后清secondary required word。EAX保留第二次live记录token加载，ECX保留actor token，EDX保留入口高word与required低word。

## 3. owner与caller

secondary required复用第188项链表owner，live记录复用startup配置owner。startup reset清零secondary required。两个静态caller均位于已关闭第193项`0x00470820`，已改为typed直连；旧刷新地址在生产源码、头文件和测试中调用为零。

缺失链表owner时在首required读取处typed-stop；required非零但缺失live记录owner时，在首次actor `+0x04`读取处停止并保留DX加载。required为零路径不要求记录owner。

## 4. 验证状态

测试覆盖required为零早退、16位回绕、signed负值夹零、高word保留、字段清零、EAX/ECX/EDX返回和第193项释放寄存器衔接。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`194/422 = 185 platform_adapted + 9 assembly_exact + 228 pending_audit`，SHA256为`3b36625fb2f7b531968a90e37018c19a31aa7e49dc8c3dc09bfcb200a9ecace6`。动态差分因原版actor记录、secondary required与两个caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
