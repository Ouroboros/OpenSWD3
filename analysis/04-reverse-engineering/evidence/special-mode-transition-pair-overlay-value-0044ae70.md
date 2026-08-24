# 特殊模式转场配对属性覆盖值 `0x0044AE70`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044AE70..0x0044AF2D`，114行、4个物理callsite，无FUNCTION CHUNK。四个直接caller均位于44A280；callee为4239D0、wsprintfA与436AD0。

输入value精确为0时立即返回，不执行任何call。非零时先以RGB(31,31,31)计算颜色；value为负再以RGB(26,0,0)覆盖颜色。因此正值运行3次call，负值运行4次call。

X偏移按threshold的signed比较选择：`>=1000`为44、`>=100`为33、`>=10`为22、`>=1`为11，否则0；最终X为`input_x+offset+4`。格式固定为`%c%-3d`，正值sign为`+`，负值及零分支语义sign为`-`。abs保持x86 `cdq/xor/sub`位运算，INT_MIN仍得到`0x80000000`并按signed参数传给格式化，不擅自修正溢出。

格式化后以属性字体、共享surface、调整X、原Y、最终颜色和flags4绘制，并透传draw返回到typed result。

44A280删除四处`draw_overlay_value` opaque命令并直接调用typed helper。其原直接路径仍为75次调用；在四项second值全非零且其中一项为负时，callee展开为13条typed命令，当前command count为84，helper count按四次直接调用加nested helper为88。second不可用仍在展开前保持33次命令typed-stop。

UT独立覆盖正值3调用、负值4调用、零值0调用、白/红颜色顺序、四档最高偏移、正负sign、INT_MIN abs32及最终draw参数；44A280 UT验证84条expanded命令的关键索引、负值红色和88 helper计数。

workpack双生成稳定为`166/227`，SHA256均为`cbf3a2485c9b5f9cfdecfa6e04f6d77b0acdac79290f0a284b94f81560a34fd4`；下一单元`0x0044AF30`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
