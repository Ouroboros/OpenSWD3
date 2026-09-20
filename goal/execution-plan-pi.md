# OpenSWD3 全项目逆向、实现与验证执行方案（Pi / v916）

> 状态：active
>
> 日期：2026-04-03
>
> 范围：全项目；当前只执行 Workpack 302，持续推进至 battle 422/422、完整战斗生命周期与 I5、模块 11、B11 和最终项目验收
>
> 当前目标：`audit_order=302 / 0x004787D0 / sub_4787D0`
>
> 串行游标：Workpack 301 已完成；本文件只授权当前 workpack，并在其唯一 REVIEW、提交、推送和 Telegram 汇报后由 inventory 推导下一目标

---

## 0. 本版变更与当前基线

Workpack 301 已关闭 `0x004787C0`：

- inventory 为 `301/422 = 291 platform_adapted + 10 assembly_exact + 121 pending_audit`；
- SHA-256 为 `ed883a34c95dd0ca329382eb1c4941a62a26752ee7ced6bc73313efa8f15cbcd`；
- `sub_4787C0` 已按完整 dword 读取、`SHR 31`、两类 fault、普通 RET、EAX/EDX/flags 与唯一 `0x00453409 -> 0x0045340E` 物理 CALL 直接 typed 化；
- Group-A 与 Group-B 继续复用 Workpacks 299–300 建立的 `field_26b8` canonical owner；旧 opaque ordinal 只保留为 reserved；
- 定向测试、AddressSanitizer、Linux core 199/199、Linux app 205/205、连续 10 轮完整 core、格式、TMP 与 release audit 全部通过；
- 原版动态差分仍因 runtime oracle 缺失登记为 `blocked_runtime_oracle`，不得伪造 `original_diff_verified`。

本版把串行游标前移到 Workpack 302。Workpack 302完成不等于Goal完成。

---

## 1. 当前 Workpack 302 权威边界

完整 LST 主体：`0x004787D0..0x004787E8`，共 25 字节、5 条实际指令、0 个 callee、0 个分支、1 个 `retn 4`，没有外部 chunk 或中段入口：

```text
004787D0  mov dx,[esp+4]
004787D5  xor eax,eax
004787D7  mov ax,[ecx+2A7Ch]
004787DE  mov [ecx+eax*2+29C4h],dx
004787E6  retn 4
```

权威 inventory 导航显示 13 个 caller 函数、40 处物理 CALL：

- `0x004539B0` ×1；
- `0x004576A0` ×1；
- `0x004582B0` ×2；
- `0x00458DE0` ×6；
- `0x0045C010` ×4；
- `0x0045D690` ×5；
- `0x00469D20` ×1；
- `0x0046EE60` ×3；
- `0x004731A0` ×2；
- `0x004758A0` ×2；
- `0x0047E5C0` ×2；
- `0x00481010` ×8；
- `0x00481A40` ×3。

必须从完整 caller LST 重新核对每个 CALL 地址、返回地址、可达条件、入口寄存器、入口 flags、栈参数来源和后缀；inventory 只作导航，不能替代审计。

全 LST 对 `actor+0x2A7C` 与 `actor+0x29C4` 家族当前命中 22 处，除目标函数外还涉及 `0x0047C222`、`0x0047C2C1`、`0x0047C2D9`、`0x0047C2E8`、`0x0047C606`、`0x0047C62A`、`0x0047C63A`、`0x0047CEC9`、`0x0047CED0`、`0x0047CEE0`、`0x0047CEEC`、`0x0047CF07`、`0x0047D3EF`、`0x0047D502`、`0x0047D657`、`0x0047D65E`、`0x0047D710`、`0x0047D72B`、`0x0047D769`和`0x0047D7A2`。必须完成字段语义、容量、读写者、初始化和 setter 同步审计，不得把当前索引视为已验证的安全数组下标。

---

## 2. 必须恢复的精确机器语义

typed leaf 必须保持以下顺序和部分提交：

1. 从 `[ESP+4]` 读取参数低 word 到 DX，保留入口 EDX 高 16 位；
2. `XOR EAX,EAX`，完整清零 EAX，并提交 XOR flags；
3. 从 `actor+0x2A7C` 读取 word 到 AX，因此正常 EAX 为零扩展索引；
4. 以完整 ECX actor token、零扩展 EAX 索引和 `actor+0x29C4` 基址计算目标 word，写入 DX 低 16 位；
5. 从 `[ESP]` 读取返回地址并执行 `RET 4`，正常 ESP 增加 8。

正常返回必须保持：

- EAX：零扩展的原 `actor+0x2A7C` word；
- ECX：入口 actor token；
- EDX：入口高 16 位与参数低 16 位拼接；
- flags：来自 `XOR EAX,EAX`，即 CF=0、PF=1、ZF=1、SF=0、OF=0，AF 未定义；后续 MOV 与 RET 不改 flags；
- EIP：调用者真实返回地址；
- ESP：入口值加 8。

至少建模以下真实停止点：

- 参数 `[ESP+4]` 读取 fault：任何指令副作用尚未发生；
- `actor+0x2A7C` 读取 fault：DX 参数与 XOR 后 EAX/flags 已提交；
- 目标 word 写入 fault：EAX 索引、DX 参数和 XOR flags 已提交，但目标未改；
- RET 返回地址读取 fault：目标 word 已写，ESP 未推进；
- 正常 `RET 4`：返回地址弹出与参数清理一次完成。

目标地址计算不得高层 clamp、取模、范围修正或改写索引。若现代 owner 需要有界容器，越界只能在原目标内存访问点形成 typed-stop，不能改变机器地址算术。

---

## 3. Canonical owner 与数据模型要求

必须先完成 `+0x2A7C` 与 `+0x29C4` 22 处访问的完整 xref 审计，再决定 owner：

- 优先复用 startup/action/lifecycle 中已有 actor canonical backing；
- 若现有字段实际属于同一物理 word 流，必须建立中性共享 backing 或 alias view，而不是新增平行缓存；
- `+0x29C4` 到 `+0x2A7B` 的连续区域与紧邻 `+0x2A7C` cursor 的关系必须由所有读写者和初始化路径证明；
- copy construction、move、assignment 与 alias 规则必须保持物理 owner 语义；
- generic setter `0x00478A70/0x00478B20` 及其他尚未回收的写端在其回收前必须同步 canonical owner；
- Group-A、Group-B 与任何调试/特殊 actor 路径都必须映射到真实现有 owner，不得用静态全局替代每 actor 存储；
- reserved 地址、枚举 ordinal 和未审 caller 位置可以保留，但生产路径不得调用已关闭 `0x004787D0` generic 边界。

---

## 4. 40 个物理 caller 的关闭规则

每一处 CALL 必须单独登记：

- CALL 地址与返回地址；
- actor token 的来源与对象组；
- 参数 word 的真实来源及 PUSH 前寄存器/flags；
- 入口 EAX/ECX/EDX、ESP 和 flags；
- 正常返回后 EAX 索引、ECX actor token、EDX 参数残值与 XOR flags 如何被后缀消费；
- 当前 CALL 的条件可达性；
- typed-stop 时已经提交的 caller 前缀与必须阻断的后缀；
- 相邻多个 CALL 使用共享 helper 时仍保留每个物理 CALL 的身份和动态顺序。

caller 必须在原现代控制流位置直接组合 typed leaf。不得：

- 保留 `0x004787D0` 的 generic opaque 生产调用；
- 把 40 个物理 CALL 折叠成一个无法追溯的逻辑事件；
- 在内部 CALL 尚未到达时伪造外层 trace；
- 用测试专用调用替代生产集成；
- 因待审父函数过大而复制整个函数；允许建立窄 reply，但必须只投影真实已执行的当前 CALL，并为后续父工作包保留精确边界。

Workpack 302只关闭经完整 caller LST 与现代路径双向证明的 CALL。无法在当前 parent 中安全直接组合的 CALL 必须保留原物理位置和明确延期目标，不能静默算作已关闭。

---

## 5. 测试要求

新增 focused typed leaf 测试，至少覆盖：

- 参数低 word 覆盖 DX、EDX 高 word保持；
- 索引零、普通值、`0xFFFF`；
- 目标 word 原值被准确替换；
- EAX 零扩展索引；
- XOR flags 与 AF definedness；
- 参数、索引、目标写、RET 四类 fault；
- 目标写后的 RET fault 部分提交；
- 正常 `RET 4` 的 ESP/EIP；
- owner alias 与所有写端同步；
- 未执行访问计数为零。

每个已关闭 caller 类型至少有 focused 测试，整体必须覆盖 40 个 CALL 的地址/返回地址集合、条件可达性、参数来源、入口寄存器/flags、动态 trace 顺序、正常后缀和 typed-stop 后缀抑制。多个相邻 CALL、相同 actor 不同参数、不同 actor 相同参数都要避免状态串槽。

测试必须注册到 `battle.legacy_battle_setup`，正式定向命令使用真实名称和 `-C Debug`。

---

## 6. 证据与 inventory

新增唯一 evidence，至少记录：

- 25 字节完整 LST、5 条指令和 `RET 4`；
- 无 callee、无分支、无外部 chunk；
- 参数/索引/目标写/RET 的真实访问顺序与 fault 前缀；
- EAX/ECX/EDX、ESP/EIP 与 XOR flags；
- `+0x2A7C/+0x29C4` 22 处 xref 及 canonical owner；
- 13 个 caller 函数、40 个物理 CALL 的完整分类；
- 所有关闭与延期边界；
- 生产 raw 地址零调用；
- `blocked_runtime_oracle` 的具体缺失后端。

只允许由 `analysis/tools/build_battle_workpack.py` 修改 `battle-function-workpack.tsv`。生成器必须连续运行两次并逐字节一致；记录新的 422 行计数和 SHA-256。PLAN、battle module 和 evidence 必须互相一致。

---

## 7. 实现与格式约束

- 所有实现由主 Agent 完成；禁止调用 subagent。
- LST 机器码是行为真值；保留访问/调用顺序、别名、寄存器、flags、fault、部分提交和 typed-stop。
- 新 C++ 文件全量 clang-format；旧文件只格式化 changed ranges。
- 不提交 `build/`。
- 不启动原版或 OpenSWD3 游戏程序。
- 项目命令设置 `TMPDIR/TMP/TEMP=$PWD/build/tmp/runtime`；构建和测试最多16并发。
- 保持 `goal/HANDOFF.md` 和仓库根 `compile_commands.json` 不存在；父级 symlink 指向 `OpenSWD3/build/linux-core/compile_commands.json`。
- 新文件 staged mode 必须为 `100644`。
- 一个 workpack 只做一个最终 REVIEW 和一个 commit。
- commit 必须使用 `$commit` Skill；主 Agent精确暂存。
- 每次提交后重新完整读取 `AGENTS.md` 和本 PLAN。

---

## 8. 验证门

定向门：

```bash
TMPDIR=$PWD/build/tmp/runtime TMP=$TMPDIR TEMP=$TMPDIR \
OPENSWD3_BUILD_JOBS=16 OPENSWD3_TEST_JOBS=16 \
./build.sh core --test

ctest --test-dir build/linux-core -C Debug \
  -R '^battle\.legacy_battle_setup$' --output-on-failure
```

正式门：

```bash
TMPDIR=$PWD/build/tmp/runtime TMP=$TMPDIR TEMP=$TMPDIR \
OPENSWD3_BUILD_JOBS=16 OPENSWD3_TEST_JOBS=16 \
./build-asan.sh --test

TMPDIR=$PWD/build/tmp/runtime TMP=$TMPDIR TEMP=$TMPDIR \
OPENSWD3_BUILD_JOBS=16 OPENSWD3_TEST_JOBS=16 \
./build.sh core --test

TMPDIR=$PWD/build/tmp/runtime TMP=$TMPDIR TEMP=$TMPDIR \
OPENSWD3_BUILD_JOBS=16 OPENSWD3_TEST_JOBS=16 \
./build.sh app --test
```

还必须完成：

- Linux core 连续10轮完整测试；
- 正式 stderr 为空；
- app 若出现第三方 SDL 自动重配告警，可分类但必须在稳定缓存复跑至干净；
- `git diff --check`；
- 新文件全量与旧文件 changed-range clang-format Werror；
- `/tmp` 审计 `confirmed_entries=0`；
- unstaged 与 staged release audit；
- 新文件 mode、暂存范围、无 tracked build artifact、symlink 与禁止文件检查。

模块10关闭记录固定验证句：

`验证：定向测试、AddressSanitizer、Linux core 199/199、Linux app 205/205 全部通过。`

---

## 9. REVIEW、提交与汇报

所有实现、证据、inventory、PLAN 和门禁完成后进行唯一最终 REVIEW：

1. 复核完整差异与 staged 差异；
2. 逐项核对 25 字节、22 处字段访问、40 个 CALL、owner alias、fault、寄存器/flags 与 typed-stop；
3. 确认生产 `0x004787D0` raw 调用为零；
4. 确认测试、格式、TMP、inventory 与 release audit；
5. 精确暂存当前 workpack 文件；
6. 使用 `$commit` Skill 生成并创建唯一提交；
7. push 并确认上游 `0 0`；
8. 发送固定五段 Telegram 汇报并关闭对应任务；
9. 重新完整读取 `AGENTS.md` 和更新后的 PLAN，再继续 inventory 中下一 `pending_audit` workpack。

Workpack 302提交、推送和汇报只代表当前 workpack 完成，不得调用 `goal_complete`。只有 Workpacks 302–422、战斗生命周期与I5、模块11、B11及最终验收全部完成并逐项复核后，才允许完成整个 Goal。
