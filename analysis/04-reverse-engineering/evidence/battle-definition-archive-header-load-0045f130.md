# 战斗定义归档头读取 `0x0045F130`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整范围与ABI

权威LST完整主体为`0x0045F130..0x0045F1A2`，从proc到endp共62行、52条实际指令、4个call、1个跳转、1个局部标签，没有外部`FUNCTION CHUNK`。

函数是双参数thiscall：ECX为第106项已关闭绑定对象token，参数1为ANSI文件名，参数2为输出token地址，并以`retn 8`回收参数。EBX不使用，ESI保存文件handle，EDI保存this token；ESI和EDI在返回前恢复。最终ECX由入口首条`push ecx`恢复为this token。

四个call依次属于`CreateFileA`、失败或成功路径`CloseHandle`、成功路径`ReadFile`及最终`CloseHandle`；继续以三项窄平台端口保留文件系统差异和调用后EDX。

## 2. 打开参数与失败路径

`CreateFileA`参数固定为：

```text
desired access      = 0x80000000 (GENERIC_READ)
share mode          = 0
security attributes = 0
creation disposition= 3 (OPEN_EXISTING)
flags/attributes    = 0x80 (FILE_ATTRIBUTE_NORMAL)
template handle     = 0
```

文件名来自启动caller构造的`data_root / "battle.ffd"`，旧文件名缓冲token为`0x004AAED0`。

返回handle完整等于全1时走失败路径。原函数仍把全1handle传给`CloseHandle`，忽略关闭结果，然后返回EAX 0、ECX this token和失败关闭callee的EDX；不读取文件、不写绑定对象、不发布输出token。typed实现不得把无效handle关闭“优化”掉。

## 3. 固定头部读取

非全1handle固定调用一次`ReadFile`：

- handle为打开返回值；
- 目标token为`this+4`；
- 目标typed span就是第106项精确绑定对象的`battle_header_bytes`；
- 请求长度固定`0x2714`；
- overlapped token为0；
- number-of-bytes-read指向caller栈局部。

原函数完全不检查`ReadFile` EAX，也不检查实际读取字节数。短读只覆盖已写前缀，剩余头部保持入口字节；读callee返回0仍继续成功尾。窄端口获得唯一`0x2714` typed span，不能写到其后的保留区和30条索引记录。

## 4. 输出token、关闭与返回

读取调用后，函数把输出参数地址载入EAX，再把`this+0x1F48`写入该地址。固定对象下发布值为`0x00501500`，对应刚读取头部内的索引区token；输出owner是启动状态唯一`archive_header_index_token`。

随后以同一handle调用`CloseHandle`。关闭入口EAX仍为输出地址token，ECX/EDX沿用`ReadFile`完整返回。函数忽略关闭EAX，把最终EAX强制写1，再恢复ECX this token；最终EDX保持成功关闭callee返回。

因此打开成功就是逻辑成功，不受读或关返回影响。正常返回EAX 1、ECX绑定对象token和最后关闭EDX。

## 5. caller回收

唯一caller是已关闭战斗启动协调器`0x00451B10`。旧`LegacyBattleDefinitionLoadPort::open_archive`高层伪边界已删除；caller直接传入启动状态中的唯一绑定对象、唯一输出token、真实归档路径和文件API窄端口。

无论本函数返回0还是1，caller都按原顺序继续调用后续`audit_order=108`定义记录读取，不把打开结果当成功门。测试锁定打开失败、无ReadFile、无输出发布但下一定义读取仍执行的行为。

## 6. 验证与动态差分

定向测试覆盖全部固定打开参数、文件名/对象/输出token、打开失败仍关闭全1handle、失败输出保持、短读前缀、ReadFile零返回忽略、未读头部字节保持、保留区和索引记录不改、成功输出token、三次API寄存器桥接、成功/失败完整返回，以及启动caller成功与失败两条直连路径。

当前缺少原版Windows handle、真实短读/失败轨迹、绑定对象完整头部、输出全局及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
