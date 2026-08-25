# 战斗动作绘制矩形surface边界放置 `0x004342E0`

状态：`assembly_exact`、`unit_tested`、`fixed_state_tested`。

## 1. 范围与叶子性质

权威LST函数范围为`0x004342E0..0x00434346`，入口`proc`至`endp`连续，没有外部`FUNCTION CHUNK`，也没有任何callee。

三个直接caller为：

- `0x00433C40`：初始化固定640×480绘制区域；
- `0x00433DC0`：取得外部尺寸后，以半宽与完整高度设置绘制区域；
- `0x00433F30`：写入宿主surface宽高后设置完整绘制区域。

三个caller当前都未进入现代实现，没有既存opaque callback可回收。后续关闭这些caller时必须直接调用本typed函数。

## 2. thiscall ABI与对象切片

ECX指向旧战斗绘制对象，四个栈参数依次为：

1. `left`；
2. `top`；
3. `width`；
4. `height`。

对象切片：

- `+0x0B50`：surface宽度；
- `+0x0B54`：surface高度；
- `+0x0B58`：发布后的left；
- `+0x0B5C`：发布后的top；
- `+0x0B60`：发布后的right；
- `+0x0B64`：发布后的bottom。

现代`LegacyBattleRenderGeometry`按相同语义显式命名六个字段。它是战斗内部绘制对象的局部几何，不替代公共framebuffer owner。

IDA把函数标为返回`int`。完整LST表明最后一次写bottom前，EAX正好保存`top + height`，函数未再覆盖EAX，因此返回寄存器残留为最终bottom。三个直接caller都不立即测试该值；`0x00433F30`会把它短暂透传给上层，但其已知caller随后覆盖。typed接口仍显式返回bottom，以保持完整ABI而不是只满足当前caller。

## 3. X轴负起点 `0x004342E0..0x004342F3`

LST先把`left`载入EDX、把`width`载入EBX：

- `left >= 0`时保持二者；
- `left < 0`时执行32位回绕的`width += left`，随后把left清零。

这不是简单把left夹为零。负起点会缩短width；若负位移大于原width，结果width可以为负。现代实现不拒绝、不置零也不改成绝对值。

## 4. Y轴负起点 `0x004342F3..0x00434303`

函数随后把`top`载入EAX、把`height`载入EDI，执行与X轴相同但独立的规则：

- `top >= 0`保持；
- `top < 0`时以32位回绕执行`height += top`，然后top清零。

X轴先处理，Y轴后处理；两轴局部值都完成后才读取surface边界，且尚未发布任何输出字段。

## 5. 右边界放置 `0x00434303..0x00434315`

函数读取`surface_width`，以32位回绕计算`left + width`，再按signed i32比较：

- 若`left + width < surface_width`，保留left；
- 若`left + width >= surface_width`，执行32位回绕的`left = surface_width - width`。

因此右侧达到或越过surface边界时，不会缩短width，而是把left反向移动，使最后的right等于surface宽度。若width大于surface宽度，left会变成负数。若width为负或加法回绕为负，比较可能不进入重定位分支。

条件是`jge`而不是`jg`。普通不回绕数值在相等时重算出的left与原left相同，但精确分支条件仍保留。

## 6. 下边界放置 `0x00434315..0x00434327`

函数读取`surface_height`，对`top + height`执行同样的回绕与signed比较：

- 小于surface高度则保留top；
- 大于等于surface高度则令`top = surface_height - height`。

它同样是移动起点，不是裁短高度。大于surface的height可产生负top；负height和回绕和值不做现代修正。

## 7. 四字段发布与返回 `0x00434327..0x00434346`

计算全部完成后，LST才按固定顺序发布：

1. `[this+0x0B5C] = top`；
2. `[this+0x0B58] = left`；
3. EAX回绕加height，EDX回绕加width；
4. `[this+0x0B60] = right`；
5. `[this+0x0B64] = bottom`；
6. EAX携带bottom返回，callee清理16字节参数。

typed实现保留top先于left、right先于bottom的赋值顺序，并返回最终bottom。

## 8. 与公共framebuffer clip的边界

现有`rendering::set_legacy_clip_rectangle`接收绝对left/top/right/bottom；右或下越界时会裁短边缘并保持起点。

本函数接收left/top/width/height；右或下达到边界时会反向移动起点以保持尺寸。两者对右侧越界输入产生不同结果，不能复用或合并。本typed实现归属`battle`局部绘制几何，不改变公共framebuffer clip行为。

## 9. 双向追溯

LST到C++：

- `0x004342E0..0x004342F3`映射为负left时width回绕缩短与left清零；
- `0x004342F3..0x00434303`映射为负top时height回绕缩短与top清零；
- `0x00434303..0x00434315`映射为右边界signed `>=`和起点重定位；
- `0x00434315..0x00434327`映射为下边界signed `>=`和起点重定位；
- `0x00434327..0x00434346`映射为top、left、right、bottom发布及bottom返回。

C++到LST：

- 两个负起点分支各有一组`test/jge/add/xor`；
- 两个边界分支各有一组回绕加法、signed `jl`和回绕减法；
- 六个typed字段分别对应对象`+0x0B50..+0x0B64`；
- 每次回绕加减都直接对应32位x86算术；
- 函数无状态拒绝、typed-stop、分配、资源读取或额外副作用。

完整正向与反向追溯未发现未解释指令、额外C++行为或遗漏出口。

## 10. 测试

LST独立测试覆盖：

- surface内部普通矩形；
- 负left/top缩短尺寸后清零；
- 右/下越界时移动起点而不裁短；
- 右/下恰好等于边界；
- width/height大于surface后产生负起点；
- 负width/height原样发布反向边；
- `INT_MAX + 1`回绕后按signed比较；
- 负surface边界减法回绕；
- 三个caller共有的640×480完整矩形固定状态；
- EAX兼容返回值等于bottom。

该函数只消费整数几何，没有适用的物理资产读取。固定640×480状态直接来自三个权威LST caller，不从现有C++推断。

定向battle聚合测试通过。

## 11. 动态差分

当前没有可用原版战斗绘制对象捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。该阻塞不改变完整叶子LST、回绕边界、typed实现和固定caller状态已经闭环的结论。
