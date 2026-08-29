# 战斗角色资源链头提交 `0x00470900`

状态：`platform_adapted`。完整LST、typed实现、caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00470900..0x0047090C`，proc至endp共8行、3条实际指令、0个call、0个跳转、0个标签、1个返回点，没有外部`FUNCTION CHUNK`。

## 2. 行为与owner

函数把actor `+0x2ECC` next resource head完整dword复制到`+0x2EC8` current resource head，相等值与零值也执行写入。EAX返回next head，ECX保留actor token，EDX保持入口值。

current/next resource head扩展在第188项唯一链表owner。已关闭第190项bit11资源链路径从旧callee改为typed直连；待审`0x00471080`caller留到所属工作包。旧地址在生产源码、头文件和测试中调用为零。

缺失actor或链表owner时在首next head读取处typed-stop，不伪造第二份资源链状态。

## 3. 验证状态

测试覆盖陈旧current被next覆盖、零next清链、相等/零语义、bit11后续资源扫描和寄存器结果。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`196/422 = 187 platform_adapted + 9 assembly_exact + 226 pending_audit`，SHA256为`61edff9d9172acbf3fe675a6ca334657d402de59dffe303618b924e2f702b412`。动态差分因原版current/next资源链头及caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
