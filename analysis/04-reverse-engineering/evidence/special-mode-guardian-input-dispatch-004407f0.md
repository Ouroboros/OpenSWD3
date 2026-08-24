# 护驾系统输入分派 `0x004407F0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。完整函数范围`0x004407F0..0x00440B18`，405行；39FD0与B480把本函数地址写入共享输入callback。callee为40DC50、437300、B9C0、B9E0、C090、440B20/440C20/440D20/440E10、441060/441160/441590及4429B0。

新增`LegacyStandardModeGuardianInputSnapshot`承载buttons、原版纵向坐标4A9928、横向坐标4A9924及fastcall ECX/EDX。所有未闭环callee通过一个可变typed invoke边界表达；每次callee后只在LST明确位置重读state/input，不假设callee不改全局。

## 区域优先级

1. interaction mode15且buttons低nibble非零时，以入口ECX/EDX调用441160并立即返回；mode5跳过所有区域，仅尾部button4调用441590。
2. buttons低2位且纵105..411、横201..433时处理16槽护驾列表。mode1先441590，再按`(Y-104)/28-1`写guardian slot并调用440B20；其他mode仅在原比较不等时写row-1并调用。保留“比较row、写row-1”的原始偏差。
3. buttons低2位且纵位于`panelY-12..panelY+207`、横位于`panelX-34..603`时处理record列表。mode0先441160再按调用后坐标更新local selection；其他mode在同row时441160，不同row时更新selection、按`list_offset+selection`索引forward node并直接B9E0发布文本。短链/null及文本失败在原读取点typed-stop。除mode15外尾调4429B0。
4. 直接复用C090检查availability record15。可用且total>10、横611..625时，mode0调用441160；mode1按纵103..115、305..319及两组动态strict边界分派440C20、440B20、440E10、440D20。
5. 无条件先清hover；纵465..479、横535..607时写-1。低2位点击先调用437300确认音，并按调用后mode/buttons/坐标继续。
6. 低2位点击且纵11..467、横5..187时处理角色切换。mode1先441590；mode0按`(Y-10)/110`查询物品`30+row`，存在时仅改party selector low16为`row-1`、保留high16，再调用441060。
7. 未命中以上路径时，button4尾门调用441590；否则返回当前buttons EAX。

UT覆盖mode15/5、护驾槽mode1双调用、列表B9C0/B9E0与null typed-stop、availability快捷区、确认hover、角色物品门与high16保留、button4尾门及availability越界。端口记录每次ECX/EDX，锁定入口寄存器和计算后寄存器边界。

定向测试通过。workpack双生成稳定为`72/227`，SHA256均为`23ae3a2bfcfb0246173fbd346f2677bd9265dd11f499c0237a72dfd0d434cfcc`；下一单元`0x00440B20`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
