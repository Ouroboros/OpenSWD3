# 在游戏内菜单中选择下一项 `0x004452B0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x004452B0..0x00445357`，80行，无FUNCTION CHUNK。无code caller；3B480把它绑定为G08动作callback。

函数先查询flag49：默认上界15，只有结果精确等于1时上界16。随后selection按u16递增并立即写回；零扩展后的结果大于上界时夹到上界，因此FFFF递增回0不会被夹取。最终selection乘6再减36，低16位同时发布到两份坐标owner；画面索引写为selection加41。

函数第二次独立查询flag49。只有第二次结果精确等于1时，画面索引56改57、57改56；第一次查询只决定上界，不能复用到画面切换。最后无条件以命令139和共享sample owner播放一次音效并原样返回callee结果。

选择前移和后移共用`LegacyGameMenuSelectionPorts`，只隔离flag查询与sample命令。UT覆盖默认上界15夹取、flag1上界16、两份坐标、双查询、画面索引交换、第一次flag2不扩上界但第二次flag1仍交换、FFFF递增回0及音效参数。

workpack双生成稳定为`119/227`，SHA256均为`6b964dfd7e7fc2941b2c7af1ad315a88a22b6ccc04eec2fa4fcc16e6f1448893`；下一单元`0x00445360`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
