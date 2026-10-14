# 战斗角色动作目标清空（0x00478B20）

## 1. 范围、边界与 ABI

权威 LST 把 `sub_478B20` 锁定为 `0x00478B20..0x00478B29`，半开区间为
`0x00478B20..0x00478B2A`。主体共 10 字节、2 条实际指令、0 个 callee、0 个分支和
1 个普通 `ret`，没有外部 chunk 或中段入口。`0x00478B2A..0x00478B2F` 是下一函数前的
对齐填充。

```text
0x00478B20  mov word ptr [ecx+0x29A2], 0xFFFF
0x00478B29  ret
```

入口 ECX 是 actor token，函数没有显式参数。字段写和 RET 都不修改 EAX、ECX、EDX 或
算术 flags。正常返回时 ESP 相对入口增加 4，EIP 为物理返回地址。

## 2. 字段、访问顺序与停止点

唯一业务写入是把 `actor+0x29A2` 的完整 word 无条件替换为 `0xFFFF`。字段原值为
`0x0000`、`0x7FFF`、`0x8000` 或 `0xFFFF` 都走同一路径，不存在读取、比较、条件跳过或
算术回绕。

严格物理访问顺序为：

1. 向 `actor+0x29A2` 写入 word `0xFFFF`；
2. RET 从 `[ESP]` 读取返回地址。

现代实现保留两个独立 typed-stop：

- 字段写不可达时不提交字段，EIP 停在 `0x00478B20`；
- RET 读取不可达时字段已经提交，ESP 不推进，EIP 停在 `0x00478B29`。

两种停止都保持入口 EAX、ECX、EDX 和 flags。只有 RET 成功时才记录返回地址读取、推进
ESP 并把 EIP 改为调用点返回地址。

## 3. canonical owner

`actor+0x29A2` 已由 `0x004786E0` getter 与 `0x00478A70` writer 收敛为唯一
`action_target` owner：

```text
Group-A  action.group_a_action_execution[index].action_target
Group-B  startup.group_b_lifecycle[index].action_execution.action_target
```

本函数直接复用 `resolve_legacy_battle_actor_action_target()`，没有新增平行 actor 数组、第二份
target 缓存或独立 resolver。本工作包五处 caller 只使用 Group-A 或 Group-B actor，不涉及
调试特殊 actor。

## 4. 五个已关闭父 CALL

完整 LST 中共有五条真实 `call sub_478B20`，全部位于已关闭父函数：

```text
0x00456F94 -> 0x00456F99  Group-A frame
0x004570F5 -> 0x004570FA  Group-A frame
0x00457EBC -> 0x00457EC1  Group-B frame
0x0045AEC2 -> 0x0045AEC7  post-action global cleanup
0x0045AF58 -> 0x0045AF5D  post-action candidate rebuild
```

五处都在原控制流位置直接组合 typed leaf。trace 保留 CALL 地址、返回地址、actor token、请求
序号和 leaf 最终状态。字段写或 RET 停止立即阻断尚未执行的父级后缀，本轮没有延期 caller。

### 4.1 Group-A frame

`0x00456F94` 处理当前 Group-A actor。入口 EAX/EDX 来自首个非 terminal 的
`0x0047CE80` 回复，flags 来自 `cmp eax,1` 的不等结果。停止时不执行随后的
`0x00456FA4` gate-decay。

`0x004570F5` 同样处理当前 Group-A actor。入口 EAX、EDX 和 flags 来自刚完成的
`0x004786E0` action-target getter。停止时不执行本地清理、selection-complete、post-action
或其后帧逻辑。

Group-A 内部 dispatch 的 request offset 以父 context offset 加已消费 CALL 数计算，连续或
嵌套调用不会重复读取旧请求。

### 4.2 Group-B frame

`0x00457EBC` 处理当前 Group-B source actor。入口 EAX、EDX 和 flags 来自刚完成的
`0x004786E0` getter；中间 `movsx` 和栈写不修改 flags。停止时不执行
`0x00457EC3` selection-complete。

嵌套 opponent-action dispatch 同样把父 request offset 与已消费 CALL 数相加，并把 nested
trace 按真实顺序合并回父结果。

### 4.3 Post-action

`0x0045AEC2` 位于全局清理分支。入口 EAX 是已观察的 Group-B count，EDX 是 packed 低
byte 加一，flags 来自二者相等的 CMP。成功后才进入 `0x0045AEDF` gate-decay；该 decay 的
EAX 为 `345*queried`，EDX 保留 packed 低 byte 加一。

`0x0045AF58` 位于候选重建分支。入口 EAX/EDX 来自 terminal 回复，flags 来自
`test eax,eax` 的零结果。成功后才进入 `0x0045AF75` gate-decay；该 decay 的 EAX 为
`345*queried`，EDX 保留 terminal 回复。

本轮按 LST 修正了这两条直接后缀此前互换的 gate-decay 地址及寄存器残值。修正后的地址也
与 Workpack 310 的十二项物理 CALL 清单一致。

## 5. 双向追溯与测试范围

LST 到 C++ 已覆盖无条件 word 写、普通 RET、两个真实访问点、EAX/ECX/EDX 与 flags 保持、
ESP/EIP 和 partial commit。C++ 到 LST 反向追溯覆盖 canonical resolver、惰性 heap-backed
request/trace、request offset、五个物理 caller 和父级后缀抑制；没有无来源业务分支或状态写。

汇编独立测试覆盖：

- Group-A、Group-B 与非法 token resolver；
- 四种字段初值都无条件写为 `0xFFFF`；
- 字段写停止与 RET 停止的不同提交前缀；
- EAX、ECX、EDX、flags、ESP、EIP 与物理返回地址；
- 显式 request offset；
- 五组 CALL/return/actor 物理身份；
- Group-A、Group-B、post-action 的 typed-stop 与后缀抑制；
- post-action 两条 gate-decay 地址和 EAX/EDX 残值；
- production generic/raw `0x00478B20` 调用归零。

最后一轮完整正向与反向追溯没有产生新的未解释差异。

## 6. 分类与动态差分状态

任意 32 位 actor token、字段页和 RET 栈页无法由现代 C++ 直接合法解引用。canonical token
resolver、原访问点 typed-stop、惰性 request/trace 和显式 request offset 是最小平台边界，
因此本目标登记为 `platform_adapted`，不能以 owner、span、测试通过或 typed-stop 单独标记为
`assembly_exact`。

当前缺少原版完整 Group-A/Group-B actor backing、`+0x29A2` 异常字段页、RET 异常栈页，
以及五个 caller/callee 的联合寄存器、flags 与 SEH 捕获后端。原版动态差分登记为
`blocked_runtime_oracle`，不得写成 `original_diff_verified`。

## 7. 验证与关闭

最终定向 setup 测试静默通过；格式化后的 Linux core 为 `199/199`，Linux app 为
`205/205`，AddressSanitizer/UBSan 为 `199/199`。连续十轮 Linux core 均以 `199/199` 和
exit 0 完成。最终日志扫描没有 OpenSWD3 源码 warning、编译 error、测试失败、sanitizer
finding 或 runtime error。

三份新 leaf 文件全量 clang-format、旧文件 changed-range clang-format、`git diff --check`、
权限位、生产 raw/generic `0x00478B20` 调用、五个 caller 覆盖、TMP 分类和完整 release diff
审计均通过。未启动原版或 OpenSWD3 游戏程序。

LST 地址摘录 SHA-256 为
`996f7090b16b1c3a86d44174e55c0ddf9916a4bc8585dab723a8ad3df76d5deb`。inventory 生成器
连续双跑逐字节一致，关闭结果为
`311/422 = 301 platform_adapted + 10 assembly_exact + 111 pending_audit`，下一条为
`audit_order=312 / 0x00478B30 / sub_478B30`。最终 inventory SHA-256 为
`805e441154895295cfae0ded8ff186748d3058f355b22dcb07b2c84da06d19cb`。
