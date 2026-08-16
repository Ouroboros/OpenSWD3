# 世界空间角色扫描（`0x00413870..0x00413FDD`）

本文只以 `/mnt/e/Game/swd3/swd3.exe_export_for_ai/swd3.exe.lst` 的机器码与指令为
行为真值。当前工作包只独立关闭外层扫描 `sub_413870`；同文件曾记录的相邻绘制、音频与
bit-29 路径不继承关闭状态。

## 1. 逐函数状态

| 地址 | 函数 | 当前状态 | 当前工作包结论 |
| --- | --- | --- | --- |
| `0x00413870` | `sub_413870` | `platform_adapted`，已独立闭环 | `0x00413870..0x0041390B` 已完成 LST→C++→LST 收敛；映射 `draw_legacy_world_roles` 及 runtime 原 stage seam |
| `0x00413910` | `sub_413910` | `pending_audit` / `not_inherited` | 本轮只核对 `sub_413870` 所需的一参数 cdecl 调用边界，不审计角色绘制函数体 |
| `0x00413CA0` | `sub_413CA0` | `pending_audit` / `not_inherited` | 本轮只核对 `sub_413870` 所需的一参数 cdecl 调用边界，不审计距离音频函数体 |
| `0x00413EA0` | `sub_413EA0` | `pending_audit` / `not_inherited` | 未审计；旧实现、测试和集成叙述不构成本轮关闭证据 |
| `0x00413F00` | `sub_413F00` | `pending_audit` / `not_inherited` | 未审计；旧实现、测试和集成叙述不构成本轮关闭证据 |

## 2. 范围、ABI 与唯一调用者

- 函数体从 `0x00413870 push ecx` 到 `0x0041390B retn`，无入口参数，也没有 callee
  栈清理立即数。
- `0x00413873..0x0041387E` 保存 `EBX/EBP/ESI/EDI`，`0x00413906..0x0041390A`
  逆序恢复这些寄存器和入口 scratch `ECX`。
- 正常外层计数在 `0x004138F4..0x00413900` 递增到三；因此正常返回的 `EAX` 确定为
  `3`。
- LST 的唯一 CODE XREF 是 `sub_412930:0x00412A8D`。该处无参数直接调用，随后
  `0x00412A92` 立即压入另一函数的参数，既不测试也不保存 `EAX`，所以返回值被忽略。
- `0x004138CE` 与 `0x004138DE` 都把当前节点指针作为唯一栈参数；调用后分别在
  `0x004138D4/0x004138E4` 执行 `add esp,4`。`sub_413910` 与 `sub_413CA0` 的入口及
  plain `retn` 与该 cdecl 边界一致。这里没有据此继承两个 callee 的函数体语义或关闭
  状态。

## 3. 基本块到 C++ 的收敛

### 3.1 三组物理顺序

`0x0041387F..0x0041389A` 按外层计数选择三个全局行头基址：

```text
iteration 0 -> 0x004A9A0C -> physical group 2
iteration 1 -> 0x004A9A04 -> physical group 0
iteration 2 -> 0x004A9A08 -> physical group 1
```

C++ 的固定数组 `{2, 0, 1}` 与该分派逐项对应；每组在进入行循环前都重新读取 camera Y
并重算起始行。

### 3.2 有符号相机商、七十行与 padding

`0x004138A0..0x004138B5` 用 `cdq; and edx,0x0F; add eax,edx; sar edi,4`
实现有符号 `trunc_toward_zero(cameraY / 16)`，随后减 `20`。`EBP=0x46` 固定每组
扫描 70 个逻辑行：

```text
q         = trunc_toward_zero(cameraY / 16)
first_row = q - 20
rows      = first_row .. first_row + 69
```

`0x004138BC..0x004138C6` 每次重新读取 `mapHeight`，先按 32 位加法形成
`u32(mapHeight + 20)`，再用 `jnb` 实现：

```text
u32(row) < u32(mapHeight + 20)
```

因此负行按无符号值跳过；普通高度下 `0..H+19` 被接受，`H+20` 被排除。这里的
`mapHeight+20` 明确按 `u32` 回绕，不提升为无界整数。C++ 保留该比较；现代 owner 又在
扫描前要求三个行头数组各至少分配 `H+40` 个槽。对接近 `UINT32_MAX`、无法形成该有界
宿主分配的状态，前置校验会返回 `invalid_spatial_index`，这是保留的安全平台适配，不是
对原裸指针越界行为的模拟。

`0x004138B8 lea ebx,[ebx+edi*4+0x50]` 证明物理槽为 `row+20`。每轮
`inc edi; add ebx,4` 同步推进逻辑行和物理槽；上界失败只跳过本轮，不提前结束其余扫描。

### 3.3 空行、链顺序与 post-callee reload

- `0x004138C8..0x004138CC` 读取行头；零头直接进入下一行，所以合法的全 null 索引不依赖
  角色存储存在。
- 非零节点在 `0x004138CE..0x004138D4` 无条件先调用 `sub_413910`。
- 返回后 `0x004138D7 cmp word ptr [esi+2Ch],0` 从角色记录重新读取 `+0x2C` 低字；只有
  低字非零才在 `0x004138DE..0x004138E4` 调用 `sub_413CA0`。高字单独非零不通过门。
- draw/audio 完成后，`0x004138E7 mov esi,[esi]` 才重新读取 `+0x00` 后继；因此两个
  callee 对节点的合法修改对当帧 gate 与后继遍历可见。
- C++ 不缓存 gate 或 next；测试端口在 draw callback 中把低字从零改为 `42`，再在 audio
  callback 中把 next 从零改为 `2`，实际观察到音频调用与第二节点绘制，固定了该顺序。

原裸指针改成一基 `u32` 链索引；零仍是 null。越界索引在实际遇到时返回
`invalid_role_link`，每条链的计数界限制环，且不会改变有效链的节点顺序。此前对
`roles.empty()` 的 blanket early return 与空行头行为冲突，现已删除：全 null 行头可完成
3 组/210 次扫描；空 span 只有在遇到非零头时才报 `invalid_role_link`。

## 4. 独立测试向量

`tests/unit/world_map/legacy_world_roles_test.cpp` 现在固定：

- camera Y `-17` 得 `q=-1`、行 `-21..48`；H=30 时每组接受 49 行，共 147 行且不访问
  row 49。camera Y `-15` 得 `q=0`、行 `-20..49`，每组接受 50 行，共 150 行，并访问
  group 2 的 row 49；`INT32_MIN/INT32_MAX` 仍完成 3 组/210 次扫描且访问零行。
- H=0/H=3、空角色 span、三组全 null 行头分别完成 3 组/210 扫描，访问 60/69 行、零
  draw/audio callback；同一空 span 在实际遇到非零头时才返回 `invalid_role_link`。
- H=3 时物理 prefix 的 row `-20/-1` 放入越界 poison link，`H+20` 的额外槽也放 poison；
  扫描仍只访问 `H+19` 的有效节点，证明 prefix skip、suffix inclusion 与上界 exclusion。
- 同一行 `1→4` 的节点顺序先于后续 group 0/1，完整回调顺序固定为 `1,4,2,3`，即链序
  与 group `2→0→1` 同时成立。
- `+0x2C=0xFFFF0000` 不进入音频，低字 `42` 才进入。
- callback mutation 同时证明 draw 后 gate reload 与 audio 后 next reload。
- 既有受检失败继续覆盖 frame load 失败、无效/循环链和任一短行头数组；安全隔离不会把
  有效输入提前拒绝。

## 5. 实现、集成与剩余阻断

- `draw_legacy_world_roles` 映射本函数；`compose_legacy_world_runtime_frame` 在
  `0x00412930` 的既有 `world_spatial_objects_00413870` stage seam 调用它。
- runtime seam 继续复用有界角色数组、三组行头、camera、共享 jitter、TSW runtime、
  framebuffer 与音频端口。现有普通角色真实 TSW framebuffer 哈希
  `0xA4766C928B05DC88` 只作为资产与 seam 验证；它不让 `sub_413910` 或
  `sub_413CA0` 继承关闭状态。
- 评审后完整门禁通过：Linux core `185/185`、Linux app `190/190`、Windows LLVM app
  `190/190`；两端应用成功链接，未启动任何游戏 EXE。
- 关闭 disposition 为 `platform_adapted`：行为核心逐指令收敛，同时明确保留有界数组
  预校验、一基链接、无效链接/环隔离。无公共 API 扩张。
- 原程序完整逐帧 framebuffer、audio 调用与共享 jitter 差分仍为
  `blocked_runtime_oracle`。需要时只准备 Frida spawn 工具并等待用户执行；本工作包不
  启动原版或 OpenSWD3 游戏 EXE。
