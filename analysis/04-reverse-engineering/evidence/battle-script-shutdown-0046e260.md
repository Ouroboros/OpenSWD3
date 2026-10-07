# 战斗脚本关闭与状态重置 `0x0046E260`

状态沿用`platform_adapted`。文件生命周期接线及验证见
[共享文件生命周期](battle-script-file-runtime-binding.md)。

## 1. 权威范围

主块46E260..46E285；外部chunk46E390..46E489属于本函数。
句柄!=FFFFFFFF才CloseHandle，包括句柄0。返回后写FFFFFFFF；
无论是否调用CloseHandle，都将opened清0，再进入重置chunk。

## 2. 重置与释放

chunk先将帧门和脚本加载开关写1，并快照分配基址53CE88。
随后按原宽度清理：

- 四个连续DWORD辅助值、value B/C、坐标及两组WORD坐标；
- packed actor的两个WORD、等待参数及等待状态低WORD；
- 两个packed值的四个WORD、四项独立WORD；
- list count、动态等待、页面offset、opened与shutdown辅助值。

frame value写FFFF。动态命令token、value A、对象token、文字offset、
frame-after-move、completion及动态对象容器均不在本函数清零集合内。

按先前快照决定是否释放：基址0时保留当前指针；非0则释放后同时清零
分配基址与当前指针。不能在状态清理之后才重新决定原分配是否存在。
释放返回值未取得原版动态证据；两个caller均不以它控制后续分支。

## 3. 生产接线及caller修正

`shutdown_script_direct`先通过同一脚本文件端口执行关闭前缀。
固定数组沿用既有宿主适配：script_capacity非0表示已有分配；
仅此时清容量、实际长度与cursor，数组字节保留但不可访问。

终止opcode保留双方清理、等待状态、全局重置、脚本关闭的次序，返回0。
case1先执行双方清理、全局重置和脚本关闭；469E5B重新读取live当前指针，
469E61加4，469E65再写回。旧实现保存入口cursor再加4与指令不符，
已改为关闭后重读；已有分配被释放时结果为4，不是旧cursor+4。
完成门置1，帧结果仍由value A传播。

普通关闭失败不阻止写入FFFFFFFF与后续重置。文件端口负责实际关闭，
不再用一次性RAII文件对象解释原持久句柄。

验证包含非零旧cursor的case1和实际文件关闭；动态分配地址、释放返回及
完整战斗返回仍未取得联合原版差分，不能据此标记整个生命周期完成。
