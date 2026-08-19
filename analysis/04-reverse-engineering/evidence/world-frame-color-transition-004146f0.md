# 世界帧颜色过渡（`0x004146F0`）

状态：`assembly_exact`、`unit_verified`、`platform_adapted`、
`sdl_runtime_integrated`；尚未 `original_diff_verified`

本证据只以 `swd3.exe.lst` 为行为真值。`sub_4146F0` 的完整物理范围是
`0x004146F0..0x004147DE`，唯一调用点 `sub_412930:0x00412BCB` 固定传入一并清理四字节，
所以它是单个 dword 参数的 cdecl。函数不保存非易失寄存器，以 plain `retn` 返回；早退
`EAX` 来自 x87 状态字，正常出口沿用 `sub_420490` 的残留值，唯一调用者均不读取。

该调用位于 `sub_412930` 三条主体路径会合后的公共尾部，在角色头像动作、条件数字和世界
指示器之后、限时消息之前执行。直接被调边界是三次无栈参数的 x87 `sub_489654` 和一次
五参数 `sub_420490`。

## 1. 状态与入口门

三组状态的地址和现代字段为：

| 通道 | current | target | step |
| --- | ---: | ---: | ---: |
| red | `0x004A93E0` | `0x004A93DC` | `0x004A9A00` |
| green | `0x004B6C70` | `0x004B6C74` | `0x004A94B8` |
| blue | `0x004C9A00` | `0x004C9A04` | `0x004C97F0` |

`0x004A9934` 是有符号 countdown。函数依次以 x87 `fcomp 0.0` 检查 red、green、blue
三个 step；只有三个比较都设置 C3 才直接返回。因每次只测试状态字的 C3，NaN 与零一样
通过该单项检查，但“一个 NaN 加另一个非零 step”仍继续推进。OpenSWD3 明确保留这个
unordered 组合，而不是用任一 NaN 整体短路。

## 2. 推进顺序

调用参数非零且 countdown 非负时先执行一次 32 位减一。随后重新读取 countdown：

- 非负：依次执行 `current += step`，三个加法和写回顺序为 red、green、blue；
- 负数：把三个 target 的原始 32 位浮点 bit 复制到 step，不推进 current；
- 无论走哪条非空路径，都继续用 current 产生本帧颜色偏移。

调用点固定传入 `1`，但 helper 仍保留参数为零时不递减、随后照常按 countdown 分支的
合同。函数不会在 countdown 变负时把 current 直接写成 target；原汇编写的是
`step = target`，该反常状态不能按常见插值逻辑修正。

## 3. 全帧像素调用

三个 current 依次按 blue、green、red 装入 x87。`sub_489654` 暂时把 x87 rounding
control 改为向零，执行 `fistp qword`，再只返回 64 位整数的低 32 位；NaN、无穷和 64 位
越界的 integer-indefinite 因此都得到低 dword 零，`4294967040.0F` 则得到 `-256`。cdecl
压栈后，`sub_420490(framebuffer,0x4B000,red,green,blue)` 接收到正常 RGB 顺序。
`0x4B000` 恰为 `640*480`，因此忽略当前 clip，对整个 16 位 framebuffer 逐像素做饱和
通道偏移。

`sub_420490` 在每个 u16 位置用 `mov eax,[esi]` 读取一个 dword，最后一个逻辑像素也会
读取其后的 word，但只写回当前 u16。现代 owned framebuffer 因此额外分配一个可读
保护 word；它不属于 physical surface，不计入 byte size、行视图、SDL 上传或逻辑哈希。
这不是新增画布像素，而是精确承接原循环读取宽度的存储边界。

## 4. 运行时与验证

`LegacyFrameColorTransitionState` 由普通世界效果状态跨帧拥有；
`update_legacy_frame_color_transition` 在 `frame_color_update_004146f0` 原 stage 直接操作
同一 framebuffer 和当前 pixel conversion。idle 与正常完成继续后续公共尾部，只有现代
存储边界失败才停止组合并报告 `frame_color_failed`。

原裸 framebuffer、固定像素格式全局和末像素越界 dword read 改由 owned framebuffer、
明确 format 与只读 guard word 承担；有效域状态与像素顺序不变，因此 closure disposition
为 `platform_adapted`。

独立 UT 固定三通道推进、参数零时不递减、递减后变负及入口已负时的 target→step 原始
行为、三项 NaN/零早退、mixed NaN 继续执行、x87 qword→low-dword 转换、全 `0x4B000`
像素覆盖、最后逻辑像素处理及保护 word 不被写回。frame-color 与 world-frame
composition/runtime synthetic/real 五项定向 CTest 通过；Linux core `185/185`、Linux app
`191/191`、Windows LLVM app `191/191` 完整门禁通过，两端应用成功链接且未启动原版或
OpenSWD3 游戏 EXE；逐帧 framebuffer 差分仍保留为动态 oracle。
