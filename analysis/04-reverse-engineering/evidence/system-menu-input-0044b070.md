# 游戏内系统菜单输入处理 `0x0044B070`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044B070..0x0044B557`，612行、21个call，无FUNCTION CHUNK；由43B480安装为callback。callee为40DC80、40DCB0、437300、43C090、44B560、44B6E0、44B840、44B930、44BDA0、44C0E0、485610、485850。

恢复的控制流如下：

- input lock非零时在任何查询前返回完整owner EAX。
- 先查询mask 0F，再读取X、Y、interaction mode和page；查询回调可修改这些字段。候选数按signed大于5判断，Y窗口按unsigned判断，两个固定X窗口及四个动态边界按signed判断。
- hover未早退后只读取一次flags低字节。bits 2/3优先退出；mode 7先打开模式14再走共享退出。mode 10在低四位非零时提交。
- mode 5的主条目按unsigned矩形和除20映射；固定条目16、17、18先写选择，再检查低两位。三个固定矩形保持各自严格开区间。
- 通用页签按Y除60，先写mode 0/page，再提交，并在提交后重读sample owner执行提示音。
- mode 1 page 4按X除26写row；保留`page-3`后再`dec eax`的两步残值。page 3按signed向零除16选择六行设置：两项上限11、一项上限2、一项service 48移除/条件重加、一项反向0..4发布、一项上限11；低位fallback无flags门直接提交row 6。
- mode 2仅page signed不小于3时按unsigned矩形和Y除66写detail selection，并在写入后检查confirm bit。
- 所有乘法除法保留x86 `mul`的EAX低积残值；未调用helper的路径不伪造业务返回。

UT覆盖入口锁、查询后snapshot、signed动态边界、mode7双调用退出、mode5主条目与固定条目、提交后sample owner重读、page 4选择与两步EAX、负row停止、value clamp、service移除/重加、共享值反向发布、row 6 fallback和mode2 detail写入。顺带按LST `mov dword`纠正44B010消息参数：tail/shared及后三字段均为完整32位值，不采用反编译`char`注释。

workpack双生成稳定为`169/227`，SHA256均为`86898f80f1e1d8212afa3c6ead89c826f58b06e9e1e3b09bf35b335b8ebe92d4`；下一单元`0x0044B560`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
