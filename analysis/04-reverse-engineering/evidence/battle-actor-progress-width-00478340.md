# 战斗角色进度宽度 `0x00478340`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST与ABI

权威LST主体为`0x00478340..0x00478364`，从proc到endp共10条实际指令、1个静态call、0个跳转和0个外部`FUNCTION CHUNK`。函数是无栈参数的thiscall，输入ECX为角色对象；唯一callee为已核定的x87向零qword转换`0x00489654`。

入口先`push ecx; xor eax,eax`，随后从角色`+0x2A12`读取u16到AX，因此进度值严格零扩展，不继承入口EAX高位。返回前`pop ecx`恢复原角色token；成功时EAX/EDX分别是signed qword结果的低/高dword。

## 2. x87计算与异常值

角色进度先写入栈dword并以`fild dword`装入x87，再严格按以下顺序计算：

```text
scaled = long_double(zero_extend_u16(actor.progress))
       / signed_i32(action_threshold)
       * 62.0
result = x87_fistp_qword_round_toward_zero(scaled)
```

分母是共享`0x004A74CC`，由已关闭战斗速度阈值发布器维护，初始值900；常量`0x00499D30`的IEEE-754单精度字节为`00 00 78 42`，即62.0。typed实现用分步volatile `long double`保留两次x87算术的扩展精度边界，并复用已核定的signed qword向零转换域。

分母为零时不增加现代错误分支：有限非零/零分子分别形成无穷/NaN，最终都由`fistp qword`产生integer-indefinite `0x8000000000000000`，即EAX为0、EDX为`0x80000000`。负分母产生负signed qword并保留EDX符号扩展。

## 3. 访问停点与寄存器

角色进度读取不可达时，停点位于`xor eax,eax`之后：EAX为0，ECX仍为角色token，EDX保持入口值，x87栈未增长，转换调用数为0。

共享阈值读取不可达时，角色进度已零扩展写入EAX并完成`fild`：EAX为该u16值，ECX仍为角色token，EDX保持入口值，x87栈深度为1，转换调用数为0。成功转换消费唯一x87值并恢复控制字，返回时x87栈深度为0。

固定映像常量62.0不建立可变owner或替代值；现代typed停点只对应两个实际可变读取。

## 4. 四个物理caller

完整LST共4个物理callsite：

- HUD `0x00459D10`：`0x00459DDE`在顶部组A primary条之后查询宽度，只消费AX零扩展值，非零才查询颜色并绘制三高条；`0x00459F70`在第二轮首次blocked查询为0后查询宽度，随后重读另一状态门，为0时把完整EAX作为分层条宽度参数。
- 调试叠加层`0x0045DEE0`：`0x0045E285`在每个组B位置查询与raster行偏移计算之后查询宽度，只消费EAX低word，按无符号列循环写两行`0xEEEE`。
- 当前目标提示`0x00466950`：`0x00466B9B`在生命指标达到15后查询组B宽度，只消费AX零扩展值；call前地址算式留下`EDX=345*code`，非零才查询颜色并绘制三高渐变。

两个组A callsite读取startup party索引对应的唯一progress owner；两个组B callsite分别以零基debug索引，或把提示路径`0x004A754C`的一基发布code映射为startup `enemies[code-1]`。后者的物理地址以`0x005229E0 + code*0x2B28`构造，恰好使code 1命中startup enemies基址`0x00525508`。四处都读取startup timing唯一阈值owner，不复制进度或阈值。

## 5. caller回收与验证

HUD、调试叠加层和当前目标提示均直接组合typed叶子；四处入口EDX分别取显式frame回复、blocked查询回复、位置查询回复和`345*code`地址中间值。旧HUD地址常量、调试`query_marker_width`和提示`query_fade_width`端口边界已删除或改为reserved兼容槽，生产调用为零。typed-stop分别保留顶部条、位置/行偏移、生命文字等已到达前缀，并阻断颜色、像素或后续HUD阶段；视觉转场、选择帧和逐帧协调器继续按既有父级状态传播。

定向测试覆盖u16零扩展、正负阈值、零阈值integer-indefinite、角色读取停点、阈值读取停点、EAX/ECX/EDX与x87栈深度，以及四个caller的正常owner和三类后缀阻断。当前缺少原版组A/组B完整对象、动态`0x004A74CC`、x87控制字异常掩码及四callsite寄存器联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
