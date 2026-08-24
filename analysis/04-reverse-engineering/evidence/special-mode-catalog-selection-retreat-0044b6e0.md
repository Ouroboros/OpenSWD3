# 特殊模式项目目录选择后退 `0x0044B6E0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044B6E0..0x0044B818`，176行、5个call，无FUNCTION CHUNK。caller为43B480、44B070、44B930、44BA20；callee为44D050、44D010和485610。

恢复按interaction mode分派：

- lock owner非零时直接返回完整EAX。
- mode 0将page预减；非负时提示音，负值时写回0但返回夹值前负EAX。
- mode 1/page 2将窗口起点减5。signed负值时先把scroll和起点清0，再以0重建；非负时以新起点重建。两路都重算visible count，并只对cursor flags低字节OR 03，保留高24位。
- mode 1/page 3与page 4将row预减，signed小于等于0时写回0，之后仍以完整sample owner提示音。其余page返回`page-4`残值。
- mode 2只在page 4预减detail selection，负值时写回0但返回夹值前EAX。
- mode 5预减19项selector，负值时回绕18，然后提示音。
- mode 3/4及越界mode保持入口mode EAX。

44B070的上方hover caller已删除opaque command并直接调用本typed helper；外层input-status查询与内层重建/计数都纳入helper count。UT覆盖页0负值、负窗口归零、重建顺序、AL OR保高位、行`<=0`夹值与音效、详情夹前EAX、19项反向回绕、完整sample owner及caller直连。

workpack双生成稳定为`171/227`，SHA256均为`ab7b05db92e94b956e8603e8a70bff4260ee7b64e04572bc1e5509eee5e8d867`；下一单元`0x0044B840`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
