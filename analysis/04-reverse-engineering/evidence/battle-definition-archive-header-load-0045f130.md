# 战斗定义归档头读取 `0x0045F130`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整范围与ABI

权威LST完整主体为`0x0045F130..0x0045F1A2`，从proc到endp共62行、52条实际指令、4个call、1个跳转、1个局部标签，没有外部`FUNCTION CHUNK`。

函数是双参数thiscall：ECX为第106项已关闭绑定对象token，参数1为ANSI文件名，参数2为输出token地址，并以`retn 8`回收参数。EBX不使用，ESI保存文件handle，EDI保存this token；ESI和EDI在返回前恢复。入口`push ecx`分配的栈槽随即作为`NumberOfBytesRead`使用；`0045F14C`先将它清零，`ReadFile`再写入实际字节数。最终`0045F16C/0045F1A1`的`pop ecx`读取这个局部值，不恢复this token。

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

返回handle完整等于全1时走失败路径。原函数仍把全1handle传给`CloseHandle`，忽略关闭结果，然后返回EAX 0、ECX 0和失败关闭callee的EDX；不读取文件、不写绑定对象、不发布输出token。typed实现不得把无效handle关闭“优化”掉。

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

随后以同一handle调用`CloseHandle`。关闭入口EAX仍为输出地址token，ECX/EDX沿用`ReadFile`完整返回。函数忽略关闭EAX，把最终EAX强制写1，再从局部已读字节数恢复ECX；最终EDX保持成功关闭callee返回。

因此打开成功就是逻辑成功，不受读或关返回影响。正常返回EAX 1、ECX实际读取字节数和最后关闭EDX。ECX既不是绑定对象token，也不是读或关API返回的ECX。

## 5. caller回收

唯一caller是已关闭战斗启动协调器`0x00451B10`。旧`LegacyBattleDefinitionLoadPort::open_archive`高层伪边界已删除；caller直接传入启动状态中的唯一绑定对象、唯一输出token、真实归档路径和文件API窄端口。

无论本函数返回0还是1，caller都按原顺序继续调用后续`audit_order=108`定义记录读取，不把打开结果当成功门。测试锁定打开失败、无ReadFile、无输出发布但下一定义读取仍执行的行为。

## 6. SDL接线时的返回合同复核

2026-10-06重新完整读取上述LST，发现旧C++和本证据均把局部栈槽误作入口ECX的保存槽。现按清零、读取覆盖及尾部pop的真实顺序修正：打开失败返回ECX 0；打开成功返回实际读取计数，保留ReadFile返回0仍继续发布索引和关闭的行为。新增计数0、1、0x2714，以及原有三字节短读测试；API返回寄存器与对象地址采用不同值，避免误绑定仍通过。`battle-archive-return-register-{core,asan,sdl}.log`确认本次战斗聚合目标core/ASan各1/1通过、SDL链接通过，三个日志无warning/error，`git diff --check`通过。该结果不证明尚未接通的SDL完整初始化或实机战斗。

## 7. 实际文件端口（SDL初始化接线中）

已增加`LegacyBattleDefinitionArchiveFileRuntime`，把上述固定open/read/seek/close请求接到`resource_io::LegacyFile`。每个成功返回的端口句柄都对应一个独占持有的真实文件对象；不是截断宿主指针，也不是无资源的占位值。关闭成功后释放对应对象；未打开、已关闭和全1句柄返回失败。多个活动文件分别持有游标，关闭一个不会释放另一个。核心仍会在打开失败后调用关闭端口。

本端口限定两项既有loader产生的固定参数域；超出该域的打开标志、异步读取参数、超出目标span的请求或非FILE_BEGIN寻址作为接线错误抛出异常，不伪装为业务成功。读取保留宿主实际计数和已写前缀；不补齐未读字节。原文件包装器在Windows ReadFile失败时会把计数强制清零，故增加显式`preserve_api_count`读取方式，仅此直接API端口采用它；旧调用默认行为不变。寻址先按i32解释低32位，再把既有文件包装器的一基返回减1，恢复API的零基位置及全1失败值。seek失败不重置游标，EOF不改写目标。

现有`LegacyFile::open`默认对应游戏文件包装器，在Windows会额外改写属性、增加顺序读取标志并执行一次归零seek。LST的这两个直接API调用均没有这些操作。因此新增显式`direct_api`打开方式，省略上述三个步骤；已有调用的默认行为不变。

平台边界仍使用宿主路径和文件API：Windows路径由既有Unicode后端处理；Linux沿用既有后端，不能将其共享模式等同于Windows强制独占。端口ECX/EDX仅透传入口诊断快照，不冒称捕获了原版Win32易失寄存器。两个loader的正常业务出口不依赖这些宿主易失值；故障点寄存器和原版API差分仍需原版证据。

新增仓库内临时文件测试覆盖头部和记录短读、缺文件、负绝对寻址失败后的游标、超EOF、关闭后访问及两个文件的独立生命周期。原始`battle.ffd`测试按物理文件范围独立读取头部和第一条战斗记录，逐字节比较。另加Windows只读属性保留用例；本轮Linux验证不会执行该分支。首批`battle-archive-filesystem-{core,file-core,asan,file-asan,sdl}.log`确认战斗与文件目标在core/ASan分别1/1通过，SDL链接通过；仅出现既有存档恢复两处警告及结果测试窄化警告。Ninja定义确认真实资产对照启用、临时文件根在仓库内。随后增加真实负偏移寻址失败后继续读取，以及记录EOF不覆盖旧字节的跨层测试，`battle-archive-filesystem-prefix-{core,asan}.log`各1/1通过，无warning/error。最后新增保留API失败计数方式后，`battle-archive-api-count-{core,file-core,asan,file-asan,sdl}.log`确认战斗与文件目标在core/ASan分别1/1通过、SDL链接通过；只有同样的既有警告，`git diff --check`通过。Windows专属分支尚未运行。该端口尚未接到SDL完整启动入口，不能作为完整初始化已完成的证据。

## 8. 验证与动态差分

定向测试覆盖全部固定打开参数、文件名/对象/输出token、打开失败仍关闭全1handle、失败输出保持、短读前缀、ReadFile零返回忽略、未读头部字节保持、保留区和索引记录不改、成功输出token、三次API寄存器桥接、成功/失败完整返回，以及启动caller成功与失败两条直连路径。

当前缺少原版Windows handle、真实短读/失败轨迹、绑定对象完整头部、输出全局及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
