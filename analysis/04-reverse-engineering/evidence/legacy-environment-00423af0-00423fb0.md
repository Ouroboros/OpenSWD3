# `Env.dat` 带标记/无标记记录与迁移

状态：B2.7a/B2.7b 与收口写入入口闭环；`assembly_exact`、`asset_verified`、`platform_adapted`、`blocked_runtime_oracle`

范围：`0x00423A10..0x00423AE7` 创建包装、`0x00423AF0..0x00423DF4` 写入器、`0x00423E00..0x00423FAA` 按键前缀写入器、`0x00423FB0..0x004242A6` 读取器，以及 `0x00424390..0x00424430` 默认按键初始化器。调用者业务状态接线不属于本单元。

来源：`swd3.exe`、去除 FLIRT 名称后的 `swd3.exe.asm`、IDA 反汇编视图 1:1 导出的 `swd3.exe.lst`、内置 CRT `memcpy` 的机器码、唯一读取调用者 `0x00425040`、当前 63 字节 `Env.dat`。ASM SHA-256 为 `f1ee7f32a79c156b75837e176abb733df7a2143d134252e507343e36857affb4`，LST SHA-256 为 `701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b`。IDA 伪码只辅助定位。

## 1. 读取入口与返回值

`0x00423FB0` 接受十个输出指针，唯一调用者为 `0x00425040`。它构造 80 字节文件对象，以 `OPEN_EXISTING + GENERIC_READ` 打开 `Buffer + "Env.dat"`：

- 首次打开失败时返回 0，不清零 16 个按键全局，也不写十个输出指针。
- 首次打开成功后先清零从 `0x004B7384` 开始的 0x80 字节，再分配并清零 0x1000 字节窗口。
- 它以文件大小作为请求长度读取，却不检查文件是否超过 0x1000，也不检查实际读取长度；短读后的剩余窗口仍为零。
- 初始文件已有 `0xFFFFFFFF` 标记时，按带标记布局解析并返回 1。
- 初始文件没有标记时，执行迁移、从无标记记录第一路径下重开 `Env.dat` 并再次读取；第二次打开和读取结果均不检查，最后强制按带标记偏移解析窗口并返回 0。

因此返回 0 同时表示“首次打开失败”和“发生过无标记布局迁移”，不是通用失败码。迁移路径即使最终产生了可用输出，也仍返回 0。

## 2. 两种物理布局

文件开头是否为 `0xFFFFFFFF` 决定读取偏移。这里仅按可证明的物理差异命名为“带标记”和“无标记”；现有汇编与单个游戏数据样本不能证明它们分别属于哪个历史发行版。无标记布局可能来自更早版本，但当前只登记为 `hypothesis_only`。同一 EXE 的 `0x00423E00` 也会直接在文件起点写 16 个按键字节，因此不能把无标记前缀简单等同于“旧版格式”。

```text
field                         marked       unmarked
layout marker                +0x00 u32    absent
16 binding bytes             +0x04        +0x00
integer parameter            +0x14 u32    +0x10
six option bytes             +0x18        +0x14
cache pixel masks            +0x1E[6]     +0x1A[6]
remaining migration area     +0x24[10]    +0x20[10]
primary NUL string           +0x2E        +0x2A
secondary NUL string         variable     variable
consumed trailing mode       next byte    next byte
```

读取器把当前 `+0x04..+0x13` 依次写入以下全局的低字节；前面的 0x80 字节清零使其高三字节保持零：

```text
4B7394 4B739C 4B7390 4B7398 4B7388 4B73B4 4B73A8 4B7384
4B73AC 4B738C 4B73C4 4B73C8 4B73CC 4B73D0 4B73A0 4B73A4
```

带标记记录的 `+0x14` 写入第七个输出指针；`+0x18..+0x1D` 分别写入第四、第五、第六、第八、第九和第十个输出指针。`+0x1E/+0x20/+0x22` 的三个小端 word 写入 `0x0049E0B4/0x0049E0B6/0x0049E0B8`，并由 `0x00425570(1)` 与当前 surface 的三个 32 位 RGB mask 比较；`+0x24` 只取最低位写入 `0x004CAE98`。因此最前六字节已确定为 CM 缓存像素格式签名，只有 `+0x24..+0x2D` 的完整业务命名仍未闭环。

两条字符串按原始 ANSI 字节复制，没有容量参数。第二个 NUL 后的第一字节同时写回第一个输出指针；再后面的字节不消费。

文件中的 16 字节是独立的序列化顺序，不是进程内连续布局。运行时实际是 `0x004B7384..0x004B7403` 的 0x80 字节兼容块，含 16 个稀疏 dword 字段；读取器先清零完整 0x80 字节，再只写各字段低字节，而设置界面会复制完整 0x80 字节。精确默认值、文件顺序与复制合同见 [`default-key-bindings-00424390.md`](default-key-bindings-00424390.md)。

## 3. 无标记布局迁移

标记不存在时，读取器只从无标记布局直接取得 `+0x2A` 开始的两条字符串和尾随模式。虽然首次成功打开后已经清零完整 0x80 字节按键块，但 `0x004240B0` 在调用写入器前会调用 `0x00424390`，把 16 个按键 dword 恢复为默认值。因此迁移写回的不是原文件按键，也不是零，而是固定默认键序列：

```text
C8 D0 CB CD 39 1C 9D 01 CF 36 13 1E 22 3B C9 D1
```

迁移写入参数固定为：

```text
integer parameter = 100
six option bytes  = 6, 6, 0x3C, 1, 2, 0x0A
pixel masks and remaining migration area = 16 zero bytes
directories       = legacy record strings
trailing mode     = legacy record trailing byte
```

`0x00423AF0` 读取原文件到同样的预清零窗口。标记不存在时，它调用内置 `0x00489EB0 memcpy` 把 `[0, 0xFFC)` 移到 `[4, 0x1000)`。该 CRT 机器码会检测重叠且在目标落入源范围时从尾部反向复制，因此物理结果等同此处所需的 `memmove`，不能按 C 标准未定义行为猜测成前向破坏。

写入器随后覆盖标记、按键、整数、六个选项和字符串；只有无标记迁移路径清零 `+0x1E..+0x2D`，带标记布局更新会保留这 16 字节。它在两条字符串后写“尾随模式、零”，计算长度 `0x32 + len(primary) + len(secondary)`，从文件开头只写这个前缀，不截断原文件。

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

实现对完整 63 字节样本解码后，读取的尾随模式为 2；重新编码的前 62 字节逐字节相同，最后一字节按记录写入器变为 0。`0x00423FB0` 不消费原文件末字节 1，但 `0x00425570` 会独立读取它；`0x004259B0` 在游戏会话开始时写 1，`0x004258E0` 在正常退出时写 0。完整生命周期见 [`environment-cache-session-marker-00425570-004259b0.md`](environment-cache-session-marker-00425570-004259b0.md)。

## 5. C++20 映射与现代边界

- `decode_legacy_environment`：按 0x1000 预清零窗口解析带标记或无标记布局，保留物理文件字节顺序。
- `migrate_unmarked_environment`：锁定默认按键、清零保留区和七个硬编码迁移值。
- `encode_legacy_environment`：生成 `0x00423AF0` 写出的带标记前缀，并按 `lstrlenA` 在输入内第一个 NUL 截止。
- `rewrite_legacy_environment`：以同一 `OPEN_EXISTING + GENERIC_READ|GENERIC_WRITE` 句柄读取原记录；带标记时只从现有文件保留 `+0x1E..+0x2D`，无标记时清零该区，随后回到偏移零写新前缀且不截断。
- `initialize_legacy_environment`：对应 `0x00423A10`，先以 `OPEN_ALWAYS + GENERIC_WRITE` 确保文件存在并关闭，再调用上述重写器重新打开。
- `write_legacy_environment_binding_prefix`：对应 `0x00423E00`，以 `OPEN_EXISTING + GENERIC_WRITE` 从偏移零写 16 字节，不截断；因此它会覆盖四字节标记及随后的 12 字节，而不是只写带标记布局的 `+0x04` 绑定区。
- `load_legacy_environment`：保留首次打开、迁移写回、按旧第一路径重开、第二次失败仍解析旧窗口、原始 1/0 返回值和不截断文件的调用顺序；原始 ANSI 路径通过显式平台 resolver 映射。
- `write_legacy_environment_cache_session_marker`：按 `0x004258E0/0x004259B0` 只覆盖物理末字节，分别写正常退出零和会话活动一。

原程序在超 0x1000 文件、无终止 NUL、尾随字节越过窗口以及过长写入时会越界。现代实现分别返回明确的 codec 状态；这些状态是 `platform_adapted` 内存安全边界，不能冒充原程序的可恢复错误语义。

UT 覆盖当前完整样本、合成无标记记录、迁移默认按键、短文件零填充、两条字符串、尾随模式、`lstrlenA` 前缀、首次打开失败、迁移写回不截断、按存储路径重开、第二次失败继续解析旧窗口、带标记时保留现有 16 字节、无标记时清零、缺文件创建包装、偏移零的 16 字节覆盖、会话标记写入，以及现代越界隔离。Windows LLVM `core`/`app` 的 34/34 CTest 均已通过，`openswd3.exe` 已重新链接。原程序动态差分仍登记为 `blocked_runtime_oracle`。

## 6. B2.7b 文件级边界

文件级实现已经覆盖首次 `OPEN_EXISTING`、首次失败不写输出、无标记布局写回、按无标记记录第一路径重开、第二次失败仍继续解析、返回 1/0，以及写前缀但不截断。收口审计另外补齐 `0x00423A10` 的“创建后重开”顺序、`0x00423AF0` 对原文件保留区的来源规则和 `0x00423E00` 的文件起点覆盖语义。原始 ANSI 路径如何映射到非 Windows 文件系统由调用者提供平台 resolver；物理 codec 不改写分隔符或编码。
