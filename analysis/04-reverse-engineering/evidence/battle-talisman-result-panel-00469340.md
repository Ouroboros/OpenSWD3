# 战斗炼符结果面板 `0x00469340`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00469340..0x004694D5`，从proc到endp共178行、103条带机器码和真实助记符的实际指令、10个静态call、2个跳转、2个局部标签和2个返回点，没有外部`FUNCTION CHUNK`。10个callsite依次为动作记录更新、矩形效果、首层九宫格、次层九宫格、面板查询、成功标题、成功格式化、成功详情、失败标题和失败详情。

唯一静态caller位于已关闭消息阶段的消息98分支。caller先把选择cache A写为1，再调用本函数并立即返回；没有计时或其他后置副作用。

## 2. 局部缓冲、底板与寄存器链

函数建立64-byte局部缓冲：第一个byte复制全局seed，其余63个byte清零。清零完成后的动作更新入口固定为EAX零、ECX零，EDX保留函数入口值。共享动作记录写为动作`0x233B`、variant零并更新。

动作返回后把live stage加40写入ECX，矩形使用`x=196`、`y=176`、宽184、高ECX、RGB `0,4,4`和mode零。矩形返回EDX只替换低word为共享动作资源，保留高16位后绘制`(200,180)..(376,196)`首层九宫格；首层返回ECX再只替换低word为同一资源，保留高16位后，以重新读取的live stage加212绘制`(200,212)..(376,live stage + 212)`次层九宫格。

矩形或任一九宫格typed-stop保留此前动作/画面前缀并阻断查询和结果文字，不执行现代补偿。

## 3. 查询与成功/失败结果

次层九宫格正常返回后固定查询`212,252,3`。返回EAX不等于1时立即返回查询callee的完整EAX/ECX/EDX，不读取结果byte也不绘制文字。

查询精确1时，函数重新读取live结果byte并只覆盖EAX低8位。结果byte精确1进入成功路径：在`256,180`绘制CP950“煉符成功”，随后读取胜利奖励十项payload数组第0项作为名称token，以CP950 `得到符咒:%s`格式化64-byte局部缓冲，再在`208,222`绘制结果。格式call前EAX为完整名称token、ECX为局部缓冲token；详情call前EAX为framebuffer token、ECX为字体token、EDX为局部缓冲token。格式结果没有在前64 bytes内产生NUL时，保留成功标题和完整64-byte前缀，在第65个目标byte typed-stop并不绘详情。

结果byte为除1外任意值时进入失败路径：依次在`256,180`绘制CP950“煉符失敗”，在`208,222`绘制“沒有得到東西”。本函数不修改字体大小；正常路径返回最后实际callee的完整EAX/ECX/EDX。

## 4. owner、caller回收与验证

本函数不新增持久state。动作记录、stage、结果byte、framebuffer、字体、矩形/九宫格资源和首项名称token均复用既有唯一owner；64-byte文字仅为局部缓冲，物理地址使用`compat::u32` token，不转换为宿主指针。

消息98已直连本实现；面板typed-stop保留caller的cache A写和面板内部前缀，并阻断主帧后续stage。旧消息98准备槽保留枚举数值并改名为reserved，生产代码零调用。主帧适配分别映射查询、成功标题、成功格式化、成功详情、失败标题和失败详情六类服务，并保留参数、文字载荷、stage/结果byte发布及EAX/ECX/EDX回复。

定向测试覆盖64-byte初始化后的动作寄存器、ECX高度、EDX/ECX资源高字链、固定查询、查询非1、查询后live结果byte重读、成功/失败精确分支、四段CP950文字、首项完整32-bit名称token、格式/详情寄存器链、双九宫格stop、格式边界、消息98 cache先行/旧槽零调用/子stop传播及主帧六服务映射。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。

当前缺少原版真实seed、framebuffer/字体/边框资源、查询callee、名称指针/格式化、动态栈、动作/画面/文字返回及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
