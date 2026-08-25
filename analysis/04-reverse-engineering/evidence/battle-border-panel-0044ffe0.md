# 战斗九块边框与渐变底板协调 `0x0044FFE0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与调用图

权威LST范围为`0x0044FFE0..0x0045026A`，入口`proc`至`endp`共303行，没有外部`FUNCTION CHUNK`。

ABI为cdecl六参数：

```text
arg0  = 边框帧资源号
arg4  = 左X
arg8  = 顶Y
argC  = 横向重复次数
arg10 = 纵向重复次数
arg14 = 32位底板颜色参数
```

函数不清理这24字节入口参数。唯一caller为`0x00466C00`，有两个静态callsite。

静态callee为：

- 帧记录查询`0x004315D0`九处；
- 通用软件blitter`0x004170E0`八处；
- 常量色垂直渐变包装`0x00450A50`一处。

三类callee均已关闭，本函数直接组合typed接口，不保留opaque callback。

## 2. 共享记录与源发布

每次帧查询返回后，旧函数依次：

1. 把帧记录指针写入`0x004FD78C`；
2. 解引用记录`+0`并写入共享源`0x004CD730`；
3. 读取记录`+4`、`+0x0C`宽和`+0x0E`高；
4. 调用软件blitter。

查询返回空指针时，帧记录全局已经写成空，但在读取`[eax]`处故障，共享源仍保留上一次值。现代`LegacyBattleBorderPanelState`分别记录当前帧记录是否发布/可用、帧索引、帧snapshot和当前源种类；frame provider失败只清当前帧记录，不提前改变源种类。

正常帧查询成功后，source种类发布为frame。底板渐变调用前改为color argument，并保留完整四字节小端颜色槽。后续帧查询再改回frame。函数完成时当前帧为索引8。

## 3. 4号帧预取与渐变底板

函数第一步查询资源的4号帧并发布其记录与源，但不读取宽高、不绘制该帧。随后调用已关闭渐变包装：

```text
x      = left
y      = top
width  = low32((horizontal_repeat_count + 2) << 4)
height = low32((vertical_repeat_count + 2) << 4)
color  = arg14
```

因此4号帧只是一次带缓存/共享发布副作用的预取；底板不是4号帧平铺，而是常量色垂直渐变。现代实现不得用通用九宫格center tile替换。

加2和左移4均为32位回绕。横纵重复次数不在渐变前夹为非负。

## 4. 九块边框顺序

### 顶边

1. 查询并绘制0号左上角于`(left, top)`；X增加0号宽；
2. 查询1号顶边一次；若横向次数大于0，重复绘制并每次按1号宽推进X；
3. 查询并绘制2号右上角于当前X；Y增加2号高。

即使横向次数小于等于0，1号帧仍查询和发布，只是不绘制。

### 左右边

若纵向次数大于0，每轮都重新：

1. 查询3号左边帧并绘制于`(left, current_y)`；
2. 以本轮3号宽计算右边X：

```text
right_x = low32(left + frame3.width * (horizontal_repeat_count + 1))
```

3. 查询5号右边帧并绘制于`(right_x, current_y)`；
4. Y只增加5号帧高度。

3号和5号帧不是循环外缓存，而是每轮重取。右边X不使用顶边实际累计X、1号宽或5号宽；行推进不使用3号高。这两个非对称必须保留。

### 底边

1. 查询并绘制6号左下角于`(left, current_y)`；X重置为`left + frame6.width`；
2. 查询7号底边一次；若横向次数大于0，重复绘制并按7号宽推进X；
3. 查询并绘制8号右下角于当前X/Y。

即使横向次数小于等于0，7号帧仍查询和发布，只是不绘制。

## 5. 循环与算术合同

横纵门都是signed `test; jle`：只有严格正次数进入循环。循环计数每次减一，以非零继续；没有上限、几何适配或尺寸一致性验证。

所有坐标加法、次数加一、宽度乘法和渐变尺寸左移都按32位回绕。帧宽高按记录u16零扩展。现代实现不以更宽整数阻止原回绕，也不要求九块尺寸相等。

横向次数为2、纵向次数为2时，动态帧查询顺序为：

```text
4, 0, 1, 2, 3, 5, 3, 5, 6, 7, 8
```

帧绘制12次，另有一次渐变。横纵次数均为0时仍查询`4,0,1,2,6,7,8`，只绘制四角和渐变。

## 6. 已关闭blitter直接组合

普通帧绘制以：

```text
x, y, piece.width, piece.height, flags=0, piece+4
```

调用通用blitter。typed实现用`LegacyFramePiece::source`表达源与palette，并把同一物理辅助数据映射给request auxiliary；opacity、目标高度、位移、phase和放大位来自每次调用前共享snapshot。

正常`completed`、`clipped_out`和`opacity_disabled`返回执行通用blitter公共后缀：清零目标高度、水平位移、纵向phase和opacity，保留跨调用放大位。其他状态代表原访问或routine未完成，函数在该callee原故障边界typed-stop，不执行后缀和剩余边框。

渐变包装遵守同一正常/typed-stop分流。

## 7. 失败前缀

现代状态区分：

- `frame_unavailable`：帧记录已发布为空，源仍是上一个成功源；
- `color_fade_typed_stop`：4号帧与颜色槽已发布，边框帧尚未开始；
- `frame_blit_typed_stop`：当前帧记录和源已发布，当前draw计入，未完成后续坐标推进；
- `completed`。

结果记录当前帧索引、查询次数、帧draw次数、渐变次数、最后blit状态及最后完成的X/Y。它们是typed诊断，不冒充旧返回结构；旧函数最终EAX只是最后一次blitter残值。

## 8. 双向追溯

LST到C++：

- `0x0044FFE0..0x00450027`：4号预取、回绕渐变尺寸及渐变调用；
- `0x00450028..0x004500C0`：0号角、1号顶边循环；
- `0x004500C0..0x00450107`：2号角及Y推进；
- `0x00450111..0x004501A7`：3/5号逐轮重取、右X乘法及5号高度推进；
- `0x004501A7..0x00450233`：6号角、7号底边循环；
- `0x00450233..0x0045026A`：8号角、栈恢复与返回。

C++到LST：

- 每次provider load对应唯一静态查询点或3/5动态循环点；
- 每次frame draw对应唯一软件blitter call；
- 唯一color fade对应已关闭包装call；
- current frame/source发布顺序对应两项旧全局写；
- 共享blitter后缀来自已关闭callee公共尾；
- 没有新增center tile、统一尺寸、自动裁边、失败跳过或坐标归一化。

完整正向与反向追溯未发现未解释基本块、循环、帧索引、callee、共享写或出口。

## 9. 验证与动态差分

定向测试使用九个不同宽高与颜色的固定帧，覆盖：

- 横2、纵2的11次查询顺序、12次帧绘制和一次渐变；
- 顶边、左右边和底边实际像素坐标；
- 右边X取3号宽、Y取5号高；
- 最终8号帧发布；
- 正常blitter单次状态清零与放大位保留；
- 零重复仍查询1号和7号帧；
- 首次3号帧缺失时保留完整顶边前缀与旧源；
- 0号帧空源在frame blit读取点停止，保留渐变与当前帧发布。

battle聚合目标零warning构建及定向测试通过。

当前没有原版战斗边框帧缓存、共享blitter状态和framebuffer的联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整303行LST、三个callee回收、typed实现和固定状态已经闭环。
