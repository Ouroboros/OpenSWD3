# 战斗升级提示面板 `0x00467AC0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00467AC0..0x00467C41`，从proc到endp共168行、99条带机器码和真实助记符的实际指令、8个静态call、4个跳转、3个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。唯一静态caller是已关闭消息阶段分派的消息100；该caller在胜利奖励结算之后、清角色目标门及递增timer之前调用本函数。

8个callsite依次为动作更新1次、矩形效果1次、九宫格2次、文字绘制2次、结算面板查询1次和`wsprintfA`1次。动作、矩形和九宫格直接组合已关闭typed实现；两类文字边界和结算查询复用上一胜利工作包的窄端口；格式化由平台边界返回64-byte局部缓冲内容。

## 2. 入口局部缓冲与底板门

入口建立64-byte局部文字缓冲，按原BSS首byte加其余零初始化。随后只把EAX低byte替换为共享过渡角色索引；索引不为`0xFF`时绘制底板。索引为`0xFF`时继续比较共享过渡模式，模式精确为1仍绘制，其他值跳到底板后的公共查询。该门不夹取actor，也不改变入口EDX；跳过底板时公共查询前EAX为`0xFF`、ECX为零。

底板复用胜利奖励唯一共享动作记录，不清整个记录，只写动作`0x233B`和基础变体0；动作更新失败没有原始分支，必须继续。随后以固定`x=196,y=176,width=188`和`transition stage+40`为高度执行模式0、颜色`0/4/4`矩形效果。

第一九宫格范围为`200,180..376,196`、flags `0x80000008`，资源低word取动作记录，资源高word保留矩形返回EDX。完成后以固定CP950`升級`、坐标`264,180`、颜色`0xFFC0`和字体16绘制标题。第二九宫格范围为`200,212..376,transition stage+212`，资源高word改为标题文字返回EDX。任一modern画面安全状态typed-stop时保留此前动作、矩形、九宫格或标题前缀。

## 3. 公共查询与live角色重读

无论底板是否绘制，函数都固定以`212,244,3`查询结算面板；只有返回EAX精确等于1才继续。查询返回后重新读取live过渡角色索引，因此callee改写actor会影响后续文本。索引为`0xFF`直接返回；其他值按i8符号扩展后首次访问十项动作标签表，负值或第十一项在该真实访问typed-stop。

动作标签读取后保留两条原算术链：ECX为`label*7`，EAX为`label*16`。随后从世界剧情VM唯一四项、每项56-byte的角色资源owner读取`+0x2C`独立等级byte。标签超出四项时在名称token加基址之前停止，返回寄存器保持EAX缩放值、ECX七倍值和清零EDX。

## 4. 原格式串与绘制

角色名称token固定为`0x0049E148 + label*16`。权威格式串从`0x004A7A38`开始，不是IDA误标出的独立`%s`，而是连续CP950字符串`%s升第%d級\0`；等级byte是第二个格式参数。格式调用前EAX是64-byte动态栈缓冲token，ECX保持`label*7`，EDX是零扩展等级byte。

平台格式边界返回实际文字与`wsprintfA`寄存器。最多63个非NUL byte可写入局部缓冲；第64个非NUL byte已经使随后NUL落到缓冲外，因此在格式副作用之后、文字绘制之前typed-stop。成功时以坐标`208,220`、颜色`0xFFC0`、字体16绘制局部文字；调用前EAX保留格式返回，ECX换为字体token，EDX换为framebuffer token。

动态栈token只作为`compat::u32`身份，不转换为宿主指针。缺少真实名字内容时不伪造名称，由端口提供格式结果并在动态差分登记运行时缺口。

## 5. owner、caller回收与验证

共享动作记录复用`LegacyBattleVictoryRewardState`；过渡actor、mode和stage复用目标选择runtime；十项动作标签复用启动动作模式source；角色等级复用世界剧情VM角色资源owner。函数不新增物理state，也不改变全局重置写集合。

消息100现在依次直连胜利奖励和本升级面板；第142项旧阶段100槽继续reserved且生产零调用。本函数typed-stop发生时，已完成胜利奖励与升级面板前缀保留，但caller的actor-retarget、双cache、target-ready、queued和timer写入全部阻断。

定向测试覆盖底板普通/跳过/模式1强制门、固定CP950标题、矩形与双九宫格几何、矩形和标题返回EDX高word链、公共查询精确1门、查询后actor重读、i8负索引、标签缩放停止寄存器、等级读取独立byte、格式token与寄存器、64-byte溢出停止、上下九宫格停止前缀、消息100正常直连与子stop传播。验证：定向测试、AddressSanitizer、Linux core `188/188`、Linux app `194/194`全部通过。源码构建零warning；app仅保留既有ALSA开发库CMake warning。

当前缺少原版动作/画面/字体surface、结算查询callee、角色名称与等级联合状态、动态栈地址、`wsprintfA`返回及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
