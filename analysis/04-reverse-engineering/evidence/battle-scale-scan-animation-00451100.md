# 战斗三档刻度扫描动画 `0x00451100`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x00451100..0x004512A6`，从`proc`到`endp`共207行，没有外部`FUNCTION CHUNK`。cdecl两参数为目标X/Y；唯一caller位于`0x004539B0`。

静态callee为三个帧查询callsite、三个clip设置callsite和三个软件blitter callsite。中间callsite在固定三轮循环中执行，因此正常动态序列为五次帧查询、五次绘制和五次clip设置。

资源固定`0x234F`：入口查询帧0一次，三轮阈值和最终扫描各查询帧1一次。

## 2. 共享状态

显式typed state映射：

- 三个连续u16阈值`0x004A74D0..0x004A74D4`；
- 扫描计数dword `0x004FD768`，算法只读写低word并保留高word；
- 选择marker `0x004FDC68`；
- 目标选择dword `0x004FDD08`，命中失败只清低word；
- 缓存frame record `0x004FD78C`与共享source `0x004CD730`；
- 共享clip与软件blitter状态。

每次查询先发布frame record，再在首次解引用后发布source。查询失败保留此前source，并在原解引用点typed-stop。

## 3. 底板帧0

入口固定查询资源`0x234F`帧0并发布record/source，以入口X/Y、记录u16宽高、flags 0和record `+4` tail绘制。

这里tail不是固定空值。modern保留frame palette并把同一span映射到物理辅助参数；indexed帧有合法palette时正常绘制。测试以indexed帧0/1证明五次draw均消费record palette。

初始draw正常公共后缀完成后，选择marker才清零。初始查询或blit typed-stop不得提前清marker。

## 4. 三项阈值竖条

循环固定三轮，阈值依次按u16零扩展。每轮先使用“当前缓存frame”的高度设置1像素宽clip：

```text
left = entry_x + threshold
right = left + 1
top = entry_y
bottom = entry_y + cached_frame.height
```

第一轮缓存仍是帧0，因此bottom取帧0高度；后两轮缓存为上一轮查询的帧1，因此取帧1高度。所有坐标低32位回绕，clip helper只夹left/top下限和right/bottom上限。

设置clip后固定查询并发布帧1，再以入口X/Y、帧1记录宽高、flags 0和record tail绘制。正常后执行公共后缀；typed-stop保留当前1像素clip并阻断余下轮次与全屏恢复。

## 5. 阈值选择状态

每轮draw正常后，函数从扫描计数低word计算：

```text
half_step = (u16(counter) >> 1) + 1
candidate = loop_index + 1
```

只有`threshold <= half_step <= threshold + 2`时命中。命中先把candidate写入选择marker：

- 若目标选择低word等于candidate，marker立即清零，目标不改；
- 否则只把目标选择低word清零，高word保留，marker保留candidate。

三轮共享同一个未递增counter，因此阈值区间若重叠可多次命中，后轮继续覆盖前轮状态。测试锁定阈值5命中candidate 2时的相等清marker和不等清目标低word两条路径。

## 6. 最终扫描竖条

三轮后以counter低word计算不加一的半速位置：

```text
scan_half = u16(counter) >> 1
left = entry_x + scan_half + 1
right = entry_x + scan_half + 2
top = entry_y
bottom = entry_y + cached_frame1.height
```

随后再次查询/发布帧1并以入口X/Y绘制。测试以counter低word8锁定最终left为`X+5`。

最终draw正常后才调用`clip(0,0,640,480)`恢复逻辑全屏；所有早期typed-stop都不恢复。

## 7. 低word计数与返回1门

全屏clip恢复后，仅对counter低word执行u16加1，高word原样保留。再计算：

```text
completed = (incremented_low >> 1) + 1
```

若完整值等于62，counter低word改写为`0x8000`、高word仍保留并返回1；否则返回0。

测试以`0xCAFE007A`证明：最终扫描位置取递增前低word122的一半，随后低word增为123，半速完成值为62，最终counter为`0xCAFE8000`并返回1。

## 8. 双向追溯

- `0x00451100..0x00451143`：帧0查询/source发布、record tail绘制与marker清零；
- `0x0045114A..0x0045119F`：三轮阈值clip、帧1查询/source发布与绘制；
- `0x004511A4..0x004511F6`：半速值、三格命中窗、marker/目标低word更新；
- `0x004511F6..0x0045125B`：循环推进、最终半速clip与帧1绘制；
- `0x00451260..0x004512A6`：全屏clip恢复、counter低word递增、62门、`0x8000`发布与0/1返回。

C++到LST反向追溯覆盖207行全部基本块、动态循环次数、record/source发布、tail、clip、共享状态和出口。

## 9. 验证与动态差分

定向测试覆盖：

- 帧0一次、帧1四次的动态查询和绘制序列；
- 第一阈值使用帧0高度、后续使用帧1高度；
- 三个阈值竖条和最终半速竖条的实际像素；
- marker相等清零与目标不等只清低word；
- 循环帧失败保留局部clip、旧source和未递增counter；
- indexed frame record palette tail正常绘制；
- counter高word保留、低word递增和完成值62返回1门；
- 正常全屏clip恢复与公共blitter后缀。

battle聚合目标零warning构建及定向测试通过。

当前没有原版三项阈值、扫描counter、选择/目标状态、frame record、共享clip/blitter状态和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整207行LST与唯一caller已完成固定状态闭环。
