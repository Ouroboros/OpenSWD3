# 战斗脚本页面加载 `0x0046E1E0`

状态沿用`platform_adapted`。生产文件接线及验证见
[共享文件生命周期](battle-script-file-runtime-binding.md)。
旧文档所述每次重新打开文件及普通API失败typed-stop已不再用于生产。

## 1. 权威范围与顺序

LST `46E1E0..46E25B`，cdecl单参数，完整读取无外部chunk。
入口push ECX提供NumberOfBytesRead初始栈字节；旧分配基址非零先释放。
分配1000h，依次发布当前指针和分配基址，以400h个DWORD清零。
原零分配的首次访问在rep stosd；既有固定数组适配不声称覆盖该动态故障。

按低DWORD的arg+200h绝对seek，共用53CCE0句柄。
46E233在seek返回后重读当前指针，46E238重读句柄，再读1000h。
46E252在read返回后无条件发布原arg到53CEA4，返回ReadFile EAX。
ECX在尾部恢复；读前EDX提供长度栈地址，返回EDX取决于ReadFile实际callee。
宿主文件结果不是原CPU捕获。

## 2. 唯一存储与文件

生产`load_legacy_battle_script_page_file`共用入战时打开的文件端口，
不重新打开路径，不新增负offset或seek/read返回0成功门。
只清活动1000h字节；宿主数组尾部保留但不能当作活动脚本读取。
实际长度是诊断值，页面offset仍在读取尝试后发布。

seek回调后重读cursor及活动容量，再形成ReadFile目的范围。
目的范围无法由当前拥有的窗口表示时，报告宿主destination_unavailable，
保留seek及其之前的状态，不调用read或发布末尾offset。
该状态标明模型的地址边界，不冒充原Win32 API错误或原版动态故障。

## 3. caller接线

分派器9个命名调用点覆盖LST的10处静态callsite；case19两分支共享调用点。
调用前不再提前归零cursor；归零由页面加载函数在发布新窗口时执行。
SDL将真正的ReadFile EAX原样回传；普通返回0不再转换成typed-stop。
仅宿主目的地址不可表示时停止后缀。各caller原有分支和后处理顺序保持。

旧`load_legacy_battle_script_page`仅保留作离线便利入口，不能用其严格失败
状态定义生产脚本语义。原动态地址、释放返回和CPU栈token仍未动态验收。
