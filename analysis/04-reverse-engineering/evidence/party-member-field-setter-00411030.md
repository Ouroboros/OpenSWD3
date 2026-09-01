# 队伍成员17字段写入 `0x00411030`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x00411030..0x0041125D`，共285行，无外部`FUNCTION CHUNK`。caller为角色/队伍调试对话、剧情VM字段写入及跨模块状态路径。

原函数参数为selector、value和四项0x38记录索引。现代接口接收caller已选定的记录引用；caller仍在原记录访问点负责索引域。selector按无符号switch 0..16分派，negative和大于16均default不写，机器返回0。

## 2. 目标宽度

写入规则为：

```text
0..13  value低u16 -> record +0x04..+0x1E
14     完整32位bit pattern -> record +0x20
15     完整32位bit pattern -> record +0x00
16     value低u8 -> record +0x2C，再处理LEVEL
其他   不写
```

0..5虽然getter按i16返回，但setter仍只截低16位；6..13同样截低16位。14/15不能截短或做饱和。

## 3. selector16与LEVEL顺序

selector16先提交低byte，再以固定group 2和`u32(value)+1`直连`0x00477290`的共享LEVEL loader，输出初值为1：

- tag 5正常成功时，用最后一个tag 0输出完整覆盖同record field14（`+0x20`）；
- 文件打开失败或记录首word非零时，callee正常返回0，输出仍为1，caller仍把1写入field14；
- loader在原内存访问点typed-stop时，保留已经写入的低byte，但不提交field14，也不伪造caller正常返回。

`LegacyPartyMemberFieldWriteResult`携带完整load结果与`level_requirement_stopped`。剧情VM opcodes 188–190在低byte已写后传播typed-stop，不推进instruction offset；队伍对话在scratch已释放后传播，不执行页面refresh。LEVEL文件会话由共享`LegacyBattleLevelDatabasePort`唯一持有。

## 4. owner回收与验证

剧情VM此前已有同语义内部setter。当前提升公开typed入口，并把opcodes188–190改为直接调用；对话主过程关闭时也必须直接复用。

`special_modes.legacy_initial_menu`覆盖0..13低字截断、14/15完整dword、selector16负值按完整u32加一、低byte先写、真实LEVEL成功、正常打开失败、分配清零typed-stop和default不写。既有VM与对话测试继续覆盖set/add/sub回绕、固定第二项记录、低byte故障前缀和instruction/page副作用阻断。
