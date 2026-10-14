# 战斗脚本窗口加载 `0x0046E0B0`

状态沿用`platform_adapted`。本次生产文件接线及验证见
[共享文件生命周期](battle-script-file-runtime-binding.md)。
旧文档把开关写成数据根token、把OPEN_ALWAYS写成OPEN_EXISTING，均已纠正。

## 1. 权威范围

LST `46E0B0..46E1D4`，物理208814..208956，cdecl单参数。
唯一直接caller在451E60传入battle ID低WORD零扩展值，返回值不作成功门。

4A7B5C是加载开关，静态初值1；零则直接返回1且保留所有已有状态。
53CEA8非零时复用53CCE0句柄；否则拼接figtalk.dat并CreateFileA：
GENERIC_READ、FILE_SHARE_READ、OPEN_ALWAYS、NORMAL。
发布返回句柄后仅与0比较；0返回0，FFFFFFFF继续并将opened写1。

## 2. 定位与读取

1. 绝对seek到204h。
2. 重读句柄，相对seek低DWORD的battle*4-4；失败时不可合并为绝对seek。
3. 重读句柄，读4字节到未初始化的栈Buffer，忽略返回和实际长度。
4. 取Buffer DWORD，加200h回绕；重读句柄并绝对seek。
5. 分配8000h，发布当前指针和分配基址，再清零2000h个DWORD。
6. 重读当前指针与句柄，尝试读8000h并返回1。

原函数不释放先前窗口，不关闭文件，也不因seek/read返回0退出。

## 3. 当前接线与边界

生产SDL调用`load_legacy_battle_script_window_file`，复用持久文件端口。
工作区拥有唯一cursor，assets拥有唯一窗口与文件状态，开关直接取共享状态。
SDL不再整体清空工作区与共享脚本状态；disabled及打开返回0保留旧窗口。
两种普通返回都继续FFD，只有缺少必要的原始栈输入会停止宿主执行。

头短读时，未读尾部只能取显式`offset_stack_bytes`快照。
无快照且不足4字节时报告`offset_stack_unavailable`；这表示输入缺失，
不声称原版发生内存故障，也不用零填充值伪造原数据偏移。

定长数组仍是既有宿主分配适配：发布后清零、短读保留零尾。
不把该适配冒充原动态分配地址、泄漏统计或分配失败的动态差分。
旧`load_legacy_battle_script_window`保留为离线便利入口，其严格失败检查
不再决定生产行为；文件名大小写适配保持。

未新增原版动态差分，原栈未知字节和动态分配证据仍须区分于本地夹具。
