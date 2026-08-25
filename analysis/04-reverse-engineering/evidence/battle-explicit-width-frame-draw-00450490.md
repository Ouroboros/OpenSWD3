# 战斗资源帧显式宽度绘制 `0x00450490`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围与ABI

权威LST完整范围为`0x00450490..0x004504D7`，从`proc`到`endp`共41行，没有外部`FUNCTION CHUNK`。

cdecl五参数：

```text
arg0  = 资源号
arg4  = 帧索引
arg8  = 目标X
argC  = 目标Y
arg10 = 显式源宽度
```

唯一caller为`0x00459DD4`，固定传资源`0x2350`、帧1；X为caller基准+100，Y为caller基准+8，显式宽度来自前置x87计算和向零转换。返回值不消费。

## 2. 帧查询与发布先于宽度门

函数先以入口资源号和帧索引调用已关闭`0x004315D0`，随后：

1. 发布帧记录到`0x004FD78C`；
2. 解引用记录`+0`取得源；
3. 发布源到`0x004CD730`；
4. 才对`arg10`执行signed `test; jle`。

因此显式宽度小于等于0时仍完成帧查询、记录发布和源发布，只是不读取记录`+4`、不读取帧高、不调用blitter。typed结果以`width_nonpositive`记录这个正常早退，不冒充查询失败或绘制完成。

帧查询失败时空记录已经发布，随后在`[eax]`读取点typed-stop；入口旧源保持不变。

## 3. 显式宽度与帧高度非对称

宽度严格为入口`arg10`，不读取记录`+0x0C`帧宽。高度才从记录`+0x0E`按u16零扩展。

绘制调用：

```text
x, y, explicit_width, frame.height, flags=0, frame_record+4
```

这意味着显式宽度可小于、等于或大于记录逻辑宽度；函数不验证，也不以帧宽夹值。raw源推进完全按传入宽度与高度消费，保持原可观察布局。

记录`+4`通过source palette和同一palette字节span表达raw/RLE routine的物理尾参数复用。

## 4. 公共后缀

通用blitter正常`completed`、`clipped_out`或`opacity_disabled`返回后，公共后缀清：

- 目标高度；
- 水平位移；
- 纵向phase；
- opacity；
- RGB偏移；
- 跳行状态。

跨调用放大位保留。其他状态在原callee故障点typed-stop，不清入口共享状态。

同单元的`0x00450270`首帧包装同步接入RGB/跳行清理，以符合相同公共后缀；其固定空palette/辅助合同不变。

## 5. 双向追溯

LST到C++：

- `0x00450490..0x0045049F`：入口资源/帧查询及显式宽度重读；
- `0x004504A3..0x004504AF`：帧记录、源解引用与发布；
- `0x004504AD..0x004504B5`：signed非正宽度门；
- `0x004504B7..0x004504CE`：记录tail、u16帧高、入口Y/X和显式宽度；
- `0x004504CF..0x004504D7`：软件blitter、栈恢复与返回。

C++到LST：

- provider输入对应唯一查询callee；
- frame/source state对应两项旧共享写；
- `explicit_width <= 0`对应唯一早退门；
- request宽度直接取入口，高度只取帧记录；
- flags 0与palette/辅助对应两个尾参数；
- shared清理来自已关闭blitter公共尾。

完整正向和反向追溯没有未解释基本块、字段、callee、共享写或出口。

## 6. 验证与动态差分

定向测试覆盖：

- 任意资源号/帧索引完整传给provider；
- 记录宽5、高6但显式宽2时，只绘制2×6目标区域；
- 正常公共后缀清单次请求、RGB状态并保留放大位；
- 显式宽0仍发布记录/source且不触碰共享绘制状态；
- 帧缺失只发布空记录，不发布新源、不绘制。

battle聚合目标零warning构建及定向测试通过。

当前没有原版显式宽度输入、帧记录、共享blitter状态和framebuffer联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整41行LST、两个callee直接组合和固定状态验证已经闭环。
