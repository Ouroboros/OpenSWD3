# 护驾交互状态切换 `0x00441160`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整范围`0x00441160..0x00441567`，460行；caller为407F0四处、B480 callback及41590。

## mode分支

- mode0先写mode1，再按`list_offset+local_selection`读取当前node，直接B9E0发布文本并调用4429B0；短链typed-stop保留先写mode1。
- mode1先定位当前node。guardian slot为0且node文本为FFDC时写mode15并播放sample140。否则执行0xB0护驾slot交换，重算总数，B9A0/BC90重建可见链，BB40调整窗口，按`party.low16*16+slot`发布文本，mode减为0，4429B0并播放sample46。
- mode5返回旧FCD48，同时写countdown480，清FCD58/FC648/FCD48，以FCED0恢复mode，并把旧值发布到FB808。
- mode15写mode1但返回入口EAX=15。
- 其他mode原样返回。

mode1内联0xB0复制、token释放/克隆、44D2D0链插删、441F70重建及secondary list分支被收敛到单一`exchange_guardian_record` typed port；该边界必须按LST完成整段原子交换并可变更record head。边界外的判定、先前副作用、总数/可见链/窗口重建、文本、mode、sample和返回值全部由typed helper控制。exchange失败在原交换起点停止，不伪造后续副作用。

407F0四处interact caller均已删除opaque invoke并直接复用本helper，传播typed-stop。UT覆盖mode0先写、mode1 sentinel与正常交换、exchange参数、mode5/15、默认mode及407F0 mode15直接caller。

定向测试通过。workpack双生成稳定为`80/227`，SHA256均为`7e3056b86a84b0a314a7558c525cd91b69ec3cd48f3a895eae3d314ad54670a1`；下一单元`0x00441590`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
