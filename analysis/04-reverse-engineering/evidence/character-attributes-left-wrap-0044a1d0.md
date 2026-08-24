# 点击左箭头时从第一名队员回到最后一名 `0x0044A1D0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044A1D0..0x0044A232`，52行、2个call，无FUNCTION CHUNK。由43B480安装为callback；callee为44AB00与485610。

本入口与44A160逐条结构相同，唯一静态差异是减1后把AX与FFFF正确比较，命中时写AX=3；44A160则把零扩展ECX与不可达的`0x000FFFF0`比较。由于两入口随后都只保留低2位，`FFFF & 3 == 3`，该差异在完整u16输入域无可观察结果差异。

因此typed实现保留独立入口，但直接复用已经按完整LST域验证的反向四槽helper：按u16减1、模4、跳过FFFF、首个可用槽写mode、重建后播放107提示音；全槽FFFF在四项域检查后停止并保持入口mode。未把两个原入口在workpack中合并计数。

UT独立调用本入口，覆盖mode0显式回绕、槽3不可用后选槽2、high16保留、重建与音效顺序、sample owner及返回。

workpack双生成稳定为`162/227`，SHA256均为`01d59515eb5dda531e8d2f715d0da6b723f29b027c7dd3608067129b83d4ba07`；下一单元`0x0044A250`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
