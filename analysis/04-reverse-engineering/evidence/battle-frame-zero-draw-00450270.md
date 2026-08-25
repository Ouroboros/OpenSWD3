# 战斗资源0号帧定点绘制包装 `0x00450270`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围与ABI

权威LST完整范围为`0x00450270..0x004502AD`，从`proc`到`endp`共34行，没有外部`FUNCTION CHUNK`。

cdecl三参数：

```text
arg0 = 帧资源号
arg4 = 目标X
arg8 = 目标Y
```

函数只查询该资源的0号帧，按记录u16宽高绘制一次，并以plain `retn`返回；caller负责清理12字节参数。

## 2. 调用图

callee只有：

- `0x004315D0`帧记录查询一次；
- `0x004170E0`通用软件blitter一次。

两者均已关闭，typed实现直接组合，不保留opaque port。

三个caller：

- `0x0045291F`：资源`0x234D`、X=0、Y=384；
- `0x00452D90`：资源`0x234D`、X=0、Y=384；
- `0x0045336A`：资源`0x234D`、X来自caller EBX、Y=384。

三个caller都在返回后直接覆盖EAX，没有消费blitter残值。

## 3. 顺序与共享发布

LST顺序：

1. `push 0; push arg0; call 0x004315D0`，固定帧索引0；
2. 把返回记录指针发布到`0x004FD78C`；
3. 解引用记录`+0`并发布共享源`0x004CD730`；
4. 先零扩展记录`+0x0E`高度，再零扩展`+0x0C`宽度；
5. 以`x,y,width,height,0,0`调用软件blitter；
6. 合计弹出查询和blitter参数32字节，返回。

帧查询失败时记录全局已经发布为空，随后在`mov ecx,[eax]`故障，共享源保留入口旧值。现代状态分别记录帧记录发布/可用和源发布snapshot；provider失败在该解引用点typed-stop，不改入口源和共享blitter请求。

查询成功后先发布完整帧与源，再尝试绘制。blitter typed-stop保留这两项发布。

## 4. 固定尾参数0

与边框协调函数传记录`+4`不同，本包装在`0x0045028F`和`0x00450293`连续两次`push 0`，因此flags与第六个palette/辅助物理参数均固定为0。

现代blitter以source layout表达查询阶段发布的“palette指针非零”选择，以palette span表达像素routine实际读取。包装调用副本保留layout，但清空palette和auxiliary：

- direct16帧按原普通copy绘制；
- indexed8帧仍按indexed family选择，但在首次真实palette读取点得到`palette_out_of_bounds` typed-stop；
- 不把帧记录中存在的palette偷偷代入固定0尾参数。

共享opacity、目标高度、水平位移、纵向phase和放大位来自入口snapshot。正常`completed`、`clipped_out`或`opacity_disabled`到达通用blitter公共后缀，清目标高度、水平位移、纵向phase和opacity，保留放大位；其他状态未到后缀，不清入口状态。

## 5. 双向追溯

LST到C++：

- `0x00450270..0x0045027C`：资源与固定索引0查询、帧记录发布；
- `0x00450281..0x00450285`：源解引用与发布；
- `0x0045028B..0x004502A4`：u16高宽读取、两个固定0、入口Y/X重读；
- `0x004502A5..0x004502AD`：typed blitter、栈恢复与返回。

C++到LST：

- 一次provider load对应唯一帧查询；
- `frame_record_*`与`current_source`对应两项共享写及失败前缀；
- call source palette清空与request auxiliary清空对应两个固定0中的尾参数；
- request flags 0对应另一个固定0；
- width/height取`LegacyFramePiece`的u16字段；
- 正常后缀清理来自已关闭blitter公共尾。

完整正向和反向追溯无未解释基本块、参数、共享写、callee或出口。

## 6. 验证与动态差分

定向测试覆盖：

- 任意资源号只查询索引0；
- 2×3 direct16帧在指定X/Y的实际六像素输出；
- 帧记录、源snapshot与宽高发布；
- 正常公共后缀四项清零和放大位保留；
- provider失败时空记录发布、旧源与共享请求保留；
- 带有效palette的indexed8帧因固定尾0在首次palette读取点typed-stop，同时保留已发布的原帧源snapshot和入口共享请求。

battle聚合目标零warning构建及定向测试通过。

当前没有原版战斗帧记录、共享源、blitter状态和framebuffer的联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整34行LST、callee直接组合和固定状态验证已经闭环。
