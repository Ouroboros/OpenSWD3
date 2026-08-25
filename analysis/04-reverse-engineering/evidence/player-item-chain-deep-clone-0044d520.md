# 深拷贝玩家道具链的176字节记录和独立名称 `0x0044D520`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x0044D520..0x0044D592`，64行，无外部FUNCTION CHUNK。直接caller为`0x004485F0`的两处根克隆；callee为每节点一次176字节分配和一次`strlen+1`名称分配。

函数从source head顺序遍历：

1. 为当前源记录分配176字节，完整复制44个dword。
2. 读取源记录`+AC`名称owner，计算包含终止0的长度并分配独立名称。
3. 把新名称owner写入clone `+AC`，复制完整名称。
4. clone前插到局部结果head。
5. 前进到源next。

因此结果链顺序与源链相反；源`A→B→C`返回`C'→B'→A'`。正常EAX为最后创建的clone，即结果head；空源返回null。

## 2. 适配边界

`LegacyStandardModeForwardNode`复制完整typed字段，`std::string`提供独立名称存储；`LegacyStandardModeRecordClonePorts::clone_record`表达原记录和名称两次分配的最窄生命周期。返回null只在当前clone分配/名称复制点停止，已完成的反序前缀保留。

源链环只在下一次将重复克隆同一节点时typed-stop；此前clone保持。原程序会无限继续分配，typed实现不伪造完整成功。

`0x004485F0`保留两种owner边界：inventory根仍以独立token克隆，selection根端口使用同一176字节深克隆语义；两种根不因宿主类型差异被错误合并。

## 3. 验证

UT覆盖空链、三节点完整克隆、第三节点分配停止和两节点源环。完整路径验证反序3/2/1、clone对象不别名源、名称内容相同但存储地址独立；停止路径验证已完成的两节点反序前缀不回滚。

workpack双生成稳定为`185/227`，SHA256为`b672162abafede3014e05ed590436ad09104be77cdaae613cfb4f4569628c783`；下一项为`0x0044D5A0`。
