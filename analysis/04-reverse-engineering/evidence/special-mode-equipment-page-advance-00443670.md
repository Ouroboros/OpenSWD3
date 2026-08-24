# 装备物品模式分页推进 `0x00443670`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00443670..0x004437B2`，166行，无FUNCTION CHUNK。code caller为F40两处，3B480另绑定为动作callback；F40现已直接回收。B9A0、B9C0、B9E0、BBE0已关闭并直接复用；444E50尚未独立关闭，仅保留visible count刷新窄端口；sample46保留平台端口。

## mode1分页

先计算`row_end = visible_count - (visible_count为偶数)`，再令`selected=row_end-1`，全部按u32回绕、signed i32比较。

- 若local小于selected，不翻页：保留local奇偶，写`parity+selected`；若仍大于等于visible，再减2。直接返回local，不更新文本、不播放sample。
- 否则先无条件写`list_offset += 24`。若新offset不小于total，恢复旧offset并把local写selected，然后走末页文本路径。
- 若仍有下一页，B9A0重建visible head，再调444E50等价刷新。callee返回后重读local/visible；local越界时按`parity - (visible偶数) + visible - 1`夹取，必要时再减2，并直接返回。
- 新页local仍有效或末页恢复时，以`selected+offset`调用B9C0/B9E0，播放sample46，最后才写`4FD080=0x30`。这与43450“先写30再sample”的顺序不同。

刷新停止保留新offset及B9A0 visible head。B9C0 null与B9E0失败保留此前local/offset/visible/text，不播放sample、不写30。

## mode2与mode15

mode2固定从party3向0扫描首word，选择最高非FFFF项；四项全FFFF时原函数继续向表前越界，modern在四项完整读取后typed-stop。

mode15直接复用BBE0，step=8，回写special offset、hover selection及visible count；随后`or ah,0x30`即OR `0x3000`并返回完整EAX。其他mode返回`mode-15`。

F40 mode1/mode15两处page advance矩形直接调用本helper，传播refresh、selected record、shared text及party search状态。

UT覆盖页内末行归一、末页恢复/文本/sample/写30、新页offset24+B9A0+两项visible刷新+local夹0、444E50停止、B9C0/B9E0停止；mode2最高party/全FFFF；mode15八项BBE0和`ABCD0001 -> ABCD3001`。F40对应scroll回归通过。

workpack双生成稳定为`101/227`，SHA256均为`c660136e0efe513d5a3005210d1bfb1b1394fb9bb514615b590503d64661c1c8`；下一单元`0x004437C0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
