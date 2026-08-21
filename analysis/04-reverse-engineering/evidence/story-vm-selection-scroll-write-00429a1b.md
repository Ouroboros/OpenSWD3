# 剧情 VM 选择滚动表写入 `0x00429A1B`

## 结论

`sub_427920` 的 opcode63 把一段由 `0xFF00` 终止的 `u16` 序列写入 64-word 世界选择滚动表，同时把 `+2` 的 prefix 零扩展到滚动间隔与当前倒计时，并快照当前视口 left/top。接受 0..56 项；57 项及以上只诊断、不推进 IP，并经共同出口发布 previous 后在原指令跨帧让出。

成功路径按 `6 + 2*count` 推进，令 `ESI=1`，经共同出口发布归一化 previous 后在同一次解释器调用继续取指。现代实现复用现有 `world_selection_words_` 与 `LegacyWorldSelectionScrollState`，不新增平行业务状态。

唯一行为依据是 `swd3.exe.lst` 的机器码和指令。既有语义表和选择滚动 C++ 只用于 owner 映射与 REVIEW，不能替代本 handler 的独立证明。

## 指令布局与两个不同哨兵

```text
+0   u16 opcode63
+2   u16 prefix / frame interval
+4   u16 word[0]
...  u16 word[count-1]
     u16 0xFF00 terminator
```

脚本终止符是 `0xFF00`；运行时目标表的空值是 `0xCFCF`。两者不可混用。terminator 不复制到目标表。

handler 从 `current+4` 开始无界 word 扫描，首项即可为 `0xFF00`，此时 count 为 0。扫描期间不读 prefix、不改目标表、不读任何运行时 owner。

## 56 项门槛与超限路径

扫描结束后执行 signed `cmp count, 0x38` / `jle`。窗口内可达 count 非负，因此有效域等价于：

- `count <= 56`：进入写入路径；
- `count > 56`：调用 diagnostic，跳到 `0x0042B0AA`，保持 `ESI=0`。

超限路径不清选择表、不写 interval/countdown、不读视口、不推进 IP；共同出口仍发布 normalized previous63，然后 yield。下一帧会原地重新扫描并再次诊断。

原版扫描没有外部边界。现代在每个 word 前检查 0x8000-byte Talk window；未找到 `0xFF00` 时在任何目标副作用和 previous publication 前返回 `operand_out_of_range`。

## 成功写入顺序

`0x00429A58..0x00429ACD` 的顺序固定为：

1. `rep stosd` 以 `0xCFCFCFCF` 写 32 个 dword，即把 64 个目标 word 全部清为 `0xCFCF`；
2. 以 `rep movsd` 复制 `floor((2*count)/4)` 个 dword，也就是先复制偶数个 word；
3. 读取当前视口 top（`xmmword_4AB980+4`）；
4. 若 count 为奇数，再以 `rep movsb` 复制最后一个 word；
5. 零扩展读取 `+2` prefix；
6. 同值先写 `dword_4AD0D0`，再写 `dword_4A94A4`；它们分别对应现代 `frame_interval` 与 `frames_remaining`；
7. 读取当前视口 left；
8. 先保存 top 到 `dword_4A992C`，再保存 left 到 `dword_4A93E4`；现代字段分别是 `saved_top` 与 `saved_left`；
9. IP 增加 `6 + 2*count`，`ESI=1`，共同出口发布 previous 并同调用继续。

handler 不修改选择滚动 cursor。64-word 表最多写 56 项，因此成功后固定至少保留 8 个 `0xCFCF` 尾 word。

## 现有运行时 owner

原版全局与现代 owner 的对应关系：

| 原版 | 现代 |
| --- | --- |
| `word_4ACE70[64]` | SDL `world_selection_words_` |
| `dword_4AD0D0` | `LegacyWorldSelectionScrollState::frame_interval` |
| `dword_4A94A4` | `frames_remaining` |
| `dword_4A93E4` | `saved_left` |
| `dword_4A992C` | `saved_top` |
| `xmmword_4AB980 +0/+4` | `LegacyWorldCameraRect::left/top` |

64-word常量移到 `legacy_world_selection_scroll.hpp`，使 transient reset、frame coordinator、SDL 和 Story VM 共享同一协议定义；这只是声明归位，不改变 reset 行为。

现代 owner 缺失按原版访问顺序 typed-stop：

- 目标表 owner 缺失：terminator/count 已验证，但还未清表；
- camera 缺失：64 项已清，偶数对已复制，奇数尾尚未复制；
- scroll state 缺失：目标表已完整复制且 camera top 已读取，prefix 与快照尚未写。

有效 owner 路径与原版完全一致；typed-stop 不回滚先前目标表写入。

## IP、previous 与精确尾

成功记录结束位置是 `current + 6 + 2*count`。完整记录恰好结束于 `0x8000` 时，选择表、两个计时字段、left/top快照、IP 与 previous63 均先完成，下一次 same-call fetch 才返回 `instruction_out_of_range`。

超限记录不推进 IP，但 previous63 仍发布；无 terminator 或 owner typed-stop 则 previous 与 IP 都保持旧值。

## 真实资产锁

对 `story-vm-talk-linear-records.tsv` 的全部 opcode63 entry 逐条回读原始 TALK 文件：

- 共 7 条，TALK1/2/3 分布 `2/1/4`，TALK4 为 0；
- 7/7 全部 raw `0x003F`、单 entry probe、长度 22；
- 7 条 count 均为 8，没有真实超 56 记录；
- prefix 仅有 2、3、4，分布 `3/1/3`；
- 原始偏移、长度、`0xFF00` terminator 与线性记录表逐条核验零错误。

真实回放使用 `TALK1.DAT@0x00022D55`：prefix 4，八个 word 为 `1,1,0,0,0xFFFF,1,0,0`，terminator `0xFF00`。执行后表第 8 项保持 `0xCFCF`，cursor 保留，interval/countdown 都为 4，left/top 被快照，并同调用进入下一条等待。

## 测试覆盖

- 四种 raw alias；
- count 0、3、56 成功路径，prefix 0 与 `0xFFFF` 零扩展；
- 64-word `0xCFCF` 清空、raw signed word 位型复制、固定尾哨兵；
- cursor 不变，frame interval/remaining 双写，left/top快照且 camera 本身不改；
- count57 在所有 owner 缺失时仍先走超限原地yield，证明超限不访问 owner；
- Talk window 无 terminator，在清表和 previous publication 前 typed-stop；
- 目标表、camera、scroll state 三个 owner 缺失及其分阶段部分效果；
- `0x7FF8`一项精确尾完成 publication 后下一 fetch 失败；
- TALK1真实8项记录回放；
- 剧情 VM 三项定向测试全部通过。

## 双向收敛与分类

LST→实现 REVIEW 发现并修正了一处初版差异：脚本 terminator 必须是 `0xFF00`，不能误用目标表空值 `0xCFCF`。真实资产测试在自洽的合成测试之外发现该问题；修正后合成、真实与初始会话三项测试重新全部通过。

分类：`platform_adapted`。适配仅限无界扫描的窗口检查、原版进程全局的 typed owner 与对应失败隔离；有效域的复制切点、计时字段、视口顺序、IP、previous、same-call/yield行为保持汇编语义。
