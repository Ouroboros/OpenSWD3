# 战斗脚本窗口加载 `0x0046E0B0`

状态：`platform_adapted`。完整LST、typed文件适配、唯一启动caller、验证和inventory双生成均已收敛。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046E0B0..0x0046E1D4`，从proc到endp共148行、91条实际指令、9个call、4个跳转、3个返回点，没有外部`FUNCTION CHUNK`。函数是cdecl单参数，caller传入完整参数低16位零扩展后的battle ID。

入口先检查数据根token；为零时直接返回1。非零时检查FIGTALK文件已打开门：未打开才拼接数据根与`figtalk.dat`并以只读、共享读、open-existing打开，句柄写入共享文件owner；已打开则直接复用旧句柄。Win32 `CreateFileA`结果只与零比较，原版没有把`INVALID_HANDLE_VALUE`当失败，本实现按平台文件合同把打开失败报告为typed状态，不伪造无效宿主句柄。

## 2. 表项、数据偏移和脚本窗口

文件定位顺序固定：

1. 从文件头绝对移动`0x204`。
2. 再按当前位置相对移动`battle_id*4-4`，合并后表项地址为`0x200+battle_id*4`。
3. 读取4-byte little-endian数据偏移。
4. 绝对移动到`0x200+数据偏移`。
5. 分配并清零固定`0x8000` bytes，把同一地址发布为脚本基址与当前cursor。
6. 从数据位置尝试读取固定`0x8000` bytes，正常尾返回1。

原程序不检查两次seek、两次ReadFile或分配结果；首次真实故障分别落在后续文件操作、`rep stosd`或脚本访问。平台适配以`LegacyFile`显式报告open/seek/read失败，以`LegacyBattleAssets::script`定长数组消除宿主空分配，同时保持先全零再固定窗口读取；短文件保留实际前缀和零尾，`figtalk_actual_size`记录实际读取长度。

## 3. typed owner与caller回收

`LegacyBattleAssets::script`是唯一0x8000-byte物理脚本owner；`LegacyBattleScriptWorkspace::cursor`只保存窗口内offset，不复制脚本内容。新增公开`load_legacy_battle_script_window`直接承接本函数，物理地址仅保留为旧证据，不作为生产调用边界。

原版唯一caller在战斗启动中先传battle ID低16位调用本函数，再加载`battle.ffd`。现代生产入口`SdlSmokeIdlePorts::initialize_battle()`先调用`load_legacy_battle_assets`；该总加载器现直接组合`load_legacy_battle_script_window`，成功后才继续FFD和战斗setup。因此FIGTALK→FFD→setup顺序保持，旧函数地址和独立opaque端口均不存在。

持久Win32文件句柄适配为每次加载的RAII文件对象；每场战斗重新打开同一文件，不跨战斗泄漏宿主句柄。大小写不敏感文件名解析保留Windows数据目录行为。

## 4. 验证状态

独立资产测试构造`0x200+battle_id*4`表项和短脚本前缀，直接调用typed窗口加载，验证偏移、6-byte实际长度、脚本内容、预先清零和窗口末尾零值；既有总加载测试继续验证FIGTALK先于FFD、错误顺序和大小写不敏感路径，真实battle 98验证实际数据偏移与首opcode。定向资产/setup测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码零warning。

inventory生成器连续双跑逐字节一致，正式计数为`164/422 = 155 platform_adapted + 9 assembly_exact + 258 pending_audit`，SHA256为`b942b4ad9007de131e998f1dde7ccba88b8a987b47cc1379f478cde573a56605`。原版持久Win32句柄、无效句柄零比较、未检查seek/read及动态分配地址缺少联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。
