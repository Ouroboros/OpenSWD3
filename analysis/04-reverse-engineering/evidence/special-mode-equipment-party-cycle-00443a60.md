# 装备物品模式party循环重建 `0x00443A60`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00443A60..0x00443B6B`，127行，无FUNCTION CHUNK。code caller为F40一处，3B480另绑定为动作callback；F40现已直接回收。BB40、B9A0、BC90、B9C0、B9E0与444E80已关闭并直接复用；444F00、444F60、444FB0继续保持最窄typed端口，sample107保留平台端口。

函数仅在mode1执行；其他mode不读写状态。进入后先调用444F00等价清理端口，再把party selector low16加1并按四槽取模，保留high16。首个候选立即写入selector；若marker为FFFF，则继续循环候选，找到后才再次写selector。四项全FFFF时原函数无限扫描，modern完成四项原始域读取后typed-stop，并保留首个候选写入。

选定party后严格按LST顺序调用444F60，再直接调用444E80；后者重建total、offset、local、visible与record head，后续全部重读。随后直接调用BB40规范窗口，B9A0定位visible head，BC90最多计24项，B9C0按`offset+local`选中记录，B9E0发布共享文本。

成功后以selected party action调用444FB0；返回值先发布到动作数owner，再播放sample107。444F00/F60/E80/FB0不可用时在各原call site停止；B9C0 null与B9E0失败保留此前party与列表重建副作用。444FB0停止时旧发布动作数保持，且不播放sample。

F40点击party区域的循环caller已直接调用本helper。helper完成后重读selector；callee若回写旧selector导致无进展，F40仍按原四槽域检查后typed-stop。callback不再经过通用cycle_party opaque端口。

UT覆盖跳过FFFF到下一有效party、high16保留、完整重建/text/action/sample顺序、非mode1无操作、全FFFF、四个pending callee停止、B9C0/B9E0停止，以及F40到目标party与无进展停止回归。

workpack双生成稳定为`105/227`，SHA256均为`7f49a7a41eb38a3559ab9ba4ad3ccec742c8be72ecf58fa8293d2851c980a88d`；下一单元`0x00443B70`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
