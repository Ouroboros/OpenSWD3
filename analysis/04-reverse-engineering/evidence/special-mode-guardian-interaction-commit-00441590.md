# 护驾交互提交与清理 `0x00441590`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。主范围`0x00441590..0x0044167A`，106行；mode0外跳的FUNCTION CHUNK `0x00440750..0x004407E4`同属本函数，已完整纳入。

## mode分支

- mode0先将lifecycle phase按u16减一；结果零时清全局mode，control bit0置位时再覆盖为`0x20000002`。随后按新phase重绑B480 callback，调用4420F0两次。chunk再清visible/local/offset/total/slot/mode/visible head/record head，按原顺序释放FCD4C、FCD50，清token；遍历原指针表对应record flags写`active=1, secondary=0`；释放FCF8C，清list token及FCD48。最终EAX为第三次release返回。
- mode1先将mode减为0，再按`party.low16*16+slot`发布文本并尾调4429B0。文本表越界/失败typed-stop保留先减mode。
- mode5写countdown480，清两组reset，以deferred mode恢复，发布旧transition value，清transition value并返回0。
- mode15直接复用41160，因此写mode1并返回入口15。
- 其他mode原样返回。

callback重绑与三次storage释放由`GuardianCommitPorts`隔离；record flags由显式span逐项写入。407F0四处commit caller均直接调用本helper并传播typed-stop。

UT覆盖phase/global mode优先级、双4420F0、三token释放顺序、record flags、完整chunk清零、mode1先减、mode5/15及407F0直接caller。定向测试通过。workpack双生成稳定为`81/227`，SHA256均为`1dc60536c5143e675fe90d33d1d60ec702cd599b179034c5e3e858bde446efda`；下一单元`0x00441680`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
