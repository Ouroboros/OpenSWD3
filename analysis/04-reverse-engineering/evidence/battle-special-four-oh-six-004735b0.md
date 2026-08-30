# 战斗动作四百零六四记录状态机 `0x004735B0`

状态：`platform_adapted`。完整LST、typed实现、唯一caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x004735B0..0x00473C0D`，proc至endp共670行、419条实际指令、19个call、29个跳转、22个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一caller位于动作dispatch的动作406分支；this为当前group-A行动者，唯一栈参数为选中group-B目标token，严格只在本函数返回一后继续发布目标、效果分数与后续画面提交。

每帧先把profile word加1500写入`+0xAF0`特殊主记录，复制特殊profile variant并置完成latch，然后typed直连已关闭动作记录更新与frame provider。frame缺失typed-stop位于`+0x254C`写零后的原解引用点。frame就绪后发布唯一共享frame token，并准备主记录draw offset、field76及局部mode flags；镜像门`+0x2B08`按word回绕调整主偏移且只翻转局部bit0。

特殊主记录field5A低字节bit3置位时，函数把`+0x2954`写为负三十一，仅在`+0x267C`低字节按位或三，破坏性清零field5A word并清零`+0x2958`。gate为零时播放主记录sample，按signed X选择正负十六声像，清零field58，发布frame高三分之一、四分之一及三份负六运动值，再执行基础双层绘制。

gate bit0与bit1可在同一帧并行推进。bit0阶段读取`+0x2958`为signed word：值大于负三十时把三份共享运动值设为该值，以主记录和局部flags加bit2绘制并word减一；值小于等于负三十时只清gate bit0。bit1阶段每帧把`+0xB88`次记录action ID写为`0x17FE`、external mode写零，再typed更新和查询frame；镜像后保存`+0x29B6`与`+0x29AE`，播放次记录sample并清零field58。`+0x2954`小于等于零时按原非对称坐标公式绘制次层并word加一；正数时把gate整写四并同时清零两个signed进度word。

gate等于四时，以目标token、`+0x630`效果记录、零、动作ID、主X、主Y、负一、零调用待审效果更新。返回非一时保留gate和记录；返回一时刷新目标，并把原函数未初始化的两个栈word作为显式兼容请求传给待审效果计算。AX按signed word扩展，仅大于等于9999时夹到9999；负值保留。值写入共享last effect，以32位回绕累加到唯一pair primary owner，再依次发布目标signed值与属性一；随后gate写`0x4000`且`+0x2958`重置为负三十一。

gate bit14阶段把`+0x6C8`第四记录action ID写为`0x1F88`、base variant写零，并通过带记录引用的窄port调用待审更新；严格只在返回一时把gate整写`0x2000`。gate bit13阶段若`+0x2958`仍小于等于零，则发布三份signed运动值，以当前局部flags加bit2绘制主frame并word加二；大于零时清零进度与gate，按`+0xAF0`、`+0xB88`、`+0x630`、`+0x6C8`顺序整记录清零并返回一。效果完成、第四记录完成和末段首帧允许在同一调用中级联，typed实现不人为拆帧。

实现新增`+0xB88`特殊次记录与`+0x6C8`效果次记录唯一owner，并补`+0x29AE`、`+0x29B6`两个word owner；`+0x630`继续复用共享效果记录，未复制物理状态。两个已关闭动作更新与frame provider typed直连；音频、绘制、效果更新、目标刷新、效果计算、属性发布和第四记录更新保留窄port。动作406唯一caller已typed化。

测试覆盖gate零基础双绘制、field5A启动、bit0/bit1同帧并行、负三十清门、负三十一至正一word推进、镜像偏移、两套sample、主次非对称坐标、gate四非完成保留、未初始化栈word显式传递、signed负效果、32位累计、目标双发布、bit14第四记录等待、bit13同帧末段绘制、阈值正数四记录清理、frame/shared原访问点typed-stop、production动作406 caller及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`225/422 = 216 platform_adapted + 9 assembly_exact + 197 pending_audit`，SHA256为`290872970a1d7f66c574120b7b5b439511928839ed6cc836b341489c059ef0d8`。动态差分因原版四记录、两套frame与音频、五层绘制、效果更新、未初始化栈word、目标刷新、数值计算、属性发布、第四记录更新和唯一caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
