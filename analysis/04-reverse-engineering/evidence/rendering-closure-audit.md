# B4 `rendering` 有限收口审计

状态：`module_closed_pending_oracle`；151 项全集已完成逐地址处置并移交

审计对象是机器目录中 `module_candidate = rendering` 的 151 个地址。逐项结果见
[`rendering-closure.tsv`](../inventory/rendering-closure.tsv)，生成器为
[`build_rendering_closure_inventory.py`](../../tools/build_rendering_closure_inventory.py)。
生成器锁定完整 LST 与模块所有权目录的哈希，任何地址未分类都会失败。本审计只核对
既定 B4 范围、实现映射和移交边界，不重新展开全局渲染调研。

## 1. 当前结果

- 95 项已有独立实现映射，16 项是已实现函数内部由跳转表选择的物理 PROC 分支；
- 35 项旧 DirectDraw/GDI/Lock 生命周期由 owned framebuffer、glyph atlas 和 SDL3
  平台边界替代；
- 2 项 RLE coverage 路径具有当前资产不可达证据，强制异常状态只保留显式边界；
- 2 项已有 B4 公共合同但等待 B10 提供 owned battle surface；
- 没有仍为 `pending` 的真实实现缺口；
- `0x00436FA0` 并非字体函数，而是直接跳到 `0x004374E0` 输入设备释放函数，移交
  `input_time_rng`，不在 B4 重复实现。

B4 自有的 20/16/12 字体实例绑定已经接入启动、显示停用/恢复和总退出。剩余两项
deferred binding 的 owner 是 B10，其现有 B4 source/lifecycle 合同已足够承接后续接线，
不构成 B4 自身的实现缺口。

## 2. 有限缺口分组

五个 command-stream 函数、三项绘制 helper、暂停层、四项动作/消息协调器、
六项描边/固定 tile/packed-row helper、六项 packed-16 颜色函数，以及
`0x00422C70/0x00423020` 两条 10.10 缩放 RLE writer 已经完成。纯函数与
软件像素 writer 不再有 `pending` 地址。

字体运行时已经在上述纯函数完成后接入启动、显示恢复和退出。战斗辅助 surface 只保留
B4 的 source/lifecycle 合同，不提前实现 B10 业务状态。

## 3. 移交结论

`0x0040F340/0x00435160/0x004351F0` 对应的 20/16/12 renderer 生命周期已经
完成，Linux `core` 64/64 与 Windows LLVM `app` 66/66 CTest 通过。B4 范围内没有
仍为 `pending` 的地址，也没有由 B4 owner 负责的 deferred binding。

原程序 framebuffer 与 DirectDraw RECT 的最终动态差分仍是已登记的
`blocked_runtime_oracle`；根据单模块移交规则，B4 状态为
`module_closed_pending_oracle`。若后续需要该动态证据，只准备捕获环境并等待用户运行
原版，不由开发流程自行启动。下一执行单元转入 B5 `audio_video` 接口级逆向与工作包建立。
