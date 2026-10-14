# 战斗角色链表动作执行 `0x00470820`

状态：`platform_adapted`。以下保留历史typed实现与门禁；完整父接口族仍`pending`。
5ad2a9aa回收释放子寄存器协议与计数；行动消耗父协议迁移及当前验证见第6节，源码df52c985已推送并核对远端。

## 1. 完整权威范围

权威LST主体为`0x00470820..0x0047088C`，proc至endp共52行、29条实际指令、3个call、3个跳转、3个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。callee为待审actor刷新`0x00470890`两条静态路径和待审selected resource释放`0x00470E20`一条路径；任一执行只调用一次刷新。

## 2. 控制流

函数先测试actor mode byte bit7。未命中时不访问链表或角色基础记录，直接调用刷新。

bit7命中且selected resource token为零时，从live actor record `+0x06`word减去primary required，保留16位回绕后按signed word比较。结果非负时保留回绕值；结果为负时先把live word夹为零，再清primary required并立即刷新返回。非负路径也在公共尾清primary required后刷新。

bit7命中且selected resource token非零时，原两个零栈DWORD不被资源释放叶读取。
成功后清selected token和primary required，再实际刷新；返回WORD不构成容量业务参数。
函数本身不改变mode bit；真实释放失败保留已到达前缀并阻断刷新。

## 3. owner、caller与stop

mode byte复用第180项物品效果owner；selected token与primary required复用第188项链表owner；live角色记录复用startup配置owner。startup reset清零链表owner。行动消耗与次项扣除直接借实际列表和配置记录，释放直接借实际链与workspace；不传actor token或寄存器参数。

唯一caller位于已关闭组A帧`0x00456680`，已从旧整函数opaque调用改为typed直连；旧地址在源码、头文件和测试中生产调用为零。缺失mode owner时在首byte读取处停止；缺失链表或live记录时保留此前mode判断并在首次相应访问处停止。

## 4. 验证状态

以下为历史验证，不覆盖本次协议删除：测试曾覆盖mode关闭仍刷新、selected资源释放双零参数、释放寄存器传递、primary减法回绕、signed负值夹零、两个字段清零与startup reset。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`193/422 = 184 platform_adapted + 9 assembly_exact + 229 pending_audit`，SHA256为`36295238c0aedabbca297b2da0fdc67bbd65a102d8ec27e40eec44a434a4dcfb`。动态差分因原版actor记录、两个callee、selected resource与caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。

## 5. 释放子协议回收

5ad2a9aa删除释放子request/reply及release_calls，丢弃业务WORD后按原顺序清理并刷新。
四个新增向量验证正常零仍完成容量刷新、未知节点及erase后失败不刷新。
唯一父45703D后跳45735E重读EAX/后继ECX，不消费释放寄存器结果。
两core/两ASan、SDL链接与13源身份通过，见
[实际资源消费](battle-actor-resource-consumption-00470e20.md)。
该次发布时父寄存器结果、刷新请求与计数仍pending；其后迁移见第6节，历史193/422不是当前进度。

## 6. 行动消耗父协议迁移

范围：首项`CHAIN-e0f6819f24dc`内`00470820/00470890`，及唯一组A帧caller
`0045703D`。`470890..4708BC`共23行；两个静态caller均在470820。
上层call后直接跳`45735E`重载EAX，不消费行动函数的寄存器回包。

删除ActionRequest、RefreshResult、ActionResult、寄存器入参与返回副本、
容量写入/要求清零/刷新调用计数，以及父结果内的行动回包和调用计数。
两函数只返回处理状态，实际WORD直接写startup角色配置记录；组A帧借同一party的
列表、物品效果状态、配置与workspace，失败直接停止该帧路径。

逐块合同：

- `470823..470834`：先读mode bit7；关闭时保留主要求和选择，仍扣次项。
- `470836..470865`：主WORD减要求后先16位回绕，再按signed负值清零；保留相邻WORD。
- `470866..47087B`：实际资源消耗返回正常零值也继续清选择与主要求；宿主访问失败保留前缀。
- `470884..47088C`：主项处理后仅调用一次次项扣除；不额外清mode位。
- `470890..4708BC`：次要求零时不访问配置；否则扣实际次WORD、signed夹零后清次要求。

现有typed-stop仅用于缺失实际存储，不冒充原版业务分支；删除无实际读取意义的actor token门槛。
六组独立向量覆盖正值、下溢、回绕后正值和signed边界；资源消耗后的配置缺失验证
已扣数量、已清主项与未清次项。帧测试改查真实配置和要求字段，取消调用次数断言。
没有新增操作编号、参数包、转发Port、计数器或默认成功分支。

最终core定向1/1通过（12.24秒），ASan定向1/1通过（21.56秒）；
SDL应用构建及六份源码身份检查通过。未运行游戏或新增原版动态差分。
本轮脚本实际使用默认64并发；后续使用OPENSWD3_BUILD_JOBS/OPENSWD3_TEST_JOBS=6，
CMAKE_BUILD_PARALLEL_LEVEL/CTEST_PARALLEL_LEVEL不能覆盖仓库脚本的显式并发参数。
日志在`build/tmp/runtime/actor-list-action-*`；独立LST分析与双向审查保存在同目录。
资源生产、消息内部协议和完整SDL接线仍pending；原版联合动态差分未验证。
LIST仍0项完成、537项待处理；本批不关闭整链，不改变B10/Workpack316验收。
