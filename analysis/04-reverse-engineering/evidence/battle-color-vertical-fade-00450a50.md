# 战斗矩形常量色垂直渐变 `0x00450A50`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST范围为`0x00450A50..0x00450A79`，入口`proc`至`endp`共27行，没有外部`FUNCTION CHUNK`。

ABI为cdecl五参数：

```text
arg0  = destination_x
arg4  = destination_y
arg8  = width
argC  = height
arg10 = 32位颜色参数槽；LST因只取地址把它标成byte
```

函数本身不清理这20字节参数。共有12个静态callsite，分布在`0x0044FFE0`、`0x00459D10`、`0x00466950`、`0x00469650`和`0x00469960`五个caller中。

唯一callee为已关闭的通用软件blitter`0x004170E0`，调用一次；本工作包直接调用其typed接口，不保留opaque callback。

## 2. 栈槽snapshot发布

函数先执行：

```text
lea eax, [esp+arg10]
push 0
mov dword_4CD730, eax
```

因此旧共享源指针不是颜色值，也不是独立像素缓冲，而是第五个入口参数完整32位栈槽的地址。发布发生在blitter调用前，调用返回后不清零；原指针随后成为调用栈生命周期相关的陈旧地址。

现代实现用`LegacyBattleColorFadeState::source_argument_slot`保存该32位参数的四个小端字节，避免跨平台暴露悬空栈指针。snapshot在调用typed blitter前发布，失败或typed-stop不回滚。blitter按原只读取低16位作为常量色，但高16位仍保留在snapshot中。

## 3. blitter调用参数

旧push顺序还原为：

```text
sub_4170E0(x, y, width, height, 8, 0)
```

模式固定为8，尾参数固定为0。现代包装从入口共享blitter请求snapshot开始，只覆盖：

- 目标X/Y；
- 源宽/高；
- flags为8；
- opacity参数为0。

目标高度、水平位移、垂直重采样状态和辅助状态等不由本包装栈参数携带，继续保留入口共享snapshot，不能擅自清零。

callee返回后旧函数只执行`add esp, 0x18`和`retn`，EAX保持callee结果。现代函数直接返回`LegacyBlitResult`作为typed等价观测。

## 4. 模式8真实职责

正常战斗颜色低word不是`0xFFFF`，模式8以raw家族偏移选择表槽`0x88`，实际routine是`raw_constant_vertical_fade`，不是普通实色copy。

该routine对矩形每行以初始opacity 15合成同一低16位颜色；计数超过`source_height >> 4`后opacity减一。因此此前计划中的“纯色矩形填充”只是临时导航描述，本函数真实职责是常量色垂直渐变矩形。

若颜色低word为`0xFFFF`，通用blitter按原选择规则增加RLE family位并落入槽8的coverage路径。当前五类caller只传RGB555颜色对或小整数颜色，该域不可达；typed callee仍保留误分类selection，并以既有`unsupported_routine`状态停止，不伪装raw成功。

## 5. 双向追溯

LST到C++：

- `mov ecx, argC`和`mov edx, arg8`对应高、宽snapshot；
- `lea arg10`及共享写对应完整四字节颜色槽发布；
- 六次push对应`x,y,width,height,8,0`；
- 唯一call直接映射已关闭blitter typed入口；
- 栈回收与EAX残值对应typed调用返回。

C++到LST：

- 四字节snapshot每个字节都来自入口颜色dword；
- 覆盖的六项request字段均来自原push；
- 未覆盖的共享request字段来自旧callee读取的共享状态，不是新增业务；
- framebuffer、clip、effect与jitter是旧全局的显式typed owner；
- 没有新增颜色转换、透明色判断、几何夹值或失败重试。

完整正向与反向追溯未发现未解释指令、参数、callee、共享写或出口。

## 6. 验证与动态差分

定向测试以4×4 framebuffer锁定：

- 32位颜色槽四字节小端snapshot；
- 模式8选择raw槽`0x88`和常量色垂直渐变routine；
- 第一行完整源色，后续行依原opacity递减；
- 共享目标高度保留并产生三行输出；
- 矩形外像素不变；
- `0xFFFF`低word保留RLE误分类selection和既有callee typed-stop。

battle聚合目标零warning构建及定向测试通过。

当前没有原版战斗颜色栈槽、共享blitter状态和framebuffer的联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整27行LST、callee回收、typed实现和固定状态已经闭环。
