# 炼妖祭坛四槽原图surface准备 `0x0043FDE0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整函数范围`0x0043FDE0..0x00440092`，311行；唯一caller为E3D0 phase1。callee为4019A0、4315D0、4321E0及失败日志nullsub_1（44A240）。

## 行为

1. 进入后严格依次清零FCAA8、FCB94、FCD08、FCB9C四块0xF0 buffer，再清零FCABC、FCAC0、FCAA0、FCD0C四块0x1B8 buffer；任何动作解析失败前均已发生全部八次清零。
2. 第一槽固定解析action `0x232C`、variant `0x4E`、reserved 0；surface写FCD1C。
3. 第二槽按FCA8C/interaction toggle在FCA88与FCBA0两份runtime record间选择，从`+0x5C`读取action，variant固定`0x44`；surface写FCB90。
4. 第三、第四槽分别从相距0xB0的FCC04/FCCB4 inline record首word读取action，variant固定`0x44`；surface写FCA9C/FCA98。
5. 每槽严格执行4321E0解析、读取C69A/C69C、4315D0取frame、4019A0生成原图surface。三个未闭环callee合并为最小`prepare_database_original_surface`平台边界；四槽请求、顺序、variant和结果owner仍由typed helper控制。
6. 任一4321E0失败立即调用无副作用日志stub并返回。保留原EAX：第一槽0、第二槽0x44、第三/第四槽对应action ID。typed状态分别为fixed/selected/first-inline/second-inline missing，且不回滚已清buffer和已写surface。
7. 全成功时返回第四surface token。E3D0 phase2已删除原整块`prepare_database_phase_2`边界，先写phase3/countdown `-40`，再直接调用并聚合四次helper；失败时保留phase/countdown和FDE0既有副作用，并在sample前传播typed-stop。E3D0 phase1的4404D0仍保持独立边界，不混入本工作包。

新增`prepare_legacy_standard_mode_database_original_surfaces`、四槽请求/结果及state-owned surface token与120×220像素数组；平台边界成功时直接填充对应像素owner，供已关闭4400A0使用。UT覆盖第二runtime record选择、四个ID/variant、八块清零、四surface token/像素写入、四种失败EAX和E3D0直接集成；独立ASan执行通过。

定向测试通过。workpack双生成稳定为`67/227`，SHA256均为`7fc5aea4c4435bb90aafea4135c0b4f7b7ae4db2f93c0d0a035d845e25361c41`；下一单元`0x004400A0`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
