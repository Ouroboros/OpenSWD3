# sample manager 链表、引用计数与回收规格

最后更新：2026-08-09

状态：`assembly_exact`、核心实现及 SDL3 输出接线已完成

完整 LST 是唯一行为真值；IDA 伪码只用于定位。

## 1. 对象状态

原对象中的关键字段为：

| 偏移 | 语义 |
|---:|---|
| `+0x50` | Miles digital driver；`0x004862A0` 原样返回 |
| `+0x54` | driver/sample manager 已初始化；`0x00485CC0` 只检查是否等于一 |
| `+0x58` | sample 播放启用；`0x00485CD0` 只检查是否等于一 |
| `+0x5C` | 请求的 sample handle 数；由 preference 减八后最多钳至 16 |
| `+0x670` | active 单链表头 |
| `+0x678` | free 单链表头 |
| `+0x67C` | 3000×16 SND 运行时表 |

每个 sample 节点只有 8 字节：`+0` 是 backend handle，`+4` 是 next。初始化逐项分配
节点和 handle，成功节点压入 free 头；第一个 handle 分配失败会停止循环，但
`0x00485C08` 仍返回一，且已建立的节点继续有效。

重写以 32 位 node index 和 backend token 代替宿主指针；active/free 头插、弹出和遍历
顺序不变。`initialize_pool()` 接收 waveOut 协商后已经得到的 handle 数；采样格式回退
状态机见 [`legacy-audio-output-004859b0-00485ca6.md`](legacy-audio-output-004859b0-00485ca6.md)。

## 2. 播放 `0x00485CE0`

有效路径依次要求 `+0x54 == 1`、`+0x58 == 1` 和非零 sound ID：

1. 已有 buffer 为空时调用 `0x00486490`；非空时直接取得来参 buffer。
2. 从 free 头弹出一个节点；不存在时直接返回，刚加载的 buffer 不释放。
3. 重新初始化 handle；buffer 以 `RIFF` 开头时调用普通 sample-file，否则调用扩展名
   固定为 `.mp3` 的 named-sample-file，并原样传最后一个辅助参数。
4. setup 失败时释放 buffer、把节点压回 free 头并返回。
5. setup 成功时将对应 SND 槽 `ref_count++`，把该槽 buffer 字段覆盖为当前 buffer；随后
   按 user-data、volume、pan、loop、start 的顺序调用 backend，最后把节点压入 active
   头。

函数所有路径最终都返回零；成功不能改成 true。音量使用 `0x00486260`，声像使用
`0x00486280`。

## 3. 停止与维护

- `0x00485E90`：按 user-data 找到并摘除 active 中第一个匹配节点；调用 end，取得
  sound ID，将槽引用减一；只有新值为零时才释放槽内最后 buffer 并清零字段，最后节点
  压回 free。返回恒为零。
- `0x00485F30`：按 active 头顺序对全部节点执行同一 end/ref-count 回收，然后逐项压入
  free，最终 active 头清零。启用时返回一，否则零。
- `0x00485FE0`：查找第一个匹配 active，音量钳制后仅在命中时提交；返回恒为零。
- `0x00486030`：查找后总是计算声像转换，命中时提交；只要 manager 启用，未命中也
  返回转换后的声像值。
- `0x00486080`：逐 active 查询 status，只有无符号值一或二视为完成。完成节点先从
  active 摘除并压入 free，再按 volume=0、end、ref-count 回收的顺序处理；其他值保留。
- `0x00486160/0x00486190/0x004861B0`：free push/pop 和 active push。
- `0x004861D0/0x00486210`：按 user-data 查找，以及查找并摘除第一个 active 节点。

销毁时，`0x00485C20` 先按 active 头顺序 end/release 节点，再按 free 头顺序做同样
操作；它不对 active 节点逐项减少 SND 引用。随后 `0x00486430` 释放 3000 槽中每个非零
的最后 buffer，再释放表并关闭 SND 文件，最后关闭输出 driver。

## 4. 必须保留的遗留结果

- 无 free 节点时，`0x00485CE0` 已取得的 buffer 泄漏，且 SND 引用不增加。
- 同一 sound ID 重叠播放时，每次都分配新 buffer 并增加同一 ref-count，但槽里只保存
  最后一次 buffer。引用归零只释放最后 buffer；此前 buffer 泄漏。
- setup 失败会释放传入的已有 buffer，不区分它是否由本次调用加载。
- 引用减法为无符号 32 位减一；若 backend user-data 与表状态失配，零可以下溢。
- stop-by-ID 和参数更新只影响 active 头方向遇到的第一个匹配节点。

重写在 manager 存活期间保留上述 token 生命周期和泄漏可见状态；C++ manager 最终
销毁时统一回收进程不再可能观察的孤儿存储。这是宿主内存安全边界，不改变游戏运行期
的槽字段、backend 顺序或播放结果。

## 5. UT 锁定项

fake backend 记录并比较每次 allocate、initialize、file setup、user-data、volume、pan、
loop、start、status、end、release 和最终 output close：

- 16 handle 上限、负数跳过、部分分配仍成功及二次初始化早退；
- RIFF 与固定 `.mp3` 两条 setup 路径及完整参数顺序；
- setup 失败回滚和无 free 节点泄漏；
- 重叠播放 ref-count/最后 buffer 缺陷；
- 无效 sound ID 安全隔离和失配 user-data 导致的无符号引用下溢；
- 单项停止、全部停止、音量/声像未命中返回；
- status 1/2 回收、其他 status 保留；
- 销毁时 active 先于 free 的 end/release、SND close、output close 顺序。

当前总回归为 Linux Clang core 69/69、Linux Clang app 与 Windows LLVM app 73/73
CTest 通过。测试未启动原版 EXE。
