# 战斗动作四百零五效果与渲染 `0x004731A0`

状态：`platform_adapted`。完整LST、typed实现、唯一caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x004731A0..0x00473598`，proc至endp共430行、277条实际指令、22个call、14个跳转、10个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。唯一caller位于动作dispatch的动作405分支；this为当前group-A行动者，唯一参数为选中group-B目标token，严格只在本函数返回一后继续发布目标、效果分数与后续画面提交。

函数先把行动者profile word加1500写入`+0xAF0`特殊动作记录，复制特殊profile variant并置完成latch，然后typed直连已关闭动作记录更新。更新返回零时保留初始化副作用并返回零。随后按记录field4A/field4C typed直连已关闭frame provider；原查询结果先写`+0x254C`再立即解引用，因此frame缺失typed-stop严格位于token写零之后。

frame就绪后发布唯一共享frame token，复制记录draw offset、field76与mode flags。`+0x2B08`等于一时只在局部EBX语义中翻转mode bit0，按word回绕把frame width减draw offset写入`+0x29B4`，field76非零时同样更新`+0x29AC`；函数不额外写`+0x26A0`。frame height按原32位乘法商计算三分之一并逻辑右移计算四分之一；`+0x2B00`等于一时三份共享运动值为负一，否则为负六。

音频路径保留已关闭sample命令的窄平台port，并保留frame token高半加field58低半的陈旧ECX参数。声像按signed绘制X与320比较，小于时传负十六，否则传正十六；field58随后破坏性清零。第一层绘制使用`x-5`、`positionY-height/3`和低四位加bit2/bit3的flags；第二层使用原X、`positionY-drawOffsetY`及镜像后的局部flags。

特殊记录field64、field66、field68以typed方式进入已关闭共享画面刷新；调用前显式注入当前EAX/ECX/EDX，保留callee早退时的寄存器链。三word任一非零时发布共享刷新pending。特殊记录field5A低字节与九按位与为零时返回零。

效果阶段在`0x00473434`从startup/lifecycle canonical owner按X后Y顺序读取目标坐标，写入既有两个零初始化dword局部槽的低word，再按原顺序计算`positionX-offset+field76`与`positionY+field78-drawOffsetY`，随后递增`+0x2F26`word。坐标typed-stop保留此前更新、frame、sample、绘制与刷新前缀，并阻断phase tick及全部效果后缀。待审效果更新callee接收目标token、`+0x630`唯一动作记录、零、运行word、两坐标、signed sourceY和一；返回非一时保留递增tick与两记录。

完成时先刷新目标，再以目标和双坐标调用待审效果计算。返回AX按signed word扩展，仅大于等于9999时夹到9999；负数保持不变。值写入共享last effect并以32位回绕累加到唯一pair primary owner。之后严格执行十一段发布：目标signed值、目标属性一、行动者动作标识、取负signed值、模式零、属性一、第二动作标识、累计值、模式八、属性一、最终视觉提交。最终提交的累计参数只替换前一callee返回EDX的低word，保留陈旧高半。

成功尾部将phase tick清零，依次整记录清零`+0x630`与`+0xAF0`并返回一。原先仅命名为动作二十七记录的`+0x630` owner已重命名为共享效果动作记录，未复制物理状态。动作405唯一caller已typed化；动作406仍保留其独立待审opaque边界。

测试覆盖动作更新、frame lookup、镜像word回绕、三份运动值、陈旧sample高半、正负声像分支中的既有路径、双绘制坐标、typed frame refresh、低字节flags门、canonical双坐标与调用点flags/寄存器、phase tick、八参数pending效果更新、非完成保留、signed负值、9999上界语义、32位累计、十一段发布、最终陈旧EDX高半、双记录清零、frame/shared/phase/坐标原访问点typed-stop、production动作405 caller及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`224/422 = 215 platform_adapted + 9 assembly_exact + 198 pending_audit`，SHA256为`ccf81d40f67ff5e28898e99fb005f7d8c072961c4f5f5c08303a5f54d80d4ddd`。动态差分因原版特殊记录、frame资源、音频、绘制、共享刷新、效果更新、目标刷新、数值计算、十一段发布和唯一caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
