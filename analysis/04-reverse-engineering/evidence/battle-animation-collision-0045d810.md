# 战斗动画沿线横向命中 `0x0045D810`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围与调用图

权威函数为`0x0045D810..0x0045D8E0`，从proc到endp完整107行、73条实际指令、1个静态call、5个分支与4个局部标签，无外部FUNCTION CHUNK。

唯一callee为已关闭的线段光栅单步推进`0x00434350`。三个直接caller也都已关闭：单体效果帧两处，群体效果帧一处。

## 2. 八参数、固定输出与局部线段

八个32位参数按顺序为两个输出地址、起点X/Y、终点X/Y、步数乘数和计数槽。三个caller的输出地址都固定指向同一对战斗共享坐标，因此typed接口直接写唯一`shared_x/shared_y`，不保留主机指针或地址token副本。

函数入口先把八个局部dword全部清零，再发布起点和终点；这正好形成`LegacyBattleLineRaster`的起点、终点、当前偏移和双误差八dword布局。终点Y参与线段推进，但完成判断只比较X。

## 3. u16计数、signed乘积与无上限循环

计数槽固定为八个u16。函数在首次真实访问时先对所选槽加一并允许u16回绕，再把零扩展计数与完整32位步数乘数做低32位`imul`：

- signed乘积小于等于0时不调用线段推进；
- signed乘积大于0时至少推进一次；
- 每次推进后先比较`current_x + start_x == end_x`；
- 未命中才递增完整u32循环索引、重新读取当前u16计数、重新做低32位乘法，并以signed `jl`决定是否继续。

原函数没有现代迭代上限，typed实现也不增加上限。唯一callee直接组合已关闭光栅函数，返回bool不参与本函数控制流。

## 4. X命中BUG、输出与尾寄存器

完成门只比较X，不检查Y。因此垂直线在第一次推进Y后，X仍相等即可立即成功；这是原始可观察行为，不修复。

无论是否进入循环、是否命中，函数都依次发布：

1. `shared_x = current_x + start_x`，低32位回绕；
2. `shared_y = current_y + start_y`，低32位回绕。

命中时随后把循环索引置为`0xFFFFFFFF`，输出完成后才清当前u16计数并返回EAX 1；未命中保留计数并返回EAX 0。普通返回的ECX保持最终Y输出，EDX保持入口start Y位形。

第九槽及更大索引只在原始`inc word_4FDF94[index]`访问点typed-stop；此前只有局部清零和参数装载，两个共享输出及全部八槽计数保持入口值。

## 5. 单一typed物理状态与caller回收

八槽计数与共享XY纳入`LegacyBattleAnimationCollisionState`，由单体和群体效果帧共用的`LegacyBattleSharedEffectFrameState`唯一继承。效果总协调器同时组合两类效果帧时仍只有一份物理计数与坐标。

单体效果帧两处和群体效果帧一处均删除旧callee token并直接组合typed实现。普通0/1返回继续驱动原有音效、完成位和后续渲染；单体第九槽typed-stop保留此前查询副作用，并阻断音效、效果绑定、完成写入与后续帧逻辑。

## 6. 测试与动态差分

定向测试覆盖第九槽停点、零乘数、u16回绕、相邻槽保留、signed负乘积、固定次数未命中、水平命中、垂直线只比较X、XY输出、完整EAX/ECX/EDX、三处caller直连、单体父级停点及单体/群体共享物理计数。

定向`1/1`、独立AddressSanitizer `1/1`、Linux core `188/188`与Linux app `194/194`通过。battle聚合源码零warning；app仅保留SDL上游缺少ALSA开发库的既有CMake环境提示。

当前缺少原版八槽计数、共享XY、三处caller输入与寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
