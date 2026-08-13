# 世界角色地表占用：`0x0040AE20/0x0040AEC0`

状态：完整 LST 与 C++ 已完成双向逐基本块追溯；有效地图状态为
`assembly_exact`，原程序动态差分仍为 `blocked_runtime_oracle`

来源：`swd3.exe.lst` 完整汇编。汇编是唯一行为真值。

## 1. 公共足迹顺序

两个函数都从角色 `+0x0C` 取得地表锚点，从动作子记录 `+0x6C/+0x70` 取得宽和高。
它们无条件先处理锚点一次，再分派以下三种特例：

- `height=1,width=2`：再处理右侧相邻格；
- `height=1,width=1`：直接返回；
- `height=2,width=1`：再处理下一行同列格。

其余尺寸进入 `height × width` 双循环，因此锚点会被重复处理一次。宽或高为零也不撤销
入口已经发生的锚点写入。现代实现保留这一重复与零尺寸行为；旧 32 位格指针改为受检
格索引，只在损坏状态越出 owned surface 时停止。

## 2. 清除占用 `0x0040AE20`

角色 `u16(+0x24)` 等于当前选中 GUID 时，逐格执行：

```text
cell &= 0xCF7FFEFF
```

否则执行：

```text
cell &= 0xCF7FFFFF
```

两者都清 bit 28/29 和 bit 23；选中角色额外清 bit 8。比较只使用角色 GUID 的零扩展
16 位值。

## 3. 标记占用 `0x0040AEC0`

基础掩码由角色 flags 精确组合：

```text
flags bit 14 clear -> 0x10000000
flags bit 14 set   -> 0x30000000
flags bit 4 set    -> OR 0x00800000
GUID == selected   -> OR 0x00000100
```

随后按公共足迹顺序逐格 OR。`test ch,0x40` 和 `test cl,0x10` 分别对应完整 dword 的
bit 14 与 bit 4，不能按反编译变量宽度猜成两个独立字段。

## 4. 收敛验证

实现位于 `legacy_world_role_surface_occupancy.cpp`。定向 UT 覆盖两种清除掩码、四项标记
位组合、`1×1/1×2/2×1` 特例、一般路径重复锚点和现代越界隔离。本轮先从 LST 独立记录
分支和写入顺序，再从实现逐项反查；最后一轮没有未解释的有效状态差异。
