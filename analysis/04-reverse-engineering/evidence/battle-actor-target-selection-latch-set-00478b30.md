# 战斗角色直接置位目标选择标记（0x00478B30）

分类保持`platform_adapted`。本置位及全部三处实际caller的语义迁移已验证。
动作及双方帧的其他协议仍待迁移，不能据此升级父链验收。

## 1. 完整物理范围与写入顺序

唯一行为真值为LST `00478B30..00478B3A`。完整11字节、2条指令，
无callee、分支、外部chunk或中段入口。`00478B3B..00478B3F`为填充。

```text
00478B30  mov dword ptr [ecx+2AA8h], 1
00478B3A  ret
```

无条件把完整DWORD写为1，没有读取旧字段、比较或截断。旧值精确1也执行
原写入路径。MOV/RET保持EAX、ECX、EDX及flags；写字段失败在提交前，
RET访问失败在提交后，保留已写入的1。

## 2. 实际共享字段与语义结果

`set_legacy_battle_actor_target_selection_latch`直接借用实际可写`u32*`，
返回完成或两个既有故障状态。队员字段属于startup runtime reset数组，
敌方字段属于startup lifecycle数组的runtime reset。没有另建状态副本、
缓存、token地址解析或复制回写，原字段生命周期保持。

缺失存储仍停在原字段写边界。不可写字段保留完整旧值，返回故障保留1。
访问描述只表达原字段写与RET访问两个既有失败条件，不新增停止位置，
不把空字段变成零或成功。父结果用可选状态区分未执行与实际执行，包含
失败执行；嵌套动作仅汇入实际存在的置位状态。

删除字段view/owners和resolver、寄存器request/reply、地址表、请求数组
及offset、栈token、flags/ESP/EIP、读写/调用计数、trace、执行器和三处
纯转发函数。叶接口不把写入的1包装成EAX。仍未迁移callee所需的前驱
寄存器及CMP结果由父层沿用实际前驱，叶结果不回传寄存器。

## 3. 全部三个实际调用方

完整LST只有以下三个真实CALL：

```text
00454BAE -> 00454BB3
00456B51 -> 00456B56
004578FB -> 00457900
```

### 3.1 动作发布：00454BAE

00454B99先写scene值，00454BA7先执行scene发布，再置位当前队员字段。
源码入口已检查队员索引小于10，直接使用同一实际索引。正常返回才OR
动作标记高字节80；两种故障保留scene前缀并阻断OR及正常收尾。
故障返回沿用scene发布的实际EAX，不返回写入值1。

### 3.2 队员候选：00456B51

命中terminal完整值不等于1的候选后，00456B47先写target-ready，再置位
当前队员字段。正常返回后00456B59才写目标，随后读取同一实际标记。
两个故障保留target-ready和此前候选工作，阻断目标写及查询后缀。
故障返回沿用实际terminal EAX；正常后续选择沿用terminal EAX/EDX及
原CMP结果。00478A70只重载AX，保留EAX高WORD、EDX和flags。

### 3.3 敌方候选：004578FB

00457754先比较实际优先角色与输入敌方索引；不相等就跳过候选路径。
命中候选后004578F1先写target-ready，再置位当前敌方字段，正常后
00457903才选择目标，00457908才写pending。两个故障保留ready及候选
前缀，阻断选择和pending；返回故障仍保留已写1。
故障返回及正常后续选择使用实际terminal前驱，EDX及CMP结果不经叶转发。

本批敌方状态下，置位前依次经过004576D9、004577A0、00457824和
004578C6四次terminal查询。测试高位回复放在最后一处，输入索引3时
共享优先角色也为3，保证向量实际进入原候选分支。

## 4. 双向复核及汇编独立向量

完整DWORD写对应唯一共享写，RET对应完成或已写结果保留的故障状态。
三个caller逐项对应前驱、字段写、原失败出口及正常后缀。未增加字段读、
布尔化、循环、callee、资源分配或副本；原所有权和生命期保持。

独立向量包括：

- 七类旧DWORD：0、1、2、00010001、7FFFFFFF、80000000、FFFFFFFF。
- 重复置位，邻接字段不变；实际队员2与敌方3独立，后续共享写再次置位。
- 七类旧值下字段不可写及返回故障，分别验证旧值保留和完整写1。
- 空字段仍为原字段故障，不制造成功。
- 三个caller的写/返回故障、共享前缀、后缀抑制及实际前驱EAX。
- 动作及队员索引2、敌方索引3使用自身字段，索引0的毒值保持。
- 双方正常选择保留前驱EAX高WORD、完整EDX及CMP flags。
- 嵌套动作汇总保留实际置位状态及同一字段写，未执行结果仍为空。

## 5. 本批验证与限制

当前源码和测试的实际门禁均通过：

- core setup：1/1，12.50s。
- core actor_frame_316：1/1，31.44s。
- AddressSanitizer setup：1/1，18.02s。
- AddressSanitizer actor_frame_316：1/1，33.51s。
- SDL openswd3编译并链接。

日志为`build/tmp/runtime/target-latch-set-direct-{core,callers-core,asan,
callers-asan,sdl}.log`。仅ASan setup含既有结算测试133行WORD转BYTE
警告一次；其他四份日志无警告或错误，没有sanitizer finding。
未修改该无关测试。`verify-target-latch-set-direct-publication.py`及
`target-latch-set-direct-publication-check.log`确认十二份源码/测试身份、
四个真实测试、SDL链接、两条完整指令及三个caller、原故障顺序、
实际索引与前驱续接、旧协议扫描和空白检查。
源码或测试改变后，旧日志与身份不再支持当前验收。

被停止的错误actor-frame名称门禁，以及terminal回复顺序和优先角色条件
尚未正确的失败运行，仅为历史，不支持当前实现验收。后两项依LST修正
测试输入，生产分支保持。没有启动原版或OpenSWD3游戏程序。

Workpack312旧寄存器合同、199/199、205/205及十轮验证已归档在Git历史，
不代替当前语义接口和实际caller门禁。分类仍为`platform_adapted`，
原版动态差分仍为`blocked_runtime_oracle`：没有原版异常字段/栈页及三个
caller的联合寄存器、flags与SEH捕获后端。B10游标、Workpack316/318、
B11实际续玩及全项目通用调用清理均未升级。
