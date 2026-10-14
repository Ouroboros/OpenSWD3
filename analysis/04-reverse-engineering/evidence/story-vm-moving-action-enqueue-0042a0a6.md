# 剧情 VM 移动画面动作创建 `0x0042A0A6`

## 结论

opcode79固定长16字节。它先分配、清零并初始化一个`0xB4`动作节点，然后分阶段读取7个u16 operand：action ID、base variant、起点X/Y tile、终点X/Y tile、signed movement。四个坐标先在16位内左移4并以i16解释；handler用wrapping i32平方和、x87 `fsqrt`和除法得到速度，把起点转float，最后前插到`dword_4AD3E8`链。成功推进16、发布normalized previous79并same-call继续。

opcode79此前没有C++ case。现代实现使用`LegacyMovingActionList`和既有0xB4节点布局，SDL接入现存`world_moving_actions_`生命周期。

## 原始顺序

1. `sub_487C10(0xB4)`；原版不检查null；
2. `rep stosd`清45个dword；
3. `sub_40DC00`初始化action；
4. 依次读取/写入action ID、base variant，并显式清variant delta；
5. 四坐标各以word左移4后写`+98/+9A/+9C/+9E`；
6. `+14`以i16 movement载入x87；
7. `dx/dy`以sign-extended i16坐标作i32差，平方和按32位`imul/add` wrapping；
8. `sqrt(sum)`，再计算`dx*(movement/distance)`与`dy*(movement/distance)`并分别store float；
9. 起点i16转换为position float；
10. 写旧head到legacy `+B0`并把新节点设为head；
11. 推进16、`ESI=1`、公共join发布previous79并same-call继续。

## x87与异常域

实现以host `long double`保持x87中间精度，最终仅在四个float store点收窄。平方和先按u32模运算再bit-cast回i32，保留原`imul/add`溢出。

- 3-4-5路径、movement5得到速度3/4；movement -5得到-3/-4；
- 起终点相同：distance0，`0*inf`得到NaN；
- wrapping平方和为负：`sqrt(negative)`和最终速度为NaN。

## 资源与平台适配

现代先在临时`std::list`中`emplace_front`，聚合零初始化后调用action initializer；每个operand缺失都会销毁未链接临时节点，IP/previous不变。`bad_alloc/length_error`映射为`moving_action_allocation_failed`，替代原版unchecked null崩溃。

固定全局list映射为nullable`runtime.moving_actions`，在全部operand与数学完成后的原始链接点首次检查；缺失返回`runtime_unavailable`。成功用`splice(begin)`前插。host list链接替代裸指针，legacy `next_pointer_32`保持零值；逐帧更新/释放继续复用既有`legacy_moving_actions`模块。

## 窗口与流控

- 分配/初始化发生在任何operand读取前；
- 7个operand逐word staged读取；
- 完整记录位于`0x7FF0`时先创建、链接、IP=`0x8000`并发布previous79，下一fetch再返回`instruction_out_of_range`；
- ordinary路径可同调用继续到下一opcode。

## 资产锁

`story-vm-talk-linear-records.tsv`中opcode79为0条物理记录、0个entry probe。候选CFG曾观察到2个节点，但不满足线性指令记录锁，不能当作真实TALK资产。分类使用`asset_absence_verified`，不提供伪造的真实回放。

## 测试覆盖

- 四raw alias精确尾，验证完整节点、3-4-5速度、前插和previous79；
- ordinary same-call继续并保留旧list顺序；
- 0..6个operand可用的七个截断点；
- owner缺失发生在完整数学后、链接前；
- zero distance NaN、signed movement、32位平方溢出NaN；
- 剧情VM三项测试通过。

## 分类

分类：`platform_adapted`。合法域节点布局、operand宽度、16/32位wrapping、x87中间顺序、链接时点、IP、previous与same-call均保持；裸分配/链指针与固定全局改为typed容器所有权。
