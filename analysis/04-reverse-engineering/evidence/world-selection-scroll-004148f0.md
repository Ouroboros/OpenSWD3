# 世界选择序列相机滚动（`0x004148F0`）

状态：`assembly_exact`、`unit_verified`、`platform_adapted`、
`sdl_runtime_integrated`；尚未 `original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。`sub_4148F0` 的完整物理范围是
`0x004148F0..0x004149A1`，无栈参数，唯一调用点是 `sub_4120B0:0x00412691`。函数只在
进入更新体后保存 `ESI`，以 plain `retn` 返回；早退与正常出口没有统一 `EAX` 合同，调用者
不读取返回值。

该函数是世界画面组合前调用的临时相机滚动状态机；与
`0x004128DF..0x0041291D` 的帧后视口恢复共同构成一对，不能只实现其中一端。

## 1. 输入与所有权

剧情 opcode 63 把最多 56 个 `u16` 项复制到 `0x004ACE70` 的 64-word 表，剩余位置预填
`0xCFCF`；同时把 opcode 的 prefix 写入当前 countdown 和 reload interval，并保存当时的
相机 left/top。opcode 64 只恢复 `0xCFCF` 表，不清其他状态。

`0x004148F0` 使用：

| 地址 | OpenSWD3 字段 | 作用 |
| ---: | --- | --- |
| `0x004ACE70` | `selection_words` | 交错的有符号 `i16 x,y` 增量和 `0xCFCF` 哨兵 |
| `0x004B7EB0` | `cursor_word_index` | 以 word 为单位，正常每次前进 2 |
| `0x004A94A4` | `frames_remaining` | 当前坐标对的剩余帧数 |
| `0x004AD0D0` | `frame_interval` | countdown 到期后的重载值 |
| `0x004A93E4` | `saved_left` | 本次滚动前的相机 left |
| `0x004A992C` | `saved_top` | 本次滚动前的相机 top |

相机矩形仍是 left/top/right/bottom 四个 32 位单元。增量使用 `MOVSX` 从 `i16` 扩展，
再按低 32 位加到矩形两条对应边。

## 2. 两个入口门

机器顺序固定为：

1. 第一项等于 `0xCFCF` 时立即返回；
2. 地图 ID 等于 `0x16` 时立即返回；
3. 只有两门都未命中才读取游标和 countdown。

因此 inactive selection 或地图 22 都不会修改游标、计数器、保存坐标或相机。实现不能
为了统一流程而先递减 countdown。

## 3. 游标、计数器与滚动顺序

当前游标指向 `0xCFCF` 时，函数先把游标写零，再读取第一对坐标。随后：

```text
delta_x = sign_extend_i16(selection_words[cursor])
delta_y = sign_extend_i16(selection_words[cursor + 1])

frames_remaining = wrapping_i32(frames_remaining - 1)
if signed(frames_remaining) <= 0:
    frames_remaining = frame_interval
    cursor_word_index = wrapping_u32(cursor + 2)

saved_left = camera.left
camera.left  += bits(delta_x)
camera.right += bits(delta_x)
saved_top = camera.top
camera.top    += bits(delta_y)
camera.bottom += bits(delta_y)
```

到期帧仍应用当前坐标对，只是先把下次调用的游标推进到下一对。游标是原始 word 下标，
函数不强制偶数对齐；奇数游标同样读取当前位置及下一 word。reload interval 也不钳正，
零 countdown 可在减成负数后载入负 interval。`DEC` 的低 32 位回绕与后续有符号 `JG`
均保留：`INT32_MIN` 递减到 `INT32_MAX` 后会被当作正数，不重载间隔。

原程序依赖固定 64-word 全局表，游标没有范围保护。OpenSWD3 接受显式 span，并只在游标
或 `y` word 超出这个现代所有权窗口时返回 `invalid_selection_window`；有效的 64-word
输入完全沿用原读取、写入和回绕顺序。

## 4. 与帧尾恢复的闭环

`sub_4120B0` 的相关顺序是：

```text
0x00412691  0x004148F0 临时滚动并保存原 left/top
0x004126B8  0x00412930 按临时相机组合世界画面
0x00412716  普通世界 presentation
0x004128DF  selection/map 门控
0x004128F7  用保存值恢复固定 640×480 相机矩形
```

组合 UT 已固定“原矩形 → 临时滚动 → 帧尾恢复到原 640×480 矩形”，同时覆盖 inactive
和地图 22 门、游标遇哨兵回零、到期前/到期帧、有符号负增量、countdown 回绕，以及现代
span 无效窗口隔离。

原固定 64-word 裸全局表改为受检 span；空表、越界游标和缺少 Y word 在相应原读取点隔离，
此前的游标回零写入仍保留。有效窗口的门、写序、回绕和相机结果不变，因此 closure
disposition 为 `platform_adapted`。

selection-scroll 与 frame-coordinator 两项定向 CTest 通过；独立向量覆盖 inactive/map 门、
哨兵回零、奇数游标、零 countdown/负 interval、到期前/到期帧、负增量、`INT32_MIN`
回绕、现代窗口失败和帧尾恢复。Linux core `185/185`、Linux app `191/191`、Windows
LLVM app `191/191` 完整门禁通过；该状态机已接入 `0x004120B0` coordinator，两端应用
成功链接且未启动游戏 EXE。原程序动态差分仍等待用户 oracle。
