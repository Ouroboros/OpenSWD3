# 按编号摘取第一条道具记录并保留返回记录next `0x0044D620`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044D620..0x0044D644`，30行，无callee、无外部FUNCTION CHUNK。直接caller位于`0x00469D20`业务路径。

函数接收head字段地址和u16记录ID，以pointer-to-link方式查找：

- 从head开始，比较每条记录`text_index`低16。
- 不匹配时前进到next，同时把link位置推进到当前记录next字段。
- 首个匹配时把该link写成匹配记录next，并返回匹配记录指针。
- 到null仍未匹配时返回null，链保持原样。

函数只摘链，不释放名称或176字节记录，也不把返回记录next清null。因此摘取head后返回记录仍指向原第二条；摘取中间记录后仍指向原后继。caller负责后续所有权。

链环只在未匹配路径将再次读取同一节点时typed-stop；此前不改链。

## 2. 验证

UT覆盖head匹配、中间匹配、未找到和两节点环。验证head/前驱改写、访问数量、未找到不改链，以及两种成功路径都保留返回记录原next。

workpack双生成稳定为`188/227`，SHA256为`40662ef0022aa484620abacd0802ffd55c1976d738cbfeaeecbc107a3a697d9f`；下一项为`0x0044D650`。
