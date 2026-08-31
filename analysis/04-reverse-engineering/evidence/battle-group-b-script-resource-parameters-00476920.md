# 战斗组B脚本资源参数写入 `0x00476920`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威主体为`0x00476920..0x00476993`，从`proc`到`endp`共45行、31条实际指令、0个call、0个跳转、0个局部标签和1个返回点。没有外部`FUNCTION CHUNK`，也没有直接callee。

唯一静态caller是已关闭的战斗脚本分派`0x00469D20`，调用站点位于脚本case 74。完整caller主体已重新提取，并以该调用站点前后的真实算式和epilogue回收旧opaque边界。

## 2. ABI与唯一owner

函数使用thiscall形态：ECX为组Bactor token，栈上唯一参数为18-byte脚本载荷的首地址，`retn 4`由callee清理该参数。

actor `+0x0C`是动态资源token。函数把脚本载荷相对偏移`+0/+2/+4/+6/+8/+0x0A/+0x0C/+0x0E/+0x10`的九个byte，依次写到资源`+0x92..+0x9A`九个连续byte。实现直接复用`LegacyBattleActorGroupBElementState::resource_token`和既有164-byte `resource_bytes`唯一owner，不新增平行资源记录或脚本参数副本。

## 3. 精确访问与写入顺序

入口先把脚本载荷token写入EAX。前八项逐项执行：

1. 从actor `+0x0C`重读资源token到EDX；
2. 从脚本载荷当前偶数偏移读取一个byte到BL；
3. 把BL写入当前连续资源参数byte。

第九项改为：

1. 从actor `+0x0C`重读资源token到ECX；
2. 从脚本载荷`+0x10`读取byte到DL；
3. 恢复入口EBX；
4. 把DL写入资源`+0x9A`。

资源token在每项脚本读取前重新取得。任一后续脚本读取或资源写入故障都不得回滚此前已经写入的参数。奇数载荷byte从不读取，也不得因为奇数byte位于现代脚本容量外而提前停止。

## 4. 返回寄存器

正常返回：

- EAX保持脚本载荷token；
- ECX为第九次重读的资源token；
- EDX高24位来自第八次资源token重读，低8位为最后一个脚本参数byte；
- EBX由原始push/pop完整保留。

前八次脚本读取停止时，ECX仍是actor token，EDX是该项刚重读的资源token。第九次脚本读取停止时，ECX已经是资源token，但DL尚未覆盖，EDX仍保留第八次重读的完整资源token。

## 5. typed故障点与部分提交

- actor缺失：停止在首次actor `+0x0C`读取点；EAX已发布脚本载荷token，ECX为caller算出的actor token，EDX保留caller地址算式留下的值，脚本尚未读取；
- 脚本偶数byte缺失：停止在该byte真实读取点；当前资源token已重读，此前资源写入保留；
- 资源token无效：先读取当前脚本byte，再停止在当前资源写入点；不得把故障提前到资源token读取；
- 固定164-byte资源owner覆盖`+0x92..+0x9A`，不增加原版不存在的中间容量门。

现代脚本数组及其`script_capacity`只在原始脚本byte读取点提供typed边界。组B八槽物理owner之外的actor索引在首次actor资源访问处typed-stop。

## 6. caller回收

脚本case 74先读取`script+2`的u16 actor并写入共享packed actor状态高word。caller按原算式形成组Bactor token，同时把callee入口EDX形成`345 * actor`，再以`script+4`作为脚本载荷token直接调用typed函数。

callee正常返回后，caller才把cursor按u32回绕前进22，返回EAX一，并由原epilogue恢复入口ECX；EDX保留callee结果。任一typed-stop都保留packed actor和已完成资源写入，阻断cursor、返回一及epilogue后缀。

旧`0x00476920`整函数opaque调用在production源码中为零；枚举数值只保留为reserved稳定槽，port不再负责九项参数的发布。

## 7. 验证与动态差分

纯函数测试覆盖actor首次读取停止、九个偶数脚本byte各自缺失时的全部部分写入边界、资源写入前已消费脚本byte、奇数byte忽略、九项完整写入及EAX/ECX/EDX返回。

caller集成测试覆盖完整case 74、脚本截断后前三项保留、缺失actor、缺失资源、packed actor前缀、cursor后缀、caller EDX算式、成功ECX恢复和旧opaque零调用。

战斗聚合定向测试、完整core AddressSanitizer `188/188`、Linux core `188/188`和Linux app `194/194`全部通过；最终日志零OpenSWD3源码warning、测试失败和sanitizer finding。

当前缺少原版组Bactor完整对象、动态资源token、资源`+0x92..+0x9A`九个byte、原始脚本载荷地址和唯一caller寄存器联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
