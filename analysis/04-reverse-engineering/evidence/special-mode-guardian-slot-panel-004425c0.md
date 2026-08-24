# 护驾十一槽与类别面板 `0x004425C0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004425C0..0x0044295B`，412行，无FUNCTION CHUNK；callers为41680与尚未关闭的44A280。41680已直接回收typed helper；44A280继续独立审计，不提前计数。

## 十一槽循环

入口生成颜色`(0x19,0x17,0x11)`。party record基址严格为`u16(party_selector)*16`，slot按`0..10`读取；每次只在原record pointer读取点做span typed-stop，保留此前行文本、选框及动作副作用。

十一项prefix按原栈上8-byte表顺序发布：七项静态五字节内标签与slot6/8/10的四空格项分别映射为十一种GuardianRenderText。每行保持格式`"%-7s%-12s"`、Y从`panel_y+6`起每次增加`0x1C`。

- 未选行X为`panel_x-panel_shift+5`。
- 选中行先把X偏移设为`FFFFFFFF`，因此文本X为`panel_x-panel_shift+4`。
- 选框保持`panel_x-panel_shift-5,row_y,panel_width-0x16,0x18,0x14,0x0D,0,5`。
- `interaction_mode`按signed dword判断；大于0时所有十一行均经`mode=1,delta=-4/-4/-4`调暗。
- 仅选中、mode恰为0且record type不为`FFDC`时，设置独立slot action残值`0x232A/0x20`并在`list_action_offset+0x21C/+0x1D4`发布动作。mode负值保留原“非零但不调暗”路径。

## 类别动作与八项调用

行循环完成后设置独立category action残值`id=0x232A,variant=0x0D`并发布prepare操作；其窄边界返回的low16表达原`word_4FC69A`副作用。随后保持442960调用顺序：

`3,0,1,6,4,7,5,2`

X固定为`panel_x-panel_shift+0x38`；Y偏移依次为`6,0x22,0x3E,0x5A,0x76,0x92,0xCA,0x102`。442960后续已独立闭环，八处均直接解析`(frame,category)`资源并发布source/坐标/u16宽高；首个资源typed-stop会保留此前副作用。全部成功后只把category action残值改为`id=0,variant=0x44`，不额外发布prepare。

41680原`draw_guardian_slot_panel`整块opaque请求已替换为直接helper，并聚合状态、color、operation和十一行row count。

UT覆盖十一prefix/name格式、选中X少1、每行Y步进、mode0非missing动作、missing/正mode动作抑制、十一行调暗、选框八参数、八类别顺序/坐标、frame word传播、末尾action残值、及第十一record读取typed-stop。定向测试通过。

workpack双生成稳定为`87/227`，SHA256均为`f8ed58ea817517b9326355b3ca896131ec3c1178f293fd1b74435b360a80662c`；下一单元`0x00442960`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
