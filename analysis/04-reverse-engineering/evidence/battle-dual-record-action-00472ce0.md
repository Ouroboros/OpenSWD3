# 战斗双记录动作逐帧演出 `0x00472CE0`

状态：`platform_adapted`。完整LST、typed实现、六处caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x00472CE0..0x00473007`，proc至endp共348行、226条实际指令、10个call、15个跳转、10个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。六个真实caller全部位于动作dispatch，分别对应动作28、29、32、34、35、36；动作29把选中group-B对象同时作为this与坐标对象，其余五项使用当前group-A行动者。次记录动作ID分别为动作28/32的0x1965、动作29的0x1791及动作34–36的0x17BA。

第一阶段把行动者profile word写入主动作记录，置完成latch并固定base variant为45。动作更新返回零时立即返回零；随后按frame索引取得图像并发布共享frame token。镜像门置一时翻转flags bit0，以frame宽度减主记录x偏移，并仅在源x word非零时执行相同镜像。共享高度分别发布无符号三分之一和四分之一，三项motion按行动者模式选择-1或-6。

第一阶段以主记录sample word调用播放与左右声像；声像参数保留call返回后ECX或EDX的陈旧高半，只覆盖低word。随后清零sample word，按镜像后flags绘制`positionX-xOffset, positionY-height/3`，再按原flags绘制`positionX-xOffset, positionY-recordY`。主记录field5A的bit0或bit3均未置位时返回零，不进入第二阶段。

第二阶段把caller提供的动作ID写入行动者`+0x630`记录，base variant置零，执行第二次动作更新与frame查询。它再次发布共享frame token、镜像flags与x偏移，再以caller对象调用坐标双输出callee。最终绘制坐标严格先按word回绕执行`outputX-xOffset`和`outputY-recordY`，再分别符号扩展。仅当次记录完成dword等于一时，才按原顺序清零次记录与主记录并返回一；完成latch保持一，不被现代化清零。

实现复用主动作记录、`+0x630`次记录、共享frame/高度/motion owner与现有frame/action provider。新增group-B八槽唯一行动者及phase owner，以`unique_ptr`按动作29首次使用物化，避免复制物理状态和大型owner压栈；group-A caller继续复用既有owner。两个未审动作更新/坐标callee与音频、绘制边界分别通过既有typed provider或窄port保留，旧整个`0x00472CE0` opaque调用已从六处caller删除。

测试覆盖双动作更新、双frame、主阶段两次绘制、次阶段坐标绘制、镜像offset、陈旧sample高半、三项motion、主field5A门、次field8C完成门、双记录清零、完成latch保留、第一frame原故障点typed-stop、五个group-A production caller、动作29 group-B唯一owner及旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`222/422 = 213 platform_adapted + 9 assembly_exact + 200 pending_audit`，SHA256为`e7f9e3dc29279a64194d625122b691b8a0d09e85734515952c56f4bfa10244cd`。动态差分因原版双方行动者双记录、frame对象、坐标、音频、绘制及六处caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
