# 清空玩家道具链并按名称owner、记录顺序释放 `0x0044D5A0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044D5A0..0x0044D5CD`，33行，无外部FUNCTION CHUNK。直接caller为`0x00448650`及后续`0x0044DA40`等清理路径；callee为同一循环内两处共享释放。

每轮严格执行：

1. 读取当前head。
2. 在任何释放前把head写成当前记录next。
3. 释放当前记录`+AC`名称owner。
4. 释放当前176字节记录。
5. 重新读取已发布的head；非null继续。

空链不调用释放。正常结束时head为null；最后EAX来自最后一次记录释放，但caller不观察该值，typed结果只记录节点数和释放调用数。

## 2. 适配边界

`LegacyStandardModeQuantityPorts`复用与数量更新相同的名称owner和记录生命周期。链环只在下一轮将重新处理已释放节点时typed-stop；第一次head发布和两次释放保留。自环停止时head仍指向已释放记录，精确表达原程序随后会再次读取free节点的unsafe边界，不伪造空链成功。

## 3. 验证

UT覆盖空链、两节点和自环。两节点验证head最终null、名称owner按`0x1111,0x2222`顺序释放、记录按first/second顺序释放及总计4次调用；自环验证先完成一次摘链和双释放，再以head仍指向原节点停止。

workpack双生成稳定为`186/227`，SHA256为`056d97438e933e16bc2ecc6c33011bd11a51deacaad34b8819f7a879a490e1c8`；下一项为`0x0044D5D0`。
