# G08特殊模式输入 `0x004450E0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004450E0..0x0044520D`，147行，无FUNCTION CHUNK。函数无code caller，由3B480在G08/arg1路径绑定到动态slot0。

入口先按默认8像素网格计算`grid=(x-220)>>3`、商和余数，上界600；再第一次查询flag49并保留其低16位残值。只有结果精确等于1时改用无符号`(x-220)/7`网格和上界630。横向必须满足u32 `x>220 && x<upper`；进入后残值改为y低16位。

主命中还要求y在`10<y<42`、buttons bit0、余数`0<r<8`。命中后第二次查询flag49，再以旧selection覆盖低16位残值。若第二次结果精确等于1且selection15，只有lifecycle1继续，其他lifecycle立即返回；其余情况仅在lifecycle非1时按旧selection调用动态表。动态表解析失败只在原间接call点typed-stop，保留双查询、旧selection残值和全部此前状态。

继续路径把selection写为`grid/10+12`，再直接调用已关闭的45210完成预减、坐标与画面索引发布，随后直接调用已关闭的45360完成阶段切换、主回调重绑、二级初始化与确认音效，最终返回45360低16位。主命中失败时，若buttons任一bit2/3且lifecycle1，则先写fallback constant12，再调用453F0端口；否则返回当前残值。453F0已直接回收：button12 fallback先写constant12，再预减lifecycle、按零值清特殊模式请求，并以新阶段重绑回调。

UT覆盖默认动态callback顺序、flag1的7像素网格、selection15在两种lifecycle下的精确特例、button12 fallback及写前副作用、外横域flag残值、动态表缺失typed-stop、双入口查询、45210嵌套查询及selection重写顺序。

workpack双生成稳定为`117/227`，SHA256均为`2b769e56b0024ad1b76f846024a9b850de5ec54a1984bb95100b9e71b9043365`；下一单元`0x00445210`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
