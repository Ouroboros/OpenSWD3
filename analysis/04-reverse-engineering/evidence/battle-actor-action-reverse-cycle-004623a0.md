# 战斗角色动作反向轮转 `0x004623A0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x004623A0..0x00462401`，从proc到endp共67行、30条实际指令、8个静态call、2个跳转、5个局部/默认标签、4个`retn`，没有外部`FUNCTION CHUNK`。函数后的`0x00462404..0x00462410`四项跳表也已审计。

两个静态caller均属于已关闭逐帧输入分派：记录6菜单前进路径和记录3选择后退路径。反向可用动作轮转callee尚未关闭；动作提交已由相邻工作包关闭，并与正向路径共用同一typed队列提交。

## 2. 入口与默认返回

函数无栈参数且不使用caller EAX。入口从共享queued角色dword装载EAX，随后先清共享pre-frame gate B，再按u32加`0xFFFFFFF8`。unsigned结果大于3时直接返回转换后的EAX并保持入口ECX/EDX。

因此code 7返回全1、code 12返回4；无效code也已经执行gate清零。区间检查和清零顺序与相邻正向轮转一致，但不能把两函数合并为同一动作顺序。

## 3. 四路反向起点

code 8、9、10、11分别经跳表选择动作起点9、10、11、8。第一次callee调用前EAX为0..3角色索引，ECX/EDX为caller入口值，唯一参数为动作起点。

第一次callee返回的完整EAX/ECX/EDX成为typed动作提交的预调用寄存器，其EAX同时作为待提交actor code。普通路径完整返回队列提交结果；队列或group-A角色typed-stop保留第一次callee与扫描前缀并向逐帧输入传播。

## 4. caller回收

旧secondary confirmation槽保留相同枚举数值并改为reserved，两个caller全部直连typed反向轮转：

- 记录6先完成已关闭菜单前进，再执行反向轮转，普通返回后才发布菜单动作2；菜单前进或动作提交typed-stop都阻断尾写；
- 记录3先按热点低word回绕更新选择；对话为空且message为3时先执行已关闭菜单选择后退，随后无条件进入反向轮转，最后直连已关闭菜单上下文后退；动作提交typed-stop阻断上下文后退，上下文typed-stop保留已完成轮转并阻断后续记录。

两个路径都不再发出reserved轮转槽，nested动作提交与上下文后退槽也为零调用。无效queued code按原default普通返回，caller继续原尾路径。

## 5. 共享owner与验证

当前角色严格复用final-actor queued owner，清零严格复用final-actor pre-frame gate B owner，没有新增重复物理状态。动作提交边界复用正向轮转已登记的同一typed操作。

定向测试覆盖code 7与12默认返回、code 8..11的`9/10/11/8`四项起点、反向callee到typed队列提交的寄存器传递、提交typed-stop传播、记录6普通直连与前置菜单typed-stop阻断、记录3菜单/轮转/上下文后退顺序、上下文typed-stop及三个reserved槽零调用。

当前缺少原版当前角色、反向动作表callee、角色队列共享副作用、两个caller输入记录、热点链及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
