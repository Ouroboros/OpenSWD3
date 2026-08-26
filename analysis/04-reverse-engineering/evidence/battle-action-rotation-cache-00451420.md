# 战斗三帧动作旋转缓存初始化 `0x00451420`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x00451420..0x0045153B`，从`proc`到`endp`共140行，没有外部`FUNCTION CHUNK`。ABI为thiscall，ECX指向扩展动作状态，callee清理五个栈参数；唯一caller位于`0x00451940`。

callee为动作更新`0x004321E0`两个callsite、帧图像查询`0x00431760`一个循环callsite，以及已关闭的literal图像循环平移`0x00433F70`一个循环callsite。typed实现直接调用`rotate_legacy_battle_literal_image`，不再保留opaque旋转边界。

## 2. 扩展动作状态与入口写序

扩展状态前`0x98`字节直接复用`LegacyActionRecord`。额外typed字段映射：

- `+0x9C,+0xA0,+0xA4`：三个帧owner token缓存；
- `+0xB4,+0xB8`：入口两个完整dword；
- `+0xBC`：条件发布的byte扩展值；
- `+0xC0`：初始动作号低word。

入口顺序为：

1. `+0xC0 = u16(arg_C)`；
2. `+0xB4 = arg_4`；
3. `+0xB8 = arg_8`；
4. `action_id = zero_extend(u16(arg_C))`；
5. `base_variant = 0`；
6. 调用动作更新。

若首个更新后EAX完整值为0，函数立即返回，不清record。测试锁定入口字段及更新器已写字段全部保留。

第一个栈参数只被装入ESI，并在调用stdcall旋转callee前复制到ECX；`0x00433F70`不读取ECX，因此它不产生可观察效果。typed签名显式保留该unused snapshot。

## 3. 动作更新寄存器snapshot

动作更新端口每次返回：

- 完整post-call EAX；
- 完整post-call EDX；
- 能代表端口全部隐藏状态的domain token。

record由端口原位更新。EAX为0只表示当前更新停止；首调与循环后续调用分别登记为不同typed状态，后续失败保留此前图像旋转、owner缓存及第二次更新写前缀。

## 4. `field_88`与陈旧EAX

每轮开头严格模拟：

```text
AL = record.field_88
if AL != 0:
    EAX &= 0xFF
    field_bc = EAX
AX = record.field_4c
frame_index = EAX
```

因此`field_88==0`时，`mov al,0`仍会清EAX最低byte，但保留更新后EAX高24位；随后`mov ax`覆盖低word，最终帧号高16位来自更新后EAX。

`field_88!=0`时，`and eax,0xFF`先清全部高位，再由`mov ax`写帧索引，所以完整帧号只有u16低word；同时`field_bc`写入该非零byte。测试分别锁定两条路径。

## 5. 三个局部FFFF槽

栈上三个u16槽入口均为`0xFFFF`，按`record.field_4c`零扩展索引。只有索引0、1、2落在原局部数组；其他值在首次栈word访问处typed-stop，不提前查询帧。

若槽已非FFFF，本轮跳过帧查询、owner写、除法和旋转，但仍继续检查command cursor并可能再次动作更新。

未访问槽的顺序为：

1. 形成资源号；
2. 查询帧图像；
3. 把查询返回owner token写到扩展状态对应缓存；
4. 把当前u16帧索引写入局部槽；
5. 执行除法；
6. 重新读取owner并解引用图像；
7. 调用literal旋转。

这意味着除零发生在owner与局部槽发布之后、图像指针解引用之前。测试以invalid pointer和低word零除数证明先触发除零，且两个缓存副作用均保留。

## 6. 陈旧EDX资源号与帧查询

资源号严格模拟`mov dx,record.field_4a`：

```text
resource_id = (post_update_edx & 0xFFFF0000) | record.field_4a
```

帧号使用第4节的完整EAX。typed帧端口返回owner token、指针有效性和可写literal图像span。

原查询返回值即使为空也会先写owner缓存和局部槽；现代只在除法后原`mov eax,[owner]`解引用点以`frame_image_pointer_invalid`停止。

## 7. 640除数与literal右移

除数只取第五参数低16位，按signed `idiv`计算：

```text
shift = 640 / zero_extend(u16(rotation_divisor))
```

低word为0在原`idiv`点typed-stop。非零除数始终为正，商使用signed向零语义。

随后直接调用已关闭callee：

```text
rotate_legacy_battle_literal_image(image, pixels_right, shift)
```

模式固定3。除数大于640时shift为0；callee按原入口门正常早退，外层继续，不把它误判为失败。magic不符、首行flag不支持和模式门同样属于原callee正常返回。

图像头、首行头、图像/临时读写越界属于现代逐访问typed-stop，外层以`rotation_typed_stop`阻断后续record清零。测试锁定短literal图像的头读取停止。

## 8. command cursor、循环与清零

每轮查询/旋转或缓存跳过后检查`record.command_cursor`（原`+0x42`）：

- 等于0：精确清零动作record前`0x98`字节并返回；owner缓存和`+B4/+B8/+BC/+C0`不清；
- 非零：把`base_variant`清零、以`+C0`的u16动作号零扩展重写`action_id`，再次调用动作更新；更新EAX非零则回到循环。

测试证明两帧完成后record完整152字节清零，owner缓存和扩展入口字段保留；也证明循环后更新EAX为0时不清record。

## 9. 非终止域

原循环没有迭代上限。modern不以任意次数截断，也不伪造成功。

每次循环顶部保存完整：

- 152字节动作record；
- 三个局部槽；
- 三个owner token；
- `field_bc`；
- 动作更新端口完整domain token。

只有上述完整状态全部重复，且中间已执行原查询/旋转/缓存跳过和更新副作用后，才证明进入同一原始状态环并返回`action_loop_nonterminating`。测试构造单帧首次旋转、第二轮缓存跳过、第三次顶部全状态重复，锁定三次更新、两轮副作用后停止。

## 10. 双向追溯

- `0x00451420..0x00451470`：扩展字段、动作号/base variant与首更新门；
- `0x00451476..0x0045149F`：`field_88`、陈旧EAX和三槽FFFF读取；
- `0x004514A7..0x004514D6`：陈旧EDX资源号、帧查询、三owner缓存和局部槽发布；
- `0x004514DB..0x004514F2`：640 signed除法、owner解引用与模式3 literal旋转；
- `0x004514F7..0x0045151B`：command cursor、动作号/base variant重置和更新循环；
- `0x00451521..0x0045153B`：更新失败返回或record `rep stosd`清零返回。

C++到LST反向追溯覆盖140行全部基本块、两个更新callsite、寄存器部分写、局部栈槽、缓存、除法、closed callee和两个返回族。

## 11. 验证与动态差分

定向测试覆盖：

- 两帧更新、陈旧EAX/EDX高字、模式3右移10的精确literal像素及record清零；
- `field_88`非零清EAX高位并发布`field_bc`；
- 首更新EAX零保留入口/更新前缀；
- 低word零除数在owner和局部槽发布后停止；
- 帧索引3在首次三槽访问停止；
- invalid image pointer在除法后停止；
- 短literal图像传播closed callee typed-stop；
- 后续更新EAX零保留旋转缓存；
- shift 0的callee正常早退与record清零；
- 完整record/槽/owner/端口token重复后的非终止停止。

battle聚合目标零warning构建及定向测试通过。

当前没有原版动作更新后EAX/EDX、三帧owner、扩展动作状态、可写literal图像和分配器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整140行LST、唯一caller及已关闭rotation callee已完成固定状态闭环。
