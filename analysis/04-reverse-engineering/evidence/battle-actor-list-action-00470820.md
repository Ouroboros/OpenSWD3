# 战斗角色链表动作执行 `0x00470820`

状态：`platform_adapted`。完整LST、typed实现、caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00470820..0x0047088C`，proc至endp共52行、29条实际指令、3个call、3个跳转、3个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。callee为待审actor刷新`0x00470890`两条静态路径和待审selected resource释放`0x00470E20`一条路径；任一执行只调用一次刷新。

## 2. 控制流

函数先测试actor mode byte bit7。未命中时不访问链表或角色基础记录，直接调用刷新。

bit7命中且selected resource token为零时，从live actor record `+0x06`word减去primary required，保留16位回绕后按signed word比较。结果非负时保留回绕值；结果为负时先把live word夹为零，再清primary required并立即刷新返回。非负路径也在公共尾清primary required后刷新。

bit7命中且selected resource token非零时，以两个零参数调用资源释放，随后清selected token和primary required，并把释放callee寄存器传给公共刷新。函数本身不改变mode bit。

## 3. owner、caller与stop

mode byte复用第180项物品效果owner；selected token与primary required复用第188项链表owner；live角色记录复用startup配置owner。startup reset清零链表owner。两个待审callee分别保留资源释放和actor刷新窄port。

唯一caller位于已关闭组A帧`0x00456680`，已从旧整函数opaque调用改为typed直连；旧地址在源码、头文件和测试中生产调用为零。缺失mode owner时在首byte读取处停止；缺失链表或live记录时保留此前mode判断并在首次相应访问处停止。

## 4. 验证状态

测试覆盖mode关闭仍刷新、selected资源释放双零参数、释放寄存器传递、primary减法回绕、signed负值夹零、两个字段清零与startup reset。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`193/422 = 184 platform_adapted + 9 assembly_exact + 229 pending_audit`，SHA256为`36295238c0aedabbca297b2da0fdc67bbd65a102d8ec27e40eec44a434a4dcfb`。动态差分因原版actor记录、两个callee、selected resource与caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
