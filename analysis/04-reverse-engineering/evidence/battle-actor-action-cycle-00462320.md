# 战斗角色动作轮转 `0x00462320`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00462320..0x00462381`，从proc到endp共67行、30条实际指令、8个静态call、2个跳转、5个局部/默认标签、4个`retn`，没有外部`FUNCTION CHUNK`。函数后的`0x00462384..0x00462390`四项跳表也已审计。

三个静态caller均属于已关闭逐帧输入分派，地址分别位于记录2确认、记录4菜单后退和记录5菜单前进路径。第一类按起点轮转到可用动作的callee尚未关闭；第二类动作提交已由相邻工作包关闭并直接组合typed队列提交。

## 2. 入口与默认返回

函数无栈参数。入口不使用caller EAX，先从共享queued角色dword装载EAX，再把共享pre-frame gate B清零。随后EAX按u32加`0xFFFFFFF8`，即计算`queued_code-8`；unsigned结果大于3时直接返回转换后的EAX，并保持入口ECX/EDX。

因此code 7返回全1，code 12返回4；两条默认路径都已经发生gate清零。实现不把这一区间检查提前，也不将默认返回规范化为布尔值。

## 3. 四路动作起点

code 8、9、10、11分别经四项跳表选择动作起点11、8、9、10。第一次callee调用前：

- EAX保持`queued_code-8`的0..3索引；
- ECX/EDX保持caller入口值；
- 唯一栈参数为对应动作起点。

第一次callee的完整EAX/ECX/EDX成为typed动作提交的预调用寄存器；其EAX同时作为待提交actor code。普通路径完整返回队列提交结果；队列或group-A角色typed-stop保留第一次callee及扫描前缀，并向逐帧输入传播。

## 4. caller回收与陈旧值

`LegacyBattleInputDispatchCall`原动作确认槽保留相同数值并改为reserved，三个caller全部直接组合typed动作轮转：

- 记录2在写option word和action kind后调用；message为1时再进入已关闭目标选择，最后才把option word恢复全1；动作提交typed-stop也会阻断目标选择与恢复；
- 记录4先完成已关闭菜单后退，动作轮转普通返回后才发布菜单动作1；菜单后退或动作提交typed-stop都会阻断尾写；
- 记录5在对话为空时先执行右向opaque动作，再进入动作轮转；动作提交typed-stop会阻断后续输入阶段。

记录2 held等于1时option word为0；signed held大于等于15且除3余1时，原EBP仍为除数3，因此调用期间option word必须为3。实现保留这项陈旧寄存器投影，而非始终写0。三个caller都不再发出reserved动作轮转槽。

## 5. 共享owner与验证

当前角色严格复用`0x0053BD54`的final-actor queued owner，清零严格复用`0x0053BFBC`的final-actor pre-frame gate B owner；没有新增重复物理状态。

定向测试覆盖code 7与12默认返回、code 8..11四项起点、轮转callee到typed队列提交的寄存器传递、提交typed-stop传播、记录2短按和长按陈旧3、记录4普通直连与前置typed-stop阻断、记录5右向调用后的直连，以及两个reserved槽零调用。

当前缺少原版当前角色、可用动作轮转callee内部动作表、角色队列共享副作用、三个caller输入记录及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
