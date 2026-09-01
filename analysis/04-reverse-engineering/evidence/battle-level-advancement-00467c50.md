# 战斗角色升级属性提交 `0x00467C50`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00467C50..0x00467EF2`，从proc到endp共260行、153条带机器码和真实助记符的实际指令、5个静态call、8个跳转、5个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。唯一静态caller是已关闭消息阶段分派的消息101；原caller只在过渡角色为`0xFF`时先调用本函数，返回后重读角色，仍为`0xFF`才进入既有选角。

5个callsite包括升级需求查询1次、56-byte角色模板生成2次、停止sample `0x12C`一次和播放sample `0x12B`一次。升级需求查询已直连共享LEVEL loader；两个56-byte模板生成callee继续由窄typed端口保留。sample停止使用固定返回EAX 1的已关闭命令语义，sample播放按live signed mix level进入typed音频边界。

## 2. live组A扫描与资格门

入口读取live组A数量并按i32 signed比较；数量不大于零直接置完成门。正数时从索引0扫描，每轮尾部重新读取live数量，不增加现代上限。组A对象使用基址`0x005029D0`、步长`0x2F34`；先读取`+0x2B00`和`+0x2B04`，任一值精确等于1就跳过。第十一对象在首次真实字段访问typed-stop，完成门保持旧值。

通过双字段门后读取对应动作标签。函数只清EDX低word，保留入口或前一callee的高16位；ECX先按`label*7`计算，再只以角色记录`+0x2C`等级byte替换CL。EBP取该byte并加一形成候选新等级，DL也写旧等级。旧等级按u16与共享阈值比较；不低于阈值时把过渡角色写`0xFF`并继续下一对象。

角色记录基址是`0x004AB790`，步长56；因此`0x004AB7BC + label*56`是记录`+0x2C`的独立等级byte，不是`+0x1C`。本工作包把世界剧情VM角色资源typed结构补齐`0x2D..0x37`尾11字节并以`static_assert`锁定56-byte物理尺寸，同时回收上一升级提示工作包的误偏移读取。

## 3. 升级需求与signed经验门

等级低于阈值时，函数先清本地需求输出，随后以一基角色标签为group、`old level+1`为level直连共享LEVEL loader。调用前EAX是一基标签，ECX是输出token，EDX保留高16位并在低byte保存旧等级。文件打开失败或记录首word非零是callee正常返回0，caller仍使用未被覆盖的零输出；目录、流或caller输出访问故障则在原点传播`level_requirement_typed_stop`，保留此前扫描前缀且不继续经验比较。

正常返回后重新读取live动作标签，以`label*56`重建记录地址，读取记录首dword并与需求输出按i32 signed比较。首dword小于需求时把过渡角色写`0xFF`并走正常循环尾；相等或更大才提交升级。标签被后续callee改到已知四项owner外时，在首次真实记录首dword访问typed-stop，并保留EAX标签、ECX需求和EDX缩放偏移。

## 4. 三份56-byte物理记录与双模板调用

符合经验门后，函数严格执行：

1. 清零`0x005028C0`起56-byte旧等级模板；
2. 清零`0x00520F80`起56-byte新等级模板；
3. 从当前角色记录完整复制56 byte到`0x004FF108`快照，包括此前typed结构缺失的尾11 byte；
4. 以旧等级、同一一基标签和共享过渡模式地址生成旧模板；
5. 重新读取live动作标签，以新等级和同一过渡模式地址生成新模板。

第一次模板call前EAX是一基标签、ECX是旧等级、EDX是`label*56`；第二次call前EAX/ECX保留第一次callee返回，只有EDX重新装载为live一基标签。两个callee都可发布新的live组A数量和过渡模式。

## 5. 原逐字段差值提交

第二次模板返回后再次读取live标签。目标记录合法时按原顺序提交：

- 等级byte直接替换为新模板`+0x2C`；
- 新旧模板`+0x0A/+0x0C/+0x0E`三个u16差值分别按u16回绕加入角色三个limit，再逐项复制到对应current；
- 角色`+0x20`完整dword直接替换为新模板值；
- 新旧模板`+0x10..+0x1E`八个u16差值逐项按u16回绕加入角色对应字段；
- 角色首dword、`+0x24` transient、`+0x28`和`+0x2D..+0x37`保持原值。

原LST用多个重叠dword读取差值，但每次只以DX或CX提交独立u16；modern逐u16计算，不把低半借位传播到下一字段。若第二次callee改出的live标签越界，停止发生在读取两份scratch前缀并完成`label*56`及新等级CL重建之后，尚未写角色记录。

## 6. 角色发布、音频和首成功早退

全部字段提交后，把当前循环索引低byte发布为过渡角色。sample停止调用前EAX为`label*56`，ECX低word是最后一个`+0x1E`字段差值，EDX是两模板`+0x1C`重叠dword差值但低byte被当前actor索引覆盖。停止`0x12C`返回后重新装载live signed mix level到EAX，再播放`0x12B`。

sample播放返回后只以当前actor索引替换AL，保留EAX高24位；索引非`0xFF`立即退出整个扫描，因此每帧只提交首个符合资格的角色。正常空扫描、耗尽或成功早退都在函数尾无条件把`0x0053C4C8`完成门置1；任何typed-stop不执行该写入。

## 7. owner、caller回收与验证

组A数量、双跳过字段、阈值、动作标签、过渡actor/mode、sample mix和四项角色资源均复用既有owner。level state承接两份56-byte模板、一份56-byte角色快照和此前未命名的完成门；后续成长对照面板又在同一逻辑owner中承接非连续的三项primary与六项secondary成长差值，成长标题框再承接独立24-byte共享标题。完成门和成长差值都不在战斗全局重置的原写集合内，不新增伪清零。

消息101现于actor为`0xFF`时先直连本实现；本函数成功发布actor后直接回到原完成查询/转场，仍无actor才调用旧选角边界。本函数typed-stop（包括共享LEVEL loader stop）阻断选角、完成查询、transition分配、message和timer写入。frame coordinator原`query_level_requirement`槽只保留reserved alias且生产零调用；SDL battle端口把LEVEL访问转发到全局唯一文件会话。

定向测试覆盖非正数量、双字段精确1跳过、live数量尾重读、等级阈值、signed经验不足、真实需求输出、LEVEL零分配故障前缀、双模板寄存器、完整56-byte快照、三组current/limit、八个u16差值、等级/field20替换、尾11 byte保留、最终音频寄存器、首成功早退、初始标签越界、第十一对象以及消息101正常直连与子stop传播。最终完整门结果见本轮工作包总证据。

当前缺少原版组A对象、需求查询与双模板callee联合状态、三份物理scratch动态内容、角色记录加载后端、sample对象及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
