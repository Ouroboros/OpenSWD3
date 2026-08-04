# 资源数据库初始化与独占文件探测

状态：B2 收口闭环；`assembly_exact`、`asset_verified`、`platform_adapted`、`blocked_runtime_oracle`

范围：`0x0040F040..0x0040F156` 与 `0x00412050..0x004120A0`。完整汇编是唯一行为真值；IDA 名称和伪码只用于定位。

## 1. `0x0040F040` 数据库初始化

函数按固定顺序把当前根路径与 `maps.dat`、`path.dat`、`talk1.dat` 拼接，并以 `OPEN_EXISTING + GENERIC_READ` 打开三个全局文件对象：

1. `maps.dat` 打开失败时报告 `RoleDataBase init Failed.`，同步请求销毁并停止。
2. `path.dat` 打开失败时报告 `PathDataBase init Failed.`，同步请求销毁并停止；已打开的 maps 对象保持原状态。
3. path 打开成功后建立只读映射，并以偏移零、长度零映射完整文件。函数不检查映射创建或视图结果，仍继续打开 talk。
4. `talk1.dat` 打开失败时报告 `StoryDataBase init Failed.`，同步请求销毁；此前文件和 path 映射保持原状态。全部成功时直接返回零。

对应实现为 `LegacyResourceDatabases::initialize`。三个 `LegacyFile` 与 path 视图由对象持有，失败路径不伪造事务回滚。SDL 启动适配器在原初始化时点调用该入口，并把三种失败映射到日志和销毁请求。

Win32 文件名匹配不区分 ASCII 大小写；当前资产名为大写。非 Windows 后端在所选数据根的直属文件中执行 ASCII 大小写等价解析，这是 `platform_adapted` 文件系统隔离，不改变原路径顺序或失败合同。

## 2. `0x00412050` 文件可独占打开探测

该函数只调用一次 `CreateFileA`：访问权限为零、共享模式为零、创建方式为 `OPEN_EXISTING`，标志 `0x10000080` 为普通文件加随机访问。失败时把系统错误格式化到全局 128 字节缓冲并返回零；成功时立即关闭句柄并返回一。它不读取文件内容，也不创建文件。

对应实现为 `legacy_exclusive_file_probe`。Windows 后端保留上述 `CreateFileW` 参数，并在失败时按原 `FORMAT_MESSAGE_FROM_SYSTEM` 合同最多写 128 字节错误文本；调用者提供的缓冲对应原全局缓冲边界。非 Windows 后端只提供“现有文件可打开”的兼容探测和宿主错误文本，无法表达 Win32 share mode，因此标记为 `platform_adapted`。原调用者属于媒体获取流程，已移交 `audio_video`，但通用探测函数仍由 B2 交付。

## 3. 验证

合成测试覆盖 maps/path/talk 三个失败停止点、失败后的部分状态保留、完整 path 映射、空 path 映射失败仍继续、大小写不同的真实 Windows 文件名，以及现有/缺失文件探测。

当前真实资产验证结果：

```text
MAPS.DAT  size=162929  sha256=a6faee71c0cb41c1be94f29152deed12ed19eb3e1842fb0f10e13e00132a20ba
PATH.DAT  size=23114   sha256=8b295815fc2e311fde2caf0557cfe3de137b12105339fc797428f99ddfae47a3
TALK1.DAT size=371450  sha256=e85520a8158ec9d01364f3b00dde7965f3dd07c5d34829f380ce8d446cf38b6f
PATH prefix=4D 5A 4A 00
```

Windows LLVM `core` 与 `app` 均通过 34/34 CTest；真实三数据库打开和完整 path 视图验证通过。原程序错误弹窗、同步 `WM_DESTROY` 与全局对象地址的动态差分仍为 `blocked_runtime_oracle`。
