# 战斗组A角色最终处理 `0x0046FFF0`

历史状态：`platform_adapted`。工作包282正在修正资料缓冲重叠；下述旧全量门与callee统计属于历史切片，不是当前发布验收。

## 1. 完整权威范围

权威LST主体为`0x0046FFF0..0x00470172`，proc至endp共175行、103条实际指令、4个call、21个跳转、11个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。callee为已关闭物品效果`0x0046F1F0`一次、已关闭profile模式选择`0x0046FF00`一次，以及待审资料缓冲加载`0x00476A80`两次。

## 2. 控制流

动作word为零时立即返回1。模式byte bit1命中时先发布完成latch，以非零替换word覆盖动作；actor flags低byte包含`0x28`时把当前动作发布到display word并把动作改为200，否则返回0。

普通路径先无条件清零`+0x2630`的四个dword。动作不为27时从live actor record `+0x26`复制word到派生首word。随后以动作word调用typed物品效果，再无条件调用typed profile模式选择。之后清零`+0x0D90`十个dword并以profile id零加载；内嵌状态bit5命中时对第二dword OR `0x100`。profile id非零时再次加载。

第二次加载存在时通常返回1；第一次加载后profile id为零时通常返回0。只有动作1、23、26且两个转场gate均为零时才发布display word并把动作改为200。第二次加载分支中actor flags缺少`0x28`且动作不是23时直接返回1；零profile分支则要求actor flags包含`0x28`才检查转场。

## 3. owner、callee与caller

最终处理状态保留完成latch、替换动作、16字节前置块、profile id和转场gate等字段。工作包282已删除其独立actor flags及40字节缓冲，资料加载直接借用传入执行状态的`profile_buffer`。`+0x0D9C`从该缓冲相对`+0x0C`读取；`0x004700CB`取WORD，其余条件测试只使用低BYTE。动作与display仍由物品效果状态持有。其他跨状态字段的整体收敛仍未完成。

待审`0x00476A80`保留为只接受缓冲token和profile id的窄port。物品效果的三个callee与profile随机callee也由窄适配器转发。

唯一上层函数`0x00456680`有两处caller，均已从旧整函数opaque调用改为typed直连；源码、头文件和测试中旧地址生产调用为零。缺失startup或任一原始owner时，在对应首次访问处停止并保留此前caller副作用。

## 4. 当前资料别名验证

以真实MON编码执行两次资料加载，覆盖flag为零、低BYTE bit3及仅高字节置位；验证动作26是否转为200由加载后的数据决定，而非旧标志副本。同时检查`+0x0D94`粒子门、`+0x0DA4`动作WORD、`+0x0DB2`坐标WORD，以及覆盖动作WORD时保留相邻资料WORD。core29/ASan19定向`1/1`通过（2.91/4.76秒），无匹配诊断、diff check通过；非本包全量放行。

## 5. 历史验证状态

单元测试覆盖零动作早退、模式替换转场、普通清零与actor record派生、双资料加载、bit5缓冲flag、零profile返回以及actor record typed-stop前缀。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`187/422 = 178 platform_adapted + 9 assembly_exact + 235 pending_audit`，SHA256为`e57b6e6a64cbc3b2353a7baa7d2f2537263a2e330aa4de26211644361f163c2d`。动态差分因原版actor、live资料记录、物品callee、随机callee、资料加载与caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
