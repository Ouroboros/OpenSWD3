# 标准模式数据库runtime record刷新 `0x0043F1E0`

状态：`platform_adapted`、`unit_tested`

## 范围与生命周期

唯一行为真值为`swd3.exe.lst`。范围`0x0043F1E0..0x0043F7AF`，633行、30个基本块；caller为DD20、DDF0、DED0、DFA0、E080、E170、E3D0共七处，callee仅476DB0两次与4885A0两次。

入口严格先释放两份runtime record `+AC`非零token，再清两份0xB0记录并写ID `FFDC`。输入任一missing ID为FFDC或inline `+2` enable为0时返回0。

## pair fast-path

有效输入先把两输出初始化为ID101、enable1、`+8/+A`清0。外部pair表从`4C97E4+1C2`开始，以无序输入pair匹配，但输出ID保持表记录固定顺序；命中后直接写两ID并返回1，不调用476DB0。

## fallback扫描

pair无命中时使用21项映射`0..9,0×5,14,13,12,10,11,0`和15×15关系表：

- 第一输出关系=`map(first category),map(second category)`，target=`(value1+value2)/2+3`，奇偶取second category，排除a7==1。
- 第二输出关系行列反转，target不加3，奇偶取first category，排除a7==2。
- 两次均扫描ID101..1199；偶category取不小于target的最近值，奇category取不大于target的最近值，严格小于best才覆盖。
- ID101..500若field2c bit800清则拒绝；随后应用a7排除。无候选保留ID101。
- 每次写输出后调用476DB0到record `+C`，实参保留`runtime legacy address high16 | ID`。

四表映射为`FCAC4=field_5e`、`FCAB0=field_60`、`FCA90=field_2c`、`FCACC=field_a7`。category超出21项时helper在原栈表读取点返回越界状态并保留此前输出初始化；现有七caller的权威有效域均由记录格式约束在0..20。

七caller现全部直接调用typed helper；旧`rebuild_*inline_records`仅保留未调用测试兼容接口，不再决定行为。

## 说明文字释放修正

`0043F28C`与`0043F2A5`分别释放非零说明指针；两次调用均在
`0043F2C1`与`0043F2CE`清空记录之前。第一份释放成功后，第一份
记录的指针字节仍保留，直到第二次释放成功，不能提前清零。

默认释放端口现在转发到MON说明堆的typed释放接口，SDL既有MON端口
负责实际释放。释放使共享文字别名失效；仅丢弃文字视图不算释放。
第一次或第二次释放停止时，返回`description_release_typed_stop`，
保留两份原始记录与已发生的释放，不继续清空、写入缺失ID或扫描。

七个直接调用点检查刷新结果。六条导航路径停止后续显示标记写入和
音效调用；确认路径停止属性计算和交互阶段切换。输入入口传播停止，
重叠导航区域不再继续触发下一回调或退出清理。

回归使用实际MON文字堆覆盖双释放、别名失效、第一次与第二次释放
失败，并覆盖六条导航、确认及重叠输入的停止。旧实现编译链接成功后
触发预期断言失败；修正后菜单定向core与ASan各1/1通过，SDL构建通过。
日志：`build/tmp/runtime/menu-description-release-{red-linked,core,asan,sdl}.log`。
本轮未启动游戏，不构成菜单实机或完整读档续玩验收。

## 历史测试与验证

UT覆盖双token释放、pair固定输出且不加载文本、两次1099项扫描、奇偶方向、bit800门、a7==1/2排除、关系行列反转及`0x004F0000|ID`文本key。既有七caller断言删除伪造inline改写事件并按真实runtime重建更新。

定向测试通过。workpack双生成稳定为`62/227`，SHA256均为`3cba64ef31ba3ac12e97f5228ddf028b4d5fa32511f1069307f8d6cee58bcd1a`；下一单元`0x0043F7C0`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
