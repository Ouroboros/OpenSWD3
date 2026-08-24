# 特殊模式选择记录克隆与筛选 `0x00448020`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00448020..0x0044822D`，276行、9个call，无FUNCTION CHUNK。唯一code caller`0x00448230`现已关闭并直接调用本typed helper。

函数先清目标链，再逐个从inventory source链深拷贝176字节记录及名称。modern通过`LegacyStandardModeRecordClonePorts`表达原malloc/free生命周期；typed `LegacyStandardModeForwardNode`复制string实现同一深拷贝，不共享源名称owner。分配失败在原clone分配点typed-stop，保留此前目标链与源字段副作用。

筛选严格保持原顺序。模式0以second value非零命中；模式3/6要求first value非零且分别命中特殊mask；其他非零模式要求命中索引mask，并使用原`(mode6_mask + mode3_mask)`而不是OR排除两类特殊flag。源记录filter flags为0时，在上述筛选后执行服务2查询与可选报告，并无条件接纳该记录。mask索引只在原表读取点检查。

接纳后，非零模式清clone second value并清source first value；模式0清clone first value并清source second value。拒绝clone按名称后记录顺序释放。目标插入保留原异常条件：当前text index小于新值，或前一link text index大于等于新值时继续；首link sentinel text index为0。不能替换成普通稳定排序。

UT覆盖模式3命中、零filter强制接纳及服务2报告、拒绝释放、10/30排序、深拷贝名称、非零模式源first/clone second清理、模式0源second/clone first清理和首分配停止；独立ASan通过。

workpack双生成稳定为`137/227`，SHA256均为`236c48a5f5b24a79747f131c3e0be411d1f458c933503c5a56ea204677a09480`；下一单元`0x00448230`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
