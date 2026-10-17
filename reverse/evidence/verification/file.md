# File（cFile）核对记录

## 0x00438000..0x00438640（第 1 遍，2026-10-18）

Win32 文件句柄与内存映射由 stdio 与读入缓冲模拟（UTF-8 路径、共享打开为用户要求的平台替换，deviations.md 0x004380B0 等一行）。

- 0x00438000 构造（句柄 −1，无映射与视图，错误文字清零）、0x00438030 析构（视图或映射释放失败时直接返回，文件保持打开）：通过。
- 0x004380B0 Open：先关旧句柄，清只读属性，打开后记文件名并定位到开头；失败时记错误文字返回 0。通过。
- 0x00438150 Close：先释放视图与映射，失败返回 0；关闭后清文件名与错误文字、句柄 −1。通过。
- 0x004381E0 CreateMapping：无句柄返回 0；PAGE_READONLY，大小取 GetFileSize。差异（diff 轮）：0 字节文件的 CreateFileMappingA 必然失败，重建总是成功；改为空文件时失败并记错误。修正后通过（exact 轮）。
- 0x00438230 CloseMapping、0x00438290 MapView（先释放旧视图，FILE_MAP_READ，调用方的偏移都是 0）：通过。
- 0x004382E0 UnmapView：只释放当前视图，否则 sub_411F90 "cFile ,CloseView Src/Dest difference " 并返回 0。差异（diff 轮）：重建用 DebugLog，改为 ErrorLog。修正后通过（exact 轮）。
- 0x00438340 Size（无句柄时返回 −1）、0x00438360 Truncate、0x00438380 Read 与 0x004383E0 Write（句柄、缓冲为空或请求 0 字节时失败；失败时字节数清 0）、0x00438440 Seek（负距离的高位为 −1，无句柄返回 0，失败返回 −1）、0x004384A0/0x004384B0/0x004384C0（结果加一）、0x004384D0 Tell、0x00438520 WriteExact（写入数须相等）、0x00438570 LastWriteTime、0x004385A0 WriteByte、0x004385B0 WriteDword、0x004385C0 ReadExact（短读也成功）、0x00438600 ReadByte、0x00438610 ReadDword、0x00438620 SetError（0x40 字节）、0x00438640 ErrorText：通过。
