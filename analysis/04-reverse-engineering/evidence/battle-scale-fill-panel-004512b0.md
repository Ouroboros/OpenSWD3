# 战斗六级竖槽填充面板 `0x004512B0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x004512B0..0x00451418`，从`proc`到`endp`共171行，没有外部`FUNCTION CHUNK`。cdecl三个参数依次为X、Y和signed等级；唯一caller位于`0x00464270`。

资源固定为`0x241A`。静态五个查询callsite在正常路径依次查询帧`2,0,2,3,1`，四个blitter callsite依次绘制帧`0,2,3,1`。首个帧2只用于读取高度，不发布source。

## 2. 六级高度算术

首个查询发布帧2 record后，以u16高度零扩展。LST用`0x2AAAAAAB`、signed `imul`高word和符号修正得到除6结果；输入范围为`0..65535`，typed实现用精确signed `/ 6`得到相同值：

```text
segment_height = u16(frame2.height) / 6
fill_height = low32(segment_height * signed_level)
```

乘法保留x86 `imul`低32位回绕。等级不取绝对值，也不夹到`0..6`。

## 3. 帧0与内容Y

第二次查询帧0并发布record/source，以入口X/Y、记录u16宽高、flags 0和record `+4` tail绘制。正常公共后缀后，内容Y才按低32位回绕增加帧0高度：

```text
content_y = entry_y + frame0.height
```

查询、source或blitter typed-stop都保留真实前缀，不提前计算后续clip。

## 4. signed等级clip

只有`signed_level < 6`时设置局部clip：

```text
left = entry_x
right = entry_x + frame0.width
top = content_y
bottom = content_y + fill_height
```

等级6及以上完全跳过局部clip，帧2/3继续沿用入口共享clip。负等级仍进入clip，bottom可以小于top；modern blitter把该负高度裁剪视为正常空裁剪，函数继续后续步骤，不擅自夹值或typed-stop。

测试锁定：等级5得到每级10、填充50；等级6跳过局部clip并得到填充60；等级-1得到填充-10、填充帧裁空、底帧Y回退到`content_y-10`。

## 5. 帧2填充

第三次查询帧2并发布record/source，以：

```text
X = entry_x + 4
Y = content_y
flags = 0
tail = frame2 record +4
```

绘制。若等级小于6，局部clip只显示scaled高度；等级6以上不进行额外限制。

## 6. 帧3透明叠层

第四次查询帧3。原顺序为：

1. 发布frame record；
2. 解引用并发布source；
3. 把共享opacity写为8；
4. 以flags `0x14`绘制。

绘制坐标固定为：

```text
X = entry_x + 11
Y = content_y + 31
```

宽高和tail来自帧3 record。查询失败发生在opacity写8之前；测试锁定此时opacity仍是上一正常公共后缀留下的0，局部clip不恢复。若帧3 source已发布后blitter typed-stop，则opacity 8和局部clip都保留，阻断全屏恢复与底帧。

模式`0x14`使用共享opacity 8，正常完成后公共后缀才把opacity和其余瞬态状态清零。

## 7. 全屏恢复与帧1底部

帧3正常完成后，函数固定调用`clip(0,0,640,480)`恢复逻辑全屏。随后第五次查询帧1并以：

```text
X = entry_x
Y = content_y + fill_height
flags = 0
tail = frame1 record +4
```

绘制底部。

全屏恢复位于帧1查询前，因此帧1查询或draw typed-stop时clip仍保持全屏。函数未构造语义返回值，plain返回末次blitter的EAX；唯一caller在call后立即以共享等级dword覆写EAX，未观察该返回寄存器。typed接口返回结构化状态和末次blitter状态，不伪造未消费的EAX。

## 8. record tail与公共后缀

四次draw全部传递各自frame record `+4` tail，不是固定空tail。测试把帧0改为indexed source并提供record palette，证明该tail正常参与绘制。

每次accepted blit才执行公共后缀，清target height、水平位移、纵向phase、opacity、RGB和跳行，同时保留放大位。任一typed-stop都不提前执行当前draw的公共后缀。

## 9. 双向追溯

- `0x004512B0..0x004512E9`：帧2高度查询、除6和signed等级低32位乘法；
- `0x004512EE..0x0045132E`：帧0 record/source、record tail绘制和内容Y推进；
- `0x0045132E..0x00451347`：signed等级6门与局部clip；
- `0x0045134A..0x00451381`：帧2查询/source发布及X+4绘制；
- `0x00451381..0x004513C5`：帧3查询/source发布、opacity 8和模式14偏移绘制；
- `0x004513C8..0x00451418`：全屏clip、帧1查询/source发布、scaled Y底部绘制与plain返回。

C++到LST反向追溯覆盖171行全部基本块、五次查询、四次draw、两种clip数量、共享opacity时机和出口。

## 10. 验证与动态差分

定向测试覆盖：

- 正常帧序列`2,0,2,3,1`及固定资源；
- 帧2高度除6、等级5/6/-1的signed缩放；
- 四帧坐标、实际像素、局部clip与全屏恢复；
- 等级6跳过局部clip；
- 帧3查询失败发生在opacity 8之前；
- 帧3 malformed draw保留opacity 8和局部clip；
- indexed帧0通过record palette tail绘制；
- 正常公共后缀和最终帧1位置。

battle聚合目标零warning构建及定向测试通过。

当前没有原版五个frame record、等级值、共享clip/opacity/blitter状态和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整171行LST与唯一caller已完成固定状态闭环。
