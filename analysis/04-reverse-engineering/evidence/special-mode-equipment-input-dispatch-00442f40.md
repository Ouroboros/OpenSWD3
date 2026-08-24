# 装备物品模式输入分派 `0x00442F40`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00442F40..0x00443446`，621行，无FUNCTION CHUNK。无direct code caller；3B480将其绑定为模式input callback。直接call site为：443BD0五处、40DC50五处、443570/443450/4437C0/443670各两处，437300、443B70、B9C0、B9E0、485610、C090、443A60、4441A0各一处。

已关闭C090、B9C0、B9E0由typed helper直接复用；后续关闭的43450推进、43570后退、43670分页推进、437C0分页后退与43A60 party循环helper也已从caller直接回收。其余未审业务callee按commit、overlay、list-kind和exit target隔离；40DC50以返回i32的item-presence端口表达，保留原逻辑对非零与精确等于1的不同判断；485610保留sample command边界。

## 入口与早退

每次先清`4FCFA4`。buttons mask5且mode17/18直接commit。cursor mode15、主按钮及`x=401..549/y=197..395`命中时按25像素行更新hover；重复行直接commit。全部边界均为严格大于/小于。

## mode1

右下overlay矩形先写`4FCFA4=FFFFFFFF`；主按钮触发437300后，buttons、mode与共享X/Y都从可变snapshot重读。随后：

- 顶部list-kind行按58像素列计算；基础3项，再查询`2*party_action+0x15/+0x16`增加可用数。命中时写`kind-1`，故第一列刻意回绕为FFFFFFFF，再调443B70。
- 二列×多行物品格按`(x-212)/197 + 2*((y-114)/25)`计算。越visible count直接返回；新行写local selection并直接B9C0/B9E0、sample46；重复行仅buttons bit1 commit。
- B9C0 null与B9E0越界分别在原读取点typed-stop，保留selection及已复制文本。

C090 index15可用时，mode1且total>24的右侧窄条分派selection retreat/advance及两个动态page矩形；空击保留调用后共享Y残值。

## mode15

C090可用后：special count>8且右侧窄条命中时分派同四类动作；否则只在`x=401..545/y=197..394`计算25像素行，并以hover record count夹到末项。新行要求主按钮写hover；重复行要求bit1 commit。原EAX把共享X/Y以及后续`mov al,buttons`残值按LST保留。

## fallback

- 主按钮+mode2矩形按25像素visible party行映射。第0行直接party0；后续行从ID31开始，仅返回值精确等于1才计作下一个party。映射完若等于当前party action先commit，随后仍重写selection。typed端在四party内无法完成映射时停止，避免宿主无限越界查询。
- 主按钮+mode1左侧party列以110像素行得到0..3，查询ID`0x1E+party`非零后反复443A60，直到调用后party selector low16匹配。每次必须重读selector；四次无进展typed-stop。
- buttons bit2的fallback调用4441A0退出。

modern input snapshot显式持有buttons、共享Y/X、cursor mode和entry EAX；overlay target可同时修改snapshot与state，caller不使用陈旧寄存器或坐标。

UT覆盖五个443BD0位置、overlay后buttons/坐标/mode重读、list kind两项presence、grid选择/重复、B9C0/B9E0停止、C090越界、mode1与mode15各四个scroll矩形、mode15行、mode2映射/重复/失败、party循环/无进展及exit。定向测试通过。

workpack双生成稳定为`98/227`，SHA256均为`9d41b86be007973c191a8a829864978a805669d64a2d086088ee45e1c0bd8cbb`；下一单元`0x00443450`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
