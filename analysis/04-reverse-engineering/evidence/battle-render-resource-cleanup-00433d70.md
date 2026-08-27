# 战斗绘制资源整体清理 `0x00433D70`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST函数范围为`0x00433D70..0x00433DB3`，入口`proc`至`endp`连续，共35行，没有外部`FUNCTION CHUNK`。

ABI为thiscall：ECX是战斗绘制owner；无栈参数。

三个直接caller为：

- `0x004518D0`尾跳wrapper；该wrapper注册为CRT退出回调，返回值被运行库忽略；
- `0x0045B630`普通调用，下一条owner无关读取覆盖EAX；
- `0x0045EA30`已关闭战斗运行时销毁直接组合typed清理，后续固定对象析构覆盖尾寄存器。

直接callee为已关闭附属缓冲释放`0x00433F00`一处，以及旧内存释放入口`0x004885A0`两处。

## 2. 完整顺序

函数严格执行三阶段：

1. 直接调用附属缓冲释放；
2. 读取owner `+0x0B44` surface行表指针；非空时释放，返回后清零；
3. 读取owner `+0x0B40`主行表指针；非空时释放，返回后清零。

每个行表分支独立。前一项为空不跳过后一项；附属缓冲释放完成后才读取surface指针，surface分支完成后才读取主表指针。

函数不清零行数、步长、surface尺寸、矩形、方向表或其他owner状态，不读取行表内容，也不根据释放结果分叉。

## 3. typed实现

现代实现直接调用已关闭`release_legacy_battle_render_auxiliary_buffer`，不保留opaque callback。两张行表沿用已关闭重建callee建立的独立`unique_ptr<u32[]>`所有权，并按surface后主表的源码顺序分别`reset()`。

现代返回`LegacyBattleRenderCleanupResult`，只记录三个分支是否持有并释放资源，方便caller与测试核对。它不宣称复现原EAX残值；三个caller均不消费该值，其中运行时销毁随后固定执行10次组A与8次组B对象析构。

关键顺序没有被RAII析构次序替代：函数体显式先执行附属缓冲callee，再检查并释放surface行表，最后检查并释放主行表。owner本身的后续析构不承担本函数语义。

## 4. 双向追溯

LST到C++：

- `0x00433D70..0x00433D73`：保存owner并直接调用已关闭附属缓冲释放；
- `0x00433D78..0x00433D80`：callee返回后读取并判断surface行表；
- `0x00433D82..0x00433D8B`：释放surface行表，返回后清零；
- `0x00433D95..0x00433D9D`：surface分支之后读取并判断主行表；
- `0x00433D9F..0x00433DA8`：释放主行表，返回后清零；
- `0x00433DB2..0x00433DB3`：公共返回。

C++到LST：

- 一个typed callee调用对应首阶段唯一call；
- 两个非空条件、两个独立释放与两个清零各有唯一指令区间；
- 行表检查没有提前到附属缓冲callee之前；
- 结果结构仅观察已执行分支，不驱动额外行为；
- 没有新增尺寸清零、方向表清零、释放失败、重试或顺序交换。

完整正向与反向追溯未发现未解释基本块、chunk、callee、字段或出口。

## 5. 测试与动态差分

定向测试覆盖：

- 三项全空时不调用releaser，三个结果均为false；
- 三项全非空时附属缓冲只释放一次并传递原token；
- 附属缓冲releaser回调期间两张行表仍同时存在，锁定首阶段顺序；
- 返回后附属token、surface行表和主行表均为空；
- 三个结果均报告释放；
- surface尺寸与方向表样本保持不变。

本函数不消费物理游戏资产。定向battle聚合测试与零warning目标构建通过。

当前没有可用原版退出回调及两条主动清理路径的owner联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整35行LST、已关闭callee回收、typed顺序和固定状态已经闭环。
