# 战斗绘制附属缓冲释放 `0x00433F00`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST函数范围为`0x00433F00..0x00433F21`，入口`proc`至`endp`连续，共19行，没有外部`FUNCTION CHUNK`。

ABI为thiscall：ECX是战斗绘制owner；无栈参数。唯一直接caller是`0x00433D70`，该caller不读取返回寄存器。

唯一callee是`0x004885A0`旧内存释放入口，一处调用。

## 2. 完整行为

函数读取owner `+0x0B68`附属缓冲指针。

- 指针为零：立即返回；不调用释放入口，也不写owner；
- 指针非零：把入口snapshot传给释放入口；释放返回后才把owner字段写零；随后返回。

清零不是释放前预发布。释放调用期间，owner仍可观察到旧指针。

函数不读取缓冲内容、不计算大小、不释放行表、不修改surface尺寸、矩形或方向表，也不返回释放状态。

## 3. typed适配

现代战斗绘制owner以32位token替代旧裸指针，并借用`LegacyBattleRenderAuxiliaryBufferReleaser`执行实际释放：

```text
token = owner.auxiliary_buffer_token
if token == 0:
    return false
releaser.release(token)
owner.auxiliary_buffer_token = 0
return true
```

布尔值仅供现代caller与测试观察是否执行释放，不宣称是原EAX合同。原唯一caller忽略EAX。

releaser端口返回void，对应旧释放callee的无状态调用；无论底层释放器内部如何处理，原函数都会在调用返回后清零owner。现代实现没有提前清零、失败分支、重试或第二次释放。

## 4. 双向追溯

LST到C++：

- `0x00433F03`：snapshot附属缓冲token；
- `0x00433F09..0x00433F0B`：零值短路；
- `0x00433F0D..0x00433F13`：以snapshot调用释放端口；
- `0x00433F16`：释放返回后清零owner；
- `0x00433F20..0x00433F21`：公共返回。

C++到LST：

- 唯一字段读、唯一条件、唯一外部调用和唯一字段写均有对应指令；
- token仅替代32位裸地址，不改变零值、调用参数或写入顺序；
- 返回bool是typed观测结果，不被上层映射为原返回寄存器；
- 没有新增资源类型判断、大小检查或异常恢复。

完整正向与反向追溯未发现未解释基本块、chunk、callee或出口。

## 5. 测试与动态差分

定向测试覆盖：

- 空token不调用releaser且其他绘制字段不变；
- 非空token只释放一次并传递原值；
- releaser回调期间owner仍持有旧token；
- 回调返回后owner token清零；
- 其他绘制字段不变。

本函数不消费物理游戏资产。定向battle聚合测试与零warning目标构建通过。

当前没有可用原版owner附属缓冲分配、释放和字段联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整19行LST、typed调用顺序和固定状态已经闭环。
