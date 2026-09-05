# 战斗组A动作执行 `0x0046F8C0`

状态：`platform_adapted`。完整LST、actor执行owner、flag状态机、slot与绘制路径、行动调度caller直连、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046F8C0..0x0046FEEE`，从proc到endp共668行，其中637个非标签物理行、386条实际指令、11个call、48个跳转、31个局部标签、5个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall，两个栈参数分别为目标actor token和0x98记录槽索引，所有出口以`retn 8`弹栈。

入口先检查actor word启动门和dword执行完成门；任一命中直接返回0。特定跳过状态、actor渲染flag与早退latch组合会先查询目标，查询返回0时写早退latch 1并立即返回该零寄存器前缀。

## 2. 记录准备与基础flag

第二跳过状态为1时把actor两个位置word固定写为0x140和0x136。函数随后发布动作pending，按特殊模式或记录mode bit1设置force gate，并构造首个0x98记录：资料值写首dword，动作类型通常为0x28；profile mode为1时置共享profile gate并选择0x29，跳过状态下回到0x28，否则复制actor word；非profile模式下可按alternate mode选择0x30或0x31。

目标与首记录交给准备callee后，actor flag bit1会把两项临时值复制到第二0x98记录、发布运行时0x4000，并消费bit1、临时值和可选force bit。运行时0x4000再驱动第二记录查询；callee返回1时清force、bit9和运行时0x4000。

actor flag负位发布两个共享全局；bit3可先按bit10执行七个signed word颜色初始化，然后清bit10和bit3、置运行时0x8000并清所选slot。bit2调用目标重置并把actor flag清零；bit0清motion、共享motion和slot，调用目标模式1后清flag。所有slot按原`slot * 0x98`寻址，越界只在首次实际clear或访问处typed-stop。

## 3. 活动slot与绘制

运行时0x8000未置时返回0。活动slot首dword默认取actor复制word，临时primary非零时覆盖；slot第二dword清零。首dword为零时直接把slot完成dword写1。

首dword非零且actor渲染flag bit0为1时，按actor位置、源目标偏移、Y辅助值和记录token调用准备绘制callee；只有返回1才发布actor完成gate、slot完成dword和slot word `+0x5A` bit0。否则走另一目标记录callee。slot word bit0随后触发目标模式1调用并清该word。

slot完成dword不为1时，以actor绘制位置、渲染flag和零资源参数调用blitter后返回0。完成dword为1、首dword非零、渲染bit0未置且motion signed word大于-32时，先发布三项motion共享值；渲染flag命中0x2C时三项清零并把motion夹为-32。资源token在此后首次解引用，缺失则typed-stop；存在时按资源尺寸、渲染flag和资源值绘制，motion减4，特殊模式1再加4，随后返回0。

## 4. 完成清理与激活倒计时

没有活动motion且actor完成gate不为1时返回0。完成时依次清首记录、第二记录、当前slot、第三和第四色记录五个0x98块，把四个目标索引写全1，清motion、actor完成、早退latch和运行时flag，并把motion辅助word写1。

共享profile gate为零时读取物品效果唯一owner中的激活byte。激活为零先清效果flag bit0并完成；激活大于零只减1，在完整清理前缀后返回0。共享profile gate非零则绕过倒计时直接完成。完成路径递增共享低byte计数，清actor profile mode和共享profile gate，返回EAX/EDX均为1。

## 5. owner与caller回收

新增每actor动作执行owner保存actor局部0x98记录、slot、位置、flag、motion与资源视图；运行时动作flag继续复用`LegacyBattleActionDispatchState`，完成字段复用startup party进度owner，效果flag与激活byte复用第180项物品效果owner。actor跳过状态通过frame coordinator从胜利奖励唯一owner绑定为只读span，不复制第二份数组。

全程序三处静态caller均位于已关闭行动调度器；typed源码将其收敛为两个直接调用位置，覆盖普通行动case 1及共享后段。目标token按side选择，槽索引固定0。旧完整函数地址调用生产零次，子callee以明确参数和寄存器的窄action port保留。

## 6. 验证状态

单元测试覆盖首actor typed-stop、两项入口门、目标查询早退、基础完成清理、激活倒计时、第二记录bit消费、颜色七参数与slot清理、bit0目标模式、渲染bit0完成、motion资源typed-stop和slot越界。行动调度回归覆盖两种side普通攻击、阻塞效果、framebuffer越界及旧opaque零调用。

验证结果：定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning，app仅有既有ALSA提示。inventory连续双生成逐字节一致，稳定为`184/422 = 175 platform_adapted + 9 assembly_exact + 238 pending_audit`，SHA256为`769db821e21fdc38842ae86bb1cbbf882b1a3453689c411e6fec2c3c8abfa506`。原版组A actor完整状态、目标actor、资源记录、九类callee副作用及三处caller寄存器联合捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。
