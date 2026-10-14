# 战斗角色启动计数查询（0x004786D0）

状态：`pending`。

## 1. 权威范围

完整LST范围为`004786D0..004786D7`，半开终点`004786D8`，8字节。
两条指令、无分支、无callee、无外部chunk或中段入口：

```text
004786D0 66 8B 81 74 2A 00 00  mov ax,[ecx+2A74h]
004786D7 C3                    retn
```

读取完整WORD，仅覆盖EAX低16位；EAX高16位、ECX、EDX与flags不变。
RET读取返回地址并推进原栈。四处物理CALL为`004566AD`、`004581FE`、
`0045DF67`、`0045DFE2`。

## 2. 实际共享字段与语义接口

`query_legacy_battle_actor_start_gate(const compat::u16*, Access)`直接借用：

- 队员：`action.group_a_action_execution[index].start_gate`。
- 敌人：`startup.group_b_lifecycle[index].action_execution.start_gate`。

查询只返回状态与`optional<u16> value`。删除本查询的地址常量、token解析、
owner/view、寄存器request/reply、栈数组、字段地址、读/调用计数，
以及两类角色帧和调试叠加层的三处转发函数。
调试接口删除本查询的reserved编号，保留其余未迁移操作的原编号。
未增加角色副本、第二套计数、包装执行器或成功占位值。

调用方外层optional表示查询实际发生；内层optional表示WORD实际读到。
不可用宿主字段或读取不可达在原字段边界返回`start_gate_read_typed_stop`，
不产生值。返回访问失败在WORD读取之后返回`return_address_read_typed_stop`，
保留观察值。两个路径都不写字段；宿主字段不可用不证明原静态内存必然异常。

## 3. 两类角色帧

`004566AD→004566B2`直接借当前队员计数，调用前EAX是
`index*2F34h`。`004581FE→00458203`位于敌人帧公共尾，
在此前逻辑执行后借当前敌人计数，EAX是`1381*index`。
尚未迁移的父级失败返回与显示调用所需EAX，在各调用点局部组合高WORD与观察值；
字段失败保留完整前驱，RET失败保留已替换低WORD。查询自身不再携带寄存器。

两处成功后都先TEST完整AX。计数非零且primary或secondary门精确等于1，
或全局override精确等于1，才给显示逻辑传1。门值2及高位带1不等于1。
保持原短路顺序：计数零不读取两个抑制门，primary等于1不读取secondary，
任一抑制门命中不读取override。传给尚存显示协议的flags来自最后一次CMP；
旧实现误用查询前SHL/SUB flags的说明撤销。

父级显示协议仍未迁移。其既有请求保存尚未恢复来源的EDX输入；本查询不再提供
EDX注入槽。测试在显示请求上提供这项原有输入，不能据此声称完整角色帧已恢复
真实寄存器来源、生产资源或全部失败行为。该父链继续pending。

查询失败抑制显示及之后的公共尾，敌人帧此前已完成的副作用仍保留。
两类帧原入口索引检查保持原位置，本切片不扩张其调用域或增加停止点。

## 4. 调试显示两处调用

敌人行`0045DF67→0045DF6C`先解析角色、读生命与level、读动作种类。
动作种类经过`AND EAX,FFFF`后已作为格式参数保存；查询此时才读取实际计数。
成功后`0045DF70`再次mask，完整WORD零扩展后用于十进制显示。

队员行`0045DFE2→0045DFE7`同样在动作种类mask之后读取实际计数，
成功后零扩展用于格式化。不得布尔化或按signed WORD显示。
每个完成行之后原循环重读角色数量；上一行绘制回调对下一角色字段的修改
必须被下一次查询看见。

失败时保留已完成行、当前动作种类及生命/level等前缀，抑制当前格式化、
绘字、循环推进和公共尾。父级EAX在读失败时保留动作种类，在RET失败时
保留已读计数；ECX保留角色地址；敌人EDX为level，队员EDX为此前字体/
绘字结果。没有新增寄存器返回层。

## 5. 独立向量与双向复核

独立LST推导记录位于`build/tmp/runtime/start-gate-query-direct-lst-analysis.md`。

- WORD：0、1、00FF、0100、7FFF、8000、FFFF，完整读值、邻居不写。
- 实际队员2与敌人3，实时重读，不修改目标计数或启动标记。
- 字段缺失、字段不可读、RET不可读、两者不可读时字段失败优先。
- 两类角色各35组计数/门组合，包含低byte零而WORD非零及高位门值。
- 敌人帧在前置terminal回调后读取改写值；读/RET失败保留前驱改写，
  缺少startup或敌人存储仍在原读取边界失败，不伪造计数0。
- 两类调试行在第一行绘制后改写第二角色，检查完整数值与读取时机；
  第二角色字段/RET失败保留第一行且抑制全部当前后缀。
- 既有队员9 RET失败保留高EAX，敌人2 RET失败保留观察值，
  显示嵌套路径继续验证实际显示参数、字段写入和失败前缀。

最终门禁均通过：

- Linux core `battle.legacy_battle_setup` 1/1，10.41秒。
- Linux core `battle.actor_frame_316` 1/1，26.49秒。
- ASan同两项分别1/1，15.81秒、25.72秒。
- SDL目标`openswd3`实际编译链接完成，未启动游戏。

使用仓库`build.sh`/`build-asan.sh`，分别指定setup与actor-frame测试目标
及精确测试正则；SDL使用`./build.sh app --build-target openswd3`。
五份最终日志位于`build/tmp/runtime/start-gate-query-direct-`前缀下，
后缀为`core.log`、`callers-core.log`、`asan.log`、`callers-asan.log`、`sdl.log`。
最终日志无warning、编译错误、测试失败或sanitizer报告。
11份源码/测试身份记录于同前缀`validated-source.sha256`，执行后校验一致。
首轮日志归档为`before-live-prefix-*`，其中setup构建含既存outcome窄化警告，
不与最终日志混用。发布检查记录于同前缀`publication-check.log`。

最后双向复核覆盖两条原指令、四个调用、完整WORD消费、原失败边界、
原短路读取顺序及真实字段借用；机器快照删除后父级所需值在调用点维护。
静态扫描核对本查询旧协议及转发已消除；它不证明全项目迁移完成。

## 6. 验收边界

保持`pending`。完整内存别名、原异常页/SEH和四调用点联合动态捕获
仍缺失，原版差分为`blocked_runtime_oracle`。
父级其余操作编号、参数数组、寄存器协议、计数、资源及完整SDL角色帧仍待迁移。
本切片不提升B11、WP316/318或实际游戏流程的验收状态。
