# 特殊模式转场滑动面板透明绘制 `0x00449C30`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00449C30..0x00449D71`，161行、6个call，无FUNCTION CHUNK。四处直接caller均位于490C0。

函数先准备action record。返回0时按原顺序读取record的action id、secondary id和error field并提交绘制错误，不再解析surface或绘制。准备成功后，以record的两个u16键解析surface token、width和height。

首次alpha三通道统一写`-25-offset`，发布surface token，并在`(x-origin_x,y-origin_y)`绘制一次，effect=4、flags=0。仅当offset<-12时进入轨迹循环：每轮先把alpha三通道写当前offset，再绘制`x+displacement-origin_x`；随后重写同一alpha，再以C++ signed向零除法绘制`x-displacement/2-origin_x`。offset逐增、displacement逐减，直到offset到-12；返回最后一次draw EAX。

490C0删除`draw_slide_panel`命令边界，四块面板直接调用typed helper；caller仍在原边界更新之后按0xD2、0x107、0x140、0x179顺序绘制。449C30的未关闭callee继续由准备、surface解析、错误报告和surface draw四个最窄端口隔离。

UT覆盖offset=-16时初始1次加四轮左右各2次，共9次draw；验证初始坐标、最后一次signed除2坐标、最终alpha=-13、surface token、helper返回及调用数。失败路径验证只执行准备和错误报告。490C0入场UT验证四caller合计36个面板端口事件且不再发布opaque滑动命令。

workpack双生成稳定为`155/227`，SHA256均为`4a1742e43775eef15646f13974d0156c389fd32f07f82d9e10dd3679d98a39bd`；下一单元`0x00449D80`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
