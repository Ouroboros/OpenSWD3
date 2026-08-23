# 标准模式纵向动画面板 `0x0043BD70`

状态：`platform_adapted`、`unit_tested`

## 1. LST物理范围与调用图

唯一行为真值为`swd3.exe.lst`。函数物理范围是`0x0043BD70..0x0043BE3B`，下一入口为`0x0043BE40`。两个直接caller为`0x00441680`与`0x00447100`。

直接callee均已有独立关闭owner：矩形效果`0x0043B110`、九宫格边框`0x0042E850`和格式化文字`0x004306C0`。modern port只转发三类精确请求，实际framebuffer、资源与字形owner继续由既有rendering实现提供，不在B9复制。

函数接收一个文字指针，并读写三个共享owner：position=`dword_4FC830`、velocity=`dword_4FC648`、边框资源低字=`word_4FC5FA`。

## 2. inactive门与速度更新

入口EAX先读取position。只有以下任一条件成立才更新和绘制：

```text
velocity != 0
position == 0x154
```

否则立即返回原position且不调用任何callee。position等于顶部`0x154`时，即使velocity为0也继续绘制。

活动路径先对velocity执行x86 `sar 1`并原位写回。modern以显式位移保留负奇数向负无穷舍入，不依赖宿主除法规则。随后position执行32位回绕减法：

```text
position = wrap_i32(position - velocity)
```

## 3. 方向边界

折半后的velocity严格大于0时向顶部移动；position小于`0x154`才钳到`0x154`并把velocity清0。velocity小于等于0时走下行分支；position大于`0x1E0`才钳到`0x1E0`并清velocity。

边界相等不清速度。velocity由1折半为0后会进入下行分支，position不变但仍绘制。position减法先回绕再做signed比较，因此`INT_MIN/INT_MAX`跨界保持LST行为。

## 4. 三callee顺序与参数

更新后严格按矩形→边框→文字调用：

```text
rectangle(0xD8, position - 8, 0x184, 0x1E6 - position,
          0, 0, 0, 4)

frame_resource = (rectangle_return & 0xFFFF0000) | word_4FC5FA

tiled_frame(frame_resource,
            0xDC, position, 0x254, 0x1D6,
            0, 0x80000008)

formatted_text(text,
               0xDC, position,
               5, 0x168, 4)
```

坐标加减均为32位回绕。原程序在矩形call后只覆盖EAX低16位，因此边框资源精确保留矩形返回高字与共享资源低字。边框返回被忽略；函数最终返回格式化文字callee的EAX。

`LegacyStandardModeAnimatedPanelPorts`按原顺序表达这三类已关闭owner。typed结果保留inactive位置返回或最终文字返回、矩形原返回、合成资源ID、是否绘制及是否边界钳制。

## 5. 验证

`special_modes.legacy_initial_menu`覆盖：

- velocity为0且position不在顶部时零callee早退并返回position。
- velocity为0且position为`0x154`时仍执行三次绘制。
- 矩形、九宫格与文字调用顺序严格为1/2/3。
- 三类请求的全部坐标、尺寸、flags、line count、width和style常量。
- 矩形返回高16位与共享frame resource低16位合成。
- 正velocity普通上移和越过顶部后的position/velocity钳制。
- 负velocity普通下移和越过底部后的钳制。
- velocity 1折半为0后position保持但继续绘制。
- `INT_MIN/INT_MAX`位置减法双向回绕。
- 活动路径最终返回formatted text port值。

定向测试通过。workpack连续生成两轮均为`24/227`，SHA256均为`8faa1d23648b963401dee5a701dc7b237e975b80ad205a650793d603e3709621`；只新增关闭`0x0043BD70`，`0x0043BE40`仍为下一独立模块9单元。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
