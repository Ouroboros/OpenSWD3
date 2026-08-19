# 普通世界开发调试叠层（`0x00413FE0..0x00414567`）

状态：`assembly_exact`、`unit_verified`、`platform_adapted`、
`sdl_runtime_integrated`；尚未 `original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。IDA 已把函数原型识别为
`int __cdecl sub_413FE0(int, int)`，逐条指令也只访问 `[esp+arg_0]` 与
`[esp+arg_4]`。

## 1. 调用合同与三个开关

普通世界主帧在 `0x004126CC..0x004126ED` 只于 `dword_4CAE98 == 1` 时调用本函数。
调用者依次压入常量 `2`、camera top、camera left，并在返回后回收 12 字节；但
`sub_413FE0` 只读取 left/top 两个参数，常量 `2` 从未被读取。它是调用者遗留的多余
栈字，不是第三参数，不能在现代接口中伪造含义。完整物理范围是
`0x00413FE0..0x00414567`，唯一调用者不读取返回寄存器；函数自身也没有统一的 `EAX`
返回合同，只按各出口恢复 `EDI/ESI/EBP/EBX` 和八字节局部栈。

函数入口无条件对 16 点文本 renderer 执行：

```text
00413FEC  sub_435660(0xFFFE)  ; 禁用背景填充
00413FFD  sub_435670(0)       ; secondary color 清零
```

随后两个内部开关也都使用“恰好等于 1”的判断：

| 原全局 | 行为 | 物理顺序 |
| --- | --- | --- |
| `0x004CAEA0` | 碰撞格/标志位轮廓 | 先执行 |
| `0x004CAE9C` | 诊断文字、地图事件、附近角色 | 后执行 |

值 `2` 不按 truthy 处理。外层 `0x004CAE98` 是开发工具总门，不应再命名成“地图标记”。

## 2. 五次碰撞格扫描

`0x00414015..0x004140CA` 以相同 camera left/top 连续调用五次 `sub_430230`：

| 次序 | cell flags mask | 颜色 | inset | outline extent |
| ---: | ---: | --- | ---: | ---: |
| 1 | `0x40000000` | `R + G + B` | 0 | 15 |
| 2 | `0x10000000` | `G + B` | 2 | 13 |
| 3 | `0x000000FF` | `R + G` | 4 | 11 |
| 4 | `0x20000000` | `R` | 6 | 9 |
| 5 | `0x00800000` | `R + B` | 8 | 7 |

`R/G/B` 分别来自 `word_49E0B4/B6/B8`。`sub_430230` 从
`((camera + 16) >> 4)` 对应的格开始，每次固定扫描 28 行 × 38 列；屏幕原点按
`-(camera & 0x0F)` 对齐。命中后调用 `sub_430180`。

轮廓函数保留原程序并不对称的写法：上下边按 `extent / 2` 个双像素写入，所以奇数
extent 的水平终点为 `x + 2 * floor(extent/2)`；左右边则写满 extent 行。实现没有把
它修正成几何上更规整的矩形。

## 3. 诊断文字

`0x004140D9..0x0041435D` 先用入口时的 camera/mouse 计算格索引并读取 cell flags
低字节；该事件号在任何文字 callback 之前冻结。随后才检查受控角色并通过 `sub_4302F0`
输出七条固定基线文字，位置为 `x = 8`、`y = 0,16,...,96`，最终由 16 点 renderer
以 flags `0x10` 绘制：

1. 主角世界坐标与 camera tile；
2. 鼠标地图格与屏幕坐标；
3. FPS、frame-used 与键盘重复参数；
4. MapID、MapCyc 与调试值；
5. unlock、UI point、game time、frame state、battle mode、button input；
6. Talk GUID/ID 与两个调试值；
7. scene/story 游标、playing、当前 mode 及 `Nil/Cyc2/Rep1` 状态。

FPS 仍执行无符号 `1000 / frame_interval`，且物理除法位于 MAct 与 Mouse 两次文字调用
之后：OpenSWD3 的零除隔离保留这两次先行副作用，再在原 DIV 点返回。旧实现曾在任何文字
之前整体拒绝零间隔，现已修正。旧版 256 字节 `vsprintf` 越界同样只建立受检失败边界；
正常输入的格式、宽度、参数顺序和输出字节不变。

### 3.1 地图事件

函数用 `(camera + mouse) >> 4` 定位格，并取 32 位 cell flags 的低字节作为事件号：

- 事件号为 0：不输出事件信息；
- 查找失败：在 `y=432` 输出原 Big5 错误文字及事件号；
- 查找成功：在 `y=448` 输出事件名，在 `y=464` 输出剧情号、两个 flag id 和状态。

两个 flag 的查询顺序不能交换：汇编先查 `event.field_0c` 高 16 位并选择
“不可通過/可通過”，再查低 16 位并选择“可點選/不可點選”。低位查询返回后，汇编在
`0x00414404` 第三次重读完整 `field_0c`，所以最终格式中的低/高数字允许与先前选定的
状态文字来自不同 callback 时点；实现与 mutation UT 已固定这一重读。找到事件后即使
现代 name span 为空，也仍提交一次只含 NUL 的 `y=448` 文字调用，保留原 `event+0x14`
调用次数。

### 3.2 附近角色

`0x00414440..0x0041455A` 从角色索引 1 开始扫描到 role count，跳过 action id 为零的
记录。鼠标世界坐标必须以无符号严格比较落在：

```text
role.x - 16 < mouse.x < role.x + 48
role.y - 64 < mouse.y < role.y + 16
```

每个命中角色在 `y=416/432` 输出 Big5 摘要和 ASCII 细节；从第二个命中开始，还会在
`y=400` 重复输出“角色重疊”。摘要返回后，`0x0041450D..0x00414520` 再重读 flags、
Path、Talk 与 GUID 供第二行使用；mutation UT 证明第二行可观察 callback 后值。角色循环
中的三个文字调用点可重复执行，因此静态共有 13 个 `sub_4302F0` call site，动态调用数
不固定。

## 4. 实现与验证边界

`draw_legacy_world_debug_overlay` 直接拥有完整函数，不再通过 generic outer stage 转交。
coordinator 在 `0x004126E8` 对应原槽传入共享 framebuffer、地图 flags、事件表、角色表、
像素 mask 和临时滚动后的 camera；SDL adapter 已把文字请求接到 16 点 legacy glyph
renderer。

独立 UT 固定五次 38×28 扫描、五种 mask/color/inset、奇数轮廓像素数、两个开关的精确
等一、七条固定文字、入口事件号快照、事件高位后低位查询及查询后数字重读、空事件名调用、
DIV-zero 前两次文字、事件格→受控角色失败顺序、附近角色严格边界、重叠输出及摘要后字段
重读。coordinator UT 另固定外层总门、原槽顺序和失败停止点；两项定向 CTest 通过。

原裸 framebuffer/cell/event/role 指针、无界 `vsprintf` 与 DIV trap 由受检 owner 和明确
失败状态隔离；协调器传入同帧 camera/mouse/debug 状态快照。有效域的五次扫描、文字字节、
查询和 reload 顺序保持不变，因此 closure disposition 为 `platform_adapted`。稳定快照的
Linux core `185/185`、Linux app `191/191`、Windows LLVM app `191/191` 完整门禁通过，
两端应用成功链接；未启动原版或新版 EXE。原程序动态差分若需要，只准备 Frida spawn
工具并等待用户执行，不由开发流程启动原版。
