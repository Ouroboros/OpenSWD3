# MemMgr、TextInput、Random、LZO 核对记录（第 1 遍，2026-10-18）

## MemoryManager 0x00438650..0x004389A0

- 0x00438650 构造：私有堆（初始大小为页大小）、32 个桶清空、预建 32 个空节点（失败时提示 "MemMgrObject node Create Failed"）。原版没有往桶里放块的函数，桶始终为空。通过。
- 0x004386E0 析构：逐桶从头取出未释放的块并报告 "Resource not release ,Size[%d]"、释放块后挂回空节点表（头插）；再从头释放全部空节点；节点数不为 0 时报告 "Resource node not match"；HeapDestroy 失败时报告系统错误。重建以后进先出的容器取放，次序相同。通过。
- 0x00438800 NewNode（计数与峰值）、0x00438840 FreeNode（块损坏时不释放节点、不减计数）、0x00438880 FreeBlock（前后两个 0xABCDDCBA 守护字，损坏时报告块地址、大小与前 16 个用户字节；块地址无法重现，重建记 0）、0x004389A0 FormatLastError：通过。

## TextInput 0x004389D0..0x00438F60

- 0x004389D0 构造（容量 + 1 字节缓冲，复制装得下的整字，光标 0，结果 0，IME 0，插入 1）、0x00438A60 GetResult、0x00438A70 析构、0x00438A80 TextFitLength（CharNextA 逐字累加到超过上限为止）、0x00438AC0/0x00438AD0/0x00438AE0、0x00438AF0 CopyText：通过。
- 0x00438B50 HandleMessage：WM_IME_CHAR 以高、低字节插入（高字节为 0 时为空串），返回 0；WM_CHAR 在 IME 时不处理；等待尾字节（全局 0x004FB7E8）时只接受 < 0x80 的字节，否则丢弃；≥ 0x80 的字节单独插入并等待；≥ 0x1F 的可见字符插入，'%' 播放音效 0x8C 并拒绝；WM_INPUTLANGCHANGE 记 ImmIsIME；WM_KEYDOWN：Backspace 先左移再删除、Enter 1、Escape 2、End/Home 移到底、左右、Insert（参数被忽略，仍为 1）、Delete。通过。
- 0x00438DC0 SetInsertMode、0x00438DD0 GetInsertMode、0x00438DE0 CursorLeft（CharPrevA，到 0 时为假）、0x00438E10 CursorRight、0x00438E50 InsertText（插入装得下的部分，再保留装得下的尾部，光标移到插入之后；非插入模式的错误释放见注释）、0x00438F60 DeleteAtCursor（以 capacity − 下一字位置 + 1 字节前移）：通过。

## Random 0x00438FA0..0x00439110

- 0x00438FA0 Seed：LCG 播种，250 项表依次取 LCG 值，再按另一组 LCG 值 > 0x4000 置位 15，最后 16 项（3, 14, …, 168）设为三角位型。0x00439020 Next（与 147 项前异或，下标到 249 回 0）、0x00439070 Below（每次尝试走两步保留后者，拒绝 ≥ limit × (0xFFFF / limit) 的值）、0x004390F0 LcgNext（× 0x015A4E35 + 1，取位 16..30）、0x00439110 LcgSeed：通过。

## LZO 0x00439120..0x00439C20

- 0x00439120/0x00439580 Compress：输入 > 13 字节时先压核心，尾部字面量（尚无输出且 ≤ 238 时一个首字节，≤ 3 时并入前一码，≤ 18、更长时零字节延长），结束标记 0x11 0 0；长度延长循环以字节运算（−255 ≡ +1 mod 256）。0x00439210/0x00439670 CompressCore：哈希 ((0x21 × (b0 ^ 32(b1 ^ 32(b2 ^ (b3 << 6))))) >> 5) & 0x3FFF/0x7FFF，偏移 > 0x800 且第四字节不同时改查 (h & 0x7FF) ^ 0x201F/0x401F；指针低于输入或偏移 0、> 0xBFFF 为未命中；M2/M3/M4 编码与 9 字节以上的延长。两者只差上述常数；重建由位宽算出，并经原版存档逐字节验证。通过。
- 0x004399E0 Run：标准 lzo1x_decompress，返回 0、输入未用完 −8、越过输入 −4，输出长度写入参数；边界检查见 deviations.md。0x00439C20 RunFast（无调用者）：同上。通过。

## 0x0046E290 TalkCurvePoint

四个点转为 float，先在 0 处求值并丢弃（0x437F90），t = step × 0.05（0x00499D20 为 double，结果存 float），0x437F40 基函数权重 × 0x437EF0 加权，x、y 各 ftol：一致，通过。
