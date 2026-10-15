# 战斗角色模式资源显示查询 `0x00470FE0`

状态：`pending`。

历史还原状态：`pending`。以下旧全量门禁属于历史还原；
完整列表、炼符网格、计数、字体及父协议的INVOKE迁移仍pending。

## 炼符列表直接读取实际资源名称与数量

源码42772057将本叶改为借实际list、完整DWORD mode/occurrence、文字span和数量WORD。
结果仅status/found；删除request、actor寄存器token、寄存器回复、输出副本和诊断计数。
4662E9/4663E9两处实际调用均不根据found分支；输出门0与正常耗尽继续旧文字/数量。
原标题004A7688的首四字节B7 D2 B2 C5按CP950解为“煉符”，后接NUL。

独立LST为214524..214610，共87行/51指令、2call/9jump/2RET，无chunk；
唯一父466190共381行，两处opcode-backed call为4662E9/4663E9。
LST SHA为701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b；
独立推导、完整父后继与双向审查记录在build/tmp/runtime/mode-resource-display-*。
mode0固定0300直达不写selected；普通序号命中先写selected，每非空节点仍比较陈旧序号。
任意非零mode只按bit27且非0300递增，完整DWORD、无界扫描和回绕保持。
BYTE mask05关闭仍成功，不读取名称/数量；首NUL及终止符复制后目标尾不清。
471053复制后重读actual current，再先取tertiary、后加secondary，只写WORD。
缺重读节点保文字/selected和旧数量；容量/访问失败复用原stop，不增加成功兜底。
不修改next、节点或存储所有权；父在primary、根发布、count和各group前借live队员。

新增305次固定执行：叶228、真实网格77。叶175为5mode×5序号×7BYTE门，
42为两路径×七数量位型×三名称，4真实head别名，4容量前缀，2缺根/候选，1缺list。
网格70为十槽×七数量，另7覆盖live队员/非法前缀、输出关闭与耗尽保持旧值、真实容量失败。
同步既有叶、重复组名、十槽、选择帧message30及原后缀；撤回脚本叶回包与计数断言。
首轮18秒编译停止因span的u8未限定，before-u8日志/SHA保留；改compat::u8后完整重跑。
最终proc_672d成功418秒：setup core1/1 11.57秒、actor core1/1 27.48秒；
setup ASan1/1 19.66秒、actor ASan1/1 30.21秒，SDL136/136重新链接。
九源码身份、完整527行生产/1151行测试差异、staged一致及双向审查通过。
源检查proc_3a7e成功，日志/manifest及差异保留；定向1/1不代表全量或实际游戏验证。
没有本批Windows、实际MON、游戏或原版联合差分；不升级assembly_exact或B10/B11状态。
其它列表/471080计数、字体、完整网格与主帧协议仍pending，不增加整族验收。

## 本批资源链头发布

3eda365b删除470FE7的叶寄存器转发和提交计数，直接发布同一实际list。
两种筛选、陈旧输出与selected写入保持；网格仍在原两次时点重新发布根。
门禁见[链头发布](battle-actor-resource-list-commit-00470900.md)。
首段及以下全量门禁为历史记录；完整查询/模式网格父协议仍pending。

## 1. 完整权威范围

权威LST主体为`0x00470FE0..0x00471070`，proc至endp共87行、51条实际指令、2个call、9个跳转、7个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。callee为`pending`的资源链头提交`0x00470900`一次，以及平台字符串复制`lstrcpyA`一次。

## 2. 两种筛选模式与陈旧计数

入口无条件提交next资源链头，之后每轮从current节点读取next并破坏性写回current。next为空返回0。

mode为零时固定比较resource id `0x0300`。命中该id直接进入输出门，不写selected token，也不检查occurrence；非固定id不增加计数，但仍比较零计数与occurrence。因此occurrence为零可在首个非固定节点陈旧成功并写selected token，正occurrence则持续扫描直到固定id或空链。

mode非零时仅当node `+0x2F` byte bit3置位且resource id不是`0x0300`时递增计数，也就是typed owner中的category mask bit27。每个非空节点之后都无条件比较计数与occurrence，因此occurrence为零仍可在首个不合格节点陈旧成功；固定id节点也可能在陈旧计数相等时被选中。普通计数命中先把节点token写入selected resource owner。

## 3. 输出门、寄存器与typed边界

候选成功后测试node `+0x46` byte mask `0x05`。mask为零时函数仍返回1，但不复制名称、不写数量，caller原有局部值保持陈旧。mask非零时复制node `+0x0C`名称，并以16位加法把secondary word `+0x08`与tertiary word `+0x0A`相加写入输出word，保留负数位型和回绕。扫描期间ECX固定为`0x0300`、EDX持有occurrence；发布数量时仅替换EDX低word。

唯一caller是`pending`的mode grid frame `0x00466190`，共有primary mode零与secondary mode一两处调用。caller的`String1`位于`-0x14..-0x01`，容量为20 bytes；typed路径在名称连终止符无法容纳时于原始复制边界停止，不静默截断。资源链与selected由实际list持有，源名称/数量来自节点；文字和数量目标借用caller原局部。

## 4. caller回收与验证

mode grid production现在从startup party span定位角色owner，primary与secondary两条查询直接调用typed实现；调用前后的两次`pending`的资源链头提交也改为typed直连。`0x00471080` secondary count已在第203项接入实际累计器，脚本化mode grid兼容仅由单测显式开启，production默认关闭。

测试覆盖mode零固定id直达且不改selected、mode一bit27计数并排除固定id、occurrence零陈旧选择、输出mask零保持陈旧值、16位数量回绕、缺项破坏性扫到空链、20-byte字符串边界typed-stop，以及mode grid production primary/group重复显示与两次typed链头提交。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`0/422 = 0 platform_adapted + 0 assembly_exact + 422 pending`，SHA256为`9c4944438acbe1b483f72b9df3a9383903af867abb8d37f9371d4b94c02604a5`。动态差分因原版mode资源链、selected token、caller字符串/数量局部和寄存器联合捕获后端缺失而登记为`pending`。
