# 原分支 primary presentation 请求

状态：21 个 primary 提交点的地址、源、RECT、等待语义和返回值策略已按完整 LST 固定为平台无关合同；稳定顶层分支已经通过该合同请求 SDL3 呈现，统一 accepted-frame 尾部的额外 present 已移除。

## 证据范围

唯一行为真值是 `swd3.exe.lst`。完整 surface 虚表 `+0x14` 调用中，写向 primary surface 的 21 项已经由 [`primary-presentation-paths.tsv`](../inventory/primary-presentation-paths.tsv) 逐地址列出，并由 [`build_frame_presentation_inventory.py`](../../../tools/build_frame_presentation_inventory.py) 锁定。

21 项合同的物理分布为：

- 18 项 source/destination RECT 都为 NULL，即整 surface 提交；
- 2 项属于 `sub_45E7D0` 战斗纵向位移，使用调用者按状态表构造的动态 source/destination RECT；
- 1 项属于 Bink，source/destination 都使用原值 `{0,0,639,479}`；
- 9 项使用 `DDBLT_WAIT (0x01000000)`，12 项 flags 为零；
- 19 项从游戏 framebuffer 提交，另两项分别从战斗快照 surface 和临时 screen surface 提交；
- 19 项忽略普通结果，暂停路径把 HRESULT 留在 EAX 但调用者不读，Bink 路径显式分派错误结果。

不得把 Bink 的 `639/479` 改为 `640/480`，也不得把两次战斗位移提交或任意两个分支提交合并。

## 现代请求合同

`LegacyPresentationSite` 直接以 21 条 Blt 指令地址作为枚举值。每个 `LegacyPresentationContract` 固定：

```text
call site
source surface role
full / dynamic / fixed-Bink RECT contract
immediate / wait synchronization
ignored / pause-return / Bink-dispatch result policy
```

`submit_legacy_presentation` 只把合同展开成 `LegacyPresentationRequest` 并交给平台端口：

- 整 surface 请求保持 source/destination RECT 不存在，不能伪造 `{0,0,640,480}`；
- 两个动态请求缺少调用者 RECT 时返回 `dynamic_rectangles_required`，不会调用平台；
- Bink 请求自动产生两份相同的 `{0,0,639,479}`；
- 未登记地址返回 `unknown_site`；
- 平台失败以 `backend_failed` 交给兼容外壳处理。

请求层不复制 DirectDraw COM ABI，也不把 `DDBLT_WAIT` 解释为某个特定 SDL/GPU API；它保留的是原提交点的同步可见顺序。

## 顶层接入修正

此前 SDL smoke 在 `run_accepted_frame` 返回后无条件上传并 present。这会给原本提前返回、未知特殊模式或关闭前跳过绘制的路径凭空增加一次提交，与汇编矛盾。

当前接入改为由已有互斥分支端口请求：

| 现代分支端口 | 原提交点 |
|---|---:|
| `step_high_priority` | `0x00408D5E` |
| `present_pause` | `0x00412046` |
| `finish_world_frame` | `0x00412716` |
| `step_standard_special_mode` | `0x0043A854` |
| `step_shop_mode` | `0x0044F765` |
| `step_battle` | `0x0045350A` |

accepted-frame 公共尾部只继续维护音频和检查关闭，不再提交画面。Bink、战斗动态矩形以及 14 个瞬时 UI/剧情/转场调用点仍由各自尚待实现的 B5/B7/B8/B9/B10 所有者在原指令位置发出；B4 已提供同一份不可随意改写的合同。

当前 SDL smoke 只消费“游戏 framebuffer + NULL RECT”的请求。快照、临时 surface、动态 RECT 和 Bink 局部提交要在 B4.8/B5/B10 接入相应 owned surface 后实现，不能在 smoke 中假装成整帧提交。

## 实现与验证映射

- 公共合同：`include/openswd3/rendering/legacy_presentation.hpp`；
- 合同表与分派：`src/rendering/legacy_presentation.cpp`；
- SDL 稳定分支接入：`src/platform/sdl3/main.cpp`；
- UT：`tests/unit/rendering/legacy_presentation_test.cpp`。

UT 逐项比较全部 21 个地址及五类合同字段，并固定 `18/2/1` RECT 分布、`9/12` wait 分布、动态 RECT 原值传递、Bink 固定 RECT、未知地址和平台失败。

当前整批回归结果：Linux `core` 52/52、Linux/Windows `app` 54/54 CTest
通过；未启动 OpenSWD3 或原程序。

当前证据等级：21 项静态合同和稳定分支请求位置为 `assembly_exact`；SDL 对完整 framebuffer 的既有上传路径为 `platform_adapted`；动态/局部 surface 接入和原程序 framebuffer 差分仍为 `blocked_runtime_oracle`。
