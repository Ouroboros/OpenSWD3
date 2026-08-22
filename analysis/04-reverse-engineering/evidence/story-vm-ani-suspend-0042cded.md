# 剧情 VM ANI暂停位 `0x0042CDED`

状态：`assembly_exact`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042CDED..0x0042CE11`

opcode：175 / `OP_175_SUSPEND_STORY_ANI`

## 1. 完整u32 owner

handler无operand，读取完整`dword_4B7AA0`并只执行：

```text
dword_4B7AA0 |= 0x00000010
```

该owner属于自定义ANI活动控制flags：

- opcode96启动前把完整dword清零，再由`%`/`*`prefix分别置bit1/bit0；
- ANI逐帧更新`sub_4154A0:0x004154D9..0x004154E0`读取低字节bit4，置位时阻止ANI更新；
- 同一更新器`0x00415657..0x0041567A`读取bit0选择ending长度；
- opcode176在独立handler `0x0042CE12`只清bit4。

原owner是dword。现代`LegacyAniActivityState::flags`因此从u8恢复为u32；ANI start的u8脚本prefix值仍零扩展写入完整owner，frame消费者继续只测试既有低位。Story VM通过窄`set_story_ani_suspended(bool)` port写actual `LegacyAniActivity` owner，SDL不建立镜像、不截断其余31位。

## 2. 写入、IP与same-call顺序

机器顺序为：

```text
读取完整flags
物理脚本指针 += 2
flags |= 0x10
写回完整flags
u16 IP += 2
ESI = 1
发布normalized previous175
same-call fetch successor
```

现代port写发生在context IP和previous更新前。bit4已经置位时幂等；包括高16位在内的其余bits全部保持。handler不service audio、不yield。

记录位于`IP=0x7FFE`时，flags写、IP=`0x8000`和previous175先提交，再由same-call下一fetch返回`instruction_out_of_range`；此前副作用不回滚。

owner已由实际ANI runtime承接，固定bit合法，无nullable、分配、I/O或平台失败边界，故分类为`assembly_exact`。

## 3. 资产absence与验证

完整线性TALK目录中opcode175为0条物理记录/0 probes，因此使用`asset_absence_verified`，不伪造真实回放。四种raw word的全文件双字节候选计数为：

```text
raw    TALK1 TALK2 TALK3 TALK4 total
00AF      16     0     2     2    20
40AF       0     0     0     0     0
80AF       0     0     0     0     0
C0AF       3     0     1     6    10
```

这些30处字样均非线性指令入口。

synthetic覆盖四raw alias、完整u32高位保持、bit4已置幂等、actual-owner port写点早于IP/previous、+2、previous175、same-call OP1025、无audio及精确窗口尾。SDL app目标编译验证port写入实际`LegacyAniActivityState::flags`；ANI actual-owner依赖与Story VM synthetic、real、initial-session共4/4通过；Linux core 186/186、app 192/192通过。未启动原版或OpenSWD3游戏EXE。
