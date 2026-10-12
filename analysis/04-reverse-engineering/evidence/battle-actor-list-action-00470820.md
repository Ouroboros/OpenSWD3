# 战斗角色链表动作执行 `0x00470820`

状态：`platform_adapted`。以下保留历史typed实现与门禁；完整父接口族仍`pending`。
5ad2a9aa回收释放子寄存器协议与计数，当前验证见第5节。

## 1. 完整权威范围

权威LST主体为`0x00470820..0x0047088C`，proc至endp共52行、29条实际指令、3个call、3个跳转、3个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。callee为待审actor刷新`0x00470890`两条静态路径和待审selected resource释放`0x00470E20`一条路径；任一执行只调用一次刷新。

## 2. 控制流

函数先测试actor mode byte bit7。未命中时不访问链表或角色基础记录，直接调用刷新。

bit7命中且selected resource token为零时，从live actor record `+0x06`word减去primary required，保留16位回绕后按signed word比较。结果非负时保留回绕值；结果为负时先把live word夹为零，再清primary required并立即刷新返回。非负路径也在公共尾清primary required后刷新。

bit7命中且selected resource token非零时，原两个零栈DWORD不被资源释放叶读取。
成功后清selected token和primary required，再实际刷新；返回WORD不构成容量业务参数。
函数本身不改变mode bit；真实释放失败保留已到达前缀并阻断刷新。

## 3. owner、caller与stop

mode byte复用第180项物品效果owner；selected token与primary required复用第188项链表owner；live角色记录复用startup配置owner。startup reset清零链表owner。原typed登记曾保留callee兼容协议；当前释放直接借实际链与workspace，刷新父协议待迁移。

唯一caller位于已关闭组A帧`0x00456680`，已从旧整函数opaque调用改为typed直连；旧地址在源码、头文件和测试中生产调用为零。缺失mode owner时在首byte读取处停止；缺失链表或live记录时保留此前mode判断并在首次相应访问处停止。

## 4. 验证状态

测试覆盖mode关闭仍刷新、selected资源释放双零参数、释放寄存器传递、primary减法回绕、signed负值夹零、两个字段清零与startup reset。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`193/422 = 184 platform_adapted + 9 assembly_exact + 229 pending_audit`，SHA256为`36295238c0aedabbca297b2da0fdc67bbd65a102d8ec27e40eec44a434a4dcfb`。动态差分因原版actor记录、两个callee、selected resource与caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。

## 5. 释放子协议回收

5ad2a9aa删除释放子request/reply及release_calls，丢弃业务WORD后按原顺序清理并刷新。
四个新增向量验证正常零仍完成容量刷新、未知节点及erase后失败不刷新。
唯一父45703D后跳45735E重读EAX/后继ECX，不消费释放寄存器结果。
两core/两ASan、SDL链接与13源身份通过，见
[实际资源消费](battle-actor-resource-consumption-00470e20.md)。
本父其它寄存器结果、刷新请求、计数和相关Port继续pending，历史193/422不是当前进度。
