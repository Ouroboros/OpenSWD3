# `.cm` 缓存总长度 `0x00425A70`

状态：B2.8 闭环；`assembly_exact`、`asset_verified`、`platform_adapted`、`blocked_runtime_oracle`

完整汇编是唯一行为真值。IDA 伪码和符号名只用于定位。

证据输入：

```text
swd3.exe.asm sha256 = f1ee7f32a79c156b75837e176abb733df7a2143d134252e507343e36857affb4
swd3.exe.lst sha256 = 701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b
```

## 1. 汇编合同

`0x00425A70..0x00425B31` 将累计值和槽号都清零，随后以无符号比较固定循环 24 次。每次构造一个 80 字节文件对象，以 `OPEN_EXISTING + GENERIC_READ` 打开：

```text
Buffer + "data\\" + decimal(slot) + ".cm"
```

槽号严格为 `0..23`，没有前导零。打开成功时调用 `0x00438340` 取得 32 位文件长度并直接加到累计值；打开失败时不读取长度，该槽贡献零。两条路径都会调用 `0x00438150` 关闭文件，再析构并释放文件对象。

累计使用 x86 `add dword`，因此按无符号 32 位回绕。文件长度查询的 `0xFFFFFFFF` 也没有单独错误分支；只要打开成功，该值就照常参与相加。函数最后原样返回累计值。

唯一直接调用点 `0x004258C4` 位于 `0x00425570` 的共同返回尾部，因此该值同时成为 `0x00425570` 的返回值。`0x00424C22` 的调用者随后执行 `(size + 0x000FFFFF) >> 20`，把缓存总字节数向上取整为 MiB；启动路径 `0x004250E9` 则忽略返回值。

## 2. C++20 映射

`legacy_cm_cache_total_size` 接受显式缓存目录，依次构造 `0.cm` 到 `23.cm` 的跨平台路径，并复用 B2.3a 的 `LegacyFile` 完成原始打开、长度查询和关闭顺序。目录的大小写和路径分隔符由调用者的平台路径边界负责；核心不在字符串内硬编码 Windows 反斜杠。

累计类型固定为 `compat::u32`，保留原始回绕。实现不解析 `.cm` 内容，也不读取 `mcache.dat`，所以没有提前接管地图缓存业务语义。

## 3. 验证

UT 覆盖目录不存在、部分槽缺失、空文件、精确的 24 个编号，以及 `24.cm`、`01.cm` 和 `mcache.dat` 不参与累计。

当前真实 `Data` 目录的结果为：

```text
0.cm  = 0x00389000 = 3,706,880 bytes
1.cm  = 0x000AE400 =   713,728 bytes
2.cm..23.cm        =         0 bytes
total = 0x00437400 = 4,420,608 bytes
```

Windows LLVM `core`/`app` 的 32/32 CTest 均已通过，`openswd3.exe` 已重新链接，测试程序对当前 24 个真实文件也返回成功。原程序动态捕获后端仍不可用，因此不标记 `original_diff_verified`。
