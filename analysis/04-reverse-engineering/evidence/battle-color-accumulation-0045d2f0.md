# 战斗三通道颜色累加 `0x0045D2F0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威函数为`0x0045D2F0..0x0045D3DE`，从proc到endp完整109行、57条实际指令、4个静态call、7个条件或无条件分支与5个局部标签，无外部FUNCTION CHUNK。

唯一caller是已关闭战斗逐帧画面协调器`0x00453200`中的一处调用。三个`0x00489654`是已闭合x87向零qword转换；尾部`0x00420490`是已闭合RGB三通道调整。四处均直接组合typed语义，不保留opaque地址端口。

## 2. 状态地址与三step入口门

九个float按LST读写关系映射为：

- 红：current `0x004FDFA4`，target `0x004FDF8C`，step `0x0052151C`；
- 绿：current `0x00521394`，target `0x00521388`，step `0x00525430`；
- 蓝：current `0x00525468`，target `0x00525448`，step `0x00520D58`。

函数依次把三个step与固定0.0比较。每次`fcomp/fnstsw/test ah,0x40`只检查x87 C3：

- 任一step为有序非零值，立即进入主体；
- 0.0与-0.0均置C3；
- NaN的unordered结果同样置C3；
- 只有三个step全部为零或unordered时才直接返回。

早退不读取参数、不递减计数、不替换step、不累加current，也不访问framebuffer。

## 3. 参数、signed计数与两条状态路径

参数非零时先读取signed计数：

- 入口计数为负，直接进入step替换路径，不执行减法；
- 入口计数非负，低32位减1并立即写回。

随后无论参数值都重新读取计数。结果非负时，严格按红、绿、蓝顺序把每个current float加对应step并回写32位float。结果为负时，不修改三个current，而是按dword位形把三项目标float复制到step。

因此计数0且参数非零先发布-1，再替换step；计数负数保持原值。逐帧caller中的ESI从入口固定为1，实际调用始终请求递减；typed入口仍保留参数0路径供完整函数合同测试。

## 4. x87转换与颜色调用

状态更新后按蓝、绿、红顺序加载三个current并调用`0x00489654`。该helper临时设置向零舍入、转换为signed qword并返回低dword：

- 正负有限值向零截断；
- qword域内但超出i32的值保留低32位回绕；
- NaN、无穷或qword越界产生integer-indefinite，低dword为0。

三次返回依次压栈后，最终`0x00420490`参数仍是红、绿、蓝顺序。固定buffer是共享战斗framebuffer，count为`0x3C000`个u16像素。typed实现调用已关闭RGB helper，只修改该前缀；第`0x3C000`像素保持不变，同时保留原helper末像素dword look-ahead读取合同。

## 5. 单一typed存储与caller回收

九个float与signed计数收敛到唯一`LegacyFrameColorTransitionState`，由`LegacyBattleColorAccumulationStatePort`虚共享。逐帧协调器在overlay最终化后直接调用本typed入口，再进入临时surface或备用surface路径；子framebuffer失败阻断surface与截图尾。

逐帧协调器此前持有的独立signed counter字段已删除，overlay创建门读取同一共享计数。战斗全局重置把十个对应dword从未映射字节像回收，固定写程序直接清同一typed状态，不保留平行副本。

## 6. 验证与动态差分

定向测试覆盖step零、负零、三项NaN早退与混合NaN继续，正计数递减累加，零计数先减后替换，参数0不减，负计数不减，float向零转换、低dword回绕、invalid低dword零、精确`0x3C000`前缀、逐帧caller顺序，以及全局重置十个dword同址清零。

当前缺少原版九float、signed计数、完整framebuffer、像素mask与逐帧状态联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
