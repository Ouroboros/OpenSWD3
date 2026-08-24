# 东西方祭坛record详情面板 `0x0043FA70`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。范围`0x0043FA70..0x0043FDCC`，390行；唯一caller为E800的四个调用点。callee为40EBF0、4239D0、436AD0、B110及BAB0。

该函数不是通用数据库UI，而是东西方祭坛的契约候选详情面板。权威静态文本经CP950解码为“東方祭壇”“西方祭壇”“%d 級”“生命/靈力/體力/攻擊/防禦/敏捷”，禁用警告为“等級不足 無法訂契約”；49E370为21项候选类别表（天神、魔神、天使……靈武器）。

## 行为

1. 无条件生成`(21,15,8)`标题色；flags bit0清或bit1000置位时再生成`(10,7,4)`详情色并把BAB0 panel style从2改为4。
2. 严格依次绘制280×375背景、232C/variant23标题动作、祭坛标题、record `+5C`/variant68动作。
3. 从`+5E`读取21项类别，从`+C`读取名称；随后按固定25像素Y步进绘制`+60`等级、signed `+70`生命、4404D0产生的左右祭坛signed灵力/体力，以及unsigned `+62/+64/+66`攻防敏。
4. 左面板读取FCD14/FCD16，右面板读取FCD18/FCD1A；typed state以两项signed数组承载，后续4404D0工作包负责写入。
5. 非激活或bit1000时调用B110语义的120×220禁用矩形；bit1000还生成`(24,10,11)`并绘制契约等级不足警告，只有该路径传播最终文本绘制EAX，其他路径返回原flags。
6. category超出21项时在原49E370裸表读取点typed-stop。名称读取受0xB0 owner约束；正常category的高字节零也保证其在字段区前终止。

新增`render_legacy_standard_mode_altar_record_panel`，以既有typed render operation展开全部动作、面板、文本和矩形。E800四个调用点均直接调用本helper，删除整块`draw_record_panel`操作；父结果聚合内部helper/operation并传播typed-stop。UT覆盖激活面板全部13项操作、signed生命、21项category边界、phase2双禁用/警告面板及phase3 -35帧；独立ASan通过。

定向测试通过。workpack双生成稳定为`66/227`，SHA256均为`6b7e83144d2f509f5089fdfb3767dfb1ba3b7621b44d018ab43025a6a840ff5d`；下一单元`0x0043FDE0`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
