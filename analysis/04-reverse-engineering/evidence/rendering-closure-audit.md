# B4 `rendering` 有限收口审计

状态：实施中；151 项全集已经固定，按真实缺口逐组收口

审计对象是机器目录中 `module_candidate = rendering` 的 151 个地址。逐项结果见
[`rendering-closure.tsv`](../inventory/rendering-closure.tsv)，生成器为
[`build_rendering_closure_inventory.py`](../../tools/build_rendering_closure_inventory.py)。
生成器锁定完整 LST 与模块所有权目录的哈希，任何地址未分类都会失败。本审计只核对
既定 B4 范围、实现映射和移交边界，不重新展开全局渲染调研。

## 1. 当前结果

- 73 项已有独立实现映射，16 项是已实现函数内部由跳转表选择的物理 PROC 分支；
- 35 项旧 DirectDraw/GDI/Lock 生命周期由 owned framebuffer、glyph atlas 和 SDL3
  平台边界替代；
- 2 项 RLE coverage 路径具有当前资产不可达证据，强制异常状态只保留显式边界；
- 5 项已有公共合同但等待运行时 owner 接线，其中三项是 B4 自己必须完成的
  20/16/12 字体实例绑定，另两项等待 B10 提供战斗 surface；
- 19 项仍是没有实现映射的真实 B4 缺口；
- `0x00436FA0` 并非字体函数，而是直接跳到 `0x004374E0` 输入设备释放函数，移交
  `input_time_rng`，不在 B4 重复实现。

这些数字只表示当前逐地址 disposition，不代表 B4 已闭环。`pending` 和 B4 自有的
deferred binding 清零前，模块不能移交。

## 2. 有限缺口分组

五个 command-stream 函数及三项绘制 helper 已经完成。剩余 19 项按调用依赖合并为以下紧密行为
单元：

1. `0x00411FA0/0x00414B60..0x004153D0`：暂停及四组借用剧情状态的动作绘制；
2. `0x00417050/0x004174D0..0x00417650/0x00417DE0`：描边包装、四个固定
   `16×16` writer 和 packed-row 效果；
3. `0x00420490..0x004207E0/0x00421FB0`：已经有静态规格、尚未转写的六个
   整帧颜色函数；
4. `0x00422C70/0x00423020`：两条 10.10 缩放 RLE writer。

字体运行时绑定单独在上述纯函数完成后接入启动与显示恢复。战斗辅助 surface 只保留
B4 的 source/lifecycle 合同，不提前实现 B10 业务状态。

## 3. 下一执行单元

下一组是 `0x00411FA0/0x00414B60..0x004153D0`。按完整 LST 固定借用状态、更新顺序、
绘制请求和终止边界后立即写 UT 与 C++20，不等待其余 13 项重新调研。
