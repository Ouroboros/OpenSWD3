# 世界空间角色扫描（`0x00413870..0x00413FDD`）

本文只以 `/mnt/e/Game/swd3/swd3.exe_export_for_ai/swd3.exe.lst` 的机器码与指令为
行为真值。当前记录分别独立关闭外层扫描 `sub_413870` 与普通角色绘制 `sub_413910`；
相邻音频与 bit-29 路径不继承关闭状态。

## 1. 逐函数状态

| 地址 | 函数 | 当前状态 | 当前工作包结论 |
| --- | --- | --- | --- |
| `0x00413870` | `sub_413870` | `platform_adapted`，已独立闭环 | `0x00413870..0x0041390B` 已完成 LST→C++→LST 收敛；映射 `draw_legacy_world_roles` 及 runtime 原 stage seam |
| `0x00413910` | `sub_413910` | `platform_adapted`，已独立闭环 | `0x00413910..0x00413C96` 已完成逐基本块 LST→C++→LST 收敛；映射 `draw_legacy_world_role` 与 SDL runtime adapter |
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

## 4. `sub_413910` 独立逐基本块闭环

### 4.1 范围、ABI、唯一调用者与直接调用边界

函数从 `0x00413910 sub esp,8` 到 `0x00413C96 retn`。入口唯一参数是栈上角色指针；
函数保存并恢复 `EBX/EBP/ESI/EDI`，plain `retn` 不清参数，唯一调用者
`sub_413870:0x004138CE..0x004138D4` 执行 `push esi; call; add esp,4`，因此是一参数
cdecl。`0x0041392D` 仅在 drawable mask 失败时令 `EAX=0`；所有其余出口汇入
`0x00413C8A` 并在 `0x00413C8D` 令 `EAX=1`。唯一调用者不读取返回值。

直接边界逐点复核如下；这些边界只证明本函数的调用顺序，不把相邻待审计函数并入本关闭：

- `0x00413983/0x00413C0D`：`sub_40DC50(service)`，一参数 cdecl，调用者各清 `4`，
  `EAX` 分别控制 service `0x0B/0x48` 分支。
- `0x004139A7`：`sub_485750(sound,x,y)`，三参数 cdecl，调用者清 `0x0C`，返回忽略。
- `0x004139C1/0x00413BA2`：`sub_4315D0(resource,frame)`，两参数 cdecl，调用者清
  `8`；`EAX` 是后续直接解引用的帧视图。
- `0x00413A0E`：`sub_4145F0(role,frame,x,y,counter,color)`，六参数 cdecl，调用者清
  `0x18`。
- `0x00413A91/0x00413B7F/0x00413BEF`：`sub_4170E0` 六参数 cdecl，调用者分别清
  `0x18/0x18`，覆盖层路径连同先前两个 frame 参数共清 `0x20`。
- `0x00413C22`：`sub_415EE0(x,y,zero_extend(guid))`，三参数 cdecl，调用者清
  `0x0C`。
- `0x00413C3D` 经 IAT 调用 `lstrlenA`，stdcall 自清一个参数；`0x00413C85` 以
  `ECX=word_4AB998` 调用 `sub_436AD0`，六个栈参数由其 `retn 18h` 清理。

### 4.2 入口门、冻结快照与包含边界（`0x0041391B..0x0041396B`）

`[role+0x10] & 0x8400` 必须精确等于 `0x8000`。通过后，汇编只在此处一次性保存：

```text
EBP        = dword role[+0x04]       // world X
var_4      = dword role[+0x08]       // world Y
var_8      = dword camera[+0x00]     // left
arg_0 slot = dword camera[+0x04]     // top，复用入口参数槽
```

随后以有符号 32 位回绕减法计算 `worldX-left`，`-320` 与 `960` 都包含，之外直接走
返回一。现代 `LegacyWorldRolePlacementSnapshot` 对应这四项；callback 后除标签明确重读
live role X/Y 外，残影、主图、加色、覆盖层和粒子调用都继续使用快照。

### 4.3 资源、service、单次音效和首帧（`0x0041396C..0x004139C7`）

动作 `+0x00` 为零时资源保持 `0xFFFF`；非零时只零扩展读取 `role[+0x8A]` 的 word
（action `+0x4A`），随后 service `0x0B` 非零立即返回一。`role[+0x98]`（action
`+0x58`）按 word 读取；非零时以冻结 `worldY/worldX` 和零扩展 sound id 调音频，返回后才
把该 word 清零。然后重新读取 action `+0x4C` word，与已选资源调用首个
`sub_4315D0`。现代实现保留“播放后清零、清零后载帧”；frame miss 改为受检
`frame_load_failed`，代替原始空指针解引用。

### 4.4 残影与载帧后 reload（`0x004139C8..0x00413A15`）

载帧返回后重新读取 role flags；`AH bit0` 且 `frame_counter&7 < 4` 才进入残影。
颜色索引是 flags `bits20..23`，从 `0x004995D4` 取 dword；X/Y 以当前 action
`+0x10/+0x14` dword offset 减冻结 world/camera 后传给 `sub_4145F0`。因此载帧 callback
可改变 flags、action offset 和 frame counter，但不能改变冻结 world/camera。独立 mutation
测试在 load 中同时改 live role 坐标与 camera，仍固定残影 `(56,154)`、主图 `(65,146)`。
残影继承进入本函数前的共享 jitter；它不会提前载入本角色的 `+0xC8/+0xC9`。

### 4.5 主图捕获点、字节状态和写入（`0x00413A16..0x00413AB7`）

`0x00413A16` 把 action `+0x18` mode flags 捕获进 `EBX`。随后以零扩展 byte 把
`role[+0xC8/+0xC9/+0xCA]` 写入共享 jitter group/phase/opacity，并把
`dword_4CDBE0` 的主绘制辅助槽清零。帧 source、flags、宽高 word、辅助流和 opacity
组成六参数 blit；位置精确为：

```text
x = zext16(role[+0x28]) - dword action[+0x10] - frozen_left + frozen_world_x
y = zext16(role[+0x2A]) - dword action[+0x14] - frozen_top  + frozen_world_y
```

所有加减为 32 位回绕。主 blit 后重新装载 frame source 全局、清辅助/三色全局并重读
role flags。现代主 request 使用捕获 flags；blitter 状态仍只作诊断，和原函数一样不改变
控制流。

### 4.6 加色分支的 live 坐标与捕获 flags（`0x00413AB8..0x00413B90`）

主图之后 `AH bit0` 决定是否装载三项地图角色颜色全局；随后再次重读 flags 的
`bits20..23`，把 `0x00499594` 的有符号 dword 同时加到红/绿/蓝。三项全零跳过；任一
非零则：

- frame source/auxiliary/宽高仍来自首帧；
- flags 必须使用 `0x00413A16` 捕获值，经 `&0x80000013 | 0x10`；
- `role[+0x28/+0x2A]` 两个 word 和 action `+0x10/+0x14` 两个 dword 在主 blit 后重新
  读取；
- world X/Y 与 camera left/top 仍使用入口冻结值。

旧 C++ 恰好反向地缓存主图坐标、重读 mode flags；现已纠正。post-main mutation 向量把
field/action offset 改成 `15/16/30/40`、mode 改成 `8`、live world/camera 改成
`1000/2000/300/400`，加色仍得到 `(55,136)` 与 flags `0x80000013`。

### 4.7 覆盖层、指针 reload 与 phase 回写（`0x00413B91..0x00413C02`）

`role[+0x3C]` 为零跳过。非零原始指针先提供覆盖动作 `+0x4A/+0x4C` 两个 word 载帧；
返回后 `0x00413BB1` 再从 role 重读 `+0x3C`，并从重载动作读取 mode、X/Y offsets。
覆盖位置为：

```text
x = zext16(role[+0x28]) - overlay.drawX - frozen_left + frozen_world_x
y = zext16(role[+0x2A]) - overlay.drawY - live main.drawY
    - frozen_top + frozen_world_y + 28
```

覆盖 blit 的 auxiliary 参数固定零。现代 token owner 在 token 未变时保留同一 action 的
live 字段；载帧 callback 改 token 时重新执行受检 resolve，空 token 返回
`overlay_resolve_failed`。无论正常跳过/绘制还是受检覆盖失败，已发生主绘制后的退出都先
把共享 jitter phase 的低 byte 写回 action `+0x89`；正常路径的原始写点是
`0x00413BF7..0x00413C02`。

### 4.8 粒子、Talk 门与 12 点标签（`0x00413C03..0x00413C89`）

写回 phase 后重新读取 role flags；`AH bit1` 且 service `0x48` 为零才调用粒子，参数是
冻结 world X/Y 与此刻 live `+0x24` guid word。随后重新读取全局 Talk target；只有
`0xFFFF` 且 role `+0x38` 非零才进入标签。`0x00413C43..0x00413C58` 先把
`len*11` 按 `u32` 回绕，再将结果 bitcast 为 `i32` 并向零截断除二；标签位置为：

```text
wrapped   = u32(len * 11)
halfWidth = trunc_toward_zero(bitcast_i32(wrapped) / 2)
x         = live role.worldX - halfWidth - frozen_left + 16
y         = live role.worldY - live action.drawY - frozen_top
```

颜色是 `word[dword_49E0C8 + role[+0x34]*4]`，文字对象固定 `word_4AB998`，style 固定
`4`。因此同一 post-main mutation 的覆盖层/粒子仍为冻结位置 `(83,161)` / `(100,200)`，
标签却为 live `(975,1920)`。现代边界以有界 label span 查找 NUL，缺失 token/terminator
分别返回明确状态；SDL adapter 复用启动 BGR888 内建颜色转色和真实 12 点 glyph runtime。

### 4.9 可达 production seam 与平台适配分类

`WorldRoleRuntimeAdapter` 已从 SDL `SdlDeferredWorldFramePorts` 的三个 no-op 路径抽出并在
普通世界帧实际构造：

- 单次位置音效调用现有 `play_legacy_spatial_sample`，借用 sample manager、调用时的受控角色
  监听者坐标和当前 mix level；
- 粒子调用持久 `LegacyAniRoleParticleEffect`、同一 secondary RNG、角色 selector 受检
  lookup、当前地图及调用时 viewport、`LegacyAniRoleParticleRuntimePorts`，并与 directional 效果
  共享原有 action record；
- 标签色调用统一的 16 项内建色转换；文字在校验刚解析 span 的 NUL 后进入实际 12 点
  `LegacyTextRendererRuntime` framebuffer。

关闭 disposition 为 `platform_adapted`：受检 TSW/overlay/label owner、role selector
lookup、label NUL、颜色索引和现代 framebuffer/audio owner 隔离原始裸指针及平台 API，
有效输入的分支、宽度、回绕、reload、写入和回调顺序保持不变。

## 5. 独立测试向量

`tests/unit/world_map/legacy_world_roles_test.cpp` 现在固定：

- post-main callback 依 LST 顺序观察 main `(65,146)`、additive `(55,136)`、overlay
  `(83,161)`、particle `(100,200)`、label `(975,1920)`；同时证明 additive 使用 live
  `+0x28/+0x2A` 与 draw offsets、捕获 mode，overlay/particle 冻结 placement、label live
  role 坐标与冻结 camera。
- 首帧 load callback 改写 role world 与 camera 后，残影仍为 `(56,154)`、主图仍为
  `(65,146)`，固定 `0x00413934..0x00413957` 快照早于首帧调用。
- action/service、单次音效清零、帧请求、ghost→main、main→additive→overlay→particle→
  label、jitter 写回及受检 frame/overlay/label 失败的既有向量继续通过；真实 TSW
  framebuffer FNV-1a64 仍为 `0xA4766C928B05DC88`。
- 标签半宽的编译期向量以 `len=195225787` 固定 `u32(len*11)=0x80000009` 后的有符号
  向零截断结果 `-1073741819`，防止误用无符号除二。
- `world_role_runtime_adapter_test.cpp` 直接打到 production adapter：构造后把监听者从
  `(1000,1000)` 更新为 `(3,4)`，实际 sample manager 仍按调用时坐标收到音效；构造后把
  camera 从离屏区域更新为目标 viewport，同 seed/节点/角色输入与直接
  `LegacyAniRoleParticleEffect::update` 的 result、RNG 进度和 emitter/node 状态一致；内建
  颜色和 12 点 glyph 写入真实 framebuffer。
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

## 6. 实现、集成与剩余阻断

- `draw_legacy_world_roles` 映射本函数；`compose_legacy_world_runtime_frame` 在
  `0x00412930` 的既有 `world_spatial_objects_00413870` stage seam 调用它。
- runtime seam 继续复用有界角色数组、三组行头、camera、共享 jitter、TSW runtime、
  framebuffer 与音频端口。现有普通角色真实 TSW framebuffer 哈希
  `0xA4766C928B05DC88` 继续作为资产与 seam 验证；`sub_413910` 另由上述完整函数体和
  runtime adapter 证据关闭，它不让 `sub_413CA0` 继承关闭状态。
- 本轮完整门禁通过：Linux core `185/185`、Linux app `191/191`、Windows LLVM app
  `191/191`，两端应用均成功链接且未启动游戏 EXE。首次并行 Windows gate 仅有既有
  `audio_video.legacy_snd_archive_real` 打开资产失败；该测试隔离重跑 `1/1` 通过，待 Linux
  gate 结束后的 Windows 全量顺序重跑 `191/191` 通过。
- `sub_413910` 关闭 disposition 为 `platform_adapted`：行为核心逐指令收敛，同时明确
  保留受检 frame/overlay/label owner、selector lookup 与平台 framebuffer/audio owner。
  唯一新增公共 helper 是 production adapter 必需且复用于既有 frame runtime 的内建颜色
  转换，不扩张无关 API。
- `sub_413CA0/sub_413EA0/sub_413F00` 明确保留 `pending_audit/not_inherited`；本节未改动
  其实现、测试或 inventory 行。
- 原程序完整逐帧 framebuffer、audio/particle/text 调用与共享 jitter 差分仍为
  `blocked_runtime_oracle`。需要时只准备 Frida spawn 工具并等待用户执行；本工作包不
  启动原版或 OpenSWD3 游戏 EXE。
