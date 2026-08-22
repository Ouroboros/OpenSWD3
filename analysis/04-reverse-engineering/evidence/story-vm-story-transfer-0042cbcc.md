# 剧情 VM 故事窗口切换：0x0042CBCC

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`platform_adapted`、`sdl_runtime_integrated`；原程序动态差分仍为`blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042CBCC..0x0042CBFE`

opcode：`161`，四个raw alias为`00A1/40A1/80A1/C0A1`。

## 1. handler合同

handler按固定顺序执行：

```text
0x0042CBCC  call _AIL_serve@0
0x0042CBD1  load saved physical script pointer
0x0042CBD5  movsx eax,word ptr [pointer+2]
0x0042CBD9  push signed story id
0x0042CBDA  push context base
0x0042CBE3  call sub_42E480
0x0042CBE8  load global TALK window pointer
0x0042CBF1  save new physical script pointer
0x0042CBF5  mov esi,1
0x0042CBFA  jmp loc_42B0AE
```

源记录固定为4字节：opcode和一个signed i16 story id。第一次audio maintenance严格发生在operand读取前。handler不把源IP增加4；callee成功时直接把新context IP清零，caller把物理脚本指针切到新TALK窗口。

caller完全忽略`sub_42E480`的EAX。`ESI=1`进入common join后发布normalized previous161，并在同次解释器调用中从新窗口offset0继续；不经公共yield尾。

## 2. `sub_42E480`资源与audio顺序

完整callee范围为`0x0042E480..0x0042E594`，唯一调用点是opcode161。

callee先关闭旧共享TALK文件owner，再把signed operand保存在ESI中并按u32位型执行除2000：商加一选择`TalkN.dat`，余数选择`0x200 + index*4`目录项。负i16因此不会先被合法性拒绝，而会尝试一个巨大文件号。

成功路径顺序固定为：

```text
关闭旧文件
构造 TalkN.dat 路径
audio maintenance #2
打开文件
读取4字节目录相对偏移
context.instruction_offset = 0
audio maintenance #3
从 0x200 + relative_offset 读取最多0x8000字节到共享窗口
audio maintenance #4
return 1
```

窗口读取前不清空目标；短文件读取会保留旧窗口未覆盖尾部。

打开失败路径在第二次audio后构造诊断并向主窗口同步发送消息2，然后返回0。caller仍会发布previous并使用全局窗口基址same-call，context IP却没有清零；这是原Win32退出请求与陈旧物理指针耦合的非规范失败域。

## 3. 现代平台边界

现代成功域继续使用`LegacyResourceDatabases::load_talk_story_window(..., false)`：

- signed i16原样传入port；
- TALK文件、目录和数据读取由RAII文件owner承接；
- `clear_before_read=false`保留未覆盖窗口尾；
- ready后提交data offset、file number、loaded owner与IP0；
- 成功固定计入四次audio、发布previous161并same-call。

资源port把原来的打开、目录读取和窗口读取合并为一次受检调用，因此第三次audio与内部目录/数据I/O的夹点不能在VM层拆开；现代在ready返回后连续执行第三、第四次audio。audio owner不消费TALK窗口或目录状态，该差异被隔离为资源平台适配，不改变VM业务状态和调用次数。

`invalid_story_id/open_failed/table_*_failed/data_*_failed`不重现同步宿主退出后继续使用陈旧物理指针。现代在受检load边界返回`load_failed`：operand截断前只保留第一次audio；load失败保留前两次audio、原IP、原previous与旧window owner。该typed-stop明确替代原Win32失败域，不伪造成功或后继执行。

## 4. 资产事实

线性TALK目录锁定90条物理记录和90个entry probe，全部位于`TALK1.DAT`，均为raw `0x00A1`、长度4。

90条记录包含89个不同的正story id，范围`2001..6911`。按机器的u32除2000规则解析目录：

```text
目标 TALK2.DAT  87
目标 TALK3.DAT   2
目标 TALK4.DAT   1
```

全部90个目录项和目标物理偏移有效，全部目标窗口的首opcode都是特殊值1026。该特殊runtime path仍按其独立工作包保持`pending_audit`，不从本handler继承关闭结论。

真实回放使用`TALK1.DAT@0x00007505`：raw `A1 00 F5 07`传入story id 2037，选择`TALK2.DAT`目录index37和data offset `0x00006CE9`；目标以`1026 -> opcode59(sound 193)`开头。解释器在同次调用执行三条指令并由opcode59让出。

## 5. 测试边界

synthetic固定：

- 四种raw alias都sign-extend story id并调用同一transfer port；
- ready路径audio事件为`#1 -> #2 -> atomic load -> #3 -> #4`；
- 新窗口从offset0 same-call，1026后抵达unsupported successor，previous仍为161；
- `clear_before_read=false`保留transfer未覆盖的旧窗口尾；
- 源记录恰好位于`IP=0x7FFC`时仍完整切换，不由旧窗口尾触发下一fetch；
- `IP=0x7FFE`缺operand时先执行第一次audio，再typed-stop；
- operand `0xFFFF`按i16成为-1；invalid/open失败均保留两次audio且不推进IP或previous；
- 既有`161 -> 25 -> 26 -> FFFF`组合链继续完成窗口终止清理，并固定opcode161四次audio。

真实TALK2 transfer回放固定file number、data offset、目标1026、后继sound id、三条同调用执行和最终yield。

## 6. 分类与验证

合法资产域的operand位宽、u32文件/目录选择、四次audio、窗口尾、IP0、previous与same-call均已完成LST→C++→LST双向收敛。文件owner、原同步宿主退出、陈旧物理指针失败域和内部I/O夹点由上述最小资源边界承接，因此分类为`platform_adapted`。

Story VM synthetic、real、initial-session-real三项定向CTest 3/3通过。workpack双生成稳定为`127/146 = 20 assembly_exact + 107 platform_adapted + 19 pending_audit`，hash为`963960243d29c9674623319eff54bd274344218463faf2cf17c0323bdc8c2052`。Linux完整门core 186/186、app 192/192通过。未经许可未启动原版或OpenSWD3游戏EXE。
