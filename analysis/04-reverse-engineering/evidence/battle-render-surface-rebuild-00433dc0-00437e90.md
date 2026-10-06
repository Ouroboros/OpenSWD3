# 战斗绘制surface协调重建 `0x00433DC0` 与渲染surface度量getter `0x00437E90`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围、ABI与归属

### `0x00437E90`

权威LST范围为`0x00437E90..0x00437EA2`，入口`proc`至`endp`连续，共14行，没有外部`FUNCTION CHUNK`。

它由渲染模块拥有，ABI为thiscall：ECX是旧DirectDraw owner，两个栈参数是输出指针。两个caller为战斗协调`0x00433DC0`和字体surface初始化`0x00435160`。

### `0x00433DC0`

权威LST范围为`0x00433DC0..0x00433E19`，入口`proc`至`endp`连续，共46行，没有外部`FUNCTION CHUNK`。

ABI为thiscall：ECX是战斗绘制owner，唯一栈参数是旧DirectDraw owner，callee清栈并`retn 4`。唯一caller是战斗初始化`0x00451B10`，调用后立即写入固定值，旧EAX残值不被消费。

直接callee为跨模块getter，以及已关闭surface行表重建`0x00433E90`、矩形`0x004342E0`和主行表重建`0x00433E20`。

## 2. getter字段职责

旧DirectDraw primary/back surface创建链把`DDSURFACEDESC`写入owner：

- owner `+0x5C`来自描述符`+0x10 lPitch`，是每行字节数；
- owner `+0x60`来自描述符`+0x08 dwHeight`，是surface高度。

`0x00437E90`按此顺序把两个i32原样写到输出指针。它不返回宽度、不把pitch除二，也不验证正数、偶数或固定640×480。

现代`query_legacy_surface_pitch_and_height`从`rendering::LegacySurfaceGeometry`复制`pitch_bytes`和`height`，忽略该结构的逻辑`width`。这是旧DirectDraw字段的typed平台适配，不把现代逻辑宽度冒充旧getter输出。

## 3. 战斗协调顺序

`0x00433DC0`严格执行：

1. 调用getter，取得入口`pitch_bytes`与`height`；
2. 以x86有符号向零除二计算`row_stride = pitch_bytes / 2`；
3. 调用surface行表重建，参数为`row_stride, height`；
4. 调用矩形callee，参数为`left=0, top=0, width=row_stride, height=height`；
5. 调用主行表重建，固定参数为`0x500, 0x300`。

B11复核撤销了此前“交换宽高并使用原始pitch”的错误结论。以函数入口ESP为S：

- 两次push后，getter的pitch输出槽是`S+4`，高度输出槽是`S-4`；
- `00433DE8`在两个行表实参已压栈时写`[esp+14h]`，即把`pitch/2`写回`S+4`；
- 行表callee在`00433EFC`执行`retn 8`，caller的ESP恢复为`S-8`；
- `00433DF1/00433DF5`分别从`S-4/S+4`读取高度与已经除二的pitch；
- 随后依次push高度、半pitch、0、0，矩形callee得到`0,0,row_stride,height`。

旧实现、旧测试与旧证据共同漏读了栈槽回写及压栈方向。修正依据是完整LST，
不是按现代尺寸习惯归一化。源逻辑width仍不参与计算。

默认pitch1280、高480时：

- surface行表为640×480，最后偏移`0x4AD80`；
- 矩形为左0、上0、右640、下480；
- 主行表随后固定为1280×768，最后偏移`0xEFB00`。

## 4. 申请失败与typed-stop

两个行表callee已关闭，故障语义直接继承且不经opaque callback：

- surface普通申请失败：旧surface元数据保留，仍执行矩形与主行表重建；
- surface容量不足：只在原首次越界行表写点typed-stop，保留新surface元数据和写入前缀，不执行矩形或主行表；
- 主表普通申请失败：旧主表元数据保留，函数正常结束；
- 主表容量不足：只在原首次越界写点typed-stop，已完成surface和矩形前缀全部保留。

行表申请字节数继续由callee按`row_count * 4`低32位回绕。协调函数不提前验证pitch、高度、奇偶、乘法或矩形。

负奇数pitch使用C++20有符号除法向零截断，与原`cdq; sub eax,edx; sar eax,1`一致；例如`-5 / 2 == -2`。

## 5. typed结果

现代`LegacyBattleRenderSurfaceRebuildResult`记录：

- getter的pitch与高度snapshot；
- surface行表callee结果；
- 矩形是否发布；
- 主行表callee结果；
- 正常、surface写越界、主表写越界三种总状态。

该结果只观察分支与typed-stop，不宣称是旧EAX合同。旧函数最终正常EAX来自主行表callee，唯一caller不使用。

## 6. 双向追溯

LST到C++：

- `0x00437E90..0x00437EA0`：pitch、高度两次原样复制；
- `0x00433DC4..0x00433DD2`：建立输出槽并调用typed getter；
- `0x00433DD7..0x00433DEC`：pitch向零除二，以高度重建surface行表；
- `0x00433DE8`：保存有符号半pitch，后续矩形宽度从该槽读取；
- `0x00433DF1..0x00433E01`：以半pitch和原高度发布矩形；
- `0x00433E06..0x00433E12`：固定1280×768主行表；
- `0x00433E17..0x00433E19`：返回并清理一个栈参数。

C++到LST：

- getter只复制两个已证明字段；
- 三个已关闭battle callee各有唯一直接调用点；
- surface参数、矩形四参数和主表固定参数均有逐项来源；
- 两个typed-stop只落在所属callee原写点；
- 没有新增宽度读取、尺寸归一化、矩形修正、重试或后缀伪造。

完整正向与反向追溯未发现未解释基本块、chunk、callee、字段、参数或出口。

## 7. 测试与动态差分

渲染getter定向测试覆盖负pitch、无关width和负高度的逐值原样复制。

battle协调测试覆盖：

- 默认pitch/高度的两张完整行表及请求字节；
- 默认矩形为0、0、640、480；
- 两次普通申请失败仍继续并保留旧元数据；
- surface写越界阻断矩形与主表；
- 主表写越界保留已完成surface与矩形；
- 七组额外pitch/高度向量覆盖奇数、零、负数与i32上下界，直接断言四边；
- 普通申请失败保留旧10×20尺寸时，矩形为-630、-460、10、20。

本函数不消费物理游戏资产。B11新增断言已在旧实现复现10处失败，
日志为`build/tmp/runtime/battle-render-rectangle-red.log`。
修正后定向core与AddressSanitizer各1/1、SDL链接通过，日志为同目录
`battle-render-rectangle-{core,asan,sdl}.log`。ASan构建仅出现既有
`legacy_battle_outcome_resolution_test.cpp:137`的u16到u8窄化警告。
唯一核心caller仍在初始化中直接调用该入口，typed-stop继续阻止后续输出配置；
本轮没有证明完整SDL初始化或实机画面。

当前没有可用原版DirectDraw描述符、战斗owner行表和矩形的联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。完整60行跨模块LST、字段来源、typed实现与固定状态已经闭环。
