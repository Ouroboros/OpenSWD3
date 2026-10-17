# tsw／act 档案核对记录

## TswArchive（第 1 遍，2026-10-18）

- 0x004315C0 SetCacheLimit、0x00431A20 FindPicture（先清当前再查）：通过。
- 0x004315D0 GetPicture：首次调用打开六个档案、清空魔法目录与帧表并读魔法目录；已用字节 ≥ 上限时先淘汰；查不到再载入，失败时当前为 0。被调函数参数为 u16。通过。
- 0x00431760 LoadPictureDirect：魔法编号（0x1771..0x2328）先缓存帧表；载入结果不检查，转为 16 位后返回。通过。
- 0x00431960 CloseAll：关闭六个句柄与额外句柄，清缓存与缓冲，魔法槽位、年龄、目录、帧表清零。通过。
- 0x00431A50 ReadMagicDirectory：3000 条 0x2C 字节记录，从 0x1C 起。通过。
- 0x00431AA0 CacheMagicFrames：五个槽位（已有则年龄清零只读头；第一个空槽；否则年龄最大的槽），经过的槽年龄加一；逐帧重读头后跳过 0x24 × 序号读帧记录，偏移改为绝对位置。帧数取 Buffer+6（0x00431B9F）。通过。
- 0x00431C50 CacheLoad：新节点挂到链首后载入，图块编号 0xFFFF 由地图模块读，其他失败时 sub_411F90 "Mem error::tsw:%d pa:%d"（魔法模式不复位），成功后转 16 位，已用字节加图片 +0x10 的大小。差异（diff 轮）：重建用 DebugLog 记录失败，改为 ErrorLog。修正后通过（exact 轮）；+0x10 大小与 data.size() 的对应在 0x00433540 核对。
- 0x00431DF0 CacheFind（键 id | frame << 16，找到移到链首）、0x00431EA0 CacheEvict（链长最大的第一条，从链尾删到低于上限或链空；空链时原版释放链头静态内存，重建跳过）、0x00431F80 CacheClear：通过。
- 0x00433270 OpenFile：GENERIC_READ、FILE_SHARE_READ、OPEN_EXISTING（共享打开为用户要求的平台替换，deviations.md 0x004380B0 等一行）。通过。

## ActArchive（第 1 遍，2026-10-18）

- 0x00432010 SetCacheLimit、0x00433080 SetUseCache：通过。
- 0x00432020 OpenFile：原版不共享；按用户要求共享打开（平台替换，deviations.md 0x004380B0 等一行）。通过。
- 0x00432050 ExtractEntry：序号 ≥ 段数或偏移为 0 时为空；结束为下一个非零偏移，最后一段为条目大小。通过。
- 0x004320E0 Load：先查索引，未命中读目录记录；整条读入后取段，失败时 wsprintfA 0x004A60C0（条目号、段号），sub_411F90，MessageBox(NULL, 文字, "ACT錯誤", 0)。差异（diff 轮）：DebugLog 改为 ErrorLog。修正后通过（exact 轮）。
- 0x004321E0 Run：持有且位置非 0 或 id 为 0 时返回 1；方向、动作、id 变化依次重置（保留旗标 0x80000000 / 0x80000003 / 0x80000003），清零字段与原版一致；取脚本（缓存或直接）；延时未尽减一；逐字执行到 DE，VO 循环、O2 停止，其余操作码先把延时设为帧延时（速度为负时取其低 15 位）；全部 29 个操作码的字段、位宽与“第二个值不前进”的行为一致；持有时位置停在 DE。通过。
- 0x00432A50 Get、0x00432EE0 CacheEvict、0x00432FA0 CacheClear、0x00433010 CloseAll（剩余字节时 MB_ICONHAND | MB_TOPMOST "ACT記憶體管理有誤!!"/"ERR"，返回 IDOK）、0x00433090 IndexInsert（链首，记录偏移 0x2C × entry − 0x10，取第 6、5 个 dword）、0x00433220 IndexClear：通过。
- 0x00432BC0 LoadDirect、0x00432D30 CacheLoad、0x00432E40 CacheFind、0x00433190 IndexLookup：链序号由 idiv 余数取低 16 位（0x00432D41），动作号为负时落在表外。差异（diff 轮）：重建对负动作号会越界访问链表与句柄数组；改为不缓存、载入失败（deviations.md 两行）；CacheLoad 失败日志改为 ErrorLog（sub_411F90 "Act error::actno:%d qq:%d"）。修正后通过（exact 轮）。
