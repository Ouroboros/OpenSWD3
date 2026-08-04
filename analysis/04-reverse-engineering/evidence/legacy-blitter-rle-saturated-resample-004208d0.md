# RLE 纵向重采样饱和加色 `0x004208D0/0x00420D70`

状态：`assembly_exact`、`unit_verified`、`asset_verified`；未初始化纵向相位已做显式 `platform_adapted` 隔离，原程序动态 framebuffer 差分仍为 `blocked_runtime_oracle`。

## 证据边界

唯一汇编主证据是完整 LST `swd3.exe.lst`：

- 分派器目标高度、裁剪与 10.10 步长：`0x00417130–0x004172D9`；
- 正向槽 `0x20`：`0x004208D0–0x00420D61`；
- 水平反向槽 `0x21`：`0x00420D70–0x0042122B`；
- 当前真实调用：`0x00452B95`、`0x00479AEC`。

`analysis/tools/build_blitter_resampling_inventory.py` 只读取完整 LST，锁定分派器、两个函数、17 条 transform 构造路径和 12 项行映射合同。ASM 与伪码不参与取证。

## 分派器输入与输出高度

源高度 `Hs` 是绘制参数 `arg_C`，目标高度 `Ht` 来自单次调用状态 `0x004CD75C`。`Ht != 0` 时，分派器按目标高度执行纵向裁剪并建立：

```text
Ht > Hs:
    enlarge = 1
    vstep = signed_idiv(wrap32(Hs << 10), Ht)

Ht <= Hs:
    enlarge = 0
    vstep = signed_idiv(wrap32(Hs << 10), Ht) - 0x400
```

目标高度路径还使用一套与普通绘制不同的纵向裁剪公式：

```text
顶部重叠：
    visible  = Ht + destination_y - clip_top - 1
    top_skip = clip_top - destination_y

底部重叠（destination_y + visible >= clip_top + clip_height）：
    visible = clip_height - destination_y + clip_top - 1
```

底部比较在相等时也进入裁剪，因此目标高度刚好贴住 clip 底边仍只绘制 `Ht-1` 行。这个减一只存在于 `Ht != 0` 分支，不能通过“把普通裁剪的源高度换成目标高度”复用。

输出矩形宽度仍是源宽度；run 和 literal 像素都不做横向重采样。分派器返回前把 `0x004CD75C` 清零，所以目标高度是一次绘制请求，而不是永久画布状态。

现代请求用 `target_height` 显式携带 `Ht`；零保持普通高度路径。

## 原始未初始化相位

`0x0C` 的正向函数在 `0x0041F963` 明确执行 `mov [ebp-8], 0`。本族不同：

- 正向首次访问是 `0x004209D2: mov eax,[ebp-8]`，首次写入在 `0x004209E5`；
- 反向首次访问是 `0x00420E91: mov eax,[ebp-8]`，首次写入在 `0x00420EA4`；
- 两个入口到首次读取之间都没有对 `[ebp-8]` 的初始化。

因此汇编不能支持“`vfrac` 初始必为零”的结论。第一次更新使用完整 32 位栈残值：

```text
sum   = wrap32(stack_residue + vstep)
q     = unsigned(sum) >> 10
vfrac = unsigned(sum) & 0x3FF
```

直接在 C++ 中读取未初始化对象会产生 UB，也无法表达原 x86 栈残值。现代边界改为显式请求字段 `vertical_resample_phase_10_10`：默认零提供确定性宿主隔离，调用链或动态 oracle 以后可以传入捕获到的完整残值。算法不会在内部擅自把非零输入清零，也不会把它自动伪装成持久游戏状态。

## 正常输出行映射

每条输出行先绘制当前源 RLE 行，随后更新 `vfrac`。低十位不变式建立后，放大路径的 `q` 只能是 0 或 1：

```text
sum   = wrap32(vfrac + vstep)
q     = unsigned(sum) >> 10
vfrac = unsigned(sum) & 0x3FF

enlarge != 0:
    q == 0     -> 重复当前源行
    q != 0     -> 只推进一条源行

enlarge == 0:
    先推进一条源行，再推进 q 条源行
```

放大分支按汇编只测试 `q` 是否为零；即使首次未初始化残值使 `q > 1`，也仍只推进一条，不能改成通用 `advance(q)`。缩小分支确实循环推进 `q` 条额外源行。

源行在达到 RLE 零长度终止行时立即结束，未填满的目标行保持原 framebuffer 内容。目标高度不是必须强行填满的后处理尺寸。

## 顶部裁剪的不对称

顶部预处理在 `0x004209BE/0x00420E7D` 先比较 `skipped_count == top_clip`。`top_clip == 0` 因而直接使用第一条 RLE 行。进入一次预处理后，计数器在源行推进前加一；相位映射结束后若仍未等于 `top_clip`，`0x00420A37/0x00420EF6` 又加一才回到循环头。因此正数顶部裁剪实际只运行 `ceil(top_clip/2)` 次预处理。这是原始双递增 BUG，不得按裁掉的输出行数循环。

每次实际预处理先无条件推进一条源行，再更新相位：

```text
advance source by 1 row
q, vfrac = update_fraction()

enlarge != 0 and q != 0: 再推进 1 行
enlarge == 0:            再推进 q 行
```

所以放大时顶部裁剪不是未裁结果的简单下半段：每次预处理至少消耗一条源行，而正常放大输出行在 `q == 0` 时会重复当前行；同时 `top_clip=2` 与 `top_clip=1` 都只执行一次预处理。这是原函数的直接不对称，UT 固定保留。

## 像素与方向

literal 像素先应用三个有符号颜色偏移，再与目标逐通道饱和相加，完全复用 `0x04` 的公式：

```text
s'c  = clamp(sc + offset_c * channel_unit_c, 0, channel_max_c)
outc = clamp(dc + s'c,                         0, channel_max_c)
```

高位 run 不读取源像素、不修改目标，只推进逻辑 X。`0x20` 从左向右写目标；`0x21` 仍正向消费源 payload，但从目标可见窗口右端向左写。两者的纵向行选择完全相同。

两个函数都使用现有 `0x84` 字节逐行 jitter 表，并在正常结束、RLE 终止和 header gate 退出时把共享 phase 推进四字节；这一点正反方向相同。

## 真实调用

- `0x00452B95`：立即数 `0x20`，源矩形 640×480，`Ht = 0x280 + odd_transition_counter`，执行纵向放大的全屏过渡。
- `0x00479AEC`：`Ht = source_height + transition_counter`；正常初始化链只允许最终槽 `0x20/0x21`，未赋值的 `0x22/0x23` 不补 fallback。

## UT 固定向量

- `3 -> 5`、零相位：源行选择 `0,0,1,1,2`；
- 目标高度恰好贴住 clip 底边：保留 `Ht-1` 的原始裁剪结果；
- `5 -> 3`、零相位：源行选择 `0,1,3`；
- `3 -> 5`、顶部裁掉一行：保留原无条件推进，源结束后剩余目标不写；
- `top_clip=2`：双递增计数只执行一轮预处理；
- `3 -> 5`、初始相位 700：源行选择和提前终止随显式栈残值改变；
- 首次商带高位：放大分支仍只推进一条源行，而不是把商当通用行数循环；
- `0x21`：只反转目标水平写入方向；
- 正反函数的 jitter phase 都按汇编推进。

真实 `all_sys.tsw` 的 16×16 RLE 帧还执行了 `16 -> 20` 正反重采样：固定行重复关系通过，目标第 21 行保持原值，`0x21` 第一行逐像素等于 `0x20` 第一行的水平反向。

## 1:1 约束

- 不做双线性插值，也不缩放 run 长度或横向坐标。
- 不把目标高度误当源高度写回资产。
- 不用普通纵向裁剪替换目标高度路径的两个减一边界。
- 不把顶部裁剪改成正常输出映射的预演。
- 不声称 `0x20/0x21` 的原始相位由汇编清零。
- 不在 C++ 中制造未初始化读取；用显式完整 32 位输入隔离该原始 BUG。
- 不把 `0x21` 解释成源数据反向读取。
- 不给未赋值的 `0x22/0x23` 添加兼容 fallback。
