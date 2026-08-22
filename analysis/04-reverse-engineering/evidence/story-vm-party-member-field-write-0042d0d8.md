# 剧情 VM 队伍成员字段写入 `0x0042D0D8`

状态：`platform_adapted`、`unit_tested`、`asset_absence_verified`、`sdl_runtime_integrated`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042D0D8..0x0042D16B`，setter `0x00411030..0x0041125D`，getter `0x004112B0..0x0041144B`

opcodes：188–190

## 1. record与refinement

三路固定长度6：

```text
+2  i16 field selector
+4  i16 value / delta
```

读取顺序严格为selector后value。signed selector `>16`在value已读后不访问party-member owner，不推进IP；发布previous、service audio一次并yield，下一帧重试。negative selector通过外门：getter default返回0，setter default不写，仍正常消费6字节。

三路固定操作第二项0x38 party-member记录：

- opcode188：把sign-extended i16 operand直接交给setter；
- opcode189：getter结果与sign-extended operand执行i32 bit-pattern回绕加法，再交给setter；
- opcode190：getter结果与sign-extended operand执行i32 bit-pattern回绕减法，再交给setter。

所有正常路径固定`IP += 6`、发布previous并same-call，无audio。

## 2. 17字段setter宽度

setter与opcodes186–187的getter共享同一完整中性owner：

```text
0..13  写结果低u16到record +0x04..+0x1E
14     写完整u32到record +0x20
15     写完整u32到record +0x00
16     先写结果低u8到record +0x2C，再进入LEVEL.DAT helper
other  不写
```

189/190计算使用getter宽度：fields0–5先i16 sign-extend，6–13先u16 zero-extend，14–15保留完整i32 bit pattern，16按u8 zero-extend。i32溢出按机器bit pattern回绕，最后才按setter目标宽度截断。

## 3. field16与LEVEL.DAT边界

field16 setter先提交低byte，然后调用`sub_477290(group=2, level=result+1, &output)`；加一按u32回绕。helper按：

```text
directory = 0x70 + 4 * (level + 100 * group)
payload   = 0x200 + u32(directory)
```

读取零填充0x400字节tag stream。tag0复制26字节并把payload末4字节写入output；tag1额外跳2字节；tag5成功返回。成功时setter把output覆盖到同record field14（+0x20）；文件打开、首word或记录失败返回false，保留旧field14。若成功前没有tag0，output保持调用方初值1。

仓库尚无B10 LEVEL数据库loader，因此VM增加可失败窄port。recording测试锁定成功/失败；SDL明确返回false，等价原LEVEL文件/记录失败，不伪造field14。无论helper成功还是失败，field16低byte已经提交，handler随后正常+6、previous、same-call。B10后续只需把真实parser接入该port。

## 4. 资产锁与验证

完整线性TALK目录中opcodes188/189/190均为0条，使用`asset_absence_verified`。全文件双字节候选总计：

```text
00BC=12  40BC=2  80BC=0  C0BC=2
00BD=37  40BD=0  80BD=1  C0BD=1
00BE=11  40BE=0  80BE=0  C0BE=10
```

候选均为operand、文本或其他非线性入口字节，不能冒充真实record。

synthetic覆盖三opcode四raw alias、全部17 setter映射、固定第二项owner、direct/set/add/sub、getter extension、i32回绕、u16/u32/u8目标截断、field16先写、LEVEL group2与result+1回绕、LEVEL成功/失败、negative selector default、高selector读取value后audio-yield重试、selector/value分阶段截断及六字节精确尾。Story VM synthetic/real/initial-session 3/3、SDL app编译、Linux core 186/186与app 192/192完整门全部通过。未启动原版或OpenSWD3游戏EXE。
