# 地图缓存验证（第 1 遍）

## 0x00425570 MapCache::Check

打开 env.dat 失败时只返回缓存字节数。模式 0：读末字节，非 0 时写回 0 并清空缓存；模式 1：显示报告的掩码（0x004233D0）与 0x49E0B4..B8 不同时，在 +0x1E 写入新掩码并清空缓存。清空：24 个 data\%d.cm 截为 0；data\mcache.dat 写 24 项 {0xFFFFFFFF, 0, 0, i}（0x180 字节）。缓冲 ExistingFileName 即 0x4B8A14，原版借它组装表；mcache.dat 的使用者 0x00426840 每次都从文件读到新分配的缓冲，0x4B8A14 只作路径缓冲，故不可观察。最后返回 0x00425A70。重建（会话标记与掩码在 [game]，偏差表第 75 行）的条件、写入与 Clear 内容一致。exact。

## 0x004258E0 MapCache::MarkClean、0x004259B0 MapCache::MarkDirty

打开 env.dat，定位末字节，分别写 0、1。重建 session_open = false／true。exact。

## 0x00425A70 MapCache::CacheBytes

24 个 data\%d.cm 中存在者的大小之和。一致。exact。

## 0x004267E0 MapCache::Pack、0x00426820 MapCache::Unpack

Pack：输出缓冲 原长 + 0x20、工作区 0x10000，LZO1X-1（0x00439120，14 位字典），输出长度经第三参数返回；最坏情况超出缓冲见偏差表第 55 行。Unpack：0x004399E0(源, 源长, 目标, &目标长)，边界见偏差表第 8 行。exact。

## 0x00426840 MapCache::GetCachedMap

打开 data\mcache.dat（OPEN_ALWAYS）失败时记 "GetCachedMap ,mcache.dat can't open" 返回 0。空文件：0x00426DF0 解到单元 0，表 [0] = {地图, 解包大小, 0, 0}、[1..23] = {−1, 0, 0, i}，写 0x180 字节并截断，打开单元 0。否则读入表（0x200 缓冲），每项年龄 + 1；依次累加大小找地图：命中时先 0x00427140 打开其单元、年龄清 0、写回 项数 × 16 字节；未命中：0x004270F0 取所需大小，无空项时视为满；当 (已用 + 所需 > 缓存 MB << 20 且项数非 0) 或满时淘汰：取大小最小的项，遇到年龄 > 缓存 / 4（无符号）的项立即取它；该项清为 {−1, 0, 0, 序号}，其单元文件截到 0x10，已用 −= 其大小；找第一个空项，没有时记 "GetCachedMap ,No free slot"；写入 {地图, 所需, 0, 序号}，写表并截断，解压后打开。第四参数（地图头）在被调用者中不使用（0x00426DF0 以 0x4B7938 判断位深），重建传位深，值相同。
- 差异（diff 轮）：缺两条错误日志。已补（文字取自 0x004A1244、0x004A1228）。复核一致（exact 轮）。

## 0x00426DF0 MapCache::DecompressMap

尺寸为 0 时 nullsub_1(「地圖快取製作」, 「這地圖是舊格式」)；打开 data\<单元>.cm 失败时记 "DecompressMap ,%s can't open"；定位 lmf 到 尺寸 + 基址，读 0x1A8 字节头（失败则关闭返回）：+0x10 解包总长写回，+0x14 块长；两个缓冲 块长 + 块长 >> 10；块数 (块长 + 总长) / 块长；逐块：长度取 +0x1C + 8k，读取失败记 "DecompressMap ,read block fail when " 并结束；解包，写入 min(块长, 剩余)（位深 0x10 时先转换 字节 / 2 个像素），剩余 −= 块长，进度 (步 / 块数 + 15)，步 += 0x2D。块长为 0 时原版除零，块表越出 0x1A8 时读头外数据，重建分别返回／停止。
- 差异（diff 轮）：缺调试日志与两条错误日志。已补。复核一致（exact 轮）。

## 0x004270F0 MapCache::ReadUnitBytes

尺寸为 0 时同上 nullsub_1；否则读 基址 + 尺寸 + 0x10 处的 dword。
- 差异（diff 轮）：缺调试日志。已补。复核一致（exact 轮）。

## 0x00427140 MapCache::OpenUnit

释放旧视图、映射、文件，清当前路径 0x4A9ACC；以只读 OPEN_ALWAYS 打开 data\<单元>.cm，失败记 "GetCacheMapUnit ,can't open %s" 返回 0；建映射失败记 "GetCacheMapUnit ,can't createmapping:%s ,"、建视图失败记 "GetCacheMapUnit ,can't createview:%s ,"（两者格式化的都是路径）；记下路径，返回视图。
- 差异（diff 轮）：缺三条错误日志。已补（文字取自 0x004A133C、0x004A1310、0x004A12E8）。复核一致（exact 轮）。
