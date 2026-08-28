# 战斗文字面板 `0x00469550`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00469550..0x00469619`，从proc到endp共107行、73条带机器码和真实助记符的实际指令、5个静态call、2个跳转、1个局部标签和2个返回点，没有外部`FUNCTION CHUNK`。两个静态caller都位于已关闭战斗HUD帧：顶部每个active组A角色绘制一次姓名面板，footer mode精确为1时绘制一次底部提示面板。

## 2. 共享动作、矩形与九宫格

函数保存EBX/EBP/ESI/EDI后把共享动作记录的action id写为`0x233B`、`+08` base variant写零，再以固定记录token调用动作更新。modern直接复用`LegacyBattleVictoryRewardState::panel_action_record`唯一owner，不建立HUD私有副本；动作更新可发布live `field_4a`。

矩形调用参数严格为`left,top,width,height,0,4,4,0`，调用入口EAX重载完整width，ECX/EDX保留动作callee返回。随后按u32回绕得到内框`left+4,top+4,left+width-4,top+height-4`。资源先从完整width重建EDX，再只用共享动作`field_4a`替换低word并保留width高16位；九宫格调用入口EAX为right、ECX为left+4、EDX为资源，参数尾固定`0,0x80000008`。

## 3. 两种文字坐标与寄存器链

九宫格返回后，函数无条件加载显式text X到EAX、text Y到ECX。只有两者都精确为零时使用相对分支：实际坐标改为`left+2,top+4`，文字调用入口EAX为text token、ECX为固定字体token，EDX保留九宫格callee返回。任一显式坐标非零时保持调用者坐标，文字调用入口EAX为固定framebuffer token、ECX为固定字体token、EDX为text token。

两路都以固定字体token、framebuffer token、颜色`0xFFC0`和字体16调用文字服务，并原样返回文字callee的EAX/ECX/EDX。modern只把物理地址建模为u32 token，不转换为宿主指针；left、top、width、height及相对坐标计算均保留低32位回绕。

## 4. caller回收与验证

HUD顶部caller在解析姓名token后直接调用typed helper，入口寄存器精确重建为`EAX=name token, ECX=top+2, EDX=left+5`，使用显式文字坐标。底部caller保留magic除3等价的signed向零结果和发布顺序，入口寄存器重建为符号修正位、`68-position`与delta，并以双零坐标触发相对分支；最终HUD返回文字callee完整EAX。旧函数token保留为reserved常量，生产代码零调用。typed helper内部四类实际服务按动作更新、矩形、九宫格、文字顺序执行，调用数从一个opaque边界展开为四次服务调用，并计入HUD父级port总数。

定向测试覆盖共享动作owner、动作记录token、两个caller入口寄存器、矩形与九宫格参数、width高word资源链、显式/双零/单非零三种坐标门、默认分支陈旧EDX、u32负坐标回绕、文字返回寄存器、旧槽零调用、HUD顶部与footer组合路径。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码构建零warning；app仅有既有ALSA开发库CMake提示。

原版动作更新、framebuffer、矩形、九宫格、字体/文字与caller寄存器联合捕获后端尚不可用，`original_diff_verified`为`blocked_runtime_oracle`。
