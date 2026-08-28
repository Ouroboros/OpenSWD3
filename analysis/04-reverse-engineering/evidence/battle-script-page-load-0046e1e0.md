# 战斗脚本页面加载 `0x0046E1E0`

状态：`platform_adapted`。完整LST、typed页面加载、十处脚本caller、SDL文件服务、验证和inventory双生成均已收敛。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046E1E0..0x0046E25B`，从proc到endp共69行、40条实际指令、4个call、1个跳转、1个返回点，没有外部`FUNCTION CHUNK`。函数是cdecl单参数，参数是i32页面数据offset。

函数先保存入口ECX作为4-byte局部`NumberOfBytesRead`初值。旧脚本base token非零时先释放；随后无条件分配`0x1000` bytes，把同一token依次发布为当前cursor和base，再以1024个dword清零。零分配未提前检查，首次`rep stosd`是原故障点。

之后以低32位回绕计算`offset+0x200`，从FIGTALK文件头绝对seek，固定尝试读取`0x1000` bytes，最后无条件发布原offset。返回EAX是`ReadFile`的完整BOOL；入口ECX在函数尾恢复，EDX保留指向局部读取长度的动态栈token。

## 2. typed页面owner与平台适配

`LegacyBattleAssets::script`继续是唯一物理脚本owner。新增`script_capacity`区分初始`0x8000`窗口和页面切换后的`0x1000`活动域；dispatcher每次脚本读取同时检查活动容量和宿主数组容量。页面切换只清前`0x1000` bytes，宿主数组尾部不再可访问，也不作为第二脚本状态。

`load_legacy_battle_script_page`使用初始窗口加载保存的FIGTALK路径，以RAII文件重新打开，按`offset+0x200`定位，先清活动页再固定读取。短文件保留读取前缀与零尾；page offset在读取尝试后发布。原版动态释放/分配适配为同一定长数组复用，持久句柄适配为作用域文件；open/seek/read失败显式返回状态。

## 3. 十处caller回收

权威LST在战斗脚本分派中有10个静态callsite。typed源码有9个命名`script_page_load`调用位置；case19的单项与随机选择两条LST分支在完成选择后汇入同一个typed调用点，因此动态行为和副作用顺序不丢失。

每个caller先完成原分支的列表扫描、查询、计数或cursor定位，再调用页面服务。调用前把workspace cursor发布为新页起点0；服务成功后继续各自原有清理、递减、帧或返回路径，失败则在该服务边界typed-stop并阻断后续caller副作用。旧`prepare_script`地址槽保持原枚举数值但改为业务命名，不平移后续ABI值。

SDL `invoke_battle_script`对该命名服务直接调用`load_legacy_battle_script_page`，把ready映射为EAX 1，失败映射为EAX 0和typed-stop；生产不再把旧函数地址当opaque no-op。

## 4. 验证状态

资产测试构造独立页面源，验证`offset+0x200`、3-byte短读、`0x1000`活动容量、页内零尾、宿主尾部未清写，以及open失败前已经清空活动页。脚本分派测试验证case48成功归零cursor并调用一次页面服务、查询失败走8-byte路径不调用、页面服务失败在cursor发布后停止；case61与65成功分支同步改为新页cursor，并验证活动页首个越界字节立即typed-stop。定向资产/setup测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning。

inventory生成器连续双跑逐字节一致，正式计数为`165/422 = 156 platform_adapted + 9 assembly_exact + 257 pending_audit`，SHA256为`741d16a1a4336e12dd9c31424f3868563beb2d60dce4afdf3cdb4f03df08517c`。原版释放/分配动态地址、持久文件句柄、未检查文件调用和局部读取长度栈token缺少联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
