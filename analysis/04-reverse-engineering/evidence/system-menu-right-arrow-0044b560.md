# 点击系统菜单右箭头时翻页或提高设置值 `0x0044B560`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044B560..0x0044B6C1`，190行、5个call，无FUNCTION CHUNK。caller为43B480、44B070、44B840、44BBD0；callee为44D050、44D010和485610。

恢复按interaction mode分派：

- lock owner非零时直接返回完整EAX。
- mode 0将page预增；signed不大于4时提示音，超过时写回4但返回夹值前增量。
- mode 1/page 2将窗口起点加5。新起点signed小于总数时依次重建窗口和重算visible count，再读取回调后的visible count并夹scroll；否则回滚起点并无条件写`visible_count-1`，保留0下溢。最后只对cursor flags低字节OR 30，保留高24位。
- mode 1/page 3将row预增并signed夹到6；page 4将row预增并signed夹到1；两路都以完整sample owner提示音。其余page返回`page-4`残值。
- mode 2只在“重新开始/结束游戏”确认框预增光标，达到2时写回“放弃”但返回夹值前EAX。
- mode 5预增19项selector，达到19时回绕0，然后提示音。
- mode 3/4及越界mode保持入口mode EAX。

44B070的下方hover caller已删除opaque command并直接调用本typed helper；其input-status调用仍计入外层helper count。UT覆盖末页回滚与visible underflow、有效页重建后的回调重读、scroll夹值、AL OR保高位、行和确认光标夹值、19项回绕、完整sample owner及caller直连。

workpack双生成稳定为`170/227`，SHA256均为`f7761bf509f6e49cb6d7fc159c98a00d638f6ee0fae1be3d4af74f966959fe01`；下一单元`0x0044B6E0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
