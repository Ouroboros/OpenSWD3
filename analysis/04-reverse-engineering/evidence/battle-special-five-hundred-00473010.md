# 战斗动作五百特殊记录协调 `0x00473010`

状态：`platform_adapted`。完整LST、typed实现、唯一caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x00473010..0x00473198`，proc至endp共175行、105条实际指令、3个call、9个跳转、7个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一caller位于动作dispatch的动作500分支：this为当前group-A行动者，唯一参数为选中group-B对象token；仅当本函数返回一时才继续发布目标和效果分数。

函数把行动者profile word加1500写入`+0xAF0`特殊动作记录，复制`+0x2A8A`到base variant并置完成latch，然后调用待审特殊动作更新callee。若记录flags bit1置位且field24非零，则先置行动者runtime gate bit14，再把field24与field28转移到`+0x468`记录的action ID与base variant。flags bit9同时置位时发布特殊记录external mode一。之后无条件破坏性清零field24、field28并清除flags bit1。

runtime gate bit14置位时，以`+0x468`记录和特殊记录field78调用待审逐帧callee；严格只在返回一时清除runtime bit14、清零特殊flags与external mode，并整记录清零`+0x468`。该清理发生在后续颜色flags读取之前，因此完成的逐帧callee会同时抑制本帧颜色路径。

特殊flags bit3置位时进入共享完成发布；bit10也置位时先写唯一颜色初始化gate，按signed word顺序把field7A至field86的当前RGB、目标RGB与countdown交给已关闭的typed颜色初始化，再破坏性清除bit10。随后仅在共享完成flags尚无bit15时置位bit15。共享完成flags bit0未置位时返回零；置位时整dword清零行动者runtime gate，整记录清零`+0xAF0`并返回一。

实现新增特殊profile variant与`+0xAF0`完整记录owner，并把原先仅服务动作二十七的`+0x267C`字段重命名为共享runtime gate。`0x0053C050`由group-A共享状态唯一持有。两个未审callee通过带动作记录引用的窄port保留；已关闭颜色初始化改为typed直连，并同步回收group-A动作执行中遗留的同一opaque颜色callee。`0x0053C030`颜色gate统一归既有color state port，移除共享状态副本。

测试覆盖事件转移、bit14、external mode、破坏性源字段清零、两个pending callee参数、逐帧非完成、signed七颜色参数、bit10单次消费、bit15发布、bit0完成门、runtime与特殊记录最终清零、原共享访问点typed-stop、production动作500 caller及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`223/422 = 214 platform_adapted + 9 assembly_exact + 199 pending_audit`，SHA256为`123a6e7c65e089a9165d570b9b62d23f41cea80dad85a05f200437b995182066`。动态差分因原版特殊记录、`+0x468`记录、两个pending callee、共享完成flags、颜色状态与唯一caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
