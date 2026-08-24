# 特殊模式转场交互更新 `0x00448840`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00448840..0x00448BA4`，421行、9个call，无FUNCTION CHUNK。唯一code caller为已关闭43B480的callback表数据槽；typed函数供该槽runtime adapter接入。

progress=2时仅当velocity>=97处理。第一入口要求primary state/gate均精确为1，pointer Y严格位于101..112、X严格位于162..198，命中写selection result 1；否则paired secondary state/gate均为1时写2。

progress=1按顺序检查四个Y开区间D2..E8、107..11D、140..156、179..18F，X均要求6E..104，分别写enabled 0..3。每次命中后若input flags低2位非零调用448EE0；后续区间必须重读可能被回调修改的pointer X/Y，因此实现保留四个顺序检查而非预计算或else-if。

progress=5先要求primary gate非零且X在A8..1C0；column严格按无符号`(X-100)>>4`计算，高16非零时只发布row selector、不执行row副作用。六个Y开区间F0..10F至190..1AF分别：样本2E索引夹11、surface索引夹11、spacing写`40*min(column,2)+60`并返回`5*column`低字节、先禁用服务48且column非零再启用、source写`4-min(column,4)`到两owner、auxiliary夹11。未命中有效区域时，仅secondary gate非零才调用退出helper。

UT覆盖progress2严格矩形与paired fallback；progress1四段选择和刷新；progress5六行selector、clamp与服务48调用顺序，以及外部退出。SDL port扩展了对应设置接口，现阶段保持最小无副作用适配，后续owner关闭时接真实平台职责。独立ASan通过。

workpack双生成稳定为`144/227`，SHA256均为`91da5495cb3074b858cca3de12d7ef080b1abe8f7547251298bab05ff9465026`；下一单元`0x00448BB0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
