# `Env.dat` 物理记录与旧布局迁移

状态：B2.7a 物理编解码闭环；`assembly_exact`、`asset_verified`、`platform_adapted`、`blocked_runtime_oracle`

范围：`0x00423AF0..0x00423DF4` 写入器与 `0x00423FB0..0x004242A6` 读取器。文件打开、重开和调用者业务状态接线留在 B2.7b；本单元只固定可独立验证的 4096 字节窗口、两种布局、迁移内容和写回前缀。

来源：`swd3.exe` 完整汇编、内置 CRT `memcpy` 的机器码、唯一读取调用者 `0x00425040`、当前 63 字节 `Env.dat`。IDA 伪码只辅助定位。

## 1. 读取入口与返回值

`0x00423FB0` 接受十个输出指针，唯一调用者为 `0x00425040`。它构造 80 字节文件对象，以 `OPEN_EXISTING + GENERIC_READ` 打开 `Buffer + "Env.dat"`：

- 首次打开失败时返回 0，不清零 16 个按键全局，也不写十个输出指针。
- 首次打开成功后先清零从 `0x004B7384` 开始的 0x80 字节，再分配并清零 0x1000 字节窗口。
- 它以文件大小作为请求长度读取，却不检查文件是否超过 0x1000，也不检查实际读取长度；短读后的剩余窗口仍为零。
- 初始文件已有 `0xFFFFFFFF` 标记时，解析当前布局并返回 1。
- 初始文件没有标记时，执行迁移、从旧记录第一路径下重开 `Env.dat` 并再次读取；第二次打开和读取结果均不检查，最后解析窗口并返回 0。

因此返回 0 同时表示“首次打开失败”和“发生过旧布局迁移”，不是通用失败码。迁移路径即使最终产生了可用输出，也仍返回 0。

## 2. 两种物理布局

当前布局在旧布局前增加四字节标记，其余字段整体后移四字节：

```text
field                         current      legacy
layout marker                +0x00 u32    absent
16 binding bytes             +0x04        +0x00
integer parameter            +0x14 u32    +0x10
six option bytes             +0x18        +0x14
preserved/migration area     +0x1E[16]    +0x1A[16]
primary NUL string           +0x2E        +0x2A
secondary NUL string         variable     variable
consumed trailing mode       next byte    next byte
```

读取器把当前 `+0x04..+0x13` 依次写入以下全局的低字节；前面的 0x80 字节清零使其高三字节保持零：

```text
4B7394 4B739C 4B7390 4B7398 4B7388 4B73B4 4B73A8 4B7384
4B73AC 4B738C 4B73C4 4B73C8 4B73CC 4B73D0 4B73A0 4B73A4
```

当前 `+0x14` 写入第七个输出指针；`+0x18..+0x1D` 分别写入第四、第五、第六、第八、第九和第十个输出指针。`+0x1E/+0x20/+0x22` 的三个小端 word 写入 `0x0049E0B4/0x0049E0B6/0x0049E0B8`；`+0x24` 只取最低位写入 `0x004CAE98`。

两条字符串按原始 ANSI 字节复制，没有容量参数。第二个 NUL 后的第一字节同时写回第一个输出指针；再后面的字节不消费。

## 3. 旧布局迁移

标记不存在时，读取器只从旧布局直接取得 `+0x2A` 开始的两条字符串和尾随模式，然后调用 `0x00423AF0`。在此之前 16 个按键全局已经被清零，所以迁移写回的按键不是旧文件值，而是 16 个零；这是原程序行为，不修复。

迁移写入参数固定为：

```text
integer parameter = 100
six option bytes  = 6, 6, 0x3C, 1, 2, 0x0A
preserved area    = 16 zero bytes
directories       = legacy record strings
trailing mode     = legacy record trailing byte
```

`0x00423AF0` 读取原文件到同样的预清零窗口。标记不存在时，它调用内置 `0x00489EB0 memcpy` 把 `[0, 0xFFC)` 移到 `[4, 0x1000)`。该 CRT 机器码会检测重叠且在目标落入源范围时从尾部反向复制，因此物理结果等同此处所需的 `memmove`，不能按 C 标准未定义行为猜测成前向破坏。

写入器随后覆盖标记、按键、整数、六个选项和字符串；只有旧布局路径清零 `+0x1E..+0x2D`，当前布局更新会保留这 16 字节。它在两条字符串后写“尾随模式、零”，计算长度 `0x32 + len(primary) + len(secondary)`，从文件开头只写这个前缀，不截断原文件。

## 4. 当前资产验证

当前 `Env.dat`：

```text
size   = 63
sha256 = 2c55dddc9a6808afda5d69688f2c27ac268caf2b9155ae82b18596ed593ed9a4
marker = FFFFFFFF
integer parameter = 100
option bytes = 06 06 3C 00 02 0A
primary directory = "E:\\Game\\swd3\\"
secondary directory = ""
post-string bytes = 02 01
```

实现对完整 63 字节样本解码后，读取的尾随模式为 2；重新编码的前 62 字节逐字节相同，最后一字节按写入器变为 0。原文件末字节 1 不在读取路径消费，不能拿它覆盖写入器的固定零。

## 5. C++20 映射与现代边界

- `decode_legacy_environment`：按 0x1000 预清零窗口解析当前或旧布局，保留原始字节顺序。
- `migrate_legacy_environment`：锁定清零按键、清零保留区和七个硬编码迁移值。
- `encode_legacy_environment`：生成 `0x00423AF0` 写出的当前布局前缀，并按 `lstrlenA` 在输入内第一个 NUL 截止。

原程序在超 0x1000 文件、无终止 NUL、尾随字节越过窗口以及过长写入时会越界。现代实现分别返回明确的 codec 状态；这些状态是 `platform_adapted` 内存安全边界，不能冒充原程序的可恢复错误语义。

UT 覆盖当前完整样本、去掉四字节标记的旧布局、迁移清零 BUG、短文件零填充、两条字符串、尾随模式、`lstrlenA` 前缀，以及四类现代越界隔离。Windows LLVM `core`/`app` 均通过 30/30 CTest，`openswd3.exe` 重新链接。原程序动态差分仍登记为 `blocked_runtime_oracle`。

## 6. B2.7b 剩余边界

下一小单元恢复并实现文件级调用顺序：首次 `OPEN_EXISTING`、首次失败不写输出、旧布局写回、按旧第一路径重开、第二次失败仍继续解析、返回 1/0，以及写前缀但不截断。原始 ANSI 路径如何映射到非 Windows 文件系统必须作为平台端口显式处理，不能在物理 codec 中偷偷改写分隔符或编码。
