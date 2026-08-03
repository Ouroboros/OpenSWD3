# 旧运行目录探测与创建 `0x00425150`

状态：B2.12 已实现；`assembly_exact`、`platform_adapted`、`blocked_runtime_oracle`

完整汇编是唯一行为真值。IDA 伪码和符号名只用于定位。

证据输入：

```text
swd3.exe.asm sha256 = f1ee7f32a79c156b75837e176abb733df7a2143d134252e507343e36857affb4
swd3.exe.lst sha256 = 701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b
```

## 1. 调用范围与路径构造

`0x00425040` 先用 `GetCurrentDirectoryA` 取得启动工作目录并在全局 `Buffer` 末尾追加反斜杠。载入 `Env.dat` 并执行首次 CM 缓存验证后，它按固定顺序以以下五个字符串调用 `0x00425150`：

```text
Save
Data
Music
ScrnShot
Video
```

`0x00425150` 用 `wsprintfA(ExistingFileName, "%s%s", Buffer, child)` 形成目标。分隔符来自调用者预先追加的反斜杠，函数自身不插入分隔符。每次调用都重新使用固定的 `Buffer`，不使用上一次调用可能改变的当前目录继续拼接。

## 2. 精确控制流

函数首先调用 `SetCurrentDirectoryA(target)`。成功时不调用创建 API，目标目录成为进程当前工作目录。

失败时，它在栈上构造 x86 的 12 字节 `SECURITY_ATTRIBUTES`：`nLength=12`、`lpSecurityDescriptor=null`、`bInheritHandle=0`，然后只调用一次 `CreateDirectoryA(target, &attributes)`。创建结果不检查，也不重新调用 `SetCurrentDirectoryA`。因此目标原先不存在但创建成功时，进程当前目录仍保持调用前的值；父目录缺失、目标是普通文件或创建失败时也继续执行。

所有路径最终都令 `EAX=1` 后返回。这个返回值不能改成实际的目录选择或创建结果。

## 3. 现代实现边界与验证

`legacy_select_or_create_directory` 接受已经由调用层解析的显式路径，以 `std::filesystem` 隔离 ANSI API 和平台分隔符差异；现有目录时切换当前目录，失败时只尝试创建单层目录，不重试切换，并恒返回 `true`。文件系统错误只用于选择原控制流，不泄漏成新的返回合同。

UT 覆盖现有目录被选中、缺失目录被创建但不选中、普通文件目标失败、父目录缺失时不递归创建，以及全部路径恒返回一。当前单元只恢复 `0x00425150` 的可观察合同；`0x00425040` 的聚合启动接线要与其余路径/环境初始化按原顺序共同接入，不能为了立即使用本 helper 改变当前数据根目录平台边界。Windows LLVM `core`/`app` 的 32/32 CTest 均已通过；原程序动态目录轨迹仍登记为 `blocked_runtime_oracle`。
