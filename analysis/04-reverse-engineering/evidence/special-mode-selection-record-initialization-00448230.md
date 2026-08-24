# 特殊模式选择记录初始化 `0x00448230`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。主物理范围`0x00448230..0x004482CD`，并包含外部FUNCTION CHUNK `0x00447FD0..0x00448017`；5个call。code caller为445430、446420、446550、446700。

函数以分类owner作为mode直接调用已关闭48020。clone typed-stop时立即返回，不执行后续源链清理。成功后逐节点检查source first/second value；两者均0时从source链摘除并经窄port释放，否则保留并继续。目标链为空时在原44D5D0分配点创建FFDC缺省节点并清next；分配失败typed-stop。

随后直接调用已关闭B980统计完整目标链，发布local count，清window offset与local cursor，并把visible head指向目标head。外部chunk先把13个row label依次写100..112，再从visible head最多检查13项并发布visible count，最终只在`labels[visible_count]`写0；第14个数组元素只用于visible count恰为13时的terminator。

445430、446420、446550、446700中原`initialize_selection_records`四处opaque caller均改为直接调用本typed helper；初始化port拆成source、mode mask、特殊mask和record lifecycle getters，陈旧方法已删除。缺省source现在按原48230生成FFDC记录，因此445430空source从旧测试中的“selected missing”改为完整初始化；分配失败前缀和helper嵌套计数同步更新。

UT覆盖直接复用48020、两个空source节点释放、保留非空source、目标计数1、窗口清零、100/0标签、空目标缺省FFDC节点，以及四处caller成功/停止/文本和分类回绕回归；独立ASan通过。

workpack双生成稳定为`138/227`，SHA256均为`5e92b2baf5a2e17c48a3eeb074996219ab07a960c0182041f3ee161666ef00da`；下一单元`0x004482E0`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
