# raw 常量纵向淡出 `0x00417EC0`

状态：`assembly_exact`、`unit_verified`；原程序动态 framebuffer 差分仍为 `blocked_runtime_oracle`。

## 证据边界

唯一汇编主证据是完整 LST `swd3.exe.lst`：

- 函数主体与局部像素块：`0x00417EC0–0x00418345`；
- raw 分派槽：`0x88 -> 0x00417EC0`；
- 当前明确调用者：`0x00450A50`。

ASM 和伪码不参与范围判定或语义取证。`analysis/tools/build_blitter_dispatch_inventory.py` 直接锁定完整 LST，机械验证 16 项像素表与 `0x14` 透明度公式逐项相同，并固定本函数的常量源与逐行档位状态机。

## 单一源颜色

`0x00417EC9–0x00417ED3` 只读取 `u16(source[0])` 到栈局部，`0x00417FC5` 随后让 `ESI` 永久指向这个局部量。像素循环中 `ESI` 不再改变。因此：

- 整个可见矩形的每个像素都使用同一个源颜色；
- 不按 `source_width/source_height` 遍历源矩形；
- 不检查透明色；
- 即使选择阶段的物理源布局为 indexed/palette，函数仍直接读取首个 `u16`，不查 palette；
- 两字节源缓冲足以完成任意正常尺寸的绘制。

当前真实调用 `0x00450A50` 把自身第五个四字节栈槽的地址作为源，所以该合同不是误识别出的“普通 raw 图像遍历”。

## 像素公式

局部表以字节索引 `0x3C` 开始，即内部 step 15。step 0..15 与 `0x14` 共用精确 packed-field 公式：

```text
step == 0:  dst
step == 15: src

step 1..14:
    ((step & 8 ? src : dst) >> 1 & M1)
  + ((step & 4 ? src : dst) >> 2 & M2)
  + ((step & 2 ? src : dst) >> 3 & M3)
  + ((step & 1 ? src : dst) >> 4 & M4)
```

四项独立移位、按通道掩码后相加，没有舍入；step 1..14 的源/目标总权重仍固定为 `15/16`。

## 逐行档位

入口 `0x00417ED6–0x00417EE6` 建立：

```text
q       = signed(source_height) sar 4
counter = 0
step    = 15
```

每条可见目标行先用当前 step 处理全部可见列，随后 `0x00418321–0x00418333` 执行：

```text
counter++
if counter > q:
    counter = 0
    step--
```

正常正高度下每档持续 `q+1` 条可见行，第 `r` 条可见行使用：

```text
step(r) = 15 - floor(r / (q + 1))
```

例如 `source_height=16` 时 `q=1`，16 条可见行依次使用 `15,15,14,14,...,8,8`。比较条件是严格大于，不得误写为 `counter >= q`。

## 裁剪不对称

本函数从可见第一行重新以 step 15、counter 0 起步。顶部裁掉的源行不会预推进淡出状态；左裁也只缩小目标列数，源颜色仍是 `source[0]`。因此同一绘制向上移出裁剪区后，留下的可见部分不会等价于未裁画面的下半段。这是原函数的直接行为，不按常规渐变语义修正。

## 验证

UT 固定了：

- `source_height=16` 时每档恰好持续两行；
- 三列、16 行只使用两字节 `source[0]`；
- indexed-layout 且空 palette 仍按首个 `u16` 工作；
- 顶部裁掉四行后，可见第一行仍使用 step 15；
- 全部结果复用 RGB555 的固定 16 项 packed-field 表。

## 1:1 约束

- 不遍历或转换假想的 raw 源矩形。
- 不把 indexed-layout 首字节解释成 palette 索引。
- 不增加透明色跳过。
- 不把 `sar 4` 改成基于可见高度的比例。
- 不把每档行数从 `q+1` 改成 `q`。
- 不让顶部裁剪预推进 step。
- 不用标准 alpha 或浮点渐变替换原 15/16 packed-field 公式。
