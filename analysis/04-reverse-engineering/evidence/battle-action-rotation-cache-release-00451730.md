# 战斗六帧旋转缓存释放 `0x00451730`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x00451730..0x00451797`，从`proc`到`endp`共62行，没有外部`FUNCTION CHUNK`。ABI为无栈参数thiscall，ECX指向扩展动作状态；caller位于`0x00451940`与`0x0045B630`。

唯一callee为旧释放入口`0x004885A0`的两个静态callsite：先释放owner内的嵌套图像，再释放owner本身。

## 2. 六个owner槽

函数从`this+0x9C`开始，以4字节步长固定循环6次，覆盖`+0x9C..+0xB0`。

这证明扩展状态的owner数组本体为六槽。此前`0x00451420`与`0x004515E0`各自只有三个局部FFFF槽，因此它们只能初始化/播放前三个索引；`0x00451540`没有三槽局部门，owner数组访问范围应按六槽建模。typed state与第四十项证据已同步修正。

## 3. 每槽释放顺序

每个槽严格执行：

1. owner token为0：整槽跳过；
2. owner非零：读取owner内首dword的image token；
3. image token非零：调用释放image；
4. image释放返回后，把owner内image指针清0；
5. 再次读取owner槽；
6. owner仍非零：调用释放owner；
7. owner释放返回后，把外部owner槽清0。

release回调期间指针保持旧值：image回调看到owner与image都未清；owner回调看到owner仍在槽中，但内部image已经清0。测试端口逐回调验证该时机。

modern state分别保存owner token、image token、mutable image span与frame record。image清零时同步清对应mutable span和frame source；owner清零时再清完整frame record。

## 4. 空owner与孤立image

owner槽为0时，函数不会尝试读取或释放内部image，也不会改该槽对应的不可达payload。测试构造owner 0但typed image token非零的孤立状态，证明循环跳过并保留该image token/span；这避免现代实现擅自释放原函数无法到达的对象。

owner非零但image token为0时，只调用owner释放并清owner槽。

## 5. 循环后状态清零

六槽处理完后按顺序：

1. `stored_action_id`低word（`+0xC0`）清0；
2. `field_bc`完整dword清0；
3. 以`ECX=0x26,EAX=0`执行`rep stosd`，完整清零动作record前`0x98`字节。

`field_b4`和`field_b8`不清。函数plain返回时EAX仍为0；typed结果固定`return_value=0`。

测试以非零record、存储动作、field_bc及保留字段证明精确清零边界；全空六槽仍执行扩展字段与record清零。

## 6. 双向追溯

- `0x00451730..0x00451740`：保存this、循环计数6与首槽地址；
- `0x00451740..0x0045175D`：owner门、image门、image释放与内部指针清0；
- `0x0045175D..0x00451772`：owner重读、owner释放与外部槽清0；
- `0x00451772..0x00451776`：4字节推进与固定六轮；
- `0x00451778..0x00451797`：stored action、field_bc、0x98 record清零及EAX 0返回。

C++到LST反向追溯覆盖62行全部基本块、两个释放callsite、六轮顺序和最终清零边界。

## 7. 验证与动态差分

定向测试覆盖：

- 六槽混合owner/image布局；
- 三次image释放、四次owner释放与两个空owner跳过；
- `image0,owner0,owner1,image3,owner3,image5,owner5`精确回调顺序；
- image回调前旧token保留、owner回调前image已清而owner未清；
- owner空但image token非零的孤立payload保留；
- released span/frame清除与孤立span/frame保留；
- stored action、field_bc、0x98 record清零；
- field_b4/field_b8保留与EAX 0返回；
- 六个全空owner不调用释放但仍完成最终清零；
- 单帧绘制索引门同步修正为六owner槽之外才typed-stop。

battle聚合目标零warning构建及定向测试通过。

当前没有原版六owner、嵌套image分配与释放回调联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整62行LST与两个caller已完成固定状态闭环。
