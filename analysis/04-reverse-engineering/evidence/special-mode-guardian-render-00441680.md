# 护驾系统主渲染 `0x00441680`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00441680..0x00441F66`，1014行，无FUNCTION CHUNK；42个直接/间接call全部逐项核对。

## 状态与transition

入口严格生成四种颜色。仅`interaction_mode`无符号小于等于1且transition gate成立时选择记录：mode0按`party.low16*16+slot`读取固定表并写deferred=0；mode1写deferred=1，local selection为0时直接取visible head，否则复用43B9A0的`local-1`推进。空selected本身不早停；仅链link读取越界在原读取点typed-stop。

selected存在且text index不是`0xFFDC`时，恢复`FC648=0x100`、mode5、sample owner转移到transition value及sample owner清零。frame counter相等时写panel offset400、x488、y120、render zero0和previous mode；之后panel offset按x86算术右移一位。

## 主面板与链表

按原顺序表达：

1. action update；共享主action写`232A/2D..2F/zero0`并在三个坐标绘制。
2. 第一frame、party label、两块tiled frame、split panel、第二frame、guardian slot panel。
3. 在上述11个draw之后第二次无条件读取party record；越界typed-stop保留此前副作用。
4. 再update并绘制列表背景。record head为空发布empty-list；否则逐行先set text color，非mode1按`(-4,-4,-4)`调整颜色，再读取节点。
5. missing记录使用`%-12s`；普通记录严格使用`%-12s %2d`及offset8/0xA两个signed i16之和。
6. selected行保留mode1/mode5-deferred规则、highlight、offset0x5C action；共享selected action写`232A/20/frame44/resource/zero0`。

visible count与total/list offset按signed dword解释。total大于visible时直接复用已关闭43AE40，低/高nibble分别递减并形成overlay bits；total大于10时调用第二根bar。两根bar保留signed ratio、坐标、186/202高度及四输出共享状态。随后按`slot, party.low16`调用未关闭442130窄render port。

## 详情、动画与尾部

party或selected-row记录有效、text非missing且slot signed小于9时，发布两行标签，生成neutral第五色，并从record `0x9E..0xA6`逐项读取signed i8：0用neutral、正数用第三色、-1..-9用第二色、小于等于-10用第四色；格式化文本由typed平台边界提供。

selected record存在时，直接复用已关闭43BD70；position/velocity与`FC830/FC648`双向同步，animated text由typed node持有。mode15最后固定追加`43BAB0(0xFE,0xE4,0x84,0x16,4)`及提示文字，并返回最后文字EAX。

`GuardianRenderPorts`只保留颜色、transition gate、文本格式、最窄render execute边界，并显式提供已关闭bar和animated-panel owner引用。UT覆盖早/晚表越界、有效transition、mode1空selected、frame几何、三action残值、两行链与格式、selected action残值、双bar/nibble、9属性、animated panel和mode15尾。定向测试通过。

workpack双生成稳定为`82/227`，SHA256均为`4256a1b9fc26acf4efe945262d64b27f8c1d28379a5bb310c42fb22b3f2b1bf8`；下一单元`0x00441F70`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
