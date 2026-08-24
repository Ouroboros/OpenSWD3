# 特殊模式转场画面初始化 `0x00448700`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。物理范围`0x00448700..0x00448833`，134行、8个call，无FUNCTION CHUNK。code caller为409540与已关闭43B480；446700的318物品路径也通过既有callback port复用该入口。

函数首先把四个边界owner统一写-16，清enabled/progress/snapshot owner，velocity写-6。模式1和2均把progress改100、velocity改-120，并分配精确0x96000字节快照后复制当前framebuffer；两模式行为相同但保留两个独立if。平台快照失败在原malloc/copy边界typed-stop，保留此前边界、progress和velocity副作用，不执行尾部owner发布。

模式3把progress与enabled均写1，再调用专用初始化。所有非停止路径清shared owner。模式0调用probe；仅返回非零时按顺序执行prepare、格式化command10、应用命令、激活surface，最后调用返回值覆盖EAX。随后清两个trailing owner并发布当前surface token。

43B480高模式分支已删除`initialize_high_mode_runtime` opaque方法，直接调用本helper；失败时不安装G09十三槽回调。446700的318路径同样直接调用，失败映射为transition visual stop。callback port收紧为typed visual state与visual ports getter，所有fixture已适配。

UT覆盖模式1/2精确快照尺寸与内容、四边界、100/-120；模式3 owner与专用调用；模式0完整五helper条件链；快照停止前缀；43B480停止时零槽写及正常caller回归。独立ASan通过。

workpack双生成稳定为`143/227`，SHA256均为`ae438836c86350a2607dc580f2dcebe4afd18cff706f228443a084ce34febb46`；下一单元`0x00448840`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
