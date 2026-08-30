# 战斗组B行动profile与mode组合 `0x004761D0`

状态：`platform_adapted`。完整LST、唯一caller回收、两个固定mode callee可达路径、共享owner、typed-stop、定向/ASan/Linux门禁和inventory双生成收敛后关闭。

## 1. 完整权威范围、call集合与ABI

权威LST主体为`0x004761D0..0x0047624A`，从proc到endp共59行、39条实际指令、3个call、2个跳转、2个局部标签和3个返回点，没有外部`FUNCTION CHUNK`。三个call依次是固定mode 2的`0x00478710`、profile loader `0x00476A80`和固定mode 1的`0x00478710`。

函数是无栈参数的thiscall，ECX为组B actor。入口保存ESI，只有selector为零的路径另保存EDI；三个出口均为普通`retn`。caller全文搜索的真实机器指令call仅有`0x00457C44`一处；机械上下文中的第二个文本命中来自反编译注释`__thiscall sub_4761D0`，不是递归call。

## 2. 非零selector路径

函数先读取actor `+0x2A8C`的u16 selector。selector非零时不访问actor资源，也不调用profile loader，而是测试40-byte profile `+0x0C` byte的bit 1：

- bit 1清零：EAX清零后直接返回，行动种类、显示种类和mode flags全部保持；
- bit 1置位：先把actor `+0x2A87`的bit 7或入，再固定展开`0x00478710(2)`，即显示种类写2、行动种类写0、callee EAX写1；随后caller清EAX并只把profile `+0x14`的word写入AX，因此最终EAX为该word的零扩展值。

两条路径都保持ECX为actor token，EDX保持入口陈旧值。actor映射缺失只在首次selector读取点typed-stop，不提前检查profile或资源。

## 3. 零selector的清零、profile加载与mode 1

selector为零时严格按原指令顺序执行：

1. EDX形成actor `+0x0D90` profile目标；
2. actor `+0x29A4`首个派生word清零；
3. `rep stosd`以10个dword清零完整40-byte profile，并留下`EAX=0, ECX=0, EDX=profile目标`；
4. 读取actor `+0x0C`资源token，再读取资源`+0x60`的word。因为ECX在`rep stosd`后为零，`mov cx`得到零扩展profile id；
5. 以`(actor+0x0D90, profile_id)`调用`0x00476A80`；
6. callee返回后重读资源token，ECX恢复actor，`mov ax,[resource+0x56]`只覆盖callee EAX低word；该word按u16加到刚清零的首个派生word；
7. 固定展开`0x00478710(1)`，只把行动种类写1，显示种类和mode flags保持；外层最终清EAX并返回0。

profile loader返回值不作为成功门；其对40-byte目标的写入保留。正常终态为`EAX=0, ECX=actor token, EDX=resource token`。资源token缺失在清派生word和profile后的首次资源byte访问点typed-stop；loader typed-stop保留清零与callee已发布的profile/寄存器前缀，并阻断资源`+0x56`、派生word相加和mode 1后缀。

## 4. owner与待审callee边界

`+0x2A8C` selector加入既有`LegacyBattleGroupBActionCompositionState`，与`+0x29A4`派生word、行动种类、显示种类和mode flags共用组B生命周期唯一owner。40-byte profile继续复用`LegacyBattleGroupBActionConfigurationState::profile_buffer`；动态164-byte资源继续复用同一actor的`resource_token/resource_bytes`，没有平行副本。

`0x00478710`整体仍是`audit_order=298`的待审函数。本工作包只展开参数1和参数2的两条固定可达路径，不提前关闭其余mode。`0x00476A80`整体仍是`audit_order=260`的待审profile loader；caller adapter只保留一个窄token，并通过既有profile payload接口发布40-byte写入。

## 5. 唯一caller回收

组B帧`0x004576A0`在packed status的signed负值路径调用本函数。调用前EAX由`status & 0x8000`形成，因此固定为`0x00008000`；EDX是进入分支时的special-selection pending完整dword。pending精确等于1时，caller先把side写1并清pending，再进入本函数。

原`0x00457C44`调用点现直接组合typed实现。成功后才把返回EAX发布到status action value并写current actor低word，随后处理status bit14的mode 2/文字、bit13的mode 6、special action、phase和目标选择。任一actor/resource/profile-loader typed-stop保留special-selection切换及函数内部已发生的清零/加载前缀，阻断status action value、current actor与全部status后缀。生产源码不再执行`0x004761D0`整函数opaque token。

## 6. 测试、验证与动态差分

纯函数测试覆盖actor首次访问、非零selector bit 1清零、profile word零扩展、固定mode 2、资源首次访问、40-byte清零顺序、profile loader ABI与typed-stop前缀、资源`+0x56/+0x60`、派生word发布、固定mode 1以及EAX/ECX/EDX线程。组Bframe集成测试覆盖成功路径、旧opaque零调用、actor typed-stop和loader typed-stop的caller前缀/后缀边界。AddressSanitizer首次复验揭示历史frame-coordinator巨型测试函数的栈帧膨胀；仅将其29个`Fixture`测试夹具改为`unique_ptr`堆分配，生产实现与断言不变。

最终`./build-asan.sh`、`./build.sh core`和`./build.sh app`分别完成AddressSanitizer core `188/188`、Linux core `188/188`与Linux app `194/194`；零OpenSWD3源码warning、sanitizer finding或测试失败。inventory生成器连续双跑逐字节一致，关闭进度为`246/422 = 237 platform_adapted + 9 assembly_exact + 176 pending_audit`，SHA256为`3ccf2b08608febd8a863f7fc31f1fd702e7978fc0250eefa6f0b7982bd8e9020`。

当前缺少原版八个组B完整actor、动态164-byte资源、40-byte profile、真实profile loader及唯一caller共享状态/寄存器联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。
