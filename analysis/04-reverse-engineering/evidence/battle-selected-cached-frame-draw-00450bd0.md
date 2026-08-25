# 战斗资源选择与缓存帧复用绘制 `0x00450BD0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST完整范围为`0x00450BD0..0x00450C4B`，从`proc`到`endp`共74行，没有外部`FUNCTION CHUNK`。

cdecl四参数依次为selector、帧号、目标X、目标Y。八个callsite均位于`0x0047CF20`，地址为`0x0047CFF1`、`0x0047D051`、`0x0047D0AF`、`0x0047D10B`、`0x0047D18D`、`0x0047D1C9`、`0x0047D22B`和`0x0047D283`。其中四个caller立即把完整返回EAX加到EBX，返回高字不能丢失。

callee只有帧查询`0x004315D0`和软件blitter`0x004170E0`，均已关闭并由typed接口直连。

## 2. selector资源分支与缓存

入口selector只对完整32位值0和1特殊处理：

```text
selector == 0 -> query(resource=0x2359, frame=arg4)
selector == 1 -> query(resource=0x2358, frame=arg4)
otherwise     -> reuse cached frame record
```

查询分支在帧查询返回后立即把frame record发布到`0x004FD78C`；随后首次解引用`[eax]`取得source并发布到`0x004CD730`。查询失败时空frame record已经发布，但旧source保持不变，函数在首次解引用点进入原故障域。

其他selector完全忽略入口帧号，不调用provider，也不重新发布source；它直接复用现有缓存frame record和旧共享source。modern state因此分别保存缓存frame和共享source，不把复用分支伪装为再次从frame提取source。

无有效缓存时，typed实现只在原首次frame record解引用点停止。定向测试锁定selector 2忽略`0xFFFFFFFF`帧参数且复用先前0号帧，也锁定无缓存selector 3不调用provider。

## 3. 独立共享模式字

帧选择后，函数读取固定共享word `0x004FD784`：

```text
flags = shared_mode_word == 0x4000 ? 0x20 : 0
```

比较是完整u16精确相等，不是bit test。该word独立于缓存frame record；复用分支即使缓存无效也先读取模式，再在后续frame尺寸解引用点停止。typed结果因此在无缓存复用时仍记录已选flags。

模式`0x4000`测试以RLE源确认flags `0x20`走有效routine并正常完成；不得依据其他包装的源类型把它统一判成未分配。

## 4. 软件绘制与固定tail

软件绘制使用：

- X/Y直接取入口参数；
- 宽高取缓存frame record u16；
- flags取上节共享模式选择；
- 第六物理tail固定0；
- source取共享source，而不是在复用分支重读frame。

fixed tail由typed调用清空source palette和request auxiliary表达。indexed8帧在完整首word与几何检查通过后，于首次palette读取点得到`palette_out_of_bounds`。

正常`completed`、`clipped_out`或`opacity_disabled`执行通用公共后缀，清目标高度、水平位移、纵向phase、opacity、RGB与跳行并保留放大位。其他typed-stop不清入口状态，也不执行宽度返回后缀。

## 5. 混合EAX宽度返回

blitter正常返回后，函数重新读取缓存frame record指针，仅执行：

```text
mov ax, [cached_frame+0x0C]
```

因此返回低16位为帧宽，高16位保留blitter返回后的EAX：

```text
return = (post_blit_eax & 0xFFFF0000) | frame.width
```

modern显式接收`post_blit_eax_snapshot`，只在正常公共后缀后拼接返回。测试锁定宽1与高字`0xABCD`返回`0xABCD0001`、缓存复用高字`0x1234`返回`0x12340001`、共享模式路径高字`0xFFFF`返回`0xFFFF0001`。

不得把typed blitter状态或宽度零扩展值直接冒充完整旧EAX。

## 6. 双向追溯

- `0x00450BD0..0x00450BF2`：selector 0/1固定资源查询和其他值分支；
- `0x00450BF7..0x00450C09`：查询frame/source发布或缓存frame复用；
- `0x00450C0E..0x00450C21`：共享模式word精确`0x4000`门与固定tail；
- `0x00450C21..0x00450C39`：记录宽高、入口Y/X和软件绘制；
- `0x00450C3E..0x00450C4B`：重读缓存frame并只覆写AX宽度。

C++到LST反向追溯覆盖三个selector域、两种固定资源、frame/source不同发布时机、共享模式、六个绘制物理参数、公共后缀和混合EAX返回；没有未解释基本块、callee、共享访问或出口。

## 7. 验证与动态差分

定向测试覆盖：

- selector 0查询资源`0x2359`并发布frame/source；
- selector 1查询资源`0x2358`；
- 其他selector忽略帧参数并复用缓存frame/source；
- 查询失败发布空frame record但保留旧source；
- 无缓存复用不查询，并保留先读取的模式flags；
- 共享模式`0x4000`选择flags `0x20`并正常绘制；
- 普通模式、复用模式与0x4000模式的EAX高字加帧宽低字返回；
- indexed固定空tail在palette读取点停止，阻断宽度后缀。

battle聚合目标零warning构建及定向测试通过。

当前没有原版缓存frame record、共享source、模式word、blitter后EAX和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整74行LST、八个caller和两个关闭callee已完成固定状态闭环。
