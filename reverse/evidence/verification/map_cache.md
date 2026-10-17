# 地图缓存验证（第 1 遍）

## 0x00425570 MapCache::Check

打开 env.dat 失败时只返回缓存字节数。模式 0：读末字节，非 0 时写回 0 并清空缓存；模式 1：显示报告的掩码（0x004233D0）与 0x49E0B4..B8 不同时，在 +0x1E 写入新掩码并清空缓存。清空：24 个 data\%d.cm 截为 0；data\mcache.dat 写 24 项 {0xFFFFFFFF, 0, 0, i}（0x180 字节）。缓冲 ExistingFileName 即 0x4B8A14，原版借它组装表；mcache.dat 的使用者 0x00426840 每次都从文件读到新分配的缓冲，0x4B8A14 只作路径缓冲，故不可观察。最后返回 0x00425A70。重建（会话标记与掩码在 [game]，偏差表第 75 行）的条件、写入与 Clear 内容一致。exact。

## 0x004258E0 MapCache::MarkClean、0x004259B0 MapCache::MarkDirty

打开 env.dat，定位末字节，分别写 0、1。重建 session_open = false／true。exact。

## 0x00425A70 MapCache::CacheBytes

24 个 data\%d.cm 中存在者的大小之和。一致。exact。
