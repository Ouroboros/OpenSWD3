# 特殊模式选择记录清理 `0x004482E0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004482E0..0x0044835D`，74行、4个call，无FUNCTION CHUNK。code caller为4455A0、446420、446550、446700和尚未关闭的4485F0。

函数逐节点先从head弹出当前记录，再观察text index。FFDC缺省记录不归还inventory；普通记录把first/second value分别按signed i16解释，只有严格大于0才调用inventory mutation，mode固定为1和2，item id均为记录text index。负值、0和8000..FFFF均不归还。

inventory副作用完成后释放名称与176字节记录。modern typed node以string持有名称，窄`LegacyStandardModeRecordCleanupPorts::release_selection_record`表达两次原free组成的生命周期提交。释放typed-stop发生在head已弹出且本节点inventory已归还之后；不回滚此前节点。空链合法零操作完成，不能用fixture伪造失败。

4455A0、446420、446550、446700中四处原`cleanup_selection_records` caller已直接调用本helper；cleanup port改为返回typed record cleanup owner，陈旧方法删除。4485F0仍待其独立workpack关闭后回收。

UT覆盖普通记录两类正数量归还、FFDC跳过、FFFF signed负数跳过first但归还second、三节点释放、head清空，以及首节点释放停止时head已弹出、inventory已提交、release count仍0。caller回归改为仅在真实非空节点释放点注入停止；独立ASan通过。

workpack双生成稳定为`139/227`，SHA256均为`35d9b7421203b56df16ca462398ee7afbe9c217d178325bf76c8cc7004d9a7ff`；下一单元`0x00448360`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
