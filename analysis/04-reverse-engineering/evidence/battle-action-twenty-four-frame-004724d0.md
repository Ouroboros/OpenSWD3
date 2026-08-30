# 战斗动作二十四双层逐帧演出 `0x004724D0`

状态：`platform_adapted`。完整LST、typed实现、caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x004724D0..0x004726F4`，proc至endp共239行、155条实际指令、6个call、8个跳转、7个局部标签、3个返回点，没有外部`FUNCTION CHUNK`。唯一真实caller是动作dispatch的case 24。

函数复用行动者`+0x338`唯一`LegacyActionRecord` owner：写入资料动作号、variant 40、special mode，并先置`+0x2AAC`完成latch。动作更新成功后读取frame并发布共享源；随后从record复制原x偏移、mode与actor辅助x。`+0x2B08`为一时翻转绘制flags低位，以frame宽度镜像record x偏移和非零辅助x。

绘制路径发布frame高度三分之一与四分之一及三项`-6`运动值。sample播放参数保留frame token高半和record sample word低半，声像参数保留sample callee返回EDX高半且固定为`-16`。首层flags保留bit31和低四位并置bit2/bit3；首层坐标为signed行动者位置减完整32位x偏移、signed y减高度三分之一，第二层保持相同x并以signed y减完整32位record y偏移。sample word在绘制前清零。

绘制后，actor action flags低字节bit0或bit3任一非零时，清整word并返回`copied_runtime_word | 0x8000`；否则record完成位不等于一时返回零。完成位等于一时清latch、清整条152字节record并返回二。已关闭动作更新与frame provider typed直连；sample和软件绘制保留窄port。case 24 caller不再调用整个旧地址，直接消费typed低word并保留后续高位目标扫描。

测试覆盖frame原访问点、宽度镜像、陈旧sample高半、双层坐标、特殊高位返回、完成清零、caller目标扫描及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`216/422 = 207 platform_adapted + 9 assembly_exact + 206 pending_audit`，SHA256为`0396b1b954d73c7ee168de52bed12094255a5ecb13fdb96c5af7e876de725f71`。动态差分因原版行动者record、frame、sample、绘制与caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
