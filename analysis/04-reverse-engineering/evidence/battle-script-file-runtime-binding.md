# FIGTALK共享文件生命周期

本批修正入战加载、4KB换页、脚本关闭的文件合同及直接调用方。
完整战斗帧、实际续玩和原动态内存分配仍未验收。

## 原始证据

完整读取46E0B0..46E1D4、46E1E0..46E25B、46E260..46E285及
46E390..46E489外块。caller451E60忽略加载返回；case1的469E56..469E65
在关闭后重新读取当前指针，再加4。终止opcode也调用同一关闭入口。

独立推导和向量见`build/tmp/runtime/battle-script-window-audit.md`。
状态引用摘录共240行；hFile的34项中24项是其他函数局部变量，
实际共享句柄只有10项直接指令引用。加载开关4A7B5C静态值为1，
不是数据根地址。53CEA8有4项直接引用，分别是读取、置1与两次清0。

具体指令说明：

- [初始窗口](battle-script-window-load-0046e0b0.md)
- [换页](battle-script-page-load-0046e1e0.md)
- [关闭及重置](battle-script-shutdown-0046e260.md)

## 生产存储和顺序

SDL持有一个文件运行时，入战、换页和关闭共用。
assets只存一份句柄与opened状态、窗口及容量；工作区保留唯一cursor。
删除入战时无指令依据的工作区与共享脚本状态整体清空，加载开关初值改1。
开关为零和打开返回零均保留旧窗口；普通返回零仍继续FFD初始化。

入战保留绝对204h、相对battle*4-4、读偏移、绝对offset+200h的原顺序。
各文件调用前重读共享句柄，seek/read返回零不形成新的成功门。
OPEN_ALWAYS、共享读与大小写文件名适配由实际LegacyFile后端承接。

换页不重开路径，先发布和清零活动页，seek返回后重读当前指针与句柄。
读取结束后再写页offset，返回真实ReadFile EAX；普通读取失败仍返回caller。
关闭仅跳过FFFFFFFF句柄，关闭返回后写FFFFFFFF，再清opened与原状态集合。
case1不再用关闭前缓存的cursor，已有窗口释放后从0加4。

## 明确的宿主边界

窗口继续使用既有固定数组分配适配，不新增原堆地址或假分配成功证据。
原4字节偏移栈Buffer未初始化。已知快照可重现短读后的混合值；缺少快照
且读取不足4字节时报告offset_stack_unavailable，不把未知后缀补成零。
此状态是原输入缺失，不是对原API返回或原故障行为的断言。

换页seek后的当前目的范围不可由现有窗口表示时，报告
 destination_unavailable，保留已发生的清零和seek，不写末尾页offset。
这也是宿主地址模型边界；未取得相应原版动态故障证据。

离线便利加载器仍保留旧严格检查，不参与SDL生产文件生命周期。
文件端口的结果不冒充原CPU寄存器捕获。

## 验证与审查

- 开关零、open零、FFFFFFFF、失败seek/read、32位乘法与偏移加法回绕。
- 短偏移读取配合显式栈尾、缺少栈尾停止；逐次句柄重读与发布后清零。
- 换页仅清4KB、失败read仍发布offset、seek回调改变当前目的地址。
- 真实混合大小写文件、短读、重入复用原句柄、换页不重开路径、实际关闭、
  OPEN_ALWAYS创建，以及真实battle98逐字节一致。
- case1从非零旧cursor进入，验证关闭真实文件并从释放后的当前指针加4。
- 最终core/ASan setup各1/1通过，5.12秒/7.64秒，SDL构建通过，日志无warning。
  日志前缀`build/tmp/runtime/battle-script-file-final-`。

完整正反向审查发现并修正了seek后目的地址重读和case1旧游标缓存差异。
未运行游戏、未新增原版动态差分；B10仍315/422，316 pending_audit。
