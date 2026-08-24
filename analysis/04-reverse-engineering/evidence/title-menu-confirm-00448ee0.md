# 确认标题画面菜单选项并执行对应操作 `0x00448EE0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。主范围`0x00448EE0..0x00449045`，纳入外部FUNCTION CHUNK `0x00498310..0x00498324`；共161行、7个call。直接caller为48700、48840，另由43B480发布到callback槽。

函数先保留`progress-1`，仅progress=1进入确认；进入后先发布velocity=100、progress=2，再按enabled逐次预减分派。selection0无helper并返回-3。selection1把velocity改97、禁用服务50，发布feature owner组，分配并清零32字节overlay storage，velocity增至98，再以kind8、X300、Y230构造overlay owner。selection2先释放共享记录，再切progress5、velocity0并发布动作232A/variant34。selection3依次发布命令16/25并执行finalize。其他selection返回第三次预减残值。

原外部chunk负责overlay构造异常路径上的storage释放；typed实现仅在32字节原始分配点捕获`bad_alloc`并以`overlay_allocation_stopped`停止，保留此前服务禁用、feature owner与velocity97副作用。构造owner返回0仍按原空owner分支完成。

48700的mode3初始化和48840四段点击均删除opaque刷新端口，直接调用同一typed确认函数；48840继续在每一段检查前读取state坐标，确认回调对state的修改不会被陈旧snapshot覆盖。SDL只实现最窄服务、记录、overlay和命令平台边界。

UT覆盖selection0残值、selection1完整owner与helper参数、selection2阶段/动作切换、selection3命令顺序，并验证48700 mode3直接进入progress2/velocity98以及48840四段caller直连。

workpack双生成稳定为`152/227`，SHA256均为`812c008c4f8af6f8cafc2129d234e33c3f864abe1d7caae455c04db384ea80d5`；下一单元`0x00449050`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
