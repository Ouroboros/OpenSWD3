# 战斗定义归档记录读取 `0x0045F1B0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整范围与ABI

权威LST完整主体为`0x0045F1B0..0x0045F29D`，从proc到endp共128行、108条实际指令、7个call、5个跳转、4个局部标签，没有外部`FUNCTION CHUNK`。

函数是四参数thiscall：ECX为第106项绑定对象token，参数依次为ANSI文件名、`0x10C`字节目标记录、完整battle ID和variant低byte；以`retn 0x10`回收参数。EBP保存`ReadFile`入口，ESI保存handle，EDI保存this，必要时EBX在signed累计循环内单独保存。正常及拒绝返回均恢复ECX this token。

静态call包括`CreateFileA`、失败关闭、第一`ReadFile`、`SetFilePointer`、第二`ReadFile`、成功关闭和两类拒绝共用的关闭站点。

## 2. 打开与固定头部重读

打开参数与第107项完全相同：只读、独占、OPEN_EXISTING、普通属性。全1handle仍调用关闭并返回EAX0、ECX this和关闭EDX。

打开成功后再次把固定`0x2714`字节读入同一绑定对象`+4`。第一读取返回和值域完全忽略，短读只覆盖实际前缀。后续所有索引均从读取后的live对象取值。

## 3. signed计数与variant门

battle ID先只保留低16位，随后读取：

```text
count = i8(*(this + 0x1F48 + battle_id_low16))
```

count按signed小于等于0时关闭并返回0。variant同样按i8解释；仅当`variant > count`的signed严格比较成立时关闭并返回0，等于count仍继续，负variant也允许继续。

battle ID超出精确`0x31F4`对象时，只在上述首个count byte真实读取typed-stop；该故障路径不执行关闭，保留打开和头部读取副作用。

## 4. signed前缀累计与偏移表

若battle ID signed大于1，ECX从1开始，依次读取`this+0x1F48+ECX`到`battle_id-1`。每个byte先按i8符号扩展，再以u32低32位累加到EDX；负byte必须产生二补数回绕。battle ID为0或1时累计和为0。

随后把variant按i8符号扩展，与累计和作u32加法，形成record index。偏移dword读取地址为：

```text
this + 8 + record_index * 4
```

负variant或负累计可以回绕到对象前后任意地址，不做现代夹值。只在该dword首次真实读取超出精确对象时typed-stop，不关闭已打开handle。

## 5. 文件偏移与固定记录读取

偏移表dword记为V，原乘法链严格等价于：

```text
file_offset = 0x2714 + V * 0x10C   (all u32 wrapping)
```

调用`SetFilePointer(handle,file_offset,0,FILE_BEGIN)`，完全忽略返回。随后固定请求`ReadFile`把`0x10C`字节写入调用方记录owner；读返回和实际长度再次完全忽略，短读保留记录未覆盖尾部。最后关闭同一handle，强制返回EAX1、恢复ECX this并保留关闭EDX。

Windows open/read/seek/close统一复用第107项文件API窄端口，不建立第二套handle状态。

## 6. caller直连与定义投影

唯一caller是战斗启动协调器。旧高层`load_definition`端口已删除；启动状态持有唯一`0x10C` raw记录owner，caller直接组合本函数，然后无条件从live raw记录按原物理offset读取：

- `+0x04` rotation divisor；
- `+0x24` secondary count低word；
- `+0x28` background action低word；
- `+0x58/+0x78`两项背景dword；
- `+0x98` enemy count低word；
- 八项role在`+0x9C+i*4`，mode在`+0xBC+i*2`，X在`+0xCC+i*4`，Y在`+0xEC+i*4`。

本函数打开失败、拒绝或typed-stop时，caller对普通0返回仍按原行为读取入口陈旧raw记录；只有typed-stop在modern故障域阻断后续启动流程。正常、打开失败与拒绝返回本身不作为caller成功门。

## 7. 验证与动态差分

定向测试覆盖固定打开参数、全1handle关闭、signed count正/零/负、variant大于/等于/负值、signed前缀正负累计、battle ID低16位、偏移dword、`V*0x10C+0x2714`、seek完整寄存器、两次读取顺序、读取返回忽略、固定`0x10C`记录、关闭寄存器、count和偏移表真实typed-stop、全部定义字段offset，以及启动caller从raw记录直接投影。

当前缺少原版Windows handle、真实归档头与记录、signed索引轨迹、SetFilePointer结果、短读/失败记录、输出全局及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
