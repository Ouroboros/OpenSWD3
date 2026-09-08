# 战斗角色当前坐标增量调整 `0x004785A0`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed:4/4`。

## 1. 完整范围与调用关系

权威LST函数为`0x004785A0..0x004785BA`，共27字节、5条实际指令和1个`retn 8`。函数没有callee、条件分支、范围外`FUNCTION CHUNK`、跳入中段或隐藏入口。

唯一caller是战斗调试快捷键总处理`0x0045D8F0`，共有四个物理callsite：

- H键Group-A：`0x0045DDF3`；
- H键Group-B：`0x0045DE1D`；
- J键Group-A：`0x0045DE61`；
- J键Group-B：`0x0045DE8B`。

## 2. 精确语义与ABI

入口`ECX`是角色对象token，两个栈参数是完整dword槽，但函数只按以下顺序读取低word：

```text
mov ax,[esp+4]
mov dx,[esp+8]
add word ptr [ecx+0x0D66],ax
add word ptr [ecx+0x0D68],dx
retn 8
```

第一个参数读取只替换`AX`并保留`EAX`高word；第二个只替换`DX`并保留`EDX`高word。`ECX`始终保持角色token，两条内存ADD不修改通用寄存器，`retn 8`由callee清理两个参数。

X与Y都是16位模65536加法，不做符号扩展、夹值或32位提升。成功返回flags来自第二条word ADD，完整保留`CF/PF/AF/ZF/SF/OF`；AF是已定义的ADD结果。

## 3. 访问顺序、别名与typed-stop

modern入口为`adjust_legacy_battle_actor_coordinates`。四个原指令访问阶段分别映射为：

1. X参数word读取；
2. Y参数word读取；
3. actor X word read-modify-write ADD；
4. actor Y word read-modify-write ADD。

参数读取失败保留尚未被对应MOV替换的寄存器和入口flags。X访问失败不提交X，也不产生ADD flags。Y访问失败保留已提交X及第一条ADD flags。成功时两次ADD都按真实顺序提交。

实现不预读两个坐标。`position_x`与`position_y`可指向同一个word；此时第二条ADD必须观察第一条已提交结果。X/Y读可达性和写可达性都来自canonical坐标view；任一RMW访问不完整时整条对应ADD不提交。

## 4. canonical owner与caller组合

角色坐标继续只由既有`LegacyBattleActorCoordinatesState/View`表达：

- Group-A优先使用startup party角色；缺少startup owner时解析器仍可回退到action owner；
- Group-B使用startup Group-B lifecycle element中的action-execution角色状态；
- 不新增平行actor数组、坐标快照或调试专用副本。

H与J都严格按Group-A后Group-B遍历当前无符号count。角色token按原常量和步长形成：

- Group-A：`0x005029D0 + index * 0x2F34`；
- Group-B：`0x00525508 + index * 0x2B28`。

H传入X低word `+10`、Y低word `0`；J传入X低word `-10`、Y低word `0`。首项调用的入口flags来自`count - 0`的32位CMP，后续项来自`index - count`的32位CMP。每次调用入口`EAX`为当前group count、`ECX`为当前actor token、`EDX`沿用H/J尾部残值；首次成功后DX被Y参数清零，后续调用继续保留同一高word。

任一leaf typed-stop立即映射为caller `actor_coordinate_adjustment_typed_stop`，并公开精确leaf结果。停止点不执行当前迭代的count重载、index/token推进、余下actor与group、对应actor delta提交或后续热键。H只在两组完整结束后写`actor_delta=10`；J只在两组完整结束后写`actor_delta=-10`。H/J同时按下时先完整加10再完整减10，坐标按word模数恢复，最终delta为-10。

调试总门完整dword不等于1时，原入口直接跳到P：不查询或执行H/J。Control+E早退也继续抑制H/J/P。P仍位于共享尾部。

## 5. opaque边界回收

旧`LegacyBattleDebugHotkeyCall::adjust_actor`枚举槽改名为`reserved_adjust_actor_slot`，地址ordinal保持不变。四个caller站点全部直接组合typed leaf；生产源码对旧opaque `0x004785A0`边界零调用。

## 6. 双向追溯与验证边界

LST到C++：两个低word参数读取、X后Y两次word RMW、callee清栈、寄存器残值、最终ADD flags和四阶段typed-stop均有唯一实现字段或分支。C++到LST：helper没有额外日志、分配、回滚、坐标变换、上限、验证callee或失败后继续路径。

单元测试覆盖正负增量、word环绕、X/Y别名、四个typed-stop阶段、Y故障时X部分提交、入口与两次ADD flags、EAX/ECX/EDX残值、访问计数、调试总门、Group-A后Group-B顺序、后续迭代CMP flags、双键恢复、delta提交边界、P后缀抑制和reserved opaque槽零调用。发布门另覆盖Linux core、AddressSanitizer、Linux app、连续十轮core、changed-range格式、inventory双生成、TMP与staged/unstaged审计。

当前缺少原版完整Group-A/Group-B actor、可写与异常内存页以及四处caller联合寄存器/SEH捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`；不以静态结果冒充动态差分。
