# 战斗组B行动执行 `0x004758A0`

状态：`platform_adapted`。完整LST、唯一物理owner、两处caller回收、分支与故障顺序测试、ASan/Linux门禁和inventory双生成收敛后关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x004758A0..0x00476076`，从proc到endp共869行、683条实际指令、32个call、61个跳转、45个局部标签和4个返回点，没有外部`FUNCTION CHUNK`。18个唯一callee及静态call数为：

```text
0x004170E0 ×2    0x00439070 ×1    0x0045AF90 ×1
0x0045D3E0 ×1    0x00478780 ×5    0x004787D0 ×2
0x0047C6B0 ×1    0x0047C950 ×1    0x0047CD60 ×2
0x0047CEC0 ×2    0x0047D640 ×4    0x0047F360 ×2
0x0047F940 ×1    0x00481A40 ×2    0x00482E90 ×2
0x004831C0 ×1    0x004838D0 ×1    0x00483B30 ×1
```

函数是thiscall：ECX为组B source actor，唯一栈参数是可来自组A或组B的显式target token，callee以`retn 4`弹栈。返回0表示当前帧未完成，返回1只来自完整收尾；profile-bit8前置callee失败、runtime bit15未建立、两个绘制尾和各类普通等待都返回0。

两个直接caller均在`0x00455D60`动作1：`0x00455FC8`传组A target，`0x004560A0`传组B target。组A路径入口EAX保留`target_index * 0xBCD`，组B路径保留`target_index * 0x565`；两者入口EDX都继承动作查询callee，typed caller显式转交这两个陈旧寄存器。caller只有在typed callee完整返回1后才发布pending、target、视觉提交、选择清理和对应side尾。

## 2. 前置门、记录准备与转移

入口先检查actor `+0x2A74`低word；非零直接返回0且不访问profile或callee。profile `+0xD9C`低byte bit3置位时，依序调用target前置callee与source完成callee；第二callee不返回1时先把`+0x2B1C` early latch写1再返回0。该路径不准备动作记录，也不访问资源。

普通路径先保存主记录原`+0x5A`低word，再以actor `+0x2A0C`和显式target准备`actor+0x338`主记录；该记录的flags位于actor `+0x392`。主记录bit1把`+0x24/+0x28`转移到`actor+0x468`回合记录，清零源pair并置`+0x267C` bit14；回合记录更新完成时清主记录flags、external mode和bit14，但保留已经转移的回合记录内容。该门按原顺序允许转移后的记录与runtime在callee尚未完成时跨帧保留。

主记录bit2先调用target通用更新，再把主记录flags清零并清`actor+0x3D0`次记录。主记录bit3只清自身bit3，随后更新source和次记录；主记录动作值复制到次记录首dword，profile缓冲的两个word复制到次记录`+0x72/+0x76`。这些字段与profile、配置阶段及动作29共用组B actor生命周期中的唯一action-execution owner，不建立平行副本。

## 3. 两段效果、颜色与画面刷新

主记录与次记录各自在flags与`0x11`相交非零时进入效果段。两段都保持：

- `0x0053BF98`复用动作分派的`active_effect_gate`，不复制共享门；
- effect latch为零时先查询门，门为零再更新target并以AX解释signed效果；
- signed效果不小于9999时把效果和`last_effect_value`都夹到9999；
- 效果以u32回绕累加到共享pair-primary，之后才执行提交callee；
- 提交失败且效果不是-1时按status选择target更新、finalize或source状态尾；
- 提交成功的首段顺序为source更新→source尾→target发布→source finalize，次段顺序为source尾→source更新→target发布→source finalize；
- 每段结束都把对应记录flags清零并把effect latch写1。

主记录flags同时包含bit3与bit10时，函数按记录内七个signed word初始化已关闭颜色累计器，并分别消费bit10与bit3；其他位不参与该颜色条件。无论是否触发颜色，随后都按主记录三个颜色word直接调用已关闭画面刷新；刷新后的三个snapshot任一非零时把`active_effect_gate`写1。runtime bit15仍为零时在这里返回0，不访问actor资源token。

## 4. 次记录、资源与两条绘制路径

profile mode不等于1时，函数先调用次记录准备callee，再按上述规则处理次效果；profile mode等于1时跳过准备与次效果。随后首次读取actor `+0x0C`资源token并发布为共享frame source，资源缺失严格停在该访问点，保留此前记录、效果、累计、颜色、刷新和共享门副作用。

次记录`+0x8C`不等于1时，只用资源`+0x04/+0x0C/+0x0E`、actor render flags和`+0x29BC/+0x29BE`执行第一条绘制路径；此路径不读取actor `+0x2548`，绘制后返回0。

次记录首dword为零时只把次记录完成位写1；profile mode为1时调用直接效果callee并同时写主、次完成位；其余路径由signed motion不大于-32或既有主记录完成位进入收尾判定。非direct且motion大于-32时才首次读取actor `+0x2548` render-source token：render flags低byte与`0x2C`相交时先把三项共享motion清零并把actor motion写-32，但仍继续绘制。render-source缺失时故障快照保持EAX为`+0x254C`资源指针token、ECX为此前signed motion、EDX为零；成功时使用render-source `+0x04`和资源宽高执行第二条绘制，并令motion低word减4回绕。special mode等于1时随后把4加回，因此净motion不变并返回0；其他情况保持减4后返回0。

## 5. 完成尾、唯一owner与caller回收

主记录`+0x8C`等于1时，函数按固定顺序清五个152-byte记录：`+0x338`主记录、`+0x3D0`次记录、`+0x468`回合记录、`+0x500`目标记录和`+0x6C8`效果记录；再无条件物化并清`+0xFCC`开始的`0x4C0`唯一工作区，把`+0x2A56`开始四个dword写全一。随后清action runtime、`+0x2AB0` auxiliary、early latch与`+0x26D6` completion word，但不清special mode或`+0x2AAC` turn completion latch。最后固定以bound 120调用随机callee，把AX加10后以u16回绕累加到`+0x2A12`完成延迟，并返回1。

组B状态直接扩展`LegacyBattleActorGroupBElementState::action_execution`；配置函数写同一`profile_value`，动作29也从同一组B lifecycle槽取得执行状态。已删除配置结构中的重复action id和动作分派中的八槽平行执行数组。颜色与画面刷新为typed直连，其余16个未关闭callee保持窄记录/actor/generic port，不恢复整函数opaque入口。

对手动作1的组A、组B两条caller都直接调用typed入口。typed-stop映射到`group_b_action_execution_typed_stop`并阻断全部caller后缀；返回0同样阻断pending、pair transition、视觉提交和side尾。生产源码不再包含`0x004758A0`整函数token。

## 6. 验证与动态阻塞

定向测试覆盖入口start gate、caller入口EAX/EDX陈旧值、profile-bit8第二callee失败与early latch、主记录准备、bit1记录转移、runtime bit14及`mov ax`高半继承、首段负效果与u32累计、次段正效果与成功发布、效果提交EAX/EDX、七word颜色初始化、刷新与直接效果、资源首次访问stop、第一条资源绘制不访问render-source、第二条render-source首次访问stop及故障寄存器、非direct motion减4、special mode净零、五记录与工作区无条件物化清理、四个全一target槽、bound 120随机完成延迟、返回0/1，以及对手动作1的组A/组B target caller、旧opaque零调用和typed-stop传播。

最终`./build-asan.sh`、`./build.sh core`和`./build.sh app`分别完成AddressSanitizer core `188/188`、Linux core `188/188`与Linux app `194/194`；零OpenSWD3源码warning、sanitizer finding或测试失败。inventory生成器连续双跑逐字节一致，关闭进度为`242/422 = 233 platform_adapted + 9 assembly_exact + 180 pending_audit`，SHA256为`f47b368d0fd12ac6b17afd257f6dacd01a38c5fc53282b1603b264956a57f8ad`。

当前缺少原版八个组B完整actor、组A/组B target、动作/profile/资源/render-source物理布局、16个未审callee、共享累计/颜色/framebuffer/RNG状态及两处caller寄存器联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。
