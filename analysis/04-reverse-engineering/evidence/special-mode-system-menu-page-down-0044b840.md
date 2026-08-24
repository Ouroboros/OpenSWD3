# 游戏内系统菜单下一页按钮 `0x0044B840`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x0044B840..0x0044B90E`，117行、4个call，无FUNCTION CHUNK。caller为43B480和44B070；callee为485610，mode 1/page 2以尾跳进入已关闭的44B560。

实现的界面行为：

- lock owner非零时不改状态，返回完整EAX。
- mode 0把页签定位到4；原页签不是4时先播放提示音，已经是4时不重复播放。
- mode 1/page 2直接复用右箭头翻页逻辑，保持原尾跳结果；page 3把设置行定位到6；page 4按原程序写2而不是常规上限1，并播放提示音；其他page返回`page-4`残值。
- mode 2/page 4将detail selection预减，负值时写回0但保留夹值前EAX。
- mode 5固定选择第15项并播放提示音，而不是选择19项列表的最后一项18；保留这一原始不对称行为。
- mode 3/4及越界mode保持入口mode EAX。

44B070的下一页按钮已删除未说明职责的回调并直接调用本typed helper；mode 1/page 2继续通过44B560重建下一页。UT覆盖页签4、列表翻页、page 4固定行2、detail负残值、mode 5固定项15、完整sample owner及caller直连。

workpack双生成稳定为`172/227`，SHA256均为`032e5ba05d2157456072fdbf9fe76bade9093b104d6910f5e7cef535e31cdc63`；下一单元`0x0044B930`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
