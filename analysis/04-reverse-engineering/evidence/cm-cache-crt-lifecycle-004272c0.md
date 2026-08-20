# CM 缓存未使用全局 owner 的 CRT 生命周期：`0x004272C0`

状态：`sub_4272C0` 已独立完成 `assembly_exact` 与 `platform_adapted`；该函数只包装一个
无业务消费者的全局文件 owner 构造/析构，现代实现以证明性消除替代。原程序动态差分待
统一 oracle。

唯一行为来源：`swd3.exe.lst` 完整汇编。IDA 原型、旧证据和既有 C++ 只用于导航。

## 1. 物理范围与 ABI

本入口没有参数，也没有 callee 清栈：

```text
0x004272C0..0x004272C5  sub_4272C0 主块
0x004272E0..0x004272EB  sub_4272C0 function chunk
0x004272D0..0x004272D5  sub_4272D0 constructor thunk
0x004272F0..0x004272F5  sub_4272F0 destructor thunk
```

主块调用 constructor thunk 后跳入 function chunk。chunk 把 destructor thunk 地址压栈，
调用 `_atexit`，清理一个参数并 `retn`。EAX 保留 `_atexit` 的结果；入口只由
`.data:0x0049E054` 的 CRT initializer 表引用，初始化调度不消费该结果。

## 2. 全局 owner 的精确状态

constructor thunk 以 `ECX = 0x004CF6E0` 尾调用 `sub_438000`。该 helper 对 `0x50` 字节
文件/映射 owner 执行：

```text
+0x00 = 0xFFFFFFFF
+0x04 = 0
+0x08 = 0
+0x0C = 0
+0x10..+0x4F = 16 个零 dword
```

全 LST 对 `0x004CF6E0..0x004CF72F` 的引用只有 constructor thunk 与 destructor thunk；
没有业务读取、写入、取址、打开文件、建立 mapping 或保存 view。下一个全局从
`0x004CF730` 开始，固定该 owner 的物理宽度为 `0x50`。

destructor thunk 以同一地址尾调用 `sub_438030`。由于 `+0x04` 从构造后始终为零且没有
其他写者，析构直接经过其 unopened 分支返回：不调用 `UnmapViewOfFile`、`CloseHandle`
或任何业务 callback。

## 3. 可观察行为与现代映射

`sub_4272C0` 不产生文件、mapping、音频、进度、日志或游戏状态副作用。唯一进程期动作是：

1. 把一个从无消费者的 BSS owner 写成固定 unopened 状态；
2. 注册一个最终只检查该 unopened 状态的 atexit destructor；
3. 留下无人消费的 `_atexit` EAX。

现代 `resource_io::LegacyFile` 已通过 C++ constructor/destructor 与 RAII 覆盖所有有消费者的
文件生命周期。为这个零消费者全局再实例化 owner 只会引入无业务意义的分配和退出注册，
因此 OpenSWD3 明确省略该对象；这属于 dead CRT lifecycle 的平台适配，不是业务逻辑缺口。
实现映射为：

- `src/resource_io/legacy_file.cpp:LegacyFile::LegacyFile`；
- `src/resource_io/legacy_file.cpp:LegacyFile::~LegacyFile`；
- `language_runtime:unused_global_elided`。

## 4. 验证与停止线

静态审计固定：

- 唯一入口引用为 `.data:0x0049E054` CRT initializer 槽；
- `sub_4272D0` 只构造 `0x004CF6E0`；
- `sub_4272F0` 只析构 `0x004CF6E0`；
- 该 `0x50` 字节全局没有第三处引用；
- unopened 析构不进入任何 OS cleanup 分支；
- 相邻 `sub_427300` 不从本文继承关闭状态。

该函数没有可构造的业务 UT 或真实资产输出，不伪报 `unit_tested`/`asset_verified`。最终
完整门禁为 Linux `core` 185/185、Linux `app` 191/191、Windows LLVM `app` 191/191
CTest；两端应用成功链接，且未启动原版或 OpenSWD3 游戏 EXE。
