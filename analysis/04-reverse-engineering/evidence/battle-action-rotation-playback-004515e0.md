# 战斗旋转缓存动作播放 `0x004515E0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x004515E0..0x0045171F`，从`proc`到`endp`共169行，没有外部`FUNCTION CHUNK`。ABI为thiscall，ECX指向`0x00451420`建立的扩展动作状态，callee清理两个栈参数；两个caller均位于`0x00453580`。

callee为动作更新`0x004321E0`两个callsite、已关闭literal循环平移`0x00433F70`一个循环callsite和通用blitter `0x004170E0`一个循环callsite。typed实现复用同一扩展状态、三owner/frame缓存、更新端口和closed rotation helper。

## 2. 零动作门与入口清零

入口先比较扩展状态`+0xC0`的存储动作号低word：

- 等于0：直接返回0，不清record、不更新、不访问缓存；
- 非零：先把动作record前`0x98`字节完整清零，再写`base_variant=0`和零扩展存储动作号，调用动作更新。

因此入口清零早于首更新。首更新后EAX为0时返回0，保留“已清record + 更新器写前缀”；不恢复入口record。测试锁定零动作完全不触状态，以及首更新失败保留wait字段等写前缀。

第一个栈参数只装入EBX，并在调用stdcall旋转callee前复制到ECX；closed callee不读取ECX，因此typed签名保留但不消费该snapshot。

## 3. 三个局部FFFF槽

栈上三个u16槽初始化为FFFF。每轮把`record.field_4c`低word作为索引；只有0、1、2可访问，其他值在首次局部槽读取点typed-stop。

首次遇到某索引时，函数立即把该索引写入局部槽，然后执行可选旋转和绘制。重复索引直接跳过旋转、owner访问和绘制，但仍进入等待word清零、command cursor和后续更新流程。

测试以两次frame 0证明首次draw一次、第二轮缓存跳过一次，而等待清零仍执行两次。

## 4. signed旋转方向

第二个参数按signed dword比较：

- 等于0：完全跳过rotation callee；
- 大于0：模式3 `pixels_right`，shift为原完整dword；
- 小于0：以`not; inc`做低32位取负，模式2 `pixels_left`。

负值不调用绝对值库，也不特殊修复`INT_MIN`；`INT_MIN`低32位取负仍为负，closed callee按`shift<=0`正常早退。

每个首次帧在局部槽写入后才读取owner并调用rotation。owner空在原首次解引用点停止；短图像等unsafe访问由closed callee typed-stop，阻断绘制和等待清零。magic/flag/shift门等原callee普通早退继续外层。

测试锁定右移1后首像素为12、左移1后首像素为2，以及rotation amount 0完全不调用callee。

## 5. 缓存owner与固定空tail绘制

可选旋转正常返回后，函数再次读取同一owner：

1. 解引用并发布source；
2. 重读u16帧索引与对应frame record；
3. 读取record flags、frame u16宽高；
4. 以扩展坐标减record偏移形成X/Y；
5. 以固定tail 0调用blitter。

坐标减法保持低32位回绕。modern从初始化工作包保存的mutable image span原位旋转，缓存frame source span继续指向同一字节，因此draw观察旋转后的图像。

固定空tail同时清call-local source palette与request auxiliary。indexed缓存会在palette读取处typed-stop；此时局部槽已写，wait words尚未清，完成返回尚未发布。

accepted blit才执行公共后缀，清target height、水平位移、纵向phase、opacity、RGB和跳行，保留放大位。

## 6. 等待word、动作循环与返回

每轮首次draw正常结束或重复帧跳过后，函数先比较`command_cursor`，随后无条件清零：

- `wait_remaining`（`+0x44`）；
- `wait_default`（`+0x46`）。

比较结果使用清零前的command cursor：

- cursor为0：再次完整清零前`0x98`字节record并返回1；
- cursor非零：把base variant清零、重写存储动作号，再次调用更新器。

后续更新EAX为0时返回0；此前draw、等待清零和更新器的新record前缀全部保留。测试证明第二更新失败时第一轮等待已清，但第二更新新写的wait仍保留。

正常完成的record清零总计两次：入口一次、完成一次。owner/frame/mutable缓存及扩展字段不清。

## 7. 非终止域

原循环无迭代上限。modern只在以下完整状态于循环顶部重复后返回`action_loop_nonterminating`：

- 152字节动作record；
- 三个局部槽；
- 三个owner token；
- 扩展`field_bc`；
- 更新端口完整domain token。

测试构造首次frame 0 draw、第二轮frame 0跳过、两轮等待清零与三次更新后完整状态重复；不在任意次数提前截断，也不伪造0/1成功返回。

## 8. 双向追溯

- `0x004515E0..0x0045160C`：局部FFFF初始化、存储动作零门与返回0；
- `0x0045160F..0x00451633`：入口record清零、动作写入、首更新及失败返回；
- `0x00451639..0x00451656`：参数snapshot、u16帧索引、局部槽读取与首次写入；
- `0x0045165B..0x00451678`：signed旋转量三路、owner解引用和closed rotation；
- `0x0045167D..0x004516C7`：source发布、frame record、固定空tail、偏移坐标与draw；
- `0x004516CF..0x004516F6`：等待word清零、动作重置和后续更新循环；
- `0x004516FC..0x0045171F`：更新失败返回0或完成record清零返回1。

C++到LST反向追溯覆盖169行全部基本块、三个callee、两个更新callsite、局部槽、signed方向、等待字段和0/1出口。

## 9. 验证与动态差分

在动作旋转缓存聚合测试中新增覆盖：

- 存储动作号0不清record且不更新；
- 两个唯一帧右移、draw、等待清零、双record清零和返回1；
- 负旋转模式2与精确左移像素；
- 零旋转、重复帧跳过及重复轮等待清零；
- 首更新失败保留入口清零后的更新前缀；
- 后续更新失败保留下一record前缀；
- 帧索引3首局部访问停止；
- 空owner在局部槽写后停止；
- 短literal图像传播closed callee typed-stop；
- indexed固定空tail在wait清零前typed-stop；
- 完整播放状态重复后的非终止停止。

battle聚合目标零warning构建及定向测试通过。

当前没有原版动作更新后record、三owner/frame/mutable image、局部槽、共享blitter状态和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整169行LST、两个caller及closed rotation callee已完成固定状态闭环。
