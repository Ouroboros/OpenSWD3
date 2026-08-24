# 特殊模式转场配对主渲染 `0x0044A280`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044A280..0x0044AAEA`，945行、62个物理callsite，无FUNCTION CHUNK。由43B480与444FC0安装为callback。

函数以typed即时命令流逐次表达颜色计算、三块边框、两块面板、三项动作、格式化文本、属性覆盖、模式摘要、十项静态标签、九格修正与末面板。物理循环内有4个互斥格式callsite和1个draw callsite；九次循环每次实际执行一次格式与一次draw。mode 0..3且四项覆盖值均非零时，运行路径共75次调用，command count与helper count均为75，最终透传4425C0返回。

关键寄存器和记录语义：

- 第一frame参数在第五次颜色返回已`and 0xFFFF`后写palette，因此仅为palette低16。
- 第二frame保留第一panel返回的high16，再覆盖palette low16。
- 第三frame的ECX high16来自调用后可变寄存器snapshot，仅在LST写CX处覆盖palette。
- 两份原记录均为56字节；first读取offset 10/12/14/16、26/28、2C和2D起9个signed byte，second读取offset 10/12/14/16。
- mode表按56字节步长读取primary u32及四个u16属性。
- second属性覆盖只在其u16非零时调用44AE70，传入值按i16符号扩展，first对应值按u16零扩展。
- 九个signed modifier严格分为0、正数、-1..-9、<=-10四类；最后一类显示`-10-value`。颜色分别来自第一、第四、第三、第五次颜色计算低16。
- 九格坐标保持`X=0xEA+0x55*(index/5)`、`Y=0x169+0x14*(index%5)`。

裸边界只在原始读取点停止：mode越界发生在前6次调用之后、first record不可用发生在前14次调用之后、second record不可用发生在前33次调用之后。此前命令和寄存器副作用均保留。初始化以分配结果发布record availability；释放时token仍保留，但typed availability在对应释放点失效。

UT覆盖完整75命令的关键索引、三组frame寄存器、动作参数、数值计算、signed overlay、四类modifier颜色与显示值、末格坐标及最终返回；另分别验证三种typed-stop的6/14/33命令边界。

workpack双生成稳定为`164/227`，SHA256均为`2c46ea8ef6d8d74a3aa664c1dc760237bee1bd22a45470f83e39193fc5f29a81`；下一单元`0x0044AB00`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
