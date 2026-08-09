# stream manager、音乐 wrapper 与过渡状态规格

最后更新：2026-08-09

状态：`assembly_exact`；核心与 fake-backend UT 已完成

完整 LST 是唯一行为真值；IDA 伪码只用于定位。实现和验证均未启动原版 EXE。

## 1. 对象与节点布局

原 stream manager 位于 `0x004C9288`。关键字段为：

| 偏移 | 语义 |
|---:|---|
| `+0x00` | Miles digital driver token |
| `+0x08` | manager 已初始化；判断时必须等于一 |
| `+0x0C` | stream 启用；fade 入口判断时必须等于一 |
| `+0x10` | `AIL_last_error()` 文本副本 |
| `+0x424` | active 单链表头 |
| `+0x43C` | free 单链表头 |

`0x004865B0` 每次调用都写入 driver、把两个标志设为一，再分配两个 24 字节节点并逐项
压入 free 头。它不查询 backend，也不因第二次初始化而早退。原节点布局为：

| 偏移 | 语义 |
|---:|---|
| `+0x00` | backend stream handle |
| `+0x04` | 独立分配的文件名副本 |
| `+0x08` | 每次 service 的 fade 减量 |
| `+0x0C` | `clamp(volume, 0, 127) << 4` 的 fixed-point 音量 |
| `+0x10` | 状态位；`0x80000000` 表示 fade 中 |
| `+0x14` | next |

重写用 32 位 node index 代替宿主指针，但保留两个头插单链表、两节点初始容量和每次
初始化再增加两个节点的行为。

## 2. 播放、查询与音量

`0x00486730` 不检查 initialized 或 enabled，只依次执行：

1. stream ID 为零或 active 中已有同 ID 时返回零；ID 由 backend user-data slot 0 查询。
2. 从 free 头弹出节点；没有节点时返回零。
3. 只清零 filename 和 fade-step，再以 offset 零调用 backend open。
4. open 失败时复制 backend error，把节点压回 free 头并返回零。
5. open 成功后复制文件名，写入 fixed-point 音量，依次提交 user-data、volume、loop、
   start，把节点压入 active 头并返回原 stream ID。

`0x004866C0` 只要求 initialized。命中 ID 后，它先后两次调用音量转换：第一次结果左移
四位保存，第二次结果提交 backend，随后读取并返回 backend 当前音量。未初始化返回零，
未命中返回负一；enabled 不参与判断。

`0x00486A10` 返回 ID 是否不存在。`0x00486A30/0x00486A50` 分别弹出 free 头和压入
active 头；`0x00486A70` 每个节点都通过 backend user-data slot 0 比较 ID。

## 3. fade 与逐帧 service

`0x00486860` 要求 initialized、enabled、非零 ID 且命中 active。它查询一次 stream
毫秒位置但忽略两个输出，把节点 `+0x10` 的高位置一，再执行：

```text
fade_step = fixed_volume / divisor
if fade_step == 0: fade_step = 1
```

除法是 signed、向零截断。divisor 为零会在原 `idiv` 产生整数除法异常；重写保留为
`SIGFPE`，不把它修复成默认值。

`0x00486900` 每帧按 active 头顺序遍历，返回恒为零：

- fade 节点先做 32 位回绕减法，再把 fixed-point 音量向零除以 16 并钳至
  `0..127`；非零时提交新音量并保留，零时回收。
- 非 fade 节点查询 backend status。status 2 先提交一次 volume 0，再进入通用回收，
  因此同一节点会提交两次 volume 0。status 4、8、16 保留；其他 status 走默认分支。
- 原汇编用 `EBX` 记录“前一个节点刚被回收”。默认 status 在该标志为一时也会被回收；
  任意保留节点把标志清零。这是原程序的级联回收缺陷，不得改成独立判断每个节点。
- 回收依次执行 volume 0、close、释放 filename，清零 handle/filename/fade-step/state，
  但不清零 fixed-point 音量；随后从 active 摘除并压入 free 头。

遍历使用 `this+0x410` 作为物理哨兵，因为该地址的 `+0x14` 正好别名 active 头
`this+0x424`。重写以显式 previous index 得到同一摘链顺序。

## 4. 七个游戏侧 stream wrapper

固定音乐 stream ID 为 100。音量缩放与 sample wrapper 相同：先做 32 位回绕左移七位，
再 signed 向零除以十一。

| 地址 | 行为 | 返回 |
|---:|---|---:|
| `0x004856C0` | gate 3 为零时早退；否则以 ID 100、缩放后的全局 mix level、loop 1 播放文件 | gate 零时 0，否则固定 1 |
| `0x00485710` | `begin_fade(100, 1)` | manager 结果 |
| `0x00485830` | 查询 ID 100 是否不存在 | 不存在 1，存在 0 |
| `0x00485850` | 缩放来参后 `set_volume(100, value)` | backend 音量、未命中 -1、未初始化 0 |
| `0x00485880` | 按过渡 mode 执行立即 fade 或指定 divisor fade | 保留原 mode 分支返回 |
| `0x004858D0` | mode 2 时轮询 ID 100；不存在则清零 mode/current | 低位逻辑为不存在 1、存在 0；其他 mode 返回 `mode-2` |
| `0x00485910` | 总是写 mode；仅 mode 2 时写 pending divisor | mode 2 返回 divisor，否则返回 mode |

`0x00485880` 的精确 mode 行为为：mode 1 用 divisor 1 发起 fade 后清零 mode/current 并
返回零；mode 2 用 pending divisor 发起 fade、把 pending 复制到 current 并返回 manager
结果；其他值返回 32 位回绕的 `mode-2`。`0x004858D0` 对 mode 0 返回零，对非零且非二
返回 `mode-2`；mode 2 只在 stream 已不存在时清零状态。

## 5. 销毁与平台安全边界

析构只在 initialized 为一时工作。它先按 active 头顺序释放 filename、close handle 和
节点，再对 free 链表执行相同步骤；因此初始或已回收的 free 节点仍会调用
`close_stream(0)`。最后把 initialized/enabled 清零并返回一。

原 `0x004865B0` 在节点分配失败后会立即解引用空指针；原 `0x00486730` 在文件名分配
失败后也会继续复制。重写只在这两个宿主内存安全边界返回失败；若 open 已成功而文件名
分配失败，会 close handle 并回收节点。正常内存下的游戏状态、链表顺序和 backend 事件
不变。divisor 零的原始致命错误没有修复。

## 6. 验证

fake backend 记录并比较 open/error、user-data、volume read/write、loop、start、status、
position 和 close：

- 每次初始化增加两个 free 节点，播放仅受 ID/free 链表限制，disabled 不阻止播放或
  音量更新；
- duplicate ID、空池、open 失败/error copy、active/free 头顺序与销毁时的 zero-handle
  close；
- fixed-point fade 的 `24, 16, 8, 0` 序列和回收时点；
- status 2 的双 volume-zero、status 4/8/16 保留、默认 status 独立保留及级联回收缺陷；
- 七个 wrapper 的 ID 100、固定返回、signed 缩放以及 mode 0/1/2/3/16 过渡结果。

Linux Clang core 为 71/71，Linux Clang app 与 Windows LLVM app 均为 75/75 CTest
通过。原程序动态 stream 状态差分仍为 `blocked_runtime_oracle`；需要时只准备 Frida
工具并等待用户运行原版。
