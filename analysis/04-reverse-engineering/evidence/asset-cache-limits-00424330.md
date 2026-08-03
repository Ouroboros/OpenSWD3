# 资产缓存容量策略 `0x00424330`

状态：B2.9 归属闭环；`assembly_exact`、`platform_adapted`，实现延后至 B6 `asset_runtime`

完整汇编是唯一行为真值。IDA 伪码和符号名只用于定位。

证据输入：

```text
swd3.exe.asm sha256 = f1ee7f32a79c156b75837e176abb733df7a2143d134252e507343e36857affb4
swd3.exe.lst sha256 = 701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b
```

## 1. 容量计算

`0x00424330..0x00424386` 建立 32 字节 `MEMORYSTATUS`，把 `dwLength` 写成 `0x20`，再调用 `GlobalMemoryStatus`。函数只读取 32 位 `dwTotalPhys`。

随后执行：

```text
EAX = 0xAAAAAAAB
EDX:EAX = EAX * dwTotalPhys
EDX >>= 2
```

该无符号乘高位序列精确等于 `floor(dwTotalPhys / 6)`。结果再夹到闭区间 `[0x00400000, 0x01000000]`，即 4 MiB 至 16 MiB。

计算值传给 `0x004315C0`。之后函数无条件把 `0x00080000`，即 512 KiB，传给 `0x00432010`，最终恒返回 1。两个 setter 都只把参数写入一个全局 dword；调用者 `0x004250E2` 不读取返回值。

## 2. 状态消费者

`0x004315C0` 写入 `0x004A6020`。该值只在 `0x004315D0..0x00431F7F` 的 TSW 资产缓存族读取，并与当前累计字节数 `0x004DAD0C` 比较；达到上限时调用 `0x00431EA0` 淘汰缓存节点。EXE 初始值为 6 MiB，启动调用会覆盖它。

`0x00432010` 写入 `0x004A6098`。该值只在 `0x00432A50..0x00432F96` 的 ACT 资产缓存族读取，并与当前累计字节数 `0x004FB304` 比较；达到上限时调用 `0x00432EE0` 淘汰缓存节点。EXE 初始值为 1 MiB，启动调用固定覆盖成 512 KiB。

函数本身没有文件、路径、容器或字节流操作。它仅为两个资产缓存发布容量策略，因此所有者是 `asset_runtime`，不是调用位置相邻的 `resource_io`。

## 3. 现代实现边界

B6 实现时把行为拆成两个明确边界：平台层提供与旧 `MEMORYSTATUS.dwTotalPhys` 对应的 32 位物理内存样本，`asset_runtime` 保留除以 6、4–16 MiB 夹取、512 KiB 固定上限、发布顺序和恒成功返回值。

当前只修正函数归属和后续实现合同，不在 B2 创建无消费者的容量状态，也不提前实现 TSW/ACT 淘汰器。原程序动态差分待 B6 一并完成。
