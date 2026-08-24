# 在游戏内菜单中选择上一项 `0x00445210`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00445210..0x004452AB`，74行，无FUNCTION CHUNK。code caller为已关闭450E0输入函数；3B480另把它绑定为G08动作callback。450E0已直接回收。

函数把selection按u16预减并立即写回。预减结果无符号小于等于10时夹为11；其余值保持，包括0预减后的FFFF。最终selection乘6再减36，低16位同时发布到两份坐标owner；画面索引写为selection加41。

随后查询flag49。只有结果精确等于1时，画面索引56改57、57改56；其他索引及其他非零结果不改。无论是否交换，最后都以命令139和共享sample owner执行一次音效，并原样返回音效callee结果。

最小平台边界为flag查询和sample命令。450E0原retreat opaque端口已删除，直接调用typed helper并计入其嵌套flag查询；后续45360仍保持提交端口。

UT覆盖预减后下界夹取、两份坐标、画面索引、sample参数与返回值、flag1双向56/57交换、flag2不交换，以及450E0动态旧选择分派后写新selection、调用45210再提交的完整顺序。

workpack双生成稳定为`118/227`，SHA256均为`810c4186a40083c0b41668b9e746ec77eab5d093ab014e13808b014685cc4b9d`；下一单元`0x004452B0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
