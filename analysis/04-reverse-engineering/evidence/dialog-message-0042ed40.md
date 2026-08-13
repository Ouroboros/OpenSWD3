# 剧情对话消息驱动（`0x0042ED40`）

来源：`swd3.exe.lst` 完整汇编。机器码与指令是唯一行为真值；IDA 伪码和符号只用于定位。

## 1. 范围与所有权

`sub_42ED40` 的完整范围是 `0x0042ED40..0x0043017C`。普通世界在
`0x00412B43` 调用它，其他场景也复用同一函数；它不是 world-map 私有的“限时 UI”，
而是 `story_scene` 拥有的对话消息链驱动。普通世界只在原帧槽借用该 owner。

实现先闭环两个可以独立按汇编验证的内核：

- `0x0042EDCF..0x0042F11E` 的开窗几何、四步过渡、边框请求与文字 clip；
- `0x0042F43A..0x0042FE14` 的逐字显示、分页和控制码协议。

随后完成 `0x0042F11E..0x0042F43A` 的输入/超时门、临时文字表面与最终合成、标题
绘制，以及 `0x0043004D..0x0043017C` 的链清理和两个 action 更新。普通世界原帧槽
现在借用同一个 `story_scene` owner 与真实 framebuffer 适配层；`0x0042ED40` 已从
normal 世界的外部 stage 列表删除。剧情 VM 创建消息与选择链输入仍由后续
`story_scene` 调用者接线，不在本函数 owner 内伪造。

## 2. 物理记录

`sub_40AFF0/sub_42ED40` 消费固定 `0x4C` 字节 IA-32 记录。现代实现保留全部字段偏移，
其中旧指针槽仍表示 32 位 token，真正的字符串、选择区和链所有权放在记录外：

| 偏移 | 字段 |
|---:|---|
| `+0x08` | flags |
| `+0x14/+0x16/+0x18` | transition、role、display counter |
| `+0x1A..+0x24` | 窗口位置、锚点、宽高 |
| `+0x28/+0x2A` | 字符延迟与倒计数 |
| `+0x2C..+0x35` | 当前/分页保存的颜色与文字样式 |
| `+0x38/+0x3C/+0x40/+0x44/+0x48` | allocation、cursor、page stop、caption、next 的旧指针槽 |

编译期 `sizeof/offsetof` 断言固定这些 ABI，不按地址数组照搬成无类型全局块。

## 3. 开窗几何

实现保留四条汇编路径：直接矩形 bit `0x40`、隐藏面板 bit `0x80`、显式锚点
bit `0x800`、alternate direct transition bit `0x8000`。普通 role、`0xFFFD`
分离锚点和显式锚点各自使用原公式；加减乘保持 IA-32 回绕，带符号除四遵循汇编的
截断结果，transition word 也保留 16 位回绕。

原函数在面板之后调用 `sub_416FF0(left, top, left + right, top + bottom)`。这个看起来
不规范的 double-origin clip 参数按指令原样保留，没有擅自“修正”为普通矩形。

## 4. 文字字节协议

控制码按内存字节顺序解释：

| 字节 | 行为 |
|---|---|
| `%Q` | 终止当前文字，设置终止位和生命周期起点 |
| `%N` | 换行；满高时形成分页边界 |
| `%L` | 延迟分页边界；不重新装载字符倒计数 |
| `%P` | 立即分页并保存颜色/样式 |
| `%Sx` | `delay = 2 * (base + (x-'0'))` |
| `%Cx` | 同时修改前景和副色 |
| `D%x` | 只修改副色；汇编真实拼写不是 `%Dx` |
| `%Gx` | 文字样式，`0→4`、`1→0x10`、其他保留原截断值 |
| `%B... .` / `%b... .` | 选择文字；大写版本建立热点并原地改成小写 |
| `%A... .` / `%a... .` | 保留旧十进制解析副作用；大写版本播放固定音效并改成小写 |
| `%K` / `%k` | 进入 closing；原字符串不改写 |

普通文字以首字节 bit 7 判断一字节或两字节，未强行套用 Unicode 字符边界。分页保存
样式是对 `+0x35` 做 OR，不是赋值。宽度溢出导致的满页也不会在非 fast 路径擅自更新
`+0x40`；只有 `%N` 分支消费换行并写入对应边界。

## 5. 受检缓冲边界

原函数使用 256 字节文字/选择临时栈缓冲区和 16 字节音效参数缓冲区。正常输入的顺序、
计数和原地改写完全保留；会造成越界的输入在现代边界返回明确状态。选择热点向量分配
失败也从 `noexcept` 边界转成状态，不触发隐式终止。空音效参数仍通过
`parse_legacy_decimal_or_terminate` 保留原版无数字故障合同。

## 6. 外层驱动与真实绘制端口

- 空链头在分配临时 surface 前直接返回；非空链复用固定 `40*11 × 11*11 = 440×121`
  的 16 位文字 surface，每帧先清零，每条消息合成后再次清零，链结束后释放借用。
- 面板资源来自消息 `+0x00` action 当前的 `field_4a`，以 margin `0x10` 调用已闭环的
  tiled-frame renderer；物理记录继续只保留 32 位 pointer token，64 位 live action
  owner 放在记录外，未破坏 `0x4C` ABI。
- 正文使用 20 点字形，caption 使用 16 点字形；两套 `0x0049E0C8` 与
  `0x0049E108` 各 16 项 BGR888 表按 `0x00424D50..0x00424DE6` 转为当前 16 位像素。
  20 点只描述 mask geometry；普通游戏循环已在 `0x0040AA6C..0x0040AA8B`
  把三套 renderer 的步进覆盖为 `22/18/16`，因此正文双字节字符按 22 前进。
  `sub_430350` 的背景 `0xFFFE`、副色、前景与 style 设置顺序保留。
- 选择框调用已闭环的 `sub_40DE50` 动画边框；caption 先按 `sub_417DE0` 混合 15 行，
  再绘制 action frame 和文字。临时 surface 首像素为 `0xFFFF` 时先改为 `0xFFFE`，
  避免被公共 blitter 误判为 RLE 源，保留原分支。
- end/next 两个持久 action 均为 `0x2329`，base variant 分别为 `0x0C/0x0E`；图标位置
  使用 panel 右下角减 action draw offset 再减 `4/8`。帧尾严格先更新 end 再更新 next，
  两次失败都只计数而不终止整帧。
- 关闭消息时保留 role flags bit `0x800` 的 action 更新门、`0xFFFD` 分离 owner、
  flagged counter 的 bit `0x8000`、`interaction_gate` 清理和先合成后删链顺序。

## 7. 实现与验证

实现与测试：

- `include/openswd3/story_scene/legacy_dialog_geometry.hpp`
- `src/story_scene/legacy_dialog_geometry.cpp`
- `include/openswd3/story_scene/legacy_dialog_text.hpp`
- `src/story_scene/legacy_dialog_text.cpp`
- `include/openswd3/story_scene/legacy_dialog_control.hpp`
- `src/story_scene/legacy_dialog_control.cpp`
- `include/openswd3/story_scene/legacy_dialog_runtime.hpp`
- `src/story_scene/legacy_dialog_runtime.cpp`
- `include/openswd3/world_map/legacy_world_dialog_runtime.hpp`
- `src/world_map/legacy_world_dialog_runtime.cpp`
- `tests/unit/story_scene/legacy_dialog_geometry_test.cpp`
- `tests/unit/story_scene/legacy_dialog_text_test.cpp`
- `tests/unit/story_scene/legacy_dialog_control_test.cpp`
- `tests/unit/story_scene/legacy_dialog_runtime_test.cpp`
- `tests/unit/world_map/legacy_world_dialog_runtime_test.cpp`

UT 覆盖物理布局、四种几何路径、role/分离锚点、回绕、全部控制码拼写、DBCS、逐字显示、
颜色/样式/速度、选择热点与原地改写、两类分页差异、`%L` 倒计数、固定缓冲区边界、
输入/超时/关闭门、surface 生命周期、外层调用顺序、真实文字/合成端口、caption、
持久 action 以及 role owner 清理。Linux Clang `core` 164/164、Windows LLVM `app`
168/168 CTest 通过；Windows 应用成功链接，未启动任何 EXE。

当前函数 owner 与普通世界调用槽验证等级为 `assembly_exact`；剧情 VM 消息生产接线不在
本函数范围，原程序像素动态差分仍为 `blocked_runtime_oracle`。
