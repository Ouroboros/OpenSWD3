# 装备物品记录列表回收 `0x00444F00`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00444F00..0x00444F53`，50行，无FUNCTION CHUNK。callers为已关闭442F10模式清理、43A60 party循环和43B70列表类别循环，三处均已直接回收。

只要当前record head非空，就先把head写为当前next。text index等于FFDC的缺省记录直接交给4885A0等价typed释放端口，不读party selector；普通记录按party selector低16位解析四party dummy root，再把当前记录前插到root.next。循环因此反转普通记录顺序，且FFDC永不回池。

party索引越界只在原party池表读取点typed-stop；source root解析为null只在原root.next读取点停止。两种停止都发生在head已经弹出之后、当前记录next尚未改写之前；result显式返回detached record，保留剩余state head。

缺省创建、释放和party root解析与E80共用`LegacyStandardModeEquipmentRecordListPorts`虚基生命周期，不复制接口。442F10、43A60、43B70原三个cleanup opaque端口已删除。caller仍把一次F00视为一次helper；F00 result分别计回池数和缺省释放数。

UT覆盖FFDC释放、两项普通记录前插到已有party池、最终空head、party索引及source root停止前缀。caller回归校正了FFDC真实释放后E80创建单缺省记录的窗口，并覆盖442F10 workspace释放、43A60/F40 party循环、43B70/F40类别循环及无进展四轮隔离。

workpack双生成稳定为`113/227`，SHA256均为`7767a25c4adf3c97038255e7542850a23da0b31f11b26ab288fad68d3bfb9a6e`；下一单元`0x00444F60`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
