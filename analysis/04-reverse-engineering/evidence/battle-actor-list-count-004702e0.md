# 战斗角色链表匹配计数 `0x004702E0`

状态：`platform_adapted`。完整LST、typed实现、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x004702E0..0x00470372`，proc至endp共94行、53条实际指令、1个call、17个跳转、8个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。唯一callee是已关闭索引提交`0x0046FFE0`。

## 2. 行为

category选择器映射与第188项相同：0到`0x10`、1到`0x0C`、2到`0x1001`，其他保持。type选择器0映射28、1映射31，其他保持；但实际分支仍是type等于28时接受节点27至30，否则一律只接受节点31。

函数先提交next list index，再从owner head按next token扫描至空。节点先测试mode byte与`0x05`，再测试category flags与mask。每个匹配节点直接对调用者已有byte计数器执行一次回绕递增；函数不清零计数器、不提前停止、不加载资料。链尾正常返回EAX零，ECX保持计数器token，EDX保持入口。

## 3. owner、stop与caller

actor索引、list owner与有序节点全部复用第186和188项唯一typed owner。缺失actor、owner或节点时在相应首次访问处停止，并保留此前索引提交、节点访问与byte递增。唯一静态caller位于已关闭列表内容函数；现有边界使用无地址语义计数port但尚未暴露节点物化owner，因此不复制第二份链表状态。源码、头文件与测试中不存在旧地址生产调用。

## 4. 验证状态

测试覆盖三类筛选、全链扫描、入口byte保留、`0xFF+2`回绕为1、非28选择器只认31及返回寄存器。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`189/422 = 180 platform_adapted + 9 assembly_exact + 233 pending_audit`，SHA256为`bc99f85812f3e56476185df288b9e44dec03a7c3f3ac75936cd94862c8128449`。动态差分因原版链表、调用者计数byte与caller联合捕获后端缺失而登记为`blocked_runtime_oracle`。
