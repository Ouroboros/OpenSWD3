# 战斗角色链表筛选读取 `0x00470180`

状态：`platform_adapted`。完整LST、typed实现、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00470180..0x004702D4`，proc至endp共191行、111条实际指令、3个call、25个跳转、16个局部标签、2个返回点，没有外部`FUNCTION CHUNK`。callee为已关闭索引提交`0x0046FFE0`一次、待审资料加载`0x00476A80`一次和平台字符串复制一次。

## 2. 选择器与扫描

首选择器映射为mask：0到`0x10`、1到`0x0C`、2到`0x1001`，其他保持原值。次选择器映射为type：0到28、1到31，其他保持原值。函数先调用typed索引提交，再从list owner首节点开始按next token顺序扫描。

节点必须同时满足category flags与mask非零、mode byte与`0x05`非零。type 28分支接受节点type 27至30；其他所有type选择器都只接受节点type 31，这是原跳转结构的非对称行为。匹配计数先递增再和occurrence比较，因此occurrence零永不命中。链尾失败写输出word `0xFFFF`并返回EAX低word `0xFFFF`。

## 3. 命中处理

type不是28或31时输出word写零，跳过资料加载与字符串复制，但仍以陈旧局部profile index查询返回表。type 28或31时调用窄资料加载并复制节点字符串。节点value word按顺序覆盖输出：bit15清除后发布、bit14转成负标记、bit11强制1000；type 31最后强制1。返回值为静态word表按loader发布或陈旧profile index索引所得。

## 4. owner、stop与caller

actor current/next index继续复用第184与186项唯一owner。新增链表owner只持有owner token、head token和有序节点；缺失owner、节点或返回表索引时在对应首次访问处typed-stop，并保留此前索引提交和扫描副作用。待审资料加载保留窄profile-id port；字符串复制使用typed字符串owner。

两个静态caller位于已关闭列表内容和窄网格函数。当前两caller在既有typed边界中使用无地址的语义化行查询port，但尚未暴露链表节点物化owner；本包不复制第二份节点状态，也不把整函数地址重新引入生产代码。节点物化接入后应直接复用本typed核心。源码、头文件与测试中不存在旧函数地址生产调用。

## 5. 验证状态

测试覆盖索引提交、顺序过滤、type范围、四级输出覆盖、type31最终强制、occurrence零永不命中和缺失节点stop。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`188/422 = 179 platform_adapted + 9 assembly_exact + 234 pending_audit`，SHA256为`c9defe880e71c231ce22f4b49c04fc9aadd1b35b9585cec25548647291e09f2c`。动态差分因原版链表、资料加载、返回表、字符串目标与两处caller联合捕获后端缺失而登记为`blocked_runtime_oracle`。
