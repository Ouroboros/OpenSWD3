# 绘制标题画面菜单、读取进度窗口和游戏设置页面 `0x004490C0`

状态：`platform_adapted`、`unit_tested`

唯一行为真值为`swd3.exe.lst`。主范围`0x004490C0..0x00449C0B`，纳入外部FUNCTION CHUNK `0x00498330..0x00498340`；共1342行、61个call。唯一引用为43B480的callback数据槽。

函数恢复为四阶段typed帧状态机，并以即时`LegacyStandardModeTransitionCommand`保持原helper调用顺序和EAX低字节：

- progress0..2先绘制基础动作，按动态enabled预减选中边界4并夹-32，再把四边界增加2并夹-12，依次发布四块滑入面板。负velocity预增；仍为负时发布三通道`5*velocity`渐变，回到0时发布progress1并清结果latch。
- progress2在velocity>=100时预增。101..104发布`8*(100-velocity)`渐变；105后按enabled分派四类runtime owner、设置阶段、返回选择或low-byte flag，并保留设置分支的双清屏。velocity<100时恢复asset探测、倒计时、两套overlay choice映射、owner update/poll、latch fallback、两段16字节文本复制、第二overlay构造、服务50启用及取消路径。
- progress5严格发布57条即时命令：frame、六label、sample/surface各12格、spacing三格与值、service两格与值、source五格与值、auxiliary十二格和cursor；同帧执行三次服务48查询、source文本长度读取，并恢复文字cursor/countdown推进。
- progress100仅在原读取点检查`0x96000` snapshot。mode1在velocity=-120发布快照blit，并恢复三段effect offset与动作232A/68；mode2每帧恢复snapshot，velocity整除3时发布-1渐变。velocity预增到非负后发布六个runtime清零owner、释放snapshot并清framebuffer。

动态enabled越界只在首个基础draw后的原数组读取点以`selector_out_of_range_stopped`停止。overlay storage只在每段16字节写入和最终32字节读取点检查；snapshot只在mode1首帧或mode2每帧原读取点检查。此前副作用全部保留。

498330外部chunk负责第二overlay构造异常时释放临时对象；typed平台边界为`noexcept`构造owner，构造生命周期及异常回收由该最窄端口承担。490C0不提前关闭449C30/449D80等callee，滑块与设置绘制继续以typed即时命令表示，待各callee独立审计后回收实现。

UT覆盖入场边界/渐变、动态selector停止、选择0与1到期分派及双清屏、两段overlay文本与完整owner、progress5的57命令/61 helper、mode1与mode2 snapshot以及精确缺失读取停止。SDL adapter即时执行清屏和真实framebuffer snapshot复制，其余命令保持隔离的窄平台边界。

workpack双生成稳定为`154/227`，SHA256均为`9ac033eb0df2f8d7b62933ef2f101218faac2e8c344220123944cd0aeed1e588`；下一单元`0x00449C30`。Linux完整门结果见最终验证；按阶段门禁不运行Windows BUILD。
