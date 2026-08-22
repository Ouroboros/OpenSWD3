# 剧情 VM ANI恢复位 `0x0042CE12`

状态：`assembly_exact`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042CE12..0x0042CE31`

opcode：176 / `OP_176_RESUME_STORY_ANI`

## 1. 完整u32 owner

handler无operand，读取完整`dword_4B7AA0`并只执行：

```text
dword_4B7AA0 &= 0xFFFFFFEF
```

该owner属于自定义ANI活动控制flags。opcode96启动前清零完整dword并由prefix设置bit1/bit0；ANI逐帧更新`sub_4154A0:0x004154D9..0x004154E0`读取低字节bit4，置位时阻止ANI更新；opcode175独立置bit4。

现代复用已经恢复为u32的`LegacyAniActivityState::flags`和窄`set_story_ani_suspended(bool)` port。opcode176传入false，SDL对actual activity owner执行完整u32 AND，不建立镜像、不截断其余31位。bit4已经清零时幂等。

## 2. 写入、IP与yield顺序

机器顺序为：

```text
读取完整flags
物理脚本指针 += 2
flags &= 0xFFFFFFEF
写回完整flags
u16 IP += 2
跳入common join
发布normalized previous176
service audio一次
yield并返回
```

`loc_427B59`在每条指令分派前把`ESI`清零；opcode176不像opcode175，不把`ESI`改为1。`loc_42B0AE`因此发布previous后进入`loc_42D4D7`的`_AIL_serve`并返回，不在same call读取后继。

现代port写发生在context IP和previous更新前；audio发生在IP、previous和flags全部提交后。记录位于`IP=0x7FFE`时，flags写、IP=`0x8000`、previous176和audio一次全部完成并yield，不因窗口尾执行下一fetch。

owner已由实际ANI runtime承接，固定bit合法，无nullable、分配、I/O或平台失败边界，故分类为`assembly_exact`。

## 3. 资产absence与验证

完整线性TALK目录中opcode176为0条物理记录/0 probes，因此使用`asset_absence_verified`，不伪造真实回放。四种raw word的全文件双字节候选计数为：

```text
raw    TALK1 TALK2 TALK3 TALK4 total
00B0       7     1     2     0    10
40B0       0     2     0     0     2
80B0       0     0     0     0     0
C0B0       0     0     2     1     3
```

这些15处字样均非线性指令入口。

synthetic覆盖四raw alias、完整u32高位保持、bit4已清幂等、actual-owner port写点早于IP/previous、audio晚于flags/IP/previous、+2、previous176、audio一次、yield不fetch及精确窗口尾。ANI owner依赖与Story VM synthetic、real、initial-session共4/4通过，SDL app编译通过；Linux core 186/186、app 192/192通过。未启动原版或OpenSWD3游戏EXE。
