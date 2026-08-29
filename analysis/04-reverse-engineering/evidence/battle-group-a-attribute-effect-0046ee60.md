# 战斗组A角色三通道属性效果触发 `0x0046EE60`

状态：`platform_adapted`。完整LST、typed三通道效果、组A帧唯一caller、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046EE60..0x0046F02E`，从proc到endp共194行，其中188个非标签物理行、126条实际指令、15个call、7个跳转、6个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。函数是无栈参数thiscall，以普通`retn`返回。

入口读取角色`+4`源记录token并检查源byte `+0x25`的bit7。该bit置位时立即返回，不读取三项累计值、不修改临时word、不调用任何callee；EAX和EDX保持入口，ECX返回源记录token。

## 2. 三通道百分比计算

三个通道依次使用角色累计u16 `+0x2F1E,+0x2F20,+0x2F22`，源记录i16系数`+0x0A,+0x0C,+0x0E`，以及临时u16 `+0x29A6,+0x29A8,+0x29AA`。累计值为零时该通道完全跳过且旧临时word保持。

活动通道执行signed `系数 * 无符号累计值`，再以magic常量`0x51EB851F`实现向零除100；商只保存低16位。低16位为零时强制写1。通道0随后对该u16取二补数并回写临时word，通道1和2保持原低16位。传给幅值callee时三通道都按i16符号扩展，不能按u16扩大。

## 3. 五callee固定序列

每个活动通道严格执行五个callee：

1. 效果参数发布`sub_47F150`；
2. 固定资源选择`sub_4787D0`；
3. signed幅值应用`sub_47D640`；
4. 六步偏移选择`sub_47CF00`；
5. 固定参数1收尾`sub_47CEC0`。

三个固定资源依次为`0x246F,0x2367,0x2366`。通道0向第一callee传`(-magnitude,0,0)`；通道1传`(0,stale,0)`；通道2传`(0,0,stale)`。后两项`stale`的高16位来自除法前完整乘积，低16位才由临时word覆盖，不能改成普通符号扩展。

偏移参数等于此前已完成通道数乘6：通道0若活动，完成后把偏移设6；通道1使用当前偏移并在完成后再加6；通道2只使用当前偏移。每个活动通道完成五callee后才清自己的临时word。

## 4. 返回寄存器与typed-stop

三项累计均为零时，EAX保留入口高16位并把低16位依次写成最终零，ECX返回源记录token，EDX保持入口。最后一个活动通道是通道2时，正常尾完整透传其收尾callee的EAX/ECX/EDX。若最后活动通道更早，后续零累计的`mov ax`会把最后callee EAX低16位改零，但保留其高16位；ECX/EDX继续保持该callee返回。

缺少角色效果owner时在读取源记录前停止。角色`+4`为零或不能解析到配置源/角色基础记录时，在首次源byte `+0x25`访问处停止；三项临时word和所有callee均保持未触碰。除此之外函数没有现代化空值或范围分支。

## 5. shared owner与caller回收

唯一caller位于组A逐帧主循环：角色行动进度返回1后、读取两项AI标记前调用。本实现复用`LegacyBattleStartupState::party[index]`唯一角色owner；三项累计值直接借用上一工作包写入的workspace尾字段，三项临时word新增为同一角色的效果状态。角色`+4`源记录按live token在四份startup配置源与召唤/护援角色自身基础记录间解析，不复制第二份源记录。

旧固定地址opaque调用已删除；五个真实callee由组A帧adapter逐项转发，caller结果合并实际端口计数。子typed-stop保留进度函数已发布的action完成、cache清理和update-ready前缀，并阻断AI选择及全部后续帧尾。

## 6. 验证状态

纯函数测试覆盖bit7早退、三项零累计、正负系数、低16位截断、零结果强制1、通道0取负、后两通道陈旧乘积高word、三种资源、0/6/12偏移、15-call顺序、活动/非活动临时word生命周期、两类typed-stop和最终寄存器。组A帧回归覆盖共享startup owner、动态源记录解析、五callee参数、旧地址零调用和缺失startup owner停止。

验证结果：定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning，app仅有既有ALSA提示。inventory连续双生成逐字节一致，稳定为`178/422 = 169 platform_adapted + 9 assembly_exact + 244 pending_audit`，SHA256为`13a1a14a300298c4f9f06163ab02ad271eb544ea92b71d33987faaf521bdfc6f`。原版组A角色对象、三项live累计、动态源记录、五个callee副作用与caller寄存器联合捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。
