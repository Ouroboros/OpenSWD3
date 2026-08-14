# 世界瞬态状态重置闭环

状态：`platform_adapted`、`assembly_exact`、`unit_tested`；原程序动态差分仍为
`blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst` 的 `0x00411D00..0x00411D8D`。该函数在存档状态恢复后
释放七类跨帧 owner，并恢复选择序列、三通道颜色步长和 ANI 逐行复制效果的初值。

## 1. 物理 ABI 与调用边界

函数没有栈参数，只保存和恢复 EDI。唯一直接调用点是
`sub_4070A0:0x0040823A`；返回后立即调用 `sub_485710`，没有分支或数据流读取本次 EAX。
函数最后一次 `xor eax,eax` 为偏移数组的 `rep stosd` 准备零值，同时使正常返回 EAX
为零；现代接口因此使用 `void`，不建立原调用者不存在的成功协议。

## 2. 严格操作次序

LST 与 `reset_legacy_world_transient_state` 的顺序逐项对应：

| 次序 | 原操作 | 现代 owner |
|---:|---|---|
| 1 | `sub_40F500` | packed-row 效果链 |
| 2 | `sub_40F540` | moving-action 链 |
| 3 | `sub_40F570` | role-head-action 链 |
| 4 | `sub_40F5A0` | 对话消息链及 bit 15 掩码 |
| 5 | `sub_40F5E0` | picture-action 双链 |
| 6 | `sub_40F630` | 四槽角色粒子链及 emitter 块 |
| 7 | `sub_40F670` | 四槽 ANI drift 的 X 位置 |
| 8 | `dword_4C97F0=0`，填充 `word_4ACE70` | blue step 清零；64 项选择表填 `0xCFCF` |
| 9 | `dword_4A94B8=0`，填充 `word_4AB2F8` | green step 清零；64 项行数填 1 |
| 10 | `dword_4A9A00=0`，填充 `word_4AB8FC` | red step 清零；64 项复制宽度填 4 |
| 11 | `word_4CAE90=0`，填充 `unk_4AC9BC` | 帧计数器清零；64 项像素偏移填 0 |

前三次 `rep stosd` 的 ECX 均为 `0x20`，即各写 128 字节、64 个 i16；最后一次 ECX
为 `0x40`，即写 256 字节、64 个 u32。颜色当前值、目标值和倒计时均不在写集合中，
不得随三个 step 一并清零。ANI drift 也只重置每个 `0x10` 槽的首 dword，其余字段由
已闭环的 `sub_40F670` 合同保留。

## 3. 现代所有权与验证

`LegacyWorldTransientResetOwners` 只聚合现有实际 owner，不复制状态。SDL 世界 owner
重建已调用该函数；原先仅含一个哨兵的临时选择 span 扩为与物理全局一致的 64 项存储。
新游戏外围初始化仍可在此合同之后执行自身更广的状态初始化，不改变本函数只写上述
字段的独立语义。

收敛复核反复执行：

- LST→C++：核对七个调用顺序、三组 step 与 i16 填充的交错次序、帧计数器和 u32
  偏移填充；
- C++→LST：核对每个被写字段的地址映射、64 项范围、填充值及所有必须保留字段；
- 调用点复核：确认零参数、EDI 保存、最终零 EAX 和唯一调用者不消费返回值；
- 定向 UT：污染全部 owner 和数组，固定链释放、对话 bit 15、drift 尾字段保持、颜色
  非 step 字段保持以及四组完整 64 项初值。

最终正向和反向复核在有效运行域没有剩余的顺序、范围、填充值、保留字段或返回合同
差异。Linux `core` 185/185、Linux `app` 190/190、Windows LLVM `app` 190/190
CTest 全部通过，两端应用成功链接且未启动任何 EXE。实现位于
`legacy_world_transient_reset.cpp::reset_legacy_world_transient_state`。
