# 透明度 blitter `0x00417950/0x0041B9F0/0x0041D340/0x0041E5C0`

状态：`assembly_exact`、`asset_verified`；原程序动态 framebuffer 差分仍为 `blocked_runtime_oracle`。

## 证据边界

唯一汇编主证据是完整 LST `swd3.exe.lst`：

- 分派门：`0x00417107–0x00417130`；
- raw `0x94`：`0x00417950–0x00417DD1`；
- RLE 逐行淡出 `0x1C`：`0x0041B9F0–0x0041CCE2`；
- RLE `0x14` 正向：`0x0041D340–0x0041E5BA`；
- RLE `0x14` 反向：`0x0041E5C0–0x0041F8C5`；
- 混合掩码建立：`0x00423686–0x00423740`。

ASM 和伪码不参与范围判定或语义取证。`analysis/tools/build_blitter_dispatch_inventory.py` 直接锁定并解析完整 LST，机械验证 raw、RLE 正反向和逐行淡出的标量跳转表使用同一套 16 项公式。

## 精确 packed-field 公式

令 `R/G/B` 为 `0x00423400` 选择后的有效通道 mask。初始化循环为 `k=1..5` 建立：

```text
M[k] = ((R >> k) & R) | ((G >> k) & G) | ((B >> k) & B)
```

原全局以高低字复制保存；16 位像素结果只取低字。透明度使用 `M1..M4`。内部 step 的精确结果为：

```text
step == 0:  dst
step == 15: src

step 1..14:
    ((step & 8 ? src : dst) >> 1 & M1)
  + ((step & 4 ? src : dst) >> 2 & M2)
  + ((step & 2 ? src : dst) >> 3 & M3)
  + ((step & 1 ? src : dst) >> 4 & M4)
```

四项分别移位、掩码后再相加，没有舍入。step 1..14 的源权重为 `step/16`，目标权重为 `(15-step)/16`，合计固定 `15/16`；不能替换为标准 alpha 或浮点插值。逐项目标和项序见 `../inventory/blitter-opacity-steps.tsv`。

## 外部 opacity 值的 raw/RLE 不对称

分派器只对 `(flags & 0xFFFC) == 0x14` 应用全局门：

```text
opacity <= 0: 整次绘制跳过
1..15:        保留 0x14
> 15:         只保留 bit31 和低两个翻转位，退化为普通 copy
```

进入具体函数后存在必须保留的不对称：

- RLE 正向 `0x0041D34C` 和反向 `0x0041E5CC` 只执行 `opacity <<= 2`，所以外部 `N` 直接选择内部 step `N`；`N=15` 精确复制源。
- raw `0x00417966–0x00417972` 先执行 `opacity--`，再左移两位，所以外部 `N` 选择内部 step `N-1`；`N=1` 保持目标，`N=15` 仍是内部 step 14，不会精确复制源。

这不是接口归一化的机会；相同外部值在两种物理布局下本来就产生不同像素。

## raw `0x94` 的源和透明色合同

`0x00417A65–0x00417A81` 每像素固定读取一个 `u16`，与转换后的 `0x004CD784` 低字比较，相等便跳过写入。它不检查调色板状态，也没有逐字节索引分支。因此即使非空调色板使分派器选择 raw 家族，`0x94` 仍按 16 位字流消费源并忽略调色板；不能套用普通 raw copy 的 indexed-8 路径。

源和目标都向前遍历。顶部/左侧裁剪先移动源起点，行尾源步长固定按 `source_width * 2`；透明源仍消费位置但不写目标。

## RLE `0x14` 正反向

RLE literal run 中每个 `u16` 源像素执行当前 step；高位 run 只移动目标，不比较 literal colorkey。反向函数只反转目标横向遍历，源 literal 仍按物理顺序读取，混合公式不变。

正反两个函数都使用 `0x84` 字节 jitter 组步长，并在正常、空 header gate 和行终止出口把全局 phase 加四、到 `0x84` 回零。它们不同于 reverse copy/constant-fill 等不推进 phase 的函数，不能按“所有反向函数都不推进”归纳。

## RLE `0x1C` 逐行淡出

`0x0041B9FC–0x0041BA14` 初始化：

```text
q         = signed(source_height + 16) >> 4
remaining = q
step      = 15
```

顶部裁剪跳过的每条完整源行以及随后处理的每条源行都先执行：

```text
remaining--
if remaining == 0:
    remaining = q
    step--
```

然后 literal 像素才使用该行 step。因此第 `y` 条原始源行（从零计）在正常正高度合同下使用：

```text
step(y) = 15 - floor((y + 1) / q)
```

顶部裁剪不会重置淡出。`source_height=16` 时 `q=2`，16 行依次使用 `15,14,14,13,13,...,8,8,7`。所有高位 run 都只跳过目标；该函数同样在出口推进 jitter phase。

## 验证

UT 固定了：

- RGB555 的全部 16 个内部结果；
- raw 外部 `1..15 -> 0..14` 与 RLE 外部 `1..15 -> 1..15`；
- raw colorkey、indexed-layout 仍读取 `u16` 的异常合同；
- 四组支持 surface mask 的 step 7 精确结果；
- RLE 反向源/目标次序与 phase 推进；
- `0x1C` 的顶部裁剪、分组预递减和调用方 `opacity_step` 被忽略。

真实 `all_sys.tsw` 资源 9128 variant 0 解压为 238 字节、16×16 RLE。以 `0xA55A` 初始化目标，由独立 LST 公式/RLE 解析脚本得到：

```text
RLE 0x14, external step 8: FNV-1a64 = 0x5AE112E4DCD510C5
RLE 0x1C:                  FNV-1a64 = 0xBB1BFBA032BF4485
```

两条路径都只改变 54 个 literal 像素，C++ UT 逐哈希复核通过。

## 1:1 约束

- 不统一 raw/RLE 的外部 step 映射。
- 不把 step 1..14 补亮为总权重 `16/16`。
- 不给 raw opacity 增加 indexed-8 调色板转换。
- 不把 raw colorkey 规则移植到 RLE literal。
- 不让顶部裁剪重置 `0x1C` 行相位。
- 不按函数方向猜测 jitter phase；使用各函数出口的实际汇编。
