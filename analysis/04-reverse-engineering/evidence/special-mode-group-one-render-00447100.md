# G01特殊模式主渲染 `0x00447100`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00447100..0x00447FCA`，1650行、59个内部label、73个call，无FUNCTION CHUNK。3B480把本入口写入G01第12槽，444FC0也把它写入第一个共享draw callback；无code caller。

入口先检查阻断owner，随后按19/17/11、13/13/9、24/10/11组合三种颜色。模式大于等于500直接调用已关闭C820运行时渲染。低模式通过`LegacyStandardModeGroupOneRenderOperation`窄端口表达平台帧、文字、动作和资源绘制，不泄漏裸surface或Win32字符串接口；所有模式判断、链读取、选择、滚动和owner写仍在typed函数中执行。

模式2保留4372D0门：按list offset预检后直接复用B9A0，非FFDC记录写FC648 owner 0x100并切模式4。进度差夹到5用于进度绘制；原始差值4最终写布局owner43。三个选择项保留31到35、35..38递增及越界回当前selection的循环。可见记录逐节点绘制名称、数量、type颜色和选择框；当前记录读取预检后直接复用B9C0。低两个transition nibble仅在进度至少4时逐级递减并生成scroll effect。

模式3按party marker跳过FFFF、发布面板Y owner并绘制选中槽；模式5绘制动作面板；模式10/11逐项加载`selected_outer+0x47`资源并绘制二级面板；模式15绘制筛选名称，按0x0F00/0xF000两级nibble递减并绘制scrollbar；模式17/18绘制对应终端文字记录。非FFDC当前记录以typed animated-record operation交给平台渲染owner。

445420动态draw分派现识别目标447100并直接调用本typed renderer；其他尚未关闭目标继续走动态端口。UT覆盖进度4、31到35选择循环、两条列表、低nibble滚动、模式3三槽、模式10/11三资源、模式15两记录与高nibble滚动、模式17终端文字、高模式C820委派及动态caller；独立ASan通过。

workpack双生成稳定为`136/227`，SHA256均为`cb4755cd0ec428154d280480c2755156cca0626248d1b1e9e66d58a47efed50f`；下一单元`0x00448020`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
