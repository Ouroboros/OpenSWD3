# 战斗角色资源链查询 `0x00470910`

状态：`platform_adapted`。完整LST、typed实现、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00470910..0x00470A05`，proc至endp共132行、86条实际指令、2个call、17个跳转、8个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。内部资源链头提交`0x00470900`已关闭，字符串复制改为typed string结果。

## 2. 遍历与陈旧分支

入口先提交next资源链头到current，并把输出flag word清零。category 0、1、2、3分别映射到掩码0x10、0x0C、0x1001、0x0800，其余值原样作为掩码。

每轮先解引用current的next并把actor current破坏性推进，再筛选category、mode bits 0/2和signed派生数量。派生扫描值按`signed word0A - unsigned word06 + signed word08`计算，仅正值递增匹配计数；但计数比较在筛选块外无条件执行。因此occurrence为零时，首个不匹配节点仍进入成功分支。成功分支只重查category与mode，不重查派生正值，并无论是否复制都返回1。

成功复制按16位回绕计算输出数量，flag先置bit15。节点bit14有效时，把低14位阈值与live actor record `+0x08` signed word比较，阈值更大则再置bit14。链尽返回0并清flag。

## 3. owner与验证

资源节点字段与current/next链头扩展于第188项唯一链表owner，live容量复用startup配置owner。权威LST无静态直接caller；后续间接或待审路径按所属工作包接入。

测试覆盖链头提交、破坏性推进、掩码/mode筛选、signed扫描派生、16位输出回绕、bit15/14、字符串复制、链尽失败及occurrence零陈旧成功。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`197/422 = 188 platform_adapted + 9 assembly_exact + 225 pending_audit`，SHA256为`c58f54c211943da1d68a40531cfd6f1749b5eac340567c397a7cdb2c13ff7191`。动态差分因原版资源链、live容量、字符串输出与调用寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
