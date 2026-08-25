# 队伍成员17字段读取 `0x004112B0`

状态：`platform_adapted`

## 1. LST锁

权威范围为`swd3.exe.lst`的`0x004112B0..0x0041144B`，共214行，无外部`FUNCTION CHUNK`。caller为角色/队伍调试对话页填充`0x00410730`和剧情VM字段指令。

原函数参数为field selector和四项0x38记录索引；每个case都按`index * 0x38`访问同一进程记录数组。现代接口接收已经由caller选定的`LegacyWorldStoryPartyMemberResources`引用，避免重复裸索引；caller仍负责在原记录读取点检查其索引域。

## 2. 17字段宽度

selector按无符号switch 0..16分派，其他值固定返回0：

```text
0..5   record +0x04..+0x0E，i16扩展到i32
6..13  record +0x10..+0x1E，u16扩展到i32
14     record +0x20，完整32位bit pattern
15     record +0x00，完整32位bit pattern
16     record +0x2C，u8扩展到i32
其他   0
```

特别地，negative selector由switch unsigned上界检查落default，不产生负记录偏移。selector 6..13不得误作signed；`0x8000`和`0xFFFF`分别返回32768和65535。

## 3. owner回收

剧情VM此前已有同语义内部实现。当前将其提升为公开typed入口，并把opcodes186–190的条件读取与加减前读取改为直接调用该入口；四项记录继续由同一VM进程状态owner持有，不建立副本。

调试对话主过程与页填充尚待后续工作包关闭，届时必须直接调用本入口。

## 4. 验证

`special_modes.legacy_initial_menu`使用相互不同的poison值覆盖全部17项、六个signed word、八个unsigned word、两个完整dword、末byte及selector -1/17 default零。既有剧情VM synthetic测试继续覆盖固定第二项记录、signed比较和加减回绕。
