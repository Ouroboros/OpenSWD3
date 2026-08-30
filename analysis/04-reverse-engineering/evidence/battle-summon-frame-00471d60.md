# 战斗召唤角色逐帧演出 `0x00471D60`

状态：`platform_adapted`。完整LST、typed实现、两个caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 完整权威范围

权威LST主体为`0x00471D60..0x00471FB4`，proc至endp共255行、161条实际指令、5个call、10个跳转、7个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。真实caller有两处：`0x004539B0`动作十五分支，以及`0x00466F70`消息九十七分支；二者都以ECX传group-A角色并传入signed显示坐标。

## typed语义

实现保留角色`+0x2F14`动作id、固定base variant三十六、special mode外部字段、动作更新失败零返回、frame读取、首个sample先于frame解引用、render gate为零时低字节bit0翻转与宽度反向偏移、两段独立phase判断、零到一同帧穿透、signed阈值除二、word加减回绕、三份共享motion、tick递增、每帧flags强制bit2绘制、终态固定sample及精确清零。

物理状态复用target-phase动作记录与尾块、group-A action-execution字段和shared motion/frame-source owner。新增召唤动作id、render flags、x偏移、phase与completion word的唯一typed字段。动作更新和frame provider直接复用typed实现；音频与软件绘制保留共享窄port。frame缺失typed-stop位于原始首次解引用，保留此前sample、sample-word清零、flags/x偏移和bit0翻转副作用。

动作十五caller改为typed直连；消息九十七caller也改为同一typed实现，原message-phase whole-function枚举槽保留为reserved，新增sample/render窄槽并贯通frame coordinator adapter。测试覆盖actor/frame typed-stop、陈旧动作偏移、render gate两路、零到一同帧穿透、一到二终态、signed motion、word回绕、精确清零、action十五与message九十七production后续行为及旧地址零调用。

定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`212/422 = 203 platform_adapted + 9 assembly_exact + 210 pending_audit`，SHA256为`373bd8bbe9088c04d1ae0c42b4075819e412b794f312a6477bad70bed52bedb7`。动态差分因原版角色、动作流、帧资源、音频、绘制和两个caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
