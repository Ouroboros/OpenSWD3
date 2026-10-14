# 战斗角色资源链查询 `0x00470910`

状态：`pending`。

历史状态：`pending`。以下历史门禁不代表当前迁移批次通过。
当前行查询及普通网格调用方已通过本批定向验证，父Port及同族剩余协议尚未验收。
源码提交f21aa0b1已推送，发布时远端main与本地一致。

## 历史资源链头发布

3eda365b删除470918的叶回包转发和提交计数，直接借实际list发布。
发布后才清flags；首节点失败保留发布和清零，occurrence零陈旧成功保持。
本批门禁见[链头发布](battle-actor-resource-list-commit-00470900.md)。
首段及以下全量门禁为历史记录；完整查询/列表父协议仍pending。

## 1. 完整权威范围

权威LST主体为`0x00470910..0x00470A05`，proc至endp共132行、86条实际指令、2个call、17个跳转、8个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。内部资源链头提交`0x00470900`已关闭。本次迁移直接写入调用方名称、flags和数量，撤销字符串副本回包。

## 2. 遍历与陈旧分支

入口先提交next资源链头到current，并把输出flag word清零。category 0、1、2、3分别映射到掩码0x10、0x0C、0x1001、0x0800，其余值原样作为掩码。

每轮先解引用current的next并把actor current破坏性推进，再筛选category、mode bits 0/2和signed派生数量。派生扫描值按`signed word0A - unsigned word06 + signed word08`计算，仅正值递增匹配计数；但计数比较在筛选块外无条件执行。因此occurrence为零时，首个不匹配节点仍进入成功分支。成功分支只重查category与mode，不重查派生正值，并无论是否复制都返回1。

成功复制按16位回绕计算输出数量，flag先置bit15。节点bit14有效时，把低14位阈值与live actor record `+0x08` signed word比较，阈值更大则再置bit14。链尽返回0并清flag。

## 3. owner与验证

资源节点字段与current/next链头扩展于第188项唯一链表owner，live容量复用startup配置owner。权威LST存在一个静态直接caller：`sub_4659C0`的`0x00465BDD`，原LST第193872行。此前“无静态直接caller”的结论有误，现已更正。

当前普通网格在每次查询前读取实时类别与角色，名称和数字使用同一40字节局部存储区。`String1=-0x28`，数字区从`-0x14`开始；名称超过19字节时允许写入数字区，随后格式化只改数字及NUL，不清旧尾。该边界依据为`465BB0/465BDD/465BFD/465C07/465CA1`。19/20/21/31/39字节名称的共享区覆盖已通过core及ASan定向验证。

本批撤销行查询request、寄存器及诊断副本、普通网格query操作编号和测试脚本回包。字体、绘制和同族其他查询协议继续留在LIST中，不勾选整条完成。

### 本批门禁与宿主边界

core与ASan的battle.legacy_battle_setup、battle.actor_frame_316各1/1通过，
SDL应用构建成功，14份源码身份一致。日志位于build/tmp/runtime/，前缀为
resource-list-display-final-，分别为setup-core、actor-core、setup-asan、
actor-asan、sdl和identities日志。未启动游戏，未取得本批原版动态差分。

独立向量覆盖四类与raw mask、完整DWORD序号、signed容量、实际输出别名、
首NUL与旧尾、逐站live角色、七行上限、512个不合格节点和真实选择帧失败传播。
短span以及无效list/node/config报告宿主访问边界错误，保留已写前缀；
这些测试不证明原版非法地址或栈越界的异常行为，不标记assembly_exact。

### 历史门禁

测试覆盖链头提交、破坏性推进、掩码/mode筛选、signed扫描派生、16位输出回绕、bit15/14、字符串复制、链尽失败及occurrence零陈旧成功。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`197/422 = 188 platform_adapted + 9 assembly_exact + 225 pending_audit`，SHA256为`c58f54c211943da1d68a40531cfd6f1749b5eac340567c397a7cdb2c13ff7191`。动态差分因原版资源链、live容量、字符串输出与调用寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
